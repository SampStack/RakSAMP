#include "load_mode.h"

#include "main.h"

#include "StringCompressor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <float.h>
#endif

namespace
{
using Clock = std::chrono::steady_clock;

constexpr int NetworkThreadSleepMilliseconds = 20;
constexpr int MaximumConnectionAttempts = 5;
constexpr int RPC_ModelRequest = 179;
constexpr int RPC_FinishDownload = 184;
constexpr int RPC_DownloadCompleted = 185;

enum class LoadSessionState
{
	Pending,
	Connecting,
	Joined,
	InGame,
	Active,
	Retrying,
	Failed
};

struct LoadSession
{
	std::size_t index = 0;
	std::string accountName;
	std::string characterName;
	std::string password;
	RakClientInterface *client = nullptr;
	LoadSessionState state = LoadSessionState::Pending;
	std::string failure;
	bool loginSubmitted = false;
	bool directCharacter = false;
	bool characterSelected = false;
	bool spawnSent = false;
	bool spectating = false;
	bool stopping = false;
	bool probeAnnounced = false;
	int connectionAttempts = 0;
	unsigned short localPort = 0;
	float position[3] = {1178.33f, -1323.49f, 14.08f};
	float facingAngle = 0.0f;
	float health = 100.0f;
	float armour = 0.0f;
	Clock::time_point nextSyncAt{};
	Clock::time_point retryAt{};

	~LoadSession()
	{
		if(client != nullptr)
		{
			stopping = true;
			client->Disconnect(0);
			RakNetworkFactory::DestroyRakClientInterface(client);
			client = nullptr;
		}
	}

	LoadSession(const LoadSession &) = delete;
	LoadSession &operator=(const LoadSession &) = delete;
	LoadSession() = default;
};

LoadSession *currentSession = nullptr;
volatile std::sig_atomic_t interrupted = 0;

LoadSession *Current()
{
	return currentSession;
}

bool IsFinite(float value)
{
#ifdef _WIN32
	return _finite(value) != 0;
#else
	return std::isfinite(value);
#endif
}

void Fail(LoadSession &session, const std::string &reason)
{
	if(session.state == LoadSessionState::Failed || session.stopping)
		return;
	session.failure = reason;
	session.state = LoadSessionState::Failed;
}

void DestroyClient(LoadSession &session)
{
	if(session.client == nullptr)
		return;
	session.client->Disconnect(0);
	RakNetworkFactory::DestroyRakClientInterface(session.client);
	session.client = nullptr;
}

void RetryOrFail(LoadSession &session, const std::string &reason)
{
	if(session.state == LoadSessionState::Active ||
		session.connectionAttempts >= MaximumConnectionAttempts)
	{
		Fail(session, reason);
		return;
	}
	session.failure = reason;
	session.state = LoadSessionState::Retrying;
	session.retryAt = Clock::now() +
		std::chrono::seconds(session.connectionAttempts);
}

RakNet::BitStream RpcInput(RPCParameters *parameters)
{
	return RakNet::BitStream(
		reinterpret_cast<unsigned char *>(parameters->input),
		(parameters->numberOfBitsOfData + 7) / 8,
		false);
}

void SendFinishedDownloading(LoadSession &session)
{
	if(settings.protocol != SampProtocol::V03DL)
		return;
	RakNet::BitStream empty;
	int rpc = RPC_FinishDownload;
	session.client->RPC(&rpc, &empty, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
		FALSE, UNASSIGNED_NETWORK_ID, nullptr);
}

void LoadInitGame(RPCParameters *)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	session->state = LoadSessionState::InGame;
	SendFinishedDownloading(*session);
}

void LoadModelRequest(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;

	RakNet::BitStream data = RpcInput(parameters);
	DWORD poolId = 0;
	int count = 0;
	if(!data.Read(poolId) || !data.Read(count) ||
		count <= 0 || poolId + 1 >= static_cast<DWORD>(count))
		SendFinishedDownloading(*session);
}

void LoadDownloadCompleted(RPCParameters *)
{
}

void SendDialogResponse(
	LoadSession &session,
	WORD dialogId,
	const std::string &response)
{
	if(response.size() > 255)
	{
		Fail(session, "dialog response exceeded 255 bytes");
		return;
	}

	RakNet::BitStream output;
	output.Write(dialogId);
	output.Write(static_cast<BYTE>(1));
	output.Write(static_cast<WORD>(-1));
	output.Write(static_cast<BYTE>(response.size()));
	output.Write(response.data(), static_cast<int>(response.size()));
	session.client->RPC(&RPC_DialogResponse, &output, HIGH_PRIORITY,
		RELIABLE_ORDERED, 0, FALSE, UNASSIGNED_NETWORK_ID, nullptr);
}

