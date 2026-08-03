#include "main.h"
#include "automation_protocol.h"
#include "key_state.h"
#include <mutex>
#include <queue>
#include <string>
#include <thread>

HWND hwnd = NULL, texthwnd = NULL, loghwnd = NULL, inputhwnd = NULL;
HINSTANCE g_hInst = NULL;
BOOL bTeleportMenuActive = FALSE;
HFONT hTeleportMenuFont = NULL;
HANDLE hTeleportMenuThread = NULL;
HWND hwndTeleportMenu = NULL;
static std::mutex commandMutex;
static std::queue<std::string> commands;
static int forcedVehicleId = -1;
static bool forcedPassenger = false;
static eRunModes forcedPreviousRunMode = RUNMODE_NORMAL;
static AutomationKeyState automationKeyState;

const AutomationKeyState &GetAutomationKeyState()
{
	return automationKeyState;
}

void ResetAutomationKeyState()
{
	automationKeyState = {};
}

static void SendAssignedDriverSync(bool force)
{
	INCAR_SYNC_DATA sync;
	memset(&sync, 0, sizeof(sync));
	sync.VehicleID = (VEHICLEID)forcedVehicleId;
	sync.fQuaternion[0] = 1.0f;
	sync.fCarHealth = 1000.0f;
	sync.bytePlayerHealth = (BYTE)settings.fPlayerHealth;
	sync.bytePlayerArmour = (BYTE)settings.fPlayerArmour;
	ApplyAutomationKeyState(sync, automationKeyState);
	SendInCarFullSyncData(&sync, 1, (PLAYERID)-1, force);
}

static void SendCurrentKeyState()
{
	if(forcedVehicleId >= 0 && forcedVehicleId < MAX_VEHICLES &&
		vehiclePool[forcedVehicleId].iDoesExist)
	{
		if(forcedPassenger)
			SendPassengerFullSyncData((VEHICLEID)forcedVehicleId, true);
		else
			SendAssignedDriverSync(true);
		return;
	}
	onFootUpdateAtNormalPos(true);
}

static bool AssignVehicle(VEHICLEID vehicleId, BYTE seatId)
{
	if(vehicleId >= MAX_VEHICLES)
		return false;

	playerInfo[g_myPlayerID].iAreWeInAVehicle = 1;
	if(forcedVehicleId < 0)
		forcedPreviousRunMode = settings.runMode;
	forcedVehicleId = vehicleId;
	forcedPassenger = seatId != 0;
	// Normal mode emits on-foot sync every frame. Pause that built-in stream
	// while the assigned occupant emits explicit vehicle sync.
	settings.runMode = RUNMODE_STILL;
	return true;
}

void NativePutPlayerInVehicle(VEHICLEID vehicleId, BYTE seatId)
{
	if(!AssignVehicle(vehicleId, seatId))
	{
		Log("[PUT_IN_VEHICLE] Ignored invalid vehicle %d.", vehicleId);
		return;
	}

	Log("[PUT_IN_VEHICLE] Entered vehicle %d as %s.", vehicleId,
		forcedPassenger ? "passenger" : "driver");
}

static int ClearAssignedVehicle()
{
	playerInfo[g_myPlayerID].iAreWeInAVehicle = 0;
	int exitedVehicleId = forcedVehicleId;
	forcedVehicleId = -1;
	forcedPassenger = false;
	settings.runMode = forcedPreviousRunMode;
	return exitedVehicleId;
}

void NativeRemovePlayerFromVehicle()
{
	Log("[REMOVE_FROM_VEHICLE] Exited vehicle %d.", ClearAssignedVehicle());
}

static void *ReadCommands(void *)
{
	char line[512];
	while(fgets(line, sizeof(line), stdin) != NULL)
	{
		std::lock_guard<std::mutex> lock(commandMutex);
		commands.push(line);
	}
	return NULL;
}

