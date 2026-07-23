#include "main.h"
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
static eRunModes forcedPreviousRunMode = RUNMODE_NORMAL;

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
		INCAR_SYNC_DATA sync;
		memset(&sync, 0, sizeof(sync));
		sync.VehicleID = (VEHICLEID)forcedVehicleId;
		sync.fQuaternion[0] = 1.0f;
		sync.fCarHealth = 1000.0f;
		sync.bytePlayerHealth = (BYTE)settings.fPlayerHealth;
		sync.bytePlayerArmour = (BYTE)settings.fPlayerArmour;
		SendInCarFullSyncData(&sync, 1, (PLAYERID)-1);
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
		playerInfo[g_myPlayerID].iAreWeInAVehicle = 1;
		if(forcedVehicleId < 0)
			forcedPreviousRunMode = settings.runMode;
		forcedVehicleId = vehicleId;
		// Normal mode emits on-foot sync every frame. Pause that built-in stream
		// while the driver is emitting explicit in-car sync.
		settings.runMode = RUNMODE_STILL;
		Log("[ENTER_VEHICLE] Entered vehicle %d as driver.", vehicleId);
		return;
	}

	if(!strcmp(buffer, "!exitvehicle"))
	{
		if(forcedVehicleId >= 0)
			SendExitVehicleNotification((VEHICLEID)forcedVehicleId);
		playerInfo[g_myPlayerID].iAreWeInAVehicle = 0;
		Log("[EXIT_VEHICLE] Exited vehicle %d.", forcedVehicleId);
		forcedVehicleId = -1;
		settings.runMode = forcedPreviousRunMode;
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
