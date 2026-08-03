/*
	Updated to 0.3.7 by P3ti
*/

#include "main.h"
#include "safe_parse.h"

#include <string>

struct stSettings settings;
TiXmlDocument xmlSettings;
static char clientConfigPath[512] = "RakSAMPClient.xml";
static bool hasProtocolOverride = false;
static SampProtocol protocolOverride = SampProtocol::V03DL;

void SetClientConfigPath(const char *path)
{
	if(path == NULL || path[0] == '\0')
		return;
	strncpy(clientConfigPath, path, sizeof(clientConfigPath) - 1);
	clientConfigPath[sizeof(clientConfigPath) - 1] = '\0';
}

const char *GetClientConfigPath()
{
	return clientConfigPath;
}

bool SetClientProtocolOverride(const char *value)
{
	SampProtocol parsed;
	if(!TryParseSampProtocol(value, parsed))
		return false;
	protocolOverride = parsed;
	hasProtocolOverride = true;
	return true;
}

namespace
{
int InvalidConfiguration(const char *message)
{
	MessageBox(NULL, message, "Invalid configuration", MB_ICONERROR);
	return 0;
}

int ParseSettings(
	stSettings &settings,
	int &normalOnfootSendRate,
	int &normalIncarSendRate,
	int &firingSendRate,
	int &sendMultiplier)
{
	// load xml
	if(!xmlSettings.LoadFile(clientConfigPath))
	{
		MessageBox(NULL, "Failed to load the config file", "Error", MB_ICONERROR);
		return 0;
	}

	TiXmlElement* rakSAMPElement = xmlSettings.FirstChildElement("RakSAMPClient");
	if(!rakSAMPElement)
		return InvalidConfiguration("Configuration must contain RakSAMPClient");
	if(rakSAMPElement)
	{
		// get console
		rakSAMPElement->QueryIntAttribute("console", (int *)&settings.iConsole);

		// get runmode
		rakSAMPElement->QueryIntAttribute("runmode", (int *)&settings.runMode);

		// get autorun
		rakSAMPElement->QueryIntAttribute("autorun", (int *)&settings.iAutorun);

		// get find
		rakSAMPElement->QueryIntAttribute("find", (int *)&settings.iFind);

		// get selected class id
		rakSAMPElement->QueryIntAttribute("select_classid", (int *)&settings.iClassID);

		// get manual spawn
		rakSAMPElement->QueryIntAttribute("manual_spawn", (int *)&settings.iManualSpawn);

		// get print_timestamps
		rakSAMPElement->QueryIntAttribute("print_timestamps", (int *)&settings.iPrintTimestamps);

		// get fps simulation
		rakSAMPElement->QueryIntAttribute("updatestats", (int *)&settings.iUpdateStats);

		// get min simulated fps
		rakSAMPElement->QueryIntAttribute("minfps", (int *)&settings.iMinFPS);

		// get max simulated fps
		rakSAMPElement->QueryIntAttribute("maxfps", (int *)&settings.iMaxFPS);

		// Protocol is explicit in modern configs. Existing 0.3.7 configs remain
		// compatible by inferring from their advertised client version.
		const char *configuredClientVersion = rakSAMPElement->Attribute("clientversion");
		const char *configuredProtocol = rakSAMPElement->Attribute("protocol");
		SampProtocol selectedProtocol = SampProtocol::V03DL;
		if(configuredProtocol != NULL && !TryParseSampProtocol(configuredProtocol, selectedProtocol))
		{
			MessageBox(NULL, "protocol must be 0.3.7 or 0.3DL", "Invalid configuration", MB_ICONERROR);
			xmlSettings.Clear();
			return 0;
		}
		if(configuredProtocol == NULL && configuredClientVersion != NULL &&
			!strncmp(configuredClientVersion, "0.3.7", 5))
			selectedProtocol = SampProtocol::V037;
		if(hasProtocolOverride)
			selectedProtocol = protocolOverride;

		settings.protocol = selectedProtocol;
		settings.iNetworkVersion = SampNetworkVersion(selectedProtocol);
		settings.iMaximumMtu = SampMaximumMtu(selectedProtocol);
		const char *advertisedVersion = configuredClientVersion != NULL
			? configuredClientVersion
			: SampClientVersion(selectedProtocol);
		strncpy(settings.szClientVersion, advertisedVersion, sizeof(settings.szClientVersion) - 1);
		settings.szClientVersion[sizeof(settings.szClientVersion) - 1] = '\0';

		// get chat color
		rakSAMPElement->QueryColorAttribute("chatcolor_rgb",
			(unsigned char *)&settings.bChatColorRed, (unsigned char *)&settings.bChatColorGreen, (unsigned char *)&settings.bChatColorBlue);

		// get client message color
		rakSAMPElement->QueryColorAttribute("clientmsg_rgb",
			(unsigned char *)&settings.bCMsgRed, (unsigned char *)&settings.bCMsgGreen, (unsigned char *)&settings.bCMsgBlue);
		
		// get checkpoint alert color
		rakSAMPElement->QueryColorAttribute("cpalert_rgb",
			(unsigned char *)&settings.bCPAlertRed, (unsigned char *)&settings.bCPAlertGreen, (unsigned char *)&settings.bCPAlertBlue);

		// get followplayer
		const char *followPlayer = rakSAMPElement->Attribute("followplayer");
		if(followPlayer != NULL &&
			!raksamp::parse::Copy(settings.szFollowingPlayerName, followPlayer))
			return InvalidConfiguration("followplayer is too long");
		rakSAMPElement->QueryIntAttribute("followplayerwithvehicleid", &settings.iFollowingWithVehicleID);
		rakSAMPElement->QueryFloatAttribute("followXOffset", &settings.fFollowXOffset);
		rakSAMPElement->QueryFloatAttribute("followYOffset", &settings.fFollowYOffset);
		rakSAMPElement->QueryFloatAttribute("followZOffset", &settings.fFollowZOffset);

		// get the first server
		TiXmlElement* serverElement = rakSAMPElement->FirstChildElement("server");
		if(serverElement)
		{
			const char *address = serverElement->GetText();
			const char *nickname = serverElement->Attribute("nickname");
			const char *password = serverElement->Attribute("password");
			std::string host;
			unsigned short port = 0;
			if(address == NULL || nickname == NULL || password == NULL ||
				!raksamp::parse::HostAndPort(address, host, port) ||
				!raksamp::parse::Copy(settings.server.szAddr, host.c_str()) ||
				!raksamp::parse::Copy(settings.server.szNickname, nickname) ||
				!raksamp::parse::Copy(settings.server.szPassword, password))
				return InvalidConfiguration(
					"server requires bounded address:port, nickname, and password");
			settings.server.iPort = port;
		}
		else
			return InvalidConfiguration("Configuration requires a server element");

		// get intervals
		TiXmlElement* intervalsElement = rakSAMPElement->FirstChildElement("intervals");
		if(intervalsElement)
		{
			intervalsElement->QueryIntAttribute("spam", (int *)&settings.uiSpamInterval);
			intervalsElement->QueryIntAttribute("fakekill", (int *)&settings.uiFakeKillInterval);
			intervalsElement->QueryIntAttribute("lag", (int *)&settings.uiLagInterval);
			intervalsElement->QueryIntAttribute("joinflood", (int *)&settings.uiJoinFloodInterval);
			intervalsElement->QueryIntAttribute("chatflood", (int *)&settings.uiChatFloodInterval);
			intervalsElement->QueryIntAttribute("classflood", (int *)&settings.uiClassFloodInterval);
			intervalsElement->QueryIntAttribute("bulletflood", (int *)&settings.uiBulletFloodInterval);
		}

		// get logging settings
		TiXmlElement* logElement = rakSAMPElement->FirstChildElement("log");
		if(logElement)
		{
			logElement->QueryIntAttribute("objects", (int *)&settings.uiObjectsLogging);
			logElement->QueryIntAttribute("pickups", (int *)&settings.uiPickupsLogging);
			logElement->QueryIntAttribute("textlabels", (int *)&settings.uiTextLabelsLogging);
			logElement->QueryIntAttribute("textdraws", (int *)&settings.uiTextDrawsLogging);
		}

		// get sendrates settings
		TiXmlElement* sendratesElement = rakSAMPElement->FirstChildElement("sendrates");
		if(sendratesElement)
		{
			sendratesElement->QueryIntAttribute("force", (int *)&settings.uiForceCustomSendRates);
			sendratesElement->QueryIntAttribute("onfoot", &normalOnfootSendRate);
			sendratesElement->QueryIntAttribute("incar", &normalIncarSendRate);
			sendratesElement->QueryIntAttribute("firing", &firingSendRate);
			sendratesElement->QueryIntAttribute("multiplier", &sendMultiplier);
		}

		// get normal mode pos
		TiXmlElement* normalPosElement = rakSAMPElement->FirstChildElement("normal_pos");
		if(normalPosElement)
		{
			normalPosElement->QueryVectorAttribute("position", (float *)&settings.fNormalModePos);
			normalPosElement->QueryFloatAttribute("rotation", &settings.fNormalModeRot);
			normalPosElement->QueryIntAttribute("force", (int *)&settings.iNormalModePosForce);
		}

		// get auto run commands
		TiXmlElement* autorunElement = rakSAMPElement->FirstChildElement("autorun");
		if(autorunElement)
		{
			for(int i = 0; i < MAX_AUTORUN_CMDS; i++)
			{
				if(autorunElement)
				{
					settings.autoRunCMDs[i].iExists = 1;
					if(!raksamp::parse::Copy(
						settings.autoRunCMDs[i].szCMD,
						autorunElement->GetText()))
						return InvalidConfiguration("autorun command is missing or too long");
					autorunElement = autorunElement->NextSiblingElement("autorun");
				}
				else
					break;
			}
		}

		TiXmlElement* findElement = rakSAMPElement->FirstChildElement("find");
		if(findElement)
		{
			for(int i = 0; i < MAX_FIND_ITEMS; i++)
			{
				if(findElement)
				{
					settings.findItems[i].iExists = 1;
					if(!raksamp::parse::Copy(
						settings.findItems[i].szFind,
						findElement->Attribute("text")) ||
						!raksamp::parse::Copy(
							settings.findItems[i].szSay,
							findElement->Attribute("say")))
						return InvalidConfiguration("find text or response is missing or too long");
					findElement->QueryColorAttribute("bk_color",
						(unsigned char *)&settings.findItems[i].bBkRed,
						(unsigned char *)&settings.findItems[i].bBkGreen,
						(unsigned char *)&settings.findItems[i].bBkBlue);
					findElement->QueryColorAttribute("text_color",
						(unsigned char *)&settings.findItems[i].bTextRed,
						(unsigned char *)&settings.findItems[i].bTextGreen,
						(unsigned char *)&settings.findItems[i].bTextBlue);

					findElement = findElement->NextSiblingElement("find");
				}
				else
					break;
			}
		}

		// get teleport locations
		TiXmlElement* teleportElement = rakSAMPElement->FirstChildElement("teleport");
		if(teleportElement)
		{
			for(int i = 0; i < MAX_TELEPORT_ITEMS; i++)
			{
				if(teleportElement)
				{
					settings.TeleportLocations[i].bCreated = 1;

					if(!raksamp::parse::Copy(
						settings.TeleportLocations[i].szName,
						teleportElement->Attribute("name")))
						return InvalidConfiguration("teleport name is missing or too long");
					teleportElement->QueryVectorAttribute("position", (float *)&settings.TeleportLocations[i].fPosition);

					teleportElement = teleportElement->NextSiblingElement("teleport");
				}
				else
					break;
			}
		}
	}
	if(settings.runMode < RUNMODE_RCON || settings.runMode > RUNMODE_PLAYROUTES ||
		settings.iClassID < 0 || settings.iMinFPS < 1 ||
		settings.iMaxFPS < settings.iMinFPS || settings.iMaxFPS > 1000 ||
		normalOnfootSendRate < 1 || normalOnfootSendRate > 1000 ||
		normalIncarSendRate < 1 || normalIncarSendRate > 1000 ||
		firingSendRate < 1 || firingSendRate > 1000 ||
		sendMultiplier < 1 || sendMultiplier > 100)
		return InvalidConfiguration("run mode, FPS, class, or send rate is outside the supported range");

	xmlSettings.Clear();

	PCHAR szCmdLine = GetCommandLineA();
	CHAR szPort[20];

	while(*szCmdLine)
	{
		if(*szCmdLine == '-' || *szCmdLine == '/')
		{
			szCmdLine++;
			switch(*szCmdLine)
			{
				case 'h':
					szCmdLine++;
					SetStringFromCommandLine(szCmdLine, settings.server.szAddr);
					break;
				case 'p':
					szCmdLine++;
					SetStringFromCommandLine(szCmdLine, szPort); settings.server.iPort = atoi(szPort);
					break;
				case 'n':
					szCmdLine++;
					SetStringFromCommandLine(szCmdLine, settings.server.szNickname);
					break;
				case 'z':
					szCmdLine++;
					SetStringFromCommandLine(szCmdLine, settings.server.szPassword);
					break;
			}
		}
		szCmdLine++;
	}

	return 1;
}
}

int LoadSettings()
{
	stSettings parsed = {};
	int normalOnfootSendRate = 0;
	int normalIncarSendRate = 0;
	int firingSendRate = 0;
	int sendMultiplier = 0;
	if(!ParseSettings(parsed, normalOnfootSendRate, normalIncarSendRate,
		firingSendRate, sendMultiplier))
		return 0;
	settings = parsed;
	iNetModeNormalOnfootSendRate = normalOnfootSendRate;
	iNetModeNormalIncarSendRate = normalIncarSendRate;
	iNetModeFiringSendRate = firingSendRate;
	iNetModeSendMultiplier = sendMultiplier;
	return 1;
}

int UnLoadSettings()
{
	memset(&settings, 0, sizeof(settings));

	return 1;
}

int ReloadSettings()
{
	if(LoadSettings())
	{
		Log("Settings reloaded");
		return 1;
	}

	Log("Failed to reload settings");

	return 0;
}