void NativePumpCommands()
{
	if(forcedVehicleId >= 0 && forcedVehicleId < MAX_VEHICLES &&
		vehiclePool[forcedVehicleId].iDoesExist)
	{
		if(forcedPassenger)
		{
			SendPassengerFullSyncData((VEHICLEID)forcedVehicleId);
		}
		else
			SendAssignedDriverSync(false);
	}

	std::string command;
	{
		std::lock_guard<std::mutex> lock(commandMutex);
		if(commands.empty())
			return;
		command = commands.front();
		commands.pop();
	}
	char buffer[512];
	strncpy(buffer, command.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	size_t length = strlen(buffer);
	while(length > 0 && (buffer[length - 1] == '\r' || buffer[length - 1] == '\n'))
		buffer[--length] = '\0';

	if(!strcmp(buffer, "!gotocp") && forcedVehicleId >= 0 && !forcedPassenger)
	{
		if(!settings.CurrentCheckpoint.bActive)
		{
			Log("[GOTOCP] There is no active checkpoint.");
			return;
		}
		settings.fNormalModePos[0] = settings.CurrentCheckpoint.fPosition[0];
		settings.fNormalModePos[1] = settings.CurrentCheckpoint.fPosition[1];
		settings.fNormalModePos[2] = settings.CurrentCheckpoint.fPosition[2];
		vehiclePool[forcedVehicleId].fPos[0] = settings.CurrentCheckpoint.fPosition[0];
		vehiclePool[forcedVehicleId].fPos[1] = settings.CurrentCheckpoint.fPosition[1];
		vehiclePool[forcedVehicleId].fPos[2] = settings.CurrentCheckpoint.fPosition[2];
		Log("[GOTOCP] The driven vehicle has been teleported to the active checkpoint.");
		return;
	}
	AutomationCommand automation;
	std::string automationError;
	const AutomationParseResult automationResult = ParseAutomationCommand(
		buffer,
		automation,
		automationError);
	if(automationResult == AutomationParseResult::Error)
	{
		Log("[AUTOMATION] %s", automationError.c_str());
		return;
	}
	if(automationResult == AutomationParseResult::Command)
	{
		if(automation.command == "capabilities")
		{
			EmitAutomationCapabilities(
				RAKSAMP_VERSION,
				SampProtocolName(settings.protocol));
			return;
		}
		strncpy(buffer, automation.line.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
	}

	std::string keyError;
	const KeyCommandResult keyResult = ApplyKeyCommand(
		buffer,
		automationKeyState,
		keyError);
	if(keyResult == KeyCommandResult::Error)
	{
		Log("[KEY] %s", keyError.c_str());
		return;
	}
	if(keyResult == KeyCommandResult::Applied)
	{
		SendCurrentKeyState();
		Log("[KEY] Keys=0x%04X Additional=%u.",
			automationKeyState.keys,
			automationKeyState.additionalKey);
		return;
	}

	if(!strncmp(buffer, "!entervehicle ", 14))
	{
		int vehicleId = atoi(&buffer[14]);
		if(vehicleId < 0 || vehicleId >= MAX_VEHICLES ||
			!vehiclePool[vehicleId].iDoesExist)
		{
			Log("[ENTER_VEHICLE] Vehicle %d is not streamed in.", vehicleId);
			return;
		}
		SendEnterVehicleNotification((VEHICLEID)vehicleId, 0);
		AssignVehicle((VEHICLEID)vehicleId, 0);
		Log("[ENTER_VEHICLE] Entered vehicle %d as driver.", vehicleId);
		return;
	}

	if(!strncmp(buffer, "!enterpassenger ", 16))
	{
		int vehicleId = atoi(&buffer[16]);
		if(vehicleId < 0 || vehicleId >= MAX_VEHICLES ||
			!vehiclePool[vehicleId].iDoesExist)
		{
			Log("[ENTER_PASSENGER] Vehicle %d is not streamed in.", vehicleId);
			return;
		}
		SendEnterVehicleNotification((VEHICLEID)vehicleId, 1);
		AssignVehicle((VEHICLEID)vehicleId, 1);
		SendPassengerFullSyncData((VEHICLEID)forcedVehicleId, true);
		Log("[ENTER_PASSENGER] Entered vehicle %d as passenger.", vehicleId);
		return;
	}

	if(!strcmp(buffer, "!exitvehicle"))
	{
		if(forcedVehicleId >= 0)
			SendExitVehicleNotification((VEHICLEID)forcedVehicleId);
		int exitedVehicleId = ClearAssignedVehicle();
		Log("[EXIT_VEHICLE] Exited vehicle %d.", exitedVehicleId);
		return;
	}

	if(!strncmp(buffer, "!death", 6))
	{
		int reason = 0;
		int killerId = 65535;
		sscanf(&buffer[6], "%d %d", &reason, &killerId);
		SendWastedNotification((BYTE)reason, (PLAYERID)killerId);
		Log("[DEATH] Sent reason %d with killer ID %d.", reason, killerId);
		return;
	}

	if(!strncmp(buffer, "!position ", 10))
	{
		float x, y, z;
		if(sscanf(&buffer[10], "%f %f %f", &x, &y, &z) != 3)
		{
			Log("[POSITION] Usage: !position <x> <y> <z>");
			return;
		}
		settings.fNormalModePos[0] = x;
		settings.fNormalModePos[1] = y;
		settings.fNormalModePos[2] = z;
		settings.fCurrentPosition[0] = x;
		settings.fCurrentPosition[1] = y;
		settings.fCurrentPosition[2] = z;
		Log("[POSITION] %.2f %.2f %.2f", x, y, z);
		return;
	}
	RunCommand(buffer, 0);
}

void SetUpConsole()
{
	setvbuf(stdout, NULL, _IONBF, 0);
	std::thread(ReadCommands, nullptr).detach();
}

void SetUpWindow(HINSTANCE) { SetUpConsole(); }
LRESULT CALLBACK TeleportMenuProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
DWORD WINAPI TeleportMenuThread(PVOID) { return 0; }
int RCONReceiveLoop() { return 0; }
void sendRconCommand(char *, int) { }
