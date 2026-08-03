/*
	Updated to 0.3.7 by P3ti
*/

#include "main.h"
#include "checked_reader.h"
#include "client_lifecycle.h"
#include "safe_parse.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

int iNetModeNormalOnfootSendRate, iNetModeNormalIncarSendRate, iNetModeFiringSendRate, iNetModeSendMultiplier;

char g_szHostName[256];
BYTE m_bLagCompensation;

PLAYERID imitateID = -1;
bool iGettingNewName=false;

int iMoney, iDrunkLevel, iLocalPlayerSkin;

struct stGTAMenu GTAMenu;

struct stSAMPDialog sampDialog;
HFONT hSAMPDlgFont = NULL;
HANDLE hDlgThread = NULL;
HWND hwndSAMPDlg = NULL;

PLAYER_SPAWN_INFO SpawnInfo;

BOOL bIsSpectating = 0;
static int RPC_ModelRequest = 179;
static int RPC_FinishDownload = 184;
static int RPC_DownloadCompleted = 185;

static void SendFinishedDownloading()
{
	if(settings.protocol != SampProtocol::V03DL)
		return;
	RakNet::BitStream empty;
	pRakClient->RPC(&RPC_FinishDownload, &empty, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

static void ModelRequest(RPCParameters *rpcParams)
{
	RakNet::BitStream data(reinterpret_cast<unsigned char *>(rpcParams->input),
		(rpcParams->numberOfBitsOfData / 8) + 1, false);
	DWORD poolId = 0;
	int count = 0;
	if(!data.Read(poolId) || !data.Read(count))
		return;
	if(count <= 0 || poolId + 1 >= static_cast<DWORD>(count))
		SendFinishedDownloading();
}

static void DownloadCompleted(RPCParameters *)
{
	Log("[0.3DL] Custom model metadata handshake complete (assets not stored).");
}

void ServerJoin(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	PLAYERID playerId = 0;
	int iUnk = 0;
	BYTE bIsNPC = 0;
	std::string playerName;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	if(!reader.Read(playerId, iUnk, bIsNPC) ||
		!reader.String8(playerName, sizeof(playerInfo[0].szPlayerName) - 1))
		return;
	
	if(playerId < 0 || playerId >= MAX_PLAYERS) return;

	playerInfo[playerId].iIsConnected = 1;
	playerInfo[playerId].byteIsNPC = bIsNPC;
	if(!raksamp::parse::Copy(
		playerInfo[playerId].szPlayerName,
		playerName.c_str()))
		return;

	//Log("***[JOIN] (%d) %s", playerId, szPlayerName);
}

void ServerQuit(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	PLAYERID playerId = 0;
	BYTE byteReason = 0;
	if(!bsData.Read(playerId) || !bsData.Read(byteReason))
		return;

	if(playerId < 0 || playerId >= MAX_PLAYERS) return;

	playerInfo[playerId].iIsConnected = 0;
	playerInfo[playerId].byteIsNPC = 0;
	//Log("***[QUIT:%d] (%d) %s", byteReason, playerId, playerInfo[playerId].szPlayerName);
	memset(playerInfo[playerId].szPlayerName, 0, 20);	
}

void InitGame(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsInitGame((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYERID MyPlayerID = 0;
	bool bLanMode = false, bStuntBonus = false;
	BYTE byteVehicleModels[212] = {};

	bool m_bZoneNames, m_bUseCJWalk, m_bAllowWeapons, m_bLimitGlobalChatRadius;
	float m_fGlobalChatRadius, m_fNameTagDrawDistance;
	bool m_bDisableEnterExits, m_bNameTagLOS, m_bManualVehicleEngineAndLight;
	bool m_bShowPlayerTags;
	int m_iShowPlayerMarkers;
	BYTE m_byteWorldTime, m_byteWeather;
	float m_fGravity;
	int m_iDeathDropMoney;
	bool m_bInstagib;

	int spawnsAvailable = 0;
	int normalOnfootSendRate = 0;
	int normalIncarSendRate = 0;
	int firingSendRate = 0;
	int sendMultiplier = 0;
	BYTE lagCompensation = 0;
	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsInitGame);
	if(!bsInitGame.ReadCompressed(m_bZoneNames) ||
		!bsInitGame.ReadCompressed(m_bUseCJWalk) ||
		!bsInitGame.ReadCompressed(m_bAllowWeapons) ||
		!bsInitGame.ReadCompressed(m_bLimitGlobalChatRadius) ||
		!reader.Finite(m_fGlobalChatRadius) ||
		!bsInitGame.ReadCompressed(bStuntBonus) ||
		!reader.Finite(m_fNameTagDrawDistance) ||
		!bsInitGame.ReadCompressed(m_bDisableEnterExits) ||
		!bsInitGame.ReadCompressed(m_bNameTagLOS) ||
		!bsInitGame.ReadCompressed(m_bManualVehicleEngineAndLight) ||
		!reader.Read(spawnsAvailable, MyPlayerID) ||
		MyPlayerID >= MAX_PLAYERS ||
		!bsInitGame.ReadCompressed(m_bShowPlayerTags) ||
		!reader.Read(m_iShowPlayerMarkers, m_byteWorldTime, m_byteWeather) ||
		!reader.Finite(m_fGravity) ||
		!bsInitGame.ReadCompressed(bLanMode) ||
		!reader.Read(m_iDeathDropMoney) ||
		!bsInitGame.ReadCompressed(m_bInstagib))
		return;

	// Server's send rate restrictions
	if(!reader.Read(normalOnfootSendRate, normalIncarSendRate,
		firingSendRate, sendMultiplier, lagCompensation))
		return;

	BYTE unknown1 = 0, unknown2 = 0, unknown3 = 0;
	std::string hostName;
	if(!reader.Read(unknown1, unknown2, unknown3) ||
		!reader.String8(hostName, sizeof(g_szHostName) - 1) ||
		!reader.Bytes(reinterpret_cast<char *>(byteVehicleModels),
			sizeof(byteVehicleModels)))
		return;
	if(!settings.uiForceCustomSendRates)
	{
		if(normalOnfootSendRate < 1 || normalOnfootSendRate > 1000 ||
			normalIncarSendRate < 1 || normalIncarSendRate > 1000 ||
			firingSendRate < 1 || firingSendRate > 1000 ||
			sendMultiplier < 1 || sendMultiplier > 100)
			return;
		iNetModeNormalOnfootSendRate = normalOnfootSendRate;
		iNetModeNormalIncarSendRate = normalIncarSendRate;
		iNetModeFiringSendRate = firingSendRate;
		iNetModeSendMultiplier = sendMultiplier;
	}

	iSpawnsAvailable = spawnsAvailable;
	m_bLagCompensation = lagCompensation;
	raksamp::parse::Copy(g_szHostName, hostName.c_str());
	g_myPlayerID = MyPlayerID;

	char szTitle[64];
	if(settings.iConsole)
	{
		snprintf(szTitle, sizeof(szTitle), "%s (%d) - %.16s - RakSAMP %s", g_szNickName, g_myPlayerID, g_szHostName, RAKSAMP_VERSION);
		SetConsoleTitle(szTitle);
		Log("Connected to %.64s\n", g_szHostName);
	}
	else
	{
		snprintf(szTitle, sizeof(szTitle), "%s (%d) - RakSAMP %s", g_szNickName, g_myPlayerID, RAKSAMP_VERSION);
		SetWindowText(hwnd, szTitle);
		Log("Connected to %.64s", g_szHostName);
	}

	iGameInited = 1;
	SendFinishedDownloading();
}

void WorldPlayerAdd(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYERID playerId = 0;
	BYTE byteFightingStyle=4;
	BYTE byteTeam=0;
	int iSkin=0;
	float vecPos[3] = {};
	float fRotation=0;
	DWORD dwColor=0;

	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	if(!reader.Read(playerId, byteTeam, iSkin))
		return;
	if(settings.protocol == SampProtocol::V03DL)
	{
		DWORD customSkin = 0;
		if(!reader.Read(customSkin))
			return;
	}
	if(!reader.Finite3(vecPos) || !reader.Finite(fRotation) ||
		!reader.Read(dwColor, byteFightingStyle))
		return;

	if(playerId < 0 || playerId >= MAX_PLAYERS) return;

	playerInfo[playerId].iIsStreamedIn = 1;
	playerInfo[playerId].onfootData.vecPos[0] = 
	playerInfo[playerId].incarData.vecPos[0] = vecPos[0];
	playerInfo[playerId].onfootData.vecPos[1] = 
	playerInfo[playerId].incarData.vecPos[1] = vecPos[1];
	playerInfo[playerId].onfootData.vecPos[2] =
	playerInfo[playerId].incarData.vecPos[2] = vecPos[2];

	//Log("[WORLD ADD] Player [%d]", playerId);
}

void WorldPlayerDeath(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYERID playerId = 0;
	if(!bsData.Read(playerId)) return;

	if(playerId < 0 || playerId >= MAX_PLAYERS) return;

	//Log("[PLAYER_DEATH] %d", playerId);
}

void WorldPlayerRemove(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYERID playerId=0;
	if(!bsData.Read(playerId)) return;

	if(playerId < 0 || playerId >= MAX_PLAYERS) return;

	playerInfo[playerId].iIsStreamedIn = 0;
	playerInfo[playerId].incarData.vecPos[0] = 0.0f;
	playerInfo[playerId].incarData.vecPos[1] = 0.0f;
	playerInfo[playerId].incarData.vecPos[2] = 0.0f;

	//Log("[PLAYER_REMOVE] %d", playerId);
}

void WorldVehicleAdd(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	NEW_VEHICLE NewVehicle = {};
	if(!bsData.Read((char *)&NewVehicle,sizeof(NEW_VEHICLE))) return;

	if(NewVehicle.VehicleId < 0 || NewVehicle.VehicleId >= MAX_VEHICLES ||
		!std::isfinite(NewVehicle.vecPos[0]) ||
		!std::isfinite(NewVehicle.vecPos[1]) ||
		!std::isfinite(NewVehicle.vecPos[2])) return;

	vehiclePool[NewVehicle.VehicleId].iDoesExist = 1;
	vehiclePool[NewVehicle.VehicleId].fPos[0] = NewVehicle.vecPos[0];
	vehiclePool[NewVehicle.VehicleId].fPos[1] = NewVehicle.vecPos[1];
	vehiclePool[NewVehicle.VehicleId].fPos[2] = NewVehicle.vecPos[2];
	vehiclePool[NewVehicle.VehicleId].iModelID = NewVehicle.iVehicleType;

	//Log("[VEHICLE_ADD:%d] ModelID: %d, Position: %0.2f, %0.2f, %0.2f",
	//	NewVehicle.VehicleId, NewVehicle.iVehicleType, NewVehicle.vecPos[0], NewVehicle.vecPos[1], NewVehicle.vecPos[2]);
}

void WorldVehicleRemove(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleID;

	if(!bsData.Read(VehicleID)) return;

	if(VehicleID < 0 || VehicleID >= MAX_VEHICLES) return;

	vehiclePool[VehicleID].iDoesExist = 0;
	vehiclePool[VehicleID].fPos[0] = 0.0f;
	vehiclePool[VehicleID].fPos[1] = 0.0f;
	vehiclePool[VehicleID].fPos[2] = 0.0f;

	//Log("[VEHICLE_REMOVE] %d", VehicleID);
}

void ConnectionRejected(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	BYTE byteRejectReason;

	if(!bsData.Read(byteRejectReason)) return;

	if(byteRejectReason==REJECT_REASON_BAD_VERSION)
	{
		Log("[RAKSAMP] Bad SA-MP version.");
	}
	else if(byteRejectReason==REJECT_REASON_BAD_NICKNAME)
	{
		char szNewNick[32], randgen[5];

		iGettingNewName = true;

		gen_random(randgen, 4);
		snprintf(szNewNick, sizeof(szNewNick), "%.26s_%s", g_szNickName, randgen);

		Log("[RAKSAMP] Bad nickname. Changing name to %s", szNewNick);

		snprintf(g_szNickName, sizeof(g_szNickName), "%s", szNewNick);
		resetPools(1, 0);
	}
	else if(byteRejectReason==REJECT_REASON_BAD_MOD)
	{
		Log("[RAKSAMP] Bad mod version.");
	}
	else if(byteRejectReason==REJECT_REASON_BAD_PLAYERID)
	{
		Log("[RAKSAMP] Bad player ID.");
	}
	else
		Log("ConnectionRejected: unknown");
}

void ClientMessage(RPCParameters *rpcParams)
{
	//if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	DWORD dwStrLen, dwColor;
	char szMsg[257];
	memset(szMsg, 0, 257);

	if(!bsData.Read(dwColor) || !bsData.Read(dwStrLen) || dwStrLen > 256 ||
		!bsData.Read(szMsg, dwStrLen))
		return;
	szMsg[dwStrLen] = 0;

	if(settings.iFind)
	{
		for(int i = 0; i < MAX_FIND_ITEMS; i++)
		{
			if(!settings.findItems[i].iExists)
				continue;

			if(strstr(szMsg, settings.findItems[i].szFind))
				if(settings.findItems[i].szSay[0] != 0x00)
					sendChat(settings.findItems[i].szSay);
		}
	}

	char szNonColorEmbeddedMsg[257];
	int iNonColorEmbeddedMsgLen = 0;

	for (size_t pos = 0; pos < strlen(szMsg) && szMsg[pos] != '\0'; pos++)
	{
		if (!((*(unsigned char*)(&szMsg[pos]) - 32) >= 0 && (*(unsigned char*)(&szMsg[pos]) - 32) < 224))
			continue;

		if(pos+7 < strlen(szMsg))
		{
			if (szMsg[pos] == '{' && szMsg[pos+7] == '}')
			{
				pos += 7;
				continue;
			}
		}

		szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = szMsg[pos];
		iNonColorEmbeddedMsgLen++;
	}

	szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = 0;

	Log("[CMSG] %s", szNonColorEmbeddedMsg);
}

void Chat(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	PLAYERID playerId;
	BYTE byteTextLen;

	unsigned char szText[256];
	memset(szText, 0, 256);

	if(!bsData.Read(playerId) || !bsData.Read(byteTextLen) ||
		!bsData.Read((char*)szText, byteTextLen))
		return;
	szText[byteTextLen] = 0;

	if(playerId < 0 || playerId >= MAX_PLAYERS)
		return;

	if(imitateID == playerId)
		sendChat((char *)szText);

	Log("[CHAT] %s: %s", playerInfo[playerId].szPlayerName, szText);

	if(settings.iFind)
	{
		for(int i = 0; i < MAX_FIND_ITEMS; i++)
		{
			if(!settings.findItems[i].iExists)
				continue;

			if(strstr((const char *)szText, settings.findItems[i].szFind))
			{
				if(settings.findItems[i].szSay[0] != 0x00)
					sendChat(settings.findItems[i].szSay);
			}
		}
	}
}

void UpdateScoresPingsIPs(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	int iPlayerScore;
	DWORD dwPlayerPing;

	for(PLAYERID i=0; i<(iBitLength/8)/9; i++)
	{
		if(!bsData.Read(playerId) || !bsData.Read(iPlayerScore) ||
			!bsData.Read(dwPlayerPing))
			return;

		if(playerId < 0 || playerId >= MAX_PLAYERS)
			continue;

		playerInfo[playerId].iScore = iPlayerScore;
		playerInfo[playerId].dwPing = dwPlayerPing;
	}
}

void SetCheckpoint(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	float position[3] = {};
	float size = 0.0f;
	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	if(!reader.Finite3(position) || !reader.Finite(size) || size <= 0.0f)
		return;
	for(std::size_t index = 0; index < 3; ++index)
		settings.CurrentCheckpoint.fPosition[index] = position[index];
	settings.CurrentCheckpoint.fSize = size;

	settings.CurrentCheckpoint.bActive = true;

	char SetCheckpointAlert[256];
	sprintf_s(SetCheckpointAlert, 256, "[CP] Checkpoint set to %.2f %.2f %.2f position. (size: %.2f)", settings.CurrentCheckpoint.fPosition[0], settings.CurrentCheckpoint.fPosition[1], settings.CurrentCheckpoint.fPosition[2], settings.CurrentCheckpoint.fSize);
	Log(SetCheckpointAlert);
}

void DisableCheckpoint(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	settings.CurrentCheckpoint.bActive = false;

	Log("[CP] Current checkpoint disabled.");
}

void Pickup(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	int PickupID = 0;
	PICKUP Pickup = {};

	if(!bsData.Read(PickupID) || !bsData.Read((PCHAR)&Pickup, sizeof(PICKUP)) ||
		!std::isfinite(Pickup.fX) || !std::isfinite(Pickup.fY) ||
		!std::isfinite(Pickup.fZ)) return;

	if(settings.uiPickupsLogging != 0)
	{
		char szCreatePickupAlert[256];
		sprintf_s(szCreatePickupAlert, sizeof(szCreatePickupAlert), "[CREATEPICKUP] ID: %d | Model: %d | Type: %d | X: %.2f | Y: %.2f | Z: %.2f", PickupID, Pickup.iModel, Pickup.iType, Pickup.fX, Pickup.fY, Pickup.fZ);
		Log(szCreatePickupAlert);
	}
}

void DestroyPickup(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	int PickupID;

	if(!bsData.Read(PickupID)) return;

	if(settings.uiPickupsLogging != 0)
	{
		Log("[DESTROYPICKUP] %d", PickupID);
	}
}

void RequestClass(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	BYTE byteRequestOutcome = 0;

	if(!bsData.Read(byteRequestOutcome))
		return;

	if(byteRequestOutcome)
	{
		PLAYER_SPAWN_INFO parsed = {};
		if(!bsData.Read(parsed.byteTeam) || !bsData.Read(parsed.iSkin))
			return;
		if(settings.protocol == SampProtocol::V03DL)
		{
			DWORD customSkin = 0;
			if(!bsData.Read(customSkin))
				return;
		}
		if(!bsData.Read(parsed.unk) ||
			!bsData.Read((PCHAR)&parsed.vecPos, sizeof(parsed.vecPos)) ||
			!bsData.Read(parsed.fRotation) ||
			!bsData.Read((PCHAR)&parsed.iSpawnWeapons, sizeof(parsed.iSpawnWeapons)) ||
			!bsData.Read((PCHAR)&parsed.iSpawnWeaponsAmmo, sizeof(parsed.iSpawnWeaponsAmmo)) ||
			!std::isfinite(parsed.vecPos[0]) || !std::isfinite(parsed.vecPos[1]) ||
			!std::isfinite(parsed.vecPos[2]) || !std::isfinite(parsed.fRotation))
			return;
		SpawnInfo = parsed;
		iLocalPlayerSkin = SpawnInfo.iSkin;
	}
}

void ScrInitMenu(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	stGTAMenu parsedMenu = {};

	BYTE byteMenuID;
	BOOL bColumns; // 0 = 1, 1 = 2
	CHAR cText[MAX_MENU_LINE];
	float fX;
	float fY;
	float fCol1;
	float fCol2 = 0.0;
	MENU_INT MenuInteraction;

	if(!bsData.Read(byteMenuID) || !bsData.Read(bColumns) ||
		!bsData.Read(cText, MAX_MENU_LINE) ||
		!bsData.Read(fX) || !bsData.Read(fY) || !bsData.Read(fCol1) ||
		(bColumns && !bsData.Read(fCol2)) ||
		!bsData.Read(MenuInteraction.bMenu))
		return;
	for (BYTE i = 0; i < MAX_MENU_ITEMS; i++)
		if(!bsData.Read(MenuInteraction.bRow[i])) return;

	cText[MAX_MENU_LINE - 1] = '\0';
	Log("[MENU] %s", cText);
	raksamp::parse::Copy(parsedMenu.szTitle, cText);

	BYTE byteColCount;
	if(!bsData.Read(cText, MAX_MENU_LINE)) return;
	cText[MAX_MENU_LINE - 1] = '\0';
	Log("[MENU] %s", cText);
	raksamp::parse::Copy(parsedMenu.szSeparator, cText);

	if(!bsData.Read(byteColCount) || byteColCount > MAX_MENU_ITEMS) return;
	parsedMenu.byteColCount = byteColCount;
	for (BYTE i = 0; i < byteColCount; i++)
	{
		if(!bsData.Read(cText, MAX_MENU_LINE)) return;
		cText[MAX_MENU_LINE - 1] = '\0';
		Log("[MENU:%d] %s", i, cText);
		raksamp::parse::Copy(parsedMenu.szColumnContent[i], cText);
	}

	if (bColumns)
	{
		if(!bsData.Read(cText, MAX_MENU_LINE)) return;
		//Log("4: %s", cText);

		if(!bsData.Read(byteColCount) || byteColCount > MAX_MENU_ITEMS) return;
		for (BYTE i = 0; i < byteColCount; i++)
		{
			if(!bsData.Read(cText, MAX_MENU_LINE)) return;
			//Log("5: %d %s", i, cText);
		}
	}
	GTAMenu = parsedMenu;
}

#ifdef _WIN32
LRESULT CALLBACK SAMPDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndEditBox = GetDlgItem(hwnd, IDE_INPUTEDIT);
	HWND hwndListBox = GetDlgItem(hwnd, IDL_LISTBOX);
	WORD wSelection;
	char szResponse[257];

	switch(msg)
	{
	case WM_CREATE:
		{
			HINSTANCE hInst = GetModuleHandle(NULL);
			switch(sampDialog.bDialogStyle)
			{
				case DIALOG_STYLE_MSGBOX:
					if(sampDialog.bButton1Len == 0 && sampDialog.bButton2Len == 0)
					{
						// no butans, no badi cana cross it
					}
					if(sampDialog.bButton1Len != 0 && sampDialog.bButton2Len == 0) // a butan
					{
						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton1, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							150, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON1, hInst, NULL);
					}
					else if(sampDialog.bButton1Len != 0 && sampDialog.bButton2Len != 0) // tu butans
					{
						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton1, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							100, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON1, hInst, NULL);

						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton2, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							210, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON2, hInst, NULL);
					}

					break;

				case DIALOG_STYLE_INPUT:
				case DIALOG_STYLE_PASSWORD:
					{
						CreateWindowEx(NULL, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
							50, 200, 300, 24, hwnd, (HMENU)IDE_INPUTEDIT, hInst, NULL);

						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton1, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							100, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON1, hInst, NULL);

						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton2, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							210, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON2, hInst, NULL);
					}

					break;

				case DIALOG_STYLE_LIST:
					{
						hwndListBox = CreateWindowEx(NULL, "LISTBOX", "",
							WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS,
							10, 10, 375, 225, hwnd, (HMENU)IDL_LISTBOX, hInst, NULL);

						char *szInfoTemp = strtok(sampDialog.szInfo, "\n");
						while(szInfoTemp != NULL)
						{
							int id = SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)szInfoTemp);
							SendMessage(hwndListBox, LB_SETITEMDATA, id, (LPARAM)id);

							szInfoTemp = strtok(NULL, "\n");
						}
						
						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton1, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							100, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON1, hInst, NULL);

						CreateWindowEx(NULL, "BUTTON", sampDialog.szButton2, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							210, 230, 100, 24, hwnd, (HMENU)IDB_BUTTON2, hInst, NULL);
					}

					break;
			}

		}
		break;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDB_BUTTON1:
					if(sampDialog.bDialogStyle == DIALOG_STYLE_LIST)
					{
						wSelection = (WORD)SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);
						if(wSelection != (WORD)-1)
						{
							SendMessage(hwndListBox, LB_GETTEXT, wSelection, (LPARAM)szResponse);
							sendDialogResponse(sampDialog.wDialogID, 1, 0, szResponse);
							PostQuitMessage(0);
						}
						break;
					}

					GetWindowText(hwndEditBox, szResponse, 257);
					sendDialogResponse(sampDialog.wDialogID, 1, 0, szResponse);
					PostQuitMessage(0);
					break;

				case IDB_BUTTON2:
					GetWindowText(hwndEditBox, szResponse, 257);
					sendDialogResponse(sampDialog.wDialogID, 0, 0, szResponse);
					PostQuitMessage(0);
					break;
			}
		}

		break;

	case WM_PAINT:
		{
			if(sampDialog.bDialogStyle != DIALOG_STYLE_LIST)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				HDC hdcMem = CreateCompatibleDC(hdc);
				SelectObject(hdc, hSAMPDlgFont);
				DrawText(hdc, sampDialog.szInfo, strlen(sampDialog.szInfo), &rect, DT_WORDBREAK | DT_EXPANDTABS);
				DeleteDC(hdcMem);
				EndPaint(hwnd, &ps);
			}
			else
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				HDC hdcMem = CreateCompatibleDC(hdc);
				DeleteDC(hdcMem);
				EndPaint(hwnd, &ps);
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