void LoadDialog(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;

	RakNet::BitStream data = RpcInput(parameters);
	WORD dialogId = 0;
	BYTE style = 0;
	BYTE length = 0;
	char title[256] = {};
	char button[256] = {};
	char secondaryButton[256] = {};
	char information[256] = {};

	if(!data.Read(dialogId) || !data.Read(style) ||
		!data.Read(length) || !data.Read(title, length))
	{
		Fail(*session, "malformed dialog RPC");
		return;
	}
	title[length] = '\0';
	if(!data.Read(length) || !data.Read(button, length))
	{
		Fail(*session, "malformed dialog RPC");
		return;
	}
	button[length] = '\0';
	if(!data.Read(length) || !data.Read(secondaryButton, length))
	{
		Fail(*session, "malformed dialog RPC");
		return;
	}
	secondaryButton[length] = '\0';
	if(!stringCompressor->DecodeString(information, sizeof(information), &data))
	{
		Fail(*session, "malformed compressed dialog text");
		return;
	}

	if(!session->loginSubmitted &&
		(style == DIALOG_STYLE_PASSWORD || style == DIALOG_STYLE_INPUT))
	{
		session->loginSubmitted = true;
		SendDialogResponse(*session, dialogId, session->password);
	}
}

void LoadShowTextDraw(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr || session->characterSelected)
		return;

	RakNet::BitStream data = RpcInput(parameters);
	WORD textDrawId = 0;
	TEXT_DRAW_TRANSMIT transmit{};
	unsigned short textLength = 0;
	char text[1024] = {};
	if(!data.Read(textDrawId) ||
		!data.Read(reinterpret_cast<PCHAR>(&transmit), sizeof(transmit)) ||
		!data.Read(textLength) ||
		textLength >= sizeof(text) ||
		!data.Read(text, textLength))
		return;
	text[textLength] = '\0';

	if(!transmit.byteSelectable || session->characterName != text)
		return;

	RakNet::BitStream output;
	output.Write(textDrawId);
	session->client->RPC(&RPC_ClickTextDraw, &output, HIGH_PRIORITY,
		RELIABLE_ORDERED, 0, FALSE, UNASSIGNED_NETWORK_ID, nullptr);
	session->characterSelected = true;
}

void LoadToggleSpectating(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;

	RakNet::BitStream data = RpcInput(parameters);
	BOOL enabled = FALSE;
	if(!data.Read(enabled))
	{
		Fail(*session, "malformed spectator RPC");
		return;
	}

	const bool wasSpectating = session->spectating;
	session->spectating = enabled != FALSE;
	if(wasSpectating && !session->spectating &&
		session->characterSelected && !session->spawnSent)
	{
		session->spawnSent = true;
		RakNet::BitStream empty;
		session->client->RPC(&RPC_Spawn, &empty, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
			FALSE, UNASSIGNED_NETWORK_ID, nullptr);
		session->state = LoadSessionState::Active;
		std::fill(session->password.begin(), session->password.end(), '\0');
		session->password.clear();
		session->nextSyncAt = Clock::now();
	}
}

void LoadSetPosition(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	RakNet::BitStream data = RpcInput(parameters);
	float position[3] = {};
	if(!data.Read(position[0]) ||
		!data.Read(position[1]) ||
		!data.Read(position[2]) ||
		!std::all_of(
			std::begin(position),
			std::end(position),
			[](float value) { return IsFinite(value); }))
	{
		Fail(*session, "malformed position RPC");
		return;
	}
	std::memcpy(session->position, position, sizeof(position));
}

void LoadSetFacingAngle(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	RakNet::BitStream data = RpcInput(parameters);
	float angle = 0.0f;
	if(!data.Read(angle) || !IsFinite(angle))
	{
		Fail(*session, "malformed facing-angle RPC");
		return;
	}
	session->facingAngle = angle;
}

