#include "main.h"

#include <csignal>
#include <filesystem>
#include <string>

TiXmlDocument xmlSettings;
char szWorkingDirectory[MAX_PATH];
int iMainLoop = 1;
RakServerInterface *pRakServer = NULL;
unsigned int _uiRndSrvChallenge;

int iPort;
unsigned short usMaxPlayers;
int iLagCompensation;

char szLogFile[MAX_PATH];
FILE *flLog = NULL;
struct stPlayerInfo playerInfo[MAX_PLAYERS];

namespace
{
std::string configPath = "RakSAMPServer.xml";

void StopServer(int)
{
	iMainLoop = 0;
}

void PrintUsage()
{
	printf("Usage: raksamp-server [--config PATH] [--check-config] [--help] [--version]\n");
}

bool LoadConfiguration()
{
	xmlSettings.Clear();
	if(!xmlSettings.LoadFile(configPath.c_str()))
	{
		fprintf(stderr, "Failed to load configuration: %s\n", configPath.c_str());
		return false;
	}

	TiXmlElement *serverElement = xmlSettings.FirstChildElement("server");
	if(!serverElement)
	{
		fprintf(stderr, "Configuration must contain a <server> element\n");
		return false;
	}

	const char *maxPlayers = serverElement->Attribute("max_players");
	const char *port = serverElement->Attribute("port");
	const char *name = serverElement->Attribute("name");
	const char *lagComp = serverElement->Attribute("lagcomp");
	const char *protocols = serverElement->Attribute("protocols");
	if(!maxPlayers || !port || !name || !lagComp)
	{
		fprintf(stderr, "Configuration requires max_players, port, name, and lagcomp\n");
		return false;
	}
	if(protocols && strcmp(protocols, "0.3.7,0.3DL") != 0 &&
		strcmp(protocols, "0.3DL,0.3.7") != 0)
	{
		fprintf(stderr, "protocols must contain both 0.3.7 and 0.3DL\n");
		return false;
	}

	long configuredPlayers = strtol(maxPlayers, NULL, 10);
	long configuredPort = strtol(port, NULL, 10);
	if(configuredPlayers < 1 || configuredPlayers > MAX_PLAYERS ||
		configuredPort < 1 || configuredPort > 65535)
	{
		fprintf(stderr, "max_players or port is outside the supported range\n");
		return false;
	}

	usMaxPlayers = static_cast<unsigned short>(configuredPlayers);
	iPort = static_cast<int>(configuredPort);
	snprintf(serverName, sizeof(serverName), "%s", name);
	iLagCompensation = atoi(lagComp) ? 1 : 0;

	std::filesystem::path parent = std::filesystem::absolute(configPath).parent_path();
	snprintf(szWorkingDirectory, sizeof(szWorkingDirectory), "%s", parent.string().c_str());
	snprintf(szLogFile, sizeof(szLogFile), "%s%cRakSAMPServer.log", szWorkingDirectory,
#ifdef _WIN32
		'\\'
#else
		'/'
#endif
	);
	return true;
}
}

int main(int argc, char *argv[])
{
	bool checkConfig = false;
	for(int i = 1; i < argc; ++i)
	{
		if(!strcmp(argv[i], "--help"))
		{
			PrintUsage();
			return 0;
		}
		if(!strcmp(argv[i], "--version"))
		{
			printf("raksamp-server %s (SA-MP 0.3.7 + 0.3DL, Lua %s)\n",
				RAKSAMP_VERSION, LUA_VERSION);
			return 0;
		}
		if(!strcmp(argv[i], "--check-config"))
		{
			checkConfig = true;
			continue;
		}
		if(!strcmp(argv[i], "--config") && i + 1 < argc)
		{
			configPath = argv[++i];
			continue;
		}
		fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
		PrintUsage();
		return 2;
	}

	if(!LoadConfiguration())
		return 2;
	if(checkConfig)
	{
		printf("%s: valid (0.3.7 + 0.3DL, port %d)\n", configPath.c_str(), iPort);
		return 0;
	}

	std::signal(SIGINT, StopServer);
	std::signal(SIGTERM, StopServer);
	char lagCompRule[] = "lagcomp";
	char lagCompOn[] = "On";
	char lagCompOff[] = "Off";
	modifyRuleValue(lagCompRule, iLagCompensation ? lagCompOn : lagCompOff);

	Log(" ");
	Log("  * ============================== *");
	Log("            RakSAMP server          ");
	Log("    Version: " RAKSAMP_VERSION "    ");
	Log("    Protocols: 0.3.7, 0.3DL         ");
	Log("  * ============================== *");
	Log(" ");

	LoadScripts();
	srand(static_cast<unsigned int>(time(NULL)));
	_uiRndSrvChallenge = static_cast<unsigned int>(rand());

	pRakServer = RakNetworkFactory::GetRakServerInterface();
	LoadBanList();
	pRakServer->Start(usMaxPlayers, 0, 5, iPort);
	pRakServer->StartOccasionalPing();
	RegisterServerRPCs(pRakServer);

	while(iMainLoop)
	{
		UpdateNetwork();
		Sleep(5);
	}

	if(flLog)
	{
		fclose(flLog);
		flLog = NULL;
	}
	pRakServer->Disconnect(300);
	RakNetworkFactory::DestroyRakServerInterface(pRakServer);
	return 0;
}

void Log(char *fmt, ...)
{
	if(!flLog)
	{
		flLog = fopen(szLogFile[0] ? szLogFile : "RakSAMPServer.log", "a");
		if(!flLog)
			return;
	}

	SYSTEMTIME time;
	GetLocalTime(&time);
	fprintf(flLog, "[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds);

	va_list consoleArgs;
	va_start(consoleArgs, fmt);
	va_list fileArgs;
	va_copy(fileArgs, consoleArgs);
	vprintf(fmt, consoleArgs);
	vfprintf(flLog, fmt, fileArgs);
	va_end(fileArgs);
	va_end(consoleArgs);
	fprintf(flLog, "\n");
	printf("\n");
	fflush(flLog);
}

unsigned char rand_byteRange(unsigned char a, unsigned char b) { return ((b-a)*((unsigned char)rand()/RAND_MAX))+a; }
unsigned short rand_shortRange(unsigned short a, unsigned short b) { return ((b-a)*((unsigned short)rand()/RAND_MAX))+a; }
unsigned int rand_intRange(unsigned int a, unsigned int b) { return ((b-a)*((unsigned int)rand()/RAND_MAX))+a; }
float rand_floatRange(float a, float b) { return ((b-a)*((float)rand()/RAND_MAX))+a; }

void gen_random(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	for(int i = 0; i < len; ++i)
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	s[len] = 0;
}