DWORD WINAPI DialogBoxThread(PVOID)
{
	WNDCLASSEX wc;
	MSG Msg;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	RECT conRect;
	if(settings.iConsole)
		GetWindowRect(GetConsoleWindow(), &conRect);
	else
		GetWindowRect(hwnd, &conRect);

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = SAMPDlgProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "dlgWndClass";
	wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
		return 0;

	hSAMPDlgFont = CreateFont(18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Tahoma");

	hwndSAMPDlg = CreateWindowEx(NULL, "dlgWndClass", sampDialog.szTitle, NULL,
		conRect.right, conRect.top, 400, 300, NULL, NULL, hInstance, NULL);

	if(hwndSAMPDlg == NULL)
		return 0;

	ShowWindow(hwndSAMPDlg, 1);
	UpdateWindow(hwndSAMPDlg);
	SetForegroundWindow(hwndSAMPDlg);

	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	sampDialog.iIsActive = 0;
	SendMessage(hwndSAMPDlg, WM_DESTROY, 0, 0);
	DestroyWindow(hwndSAMPDlg);
	UnregisterClass("dlgWndClass", GetModuleHandle(NULL));
	hSAMPDlgFont = NULL;
	TerminateThread(hDlgThread, 0);

	return 0;
}
#endif

void ScrDialogBox(RPCParameters *rpcParams)
{
	if(!iGameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	WORD dialogId = 0;
	BYTE style = 0;
	std::string title;
	std::string button1;
	std::string button2;
	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	char information[sizeof(sampDialog.szInfo)] = {};
	if(!reader.Read(dialogId, style) ||
		!reader.String8(title, sizeof(sampDialog.szTitle) - 1) ||
		!reader.String8(button1, sizeof(sampDialog.szButton1) - 1) ||
		!reader.String8(button2, sizeof(sampDialog.szButton2) - 1) ||
		!stringCompressor->DecodeString(information, sizeof(information), &bsData))
		return;
	sampDialog.wDialogID = dialogId;
	sampDialog.bDialogStyle = style;
	sampDialog.bTitleLength = static_cast<BYTE>(title.size());
	sampDialog.bButton1Len = static_cast<BYTE>(button1.size());
	sampDialog.bButton2Len = static_cast<BYTE>(button2.size());
	raksamp::parse::Copy(sampDialog.szTitle, title.c_str());
	raksamp::parse::Copy(sampDialog.szButton1, button1.c_str());
	raksamp::parse::Copy(sampDialog.szButton2, button2.c_str());
	raksamp::parse::Copy(sampDialog.szInfo, information);
	Log("[DIALOG] id=%d style=%d title=%s button1=%s button2=%s info=%s",
		sampDialog.wDialogID, sampDialog.bDialogStyle, sampDialog.szTitle,
		sampDialog.szButton1, sampDialog.szButton2, sampDialog.szInfo);

	switch(sampDialog.bDialogStyle)
	{
		case DIALOG_STYLE_MSGBOX:
		case DIALOG_STYLE_INPUT:
		case DIALOG_STYLE_LIST:
		case DIALOG_STYLE_PASSWORD:
			if(!sampDialog.iIsActive)
			{
				sampDialog.iIsActive = 1;
			#ifdef _WIN32
				hDlgThread = CreateThread(NULL, 0, DialogBoxThread, NULL, 0, NULL);
			#endif
			}
		break;

		default:
			if(sampDialog.iIsActive)
			{
				sampDialog.iIsActive = 0;
			#ifdef _WIN32
				SendMessage(hwndSAMPDlg, WM_DESTROY, 0, 0);
				DestroyWindow(hwndSAMPDlg);
				UnregisterClass("dlgWndClass", GetModuleHandle(NULL));
				hSAMPDlgFont = NULL;
				TerminateThread(hDlgThread, 0);
			#endif
			}
		break;
	}
}

void ScrGameText(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	char szMessage[400];
	int iType, iTime, iLength;

	if(!bsData.Read(iType) || !bsData.Read(iTime) || !bsData.Read(iLength) ||
		iLength < 0 || iLength >= static_cast<int>(sizeof(szMessage)) ||
		!bsData.Read(szMessage, iLength))
		return;
	szMessage[iLength] = '\0';

	Log("[GAMETEXT] %s", szMessage);
}

void ScrPlayAudioStream(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);
	unsigned char bURLLen;
	char szURL[256];

	if(!bsData.Read(bURLLen) || !bsData.Read(szURL, bURLLen))
		return;
	szURL[bURLLen] = 0;

	Log("[AUDIO_STREAM] %s", szURL);
}

void ScrSetDrunkLevel(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	int parsed = 0;
	if(bsData.Read(parsed)) iDrunkLevel = parsed;
}

void ScrHaveSomeMoney(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	int iGivenMoney = 0;
	if(!bsData.Read(iGivenMoney)) return;
	const long long updated = static_cast<long long>(iMoney) + iGivenMoney;
	iMoney = static_cast<int>(std::clamp(
		updated,
		static_cast<long long>(std::numeric_limits<int>::min()),
		static_cast<long long>(std::numeric_limits<int>::max())));
}

void ScrResetMoney(RPCParameters *rpcParams)
{
	iMoney = 0;
}

void ScrResetPlayerWeapons(RPCParameters *rpcParams)
{
	(void)rpcParams;
	ResetWeaponInventory();
}

void ScrGivePlayerWeapon(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(
		reinterpret_cast<unsigned char *>(rpcParams->input),
		(rpcParams->numberOfBitsOfData / 8) + 1,
		false);
	DWORD weaponId = 0;
	DWORD ammo = 0;
	if(bsData.Read(weaponId) && bsData.Read(ammo))
		SetWeaponInventoryEntry(weaponId, ammo);
}

void ScrSetWeaponAmmo(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(
		reinterpret_cast<unsigned char *>(rpcParams->input),
		(rpcParams->numberOfBitsOfData / 8) + 1,
		false);
	BYTE weaponId = 0;
	WORD ammo = 0;
	if(bsData.Read(weaponId) && bsData.Read(ammo))
		SetWeaponInventoryEntry(weaponId, ammo);
}

void ScrSetPlayerPos(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	if(settings.iNormalModePosForce == 0)
	{
		float parsed[3] = {};
		raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
		if(!reader.Finite3(parsed))
			return;
		for(std::size_t index = 0; index < 3; ++index)
			settings.fNormalModePos[index] = parsed[index];
	}
}

void ScrSetPlayerFacingAngle(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	if(settings.iNormalModePosForce == 0)
	{
		float parsed = 0.0f;
		raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
		if(reader.Finite(parsed))
			settings.fNormalModeRot = parsed;
	}
}

void ScrPutPlayerInVehicle(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	VEHICLEID vehicleId;
	BYTE seatId;
	if(!bsData.Read(vehicleId) || !bsData.Read(seatId))
		return;

	NativePutPlayerInVehicle(vehicleId, seatId);
}

void ScrRemovePlayerFromVehicle(RPCParameters *)
{
	NativeRemovePlayerFromVehicle();
}

void ScrSetSpawnInfo(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	PLAYER_SPAWN_INFO parsed = {};
	if(!bsData.Read(parsed.byteTeam) || !bsData.Read(parsed.iSkin))
		return;
	if(settings.protocol == SampProtocol::V03DL)
	{
		DWORD customSkin = 0;
		if(!bsData.Read(customSkin))
			return;
	}
	if(!bsData.Read(parsed.unk) ||
		!bsData.Read((PCHAR)&parsed.vecPos, sizeof(parsed.vecPos)) ||
		!bsData.Read(parsed.fRotation) ||
		!bsData.Read((PCHAR)&parsed.iSpawnWeapons, sizeof(parsed.iSpawnWeapons)) ||
		!bsData.Read((PCHAR)&parsed.iSpawnWeaponsAmmo, sizeof(parsed.iSpawnWeaponsAmmo)) ||
		!std::isfinite(parsed.vecPos[0]) || !std::isfinite(parsed.vecPos[1]) ||
		!std::isfinite(parsed.vecPos[2]) || !std::isfinite(parsed.fRotation))
		return;
	SpawnInfo = parsed;

	if(settings.iNormalModePosForce == 0)
	{
		settings.fNormalModePos[0] = SpawnInfo.vecPos[0];
		settings.fNormalModePos[1] = SpawnInfo.vecPos[1];
		settings.fNormalModePos[2] = SpawnInfo.vecPos[2];
	}
}

void ScrSetPlayerHealth(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	float parsed = 0.0f;
	if(bsData.Read(parsed) && std::isfinite(parsed))
		settings.fPlayerHealth = parsed;
}

void ScrSetPlayerArmour(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	float parsed = 0.0f;
	if(bsData.Read(parsed) && std::isfinite(parsed))
		settings.fPlayerArmour = parsed;
}

void ScrSetPlayerSkin(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	int iPlayerID;
	unsigned int uiSkin;

	if(settings.protocol == SampProtocol::V03DL)
	{
		PLAYERID playerID = 0;
		if(!bsData.Read(playerID)) return;
		iPlayerID = playerID;
	}
	else
		if(!bsData.Read(iPlayerID)) return;
	if(!bsData.Read(uiSkin)) return;
	if(settings.protocol == SampProtocol::V03DL)
	{
		DWORD customSkin = 0;
		if(!bsData.Read(customSkin)) return;
	}

	if(iPlayerID < 0 || iPlayerID >= MAX_PLAYERS)
		return;

	if(iGameInited && g_myPlayerID == iPlayerID)
		iLocalPlayerSkin = uiSkin;
}

void ScrCreateObject(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	unsigned short ObjectID = 0;
	if(!bsData.Read(ObjectID)) return;

	DWORD ModelID = 0;
	if(!bsData.Read(ModelID)) return;

	float vecPos[3] = {};
	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	if(!reader.Finite3(vecPos)) return;

	float vecRot[3] = {};
	if(!reader.Finite3(vecRot)) return;

	float fDrawDistance = 0.0f;
	if(!reader.Finite(fDrawDistance) || fDrawDistance < 0.0f) return;

	if(settings.uiObjectsLogging != 0)
	{
		char szCreateObjectAlert[256];
		sprintf_s(szCreateObjectAlert, sizeof(szCreateObjectAlert), "[OBJECT] %d, %u, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.2f", ObjectID, ModelID, vecPos[0], vecPos[1], vecPos[2], vecRot[0], vecRot[1], vecRot[2], fDrawDistance);
		Log(szCreateObjectAlert);
	}
}

void ScrCreate3DTextLabel(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	WORD ID;
	CHAR Text[256];
	DWORD dwColor;
	FLOAT vecPos[3];
	FLOAT DrawDistance;
	BYTE UseLOS;
	WORD PlayerID;
	WORD VehicleID;

	raksamp::protocol::CheckedReader<RakNet::BitStream> reader(bsData);
	if(!reader.Read(ID, dwColor) || !reader.Finite3(vecPos) ||
		!reader.Finite(DrawDistance) || DrawDistance < 0.0f ||
		!reader.Read(UseLOS, PlayerID, VehicleID) ||
		!stringCompressor->DecodeString(Text, sizeof(Text), &bsData))
		return;

	if(settings.uiTextLabelsLogging != 0)
	{
		char szCreate3DTextLabelAlert[256];
		sprintf_s(szCreate3DTextLabelAlert, sizeof(szCreate3DTextLabelAlert), "[TEXTLABEL] %d - %s (%X, %.3f, %.3f, %.3f, %.2f, %i, %d, %d)", ID, Text, dwColor, vecPos[0], vecPos[1], vecPos[2], DrawDistance, UseLOS, PlayerID, VehicleID);
		Log(szCreate3DTextLabelAlert);
	}
}

void ScrShowTextDraw(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	WORD wTextID;
	TEXT_DRAW_TRANSMIT TextDrawTransmit;

	CHAR cText[1024];
	unsigned short cTextLen = 0;

	if(!bsData.Read(wTextID) ||
		!bsData.Read((PCHAR)&TextDrawTransmit, sizeof(TEXT_DRAW_TRANSMIT)) ||
		!bsData.Read(cTextLen) ||
		cTextLen >= sizeof(cText) ||
		!bsData.Read(cText, cTextLen))
		return;
	cText[cTextLen] = '\0';

	if(settings.uiTextDrawsLogging != 0)
		SaveTextDrawData(wTextID, &TextDrawTransmit, cText);
	
	if(TextDrawTransmit.byteSelectable)
		Log("[SELECTABLE-TEXTDRAW] ID: %d, Text: %s.", wTextID, cText);
}

void ScrHideTextDraw(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	WORD wTextID;
	if(!bsData.Read(wTextID)) return;

	if(settings.uiTextDrawsLogging != 0)
		Log("[TEXTDRAW:HIDE] ID: %d.", wTextID);
}

void ScrEditTextDraw(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	WORD wTextID;
	CHAR cText[1024];
	unsigned short cTextLen = 0;

	if(!bsData.Read(wTextID) ||
		!bsData.Read(cTextLen) ||
		cTextLen >= sizeof(cText) ||
		!bsData.Read(cText, cTextLen))
		return;
	cText[cTextLen] = '\0';

	if(settings.uiTextDrawsLogging != 0)
		Log("[TEXTDRAW:EDIT] ID: %d, Text: %s.", wTextID, cText);
}

void ScrTogglePlayerSpectating(RPCParameters *rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	BOOL bToggle;

	if(!bsData.Read(bToggle))
		return;

	if(ShouldSpawnAfterSpectatorExit(
		bIsSpectating != 0,
		bToggle != 0))
	{
		sampSpawnAfterSpectating();
		iSpawned = 1;
	}

	bIsSpectating = bToggle;
}

void RegisterRPCs(RakClientInterface *pRakClient)
{
	if (pRakClient == ::pRakClient)
	{
		// Core RPCs
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerJoin, ServerJoin);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerQuit, ServerQuit);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_InitGame, InitGame);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd, WorldPlayerAdd);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath, WorldPlayerDeath);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove, WorldPlayerRemove);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd, WorldVehicleAdd);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove, WorldVehicleRemove);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ConnectionRejected, ConnectionRejected);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClientMessage, ClientMessage);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_Chat, Chat);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs, UpdateScoresPingsIPs);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetCheckpoint, SetCheckpoint);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisableCheckpoint, DisableCheckpoint);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_Pickup, Pickup);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_DestroyPickup, DestroyPickup);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_RequestClass, RequestClass);
		if(settings.protocol == SampProtocol::V03DL)
		{
			pRakClient->RegisterAsRemoteProcedureCall(&RPC_ModelRequest, ModelRequest);
			pRakClient->RegisterAsRemoteProcedureCall(&RPC_DownloadCompleted, DownloadCompleted);
		}

		// Scripting RPCs
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrInitMenu, ScrInitMenu);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrDialogBox, ScrDialogBox);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrDisplayGameText, ScrGameText);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_PlayAudioStream, ScrPlayAudioStream);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerDrunkLevel, ScrSetDrunkLevel);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrHaveSomeMoney, ScrHaveSomeMoney);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrResetMoney, ScrResetMoney);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerPos, ScrSetPlayerPos);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerFacingAngle, ScrSetPlayerFacingAngle);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrPutPlayerInVehicle, ScrPutPlayerInVehicle);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrRemovePlayerFromVehicle, ScrRemovePlayerFromVehicle);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetSpawnInfo, ScrSetSpawnInfo);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerHealth, ScrSetPlayerHealth);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrResetPlayerWeapons, ScrResetPlayerWeapons);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrGivePlayerWeapon, ScrGivePlayerWeapon);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetWeaponAmmo, ScrSetWeaponAmmo);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerArmour, ScrSetPlayerArmour);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrSetPlayerSkin, ScrSetPlayerSkin);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrCreateObject, ScrCreateObject);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrCreate3DTextLabel, ScrCreate3DTextLabel);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrShowTextDraw, ScrShowTextDraw);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrHideTextDraw, ScrHideTextDraw);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrEditTextDraw, ScrEditTextDraw);
		pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrTogglePlayerSpectating, ScrTogglePlayerSpectating);
	}
}

void UnRegisterRPCs(RakClientInterface * pRakClient)
{
	if (pRakClient == ::pRakClient)
	{
		// Core RPCs
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerJoin);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerQuit);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_InitGame);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ConnectionRejected);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClientMessage);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Chat);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetCheckpoint);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisableCheckpoint);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Pickup);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DestroyPickup);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RequestClass);
		if(settings.protocol == SampProtocol::V03DL)
		{
			pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ModelRequest);
			pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DownloadCompleted);
		}

		// Scripting RPCs
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrInitMenu);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrDialogBox);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrDisplayGameText);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlayAudioStream);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerDrunkLevel);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrHaveSomeMoney);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrResetMoney);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerPos);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerFacingAngle);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrPutPlayerInVehicle);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrRemovePlayerFromVehicle);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetSpawnInfo);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerHealth);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrResetPlayerWeapons);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrGivePlayerWeapon);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetWeaponAmmo);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerArmour);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrSetPlayerSkin);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrCreateObject);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrCreate3DTextLabel);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrShowTextDraw);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrHideTextDraw);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrEditTextDraw);
		pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrTogglePlayerSpectating);
	}
}