void LoadSetSpawnInfo(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;

	RakNet::BitStream data = RpcInput(parameters);
	BYTE team = 0;
	int skin = 0;
	DWORD customSkin = 0;
	BYTE unknown = 0;
	if(!data.Read(team) ||
		!data.Read(skin) ||
		(settings.protocol == SampProtocol::V03DL &&
			!data.Read(customSkin)) ||
		!data.Read(unknown) ||
		!data.Read(
			reinterpret_cast<PCHAR>(session->position),
			sizeof(session->position)) ||
		!data.Read(session->facingAngle) ||
		!std::all_of(
			std::begin(session->position),
			std::end(session->position),
			[](float value) { return IsFinite(value); }) ||
		!IsFinite(session->facingAngle))
	{
		Fail(*session, "malformed spawn-info RPC");
	}
}

void LoadSetHealth(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	RakNet::BitStream data = RpcInput(parameters);
	float health = 0.0f;
	if(!data.Read(health) || !IsFinite(health))
	{
		Fail(*session, "malformed health RPC");
		return;
	}
	session->health = health;
}

void LoadSetArmour(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	RakNet::BitStream data = RpcInput(parameters);
	float armour = 0.0f;
	if(!data.Read(armour) || !IsFinite(armour))
	{
		Fail(*session, "malformed armour RPC");
		return;
	}
	session->armour = armour;
}

void LoadConnectionRejected(RPCParameters *parameters)
{
	LoadSession *session = Current();
	if(session == nullptr)
		return;
	RakNet::BitStream data = RpcInput(parameters);
	BYTE reason = 0;
	data.Read(reason);
	Fail(*session, "server rejected join (reason " + std::to_string(reason) + ")");
}

void RegisterLoadCallbacks(LoadSession &session)
{
	session.client->RegisterAsRemoteProcedureCall(&RPC_InitGame, LoadInitGame);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ConnectionRejected,
		LoadConnectionRejected);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrDialogBox, LoadDialog);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrShowTextDraw,
		LoadShowTextDraw);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrTogglePlayerSpectating,
		LoadToggleSpectating);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerPos,
		LoadSetPosition);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerFacingAngle,
		LoadSetFacingAngle);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrSetSpawnInfo,
		LoadSetSpawnInfo);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerHealth,
		LoadSetHealth);
	session.client->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerArmour,
		LoadSetArmour);
	if(settings.protocol == SampProtocol::V03DL)
	{
		int modelRequest = RPC_ModelRequest;
		int downloadCompleted = RPC_DownloadCompleted;
		session.client->RegisterAsRemoteProcedureCall(&modelRequest,
			LoadModelRequest);
		session.client->RegisterAsRemoteProcedureCall(&downloadCompleted,
			LoadDownloadCompleted);
	}
}

bool StartSession(LoadSession &session)
{
	DestroyClient(session);
	session.loginSubmitted = false;
	session.characterSelected = session.directCharacter;
	session.spawnSent = false;
	session.spectating = false;
	session.failure.clear();
	++session.connectionAttempts;
	session.client = RakNetworkFactory::GetRakClientInterface();
	if(session.client == nullptr)
	{
		Fail(session, "could not allocate RakNet client");
		return false;
	}

	session.client->SetMTUSize(settings.iMaximumMtu);
	session.client->SetPassword(settings.server.szPassword);
	RegisterLoadCallbacks(session);
	if(!session.client->Connect(
		settings.server.szAddr,
		static_cast<unsigned short>(settings.server.iPort),
		0,
		0,
		NetworkThreadSleepMilliseconds))
	{
		Fail(session, "could not start connection");
		return false;
	}
	session.state = LoadSessionState::Connecting;
	session.localPort = session.client->GetInternalID().port;
	return true;
}

void SendAuthKey(LoadSession &session, Packet *packet)
{
	if(packet->length < 3)
	{
		Fail(session, "malformed auth-key challenge");
		return;
	}

	const BYTE challengeLength = packet->data[1];
	if(static_cast<unsigned int>(challengeLength) + 2 > packet->length)
	{
		Fail(session, "malformed auth-key challenge");
		return;
	}
	const char *challengeData =
		reinterpret_cast<char *>(packet->data + 2);
	const std::size_t challengeSize = strnlen(
		challengeData,
		std::min<std::size_t>(challengeLength, packet->length - 2));
	const std::string challenge(challengeData, challengeSize);

	const char *answer = nullptr;
	for(int index = 0; index < 512; ++index)
	{
		if(challenge == AuthKeyTable[index][0])
		{
			answer = AuthKeyTable[index][1];
			break;
		}
	}
	if(answer == nullptr)
	{
		Fail(session, "unknown auth-key challenge '" + challenge +
			"' (length " + std::to_string(challengeLength) + ")");
		return;
	}

	const std::size_t answerLength = std::strlen(answer);
	RakNet::BitStream output;
	output.Write(static_cast<BYTE>(ID_AUTH_KEY));
	output.Write(static_cast<BYTE>(answerLength));
	output.Write(answer, static_cast<int>(answerLength));
	session.client->Send(&output, SYSTEM_PRIORITY, RELIABLE, 0);
}

