#pragma once

#include <cstring>

enum class SampProtocol
{
	V037,
	V03DL
};

inline const char *SampProtocolName(SampProtocol protocol)
{
	return protocol == SampProtocol::V03DL ? "0.3DL" : "0.3.7";
}

inline const char *SampClientVersion(SampProtocol protocol)
{
	return protocol == SampProtocol::V03DL ? "0.3.DL-R1" : "0.3.7";
}

inline int SampNetworkVersion(SampProtocol protocol)
{
	return protocol == SampProtocol::V03DL ? 4062 : 4057;
}

inline int SampMaximumMtu(SampProtocol protocol)
{
	return protocol == SampProtocol::V03DL ? 1500 : 576;
}

inline bool TryParseSampProtocol(const char *value, SampProtocol &protocol)
{
	if(value == NULL)
		return false;
	if(!std::strcmp(value, "0.3.7") || !std::strcmp(value, "037"))
	{
		protocol = SampProtocol::V037;
		return true;
	}
	if(!std::strcmp(value, "0.3DL") || !std::strcmp(value, "0.3.DL") ||
		!std::strcmp(value, "03DL"))
	{
		protocol = SampProtocol::V03DL;
		return true;
	}
	return false;
}