void SendClientJoin(LoadSession &session, Packet *packet)
{
	RakNet::BitStream input(packet->data, packet->length, false);
	PLAYERID playerId = 0;
	unsigned int challenge = 0;
	input.IgnoreBits(8);
	input.IgnoreBits(32);
	input.IgnoreBits(16);
	if(!input.Read(playerId) || !input.Read(challenge))
	{
		Fail(session, "malformed connection acceptance");
		return;
	}

	const int version = settings.iNetworkVersion;
	const unsigned int challengeResponse = challenge ^ version;
	const BYTE mod = 1;
	char gpci[64] = {};
	// SA-MP serials must remain divisible by 1001. gen_gpci's random source
	// already produces a distinct value for each call in this process.
	gen_gpci(gpci, 0x3e9);
	const BYTE accountLength = static_cast<BYTE>(session.accountName.size());
	const BYTE gpciLength = static_cast<BYTE>(std::strlen(gpci));
	const BYTE clientVersionLength =
		static_cast<BYTE>(std::strlen(settings.szClientVersion));

	RakNet::BitStream output;
	output.Write(version);
	output.Write(mod);
	output.Write(accountLength);
	output.Write(session.accountName.data(), accountLength);
	output.Write(challengeResponse);
	output.Write(gpciLength);
	output.Write(gpci, gpciLength);
	output.Write(clientVersionLength);
	output.Write(settings.szClientVersion, clientVersionLength);
	session.client->RPC(&RPC_ClientJoin, &output, HIGH_PRIORITY, RELIABLE, 0,
		FALSE, UNASSIGNED_NETWORK_ID, nullptr);
	session.state = LoadSessionState::Joined;
}

unsigned char PacketIdentifier(Packet *packet)
{
	if(packet->length == 0)
		return 0;
	if(packet->data[0] != ID_TIMESTAMP)
		return packet->data[0];
	const std::size_t offset = sizeof(unsigned char) + sizeof(unsigned int);
	return packet->length > offset ? packet->data[offset] : 0;
}

void ProcessNetwork(LoadSession &session)
{
	if(session.client == nullptr || session.state == LoadSessionState::Failed)
		return;

	while(true)
	{
		currentSession = &session;
		Packet *packet = session.client->Receive();
		currentSession = nullptr;
		if(packet == nullptr)
			break;

		switch(PacketIdentifier(packet))
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				SendClientJoin(session, packet);
				break;
			case ID_AUTH_KEY:
				SendAuthKey(session, packet);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				RetryOrFail(session, "server closed the connection");
				break;
			case ID_CONNECTION_BANNED:
				Fail(session, "server banned the connection");
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				RetryOrFail(session, "connection attempt failed");
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				Fail(session, "server has no free player slots");
				break;
			case ID_INVALID_PASSWORD:
				Fail(session, "invalid server password");
				break;
			case ID_CONNECTION_LOST:
				RetryOrFail(session, "connection lost");
				break;
		}
		session.client->DeallocatePacket(packet);
		if(session.state == LoadSessionState::Retrying)
		{
			DestroyClient(session);
			break;
		}
	}
}

BYTE ClampedVital(float value)
{
	return static_cast<BYTE>(std::lround(std::clamp(value, 0.0f, 100.0f)));
}

void SendSync(
	LoadSession &session,
	Clock::time_point now,
	int syncRate,
	bool antiCheatProbeEnabled)
{
	if(session.client == nullptr ||
		session.state == LoadSessionState::Failed ||
		now < session.nextSyncAt)
		return;

	const auto interval = std::chrono::microseconds(1000000 / syncRate);
	session.nextSyncAt = now + interval;
	if(session.spectating)
	{
		SPECTATOR_SYNC_DATA sync{};
		std::memcpy(sync.vecPos, session.position, sizeof(session.position));
		RakNet::BitStream output;
		output.Write(static_cast<BYTE>(ID_SPECTATOR_SYNC));
		output.Write(reinterpret_cast<PCHAR>(&sync), sizeof(sync));
		session.client->Send(&output, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
		return;
	}
	if(session.state != LoadSessionState::Active)
		return;

	ONFOOT_SYNC_DATA sync{};
	std::memcpy(sync.vecPos, session.position, sizeof(session.position));
	sync.fQuaternion[0] = 1.0f;
	sync.byteHealth = ClampedVital(session.health);
	sync.byteArmour = ClampedVital(session.armour);
	if(antiCheatProbeEnabled)
	{
		if(!session.probeAnnounced)
		{
			session.probeAnnounced = true;
			std::printf(
				"[LOAD] probe account=%s index=%zu local-port=%hu\n",
				session.accountName.c_str(),
				session.index,
				session.localPort);
			std::fflush(stdout);
		}
		// The roleplay load harness uses these intentionally impossible values
		// to prove movement, vitals, and weapon corrections under concurrency.
		sync.byteHealth = 255;
		sync.byteArmour = 100;
		sync.byteCurrentWeapon = 24;
		sync.vecMoveSpeed[0] = 10.0f;
	}
	RakNet::BitStream output;
	output.Write(static_cast<BYTE>(ID_PLAYER_SYNC));
	output.Write(reinterpret_cast<PCHAR>(&sync), sizeof(sync));
	session.client->Send(&output, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}

std::size_t CountState(
	const std::vector<std::unique_ptr<LoadSession>> &sessions,
	LoadSessionState state)
{
	return static_cast<std::size_t>(std::count_if(
		sessions.begin(),
		sessions.end(),
		[state](const std::unique_ptr<LoadSession> &session)
		{
			return session->state == state;
		}));
}

void InterruptHandler(int)
{
	interrupted = 1;
}
}

int RunLoadMode(const LoadModeOptions &options)
{
	std::vector<std::unique_ptr<LoadSession>> sessions;
	sessions.reserve(static_cast<std::size_t>(options.clientCount));
	for(int index = 0; index < options.clientCount; ++index)
	{
		auto session = std::make_unique<LoadSession>();
		session->index = static_cast<std::size_t>(index);
		session->accountName = MakeLoadAccountName(options, session->index);
		session->characterName = MakeLoadCharacterName(options, session->index);
		session->directCharacter = options.directCharacter;
		session->password = options.password;
		sessions.push_back(std::move(session));
	}

	interrupted = 0;
	std::signal(SIGINT, InterruptHandler);
	std::signal(SIGTERM, InterruptHandler);
	const auto startedAt = Clock::now();
	auto nextConnectionAt = startedAt;
	auto nextProgressAt = startedAt;
	Clock::time_point soakStartedAt{};
	Clock::time_point coordinatedWaitStartedAt{};
	std::size_t nextSession = 0;
	bool coordinatedStartFailed = false;
	const auto connectionInterval = std::chrono::microseconds(
		1000000 / options.connectRatePerSecond);

	std::printf(
		"[LOAD] Starting %d clients at %d/s, %d sync/s, protocol %s, "
		"anti-cheat probes=%d\n",
		options.clientCount,
		options.connectRatePerSecond,
		options.syncRatePerSecond,
		SampProtocolName(settings.protocol),
		options.antiCheatProbeClients);
	std::fflush(stdout);

	while(!interrupted)
	{
		const auto now = Clock::now();
		for(auto &session : sessions)
		{
			ProcessNetwork(*session);
			const bool probeEnabled =
				soakStartedAt != Clock::time_point{} &&
				session->index <
					static_cast<std::size_t>(options.antiCheatProbeClients);
			SendSync(
				*session,
				now,
				options.syncRatePerSecond,
				probeEnabled);
		}

		const std::size_t failed = CountState(sessions, LoadSessionState::Failed);
		const std::size_t active = CountState(sessions, LoadSessionState::Active);
		if(failed > 0)
			break;

		// open.mp intentionally permits only one in-flight handshake per
		// non-loopback source IP. Docker Desktop presents all native clients as
		// one gateway IP, so gate only the handshake while allowing every
		// connected session to authenticate and sync concurrently.
		const bool handshakeInFlight =
			CountState(sessions, LoadSessionState::Connecting) > 0;
		if(!handshakeInFlight && now >= nextConnectionAt)
		{
			auto retry = std::find_if(
				sessions.begin(),
				sessions.end(),
				[now](const std::unique_ptr<LoadSession> &session)
				{
					return session->state == LoadSessionState::Retrying &&
						now >= session->retryAt;
				});
			LoadSession *startedSession = nullptr;
			if(retry != sessions.end())
			{
				StartSession(**retry);
				startedSession = retry->get();
			}
			else if(nextSession < sessions.size())
			{
				StartSession(*sessions[nextSession]);
				startedSession = sessions[nextSession].get();
				++nextSession;
			}
			if(startedSession != nullptr && startedSession->localPort != 0)
			{
				const auto duplicate = std::find_if(
					sessions.begin(),
					sessions.end(),
					[startedSession](const std::unique_ptr<LoadSession> &other)
					{
						return other.get() != startedSession &&
							other->localPort == startedSession->localPort;
					});
				if(duplicate != sessions.end())
				{
					Fail(
						*startedSession,
						"duplicate local UDP port " +
							std::to_string(startedSession->localPort));
				}
			}
			nextConnectionAt = now + connectionInterval;
		}

		if(active == sessions.size() && soakStartedAt == Clock::time_point{})
		{
			if(options.startFile.empty())
			{
				soakStartedAt = now;
				std::printf("[LOAD] READY active=%zu; beginning %d-second soak\n",
					active, options.durationSeconds);
				std::fflush(stdout);
			}
			else
			{
				if(coordinatedWaitStartedAt == Clock::time_point{})
				{
					coordinatedWaitStartedAt = now;
					std::printf(
						"[LOAD] READY active=%zu; waiting for coordinated start\n",
						active);
					std::fflush(stdout);
				}

				std::error_code error;
				const bool startSignalled =
					std::filesystem::exists(options.startFile, error);
				if(error)
				{
					std::fprintf(
						stderr,
						"[LOAD] FAIL cannot inspect start file: %s\n",
						error.message().c_str());
					coordinatedStartFailed = true;
					break;
				}
				if(startSignalled)
				{
					soakStartedAt = now;
					std::printf(
						"[LOAD] START active=%zu; beginning %d-second soak\n",
						active,
						options.durationSeconds);
					std::fflush(stdout);
				}
				else if(
					now - coordinatedWaitStartedAt >=
					std::chrono::seconds(options.readyTimeoutSeconds))
				{
					std::fprintf(
						stderr,
						"[LOAD] FAIL coordinated start timeout\n");
					coordinatedStartFailed = true;
					break;
				}
			}
		}
		if(soakStartedAt != Clock::time_point{} &&
			now - soakStartedAt >= std::chrono::seconds(options.durationSeconds))
			break;
		if(active != sessions.size() &&
			now - startedAt >= std::chrono::seconds(options.readyTimeoutSeconds))
		{
			for(auto &session : sessions)
			{
				if(session->state != LoadSessionState::Active &&
					session->state != LoadSessionState::Failed)
					Fail(*session, "readiness timeout");
			}
			break;
		}

		if(now >= nextProgressAt &&
			soakStartedAt == Clock::time_point{} &&
			coordinatedWaitStartedAt == Clock::time_point{})
		{
			const std::size_t started = sessions.size() -
				CountState(sessions, LoadSessionState::Pending);
			int attempts = 0;
			for(const auto &session : sessions)
				attempts += session->connectionAttempts;
			const int retries =
				(std::max)(0, attempts - static_cast<int>(started));
			std::printf(
				"[LOAD] progress started=%zu active=%zu failed=%zu retries=%d\n",
				started, active, failed, retries);
			std::fflush(stdout);
			nextProgressAt = now + std::chrono::seconds(1);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	const std::size_t active = CountState(sessions, LoadSessionState::Active);
	const std::size_t failed = CountState(sessions, LoadSessionState::Failed);
	if(interrupted)
	{
		std::fprintf(stderr, "[LOAD] INTERRUPTED active=%zu/%zu\n",
			active, sessions.size());
		return 130;
	}
	if(coordinatedStartFailed || failed > 0 || active != sessions.size())
	{
		std::fprintf(stderr, "[LOAD] FAIL active=%zu/%zu failed=%zu\n",
			active, sessions.size(), failed);
		int printed = 0;
		for(const auto &session : sessions)
		{
			if(session->state == LoadSessionState::Failed && printed++ < 10)
				std::fprintf(stderr, "[LOAD]   %s: %s\n",
					session->accountName.c_str(), session->failure.c_str());
		}
		if(failed > 10)
			std::fprintf(stderr, "[LOAD]   ... and %zu more failures\n",
				failed - 10);
		return 1;
	}

	std::printf("[LOAD] PASS clients=%zu active=%zu duration=%ds\n",
		sessions.size(), active, options.durationSeconds);
	return 0;
}
