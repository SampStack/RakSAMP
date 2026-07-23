#include "protocol.h"

#include <cassert>
#include <cstring>

int main()
{
	SampProtocol protocol = SampProtocol::V03DL;
	assert(TryParseSampProtocol("0.3.7", protocol));
	assert(protocol == SampProtocol::V037);
	assert(SampNetworkVersion(protocol) == 4057);
	assert(SampMaximumMtu(protocol) == 576);

	assert(TryParseSampProtocol("0.3DL", protocol));
	assert(protocol == SampProtocol::V03DL);
	assert(SampNetworkVersion(protocol) == 4062);
	assert(SampMaximumMtu(protocol) == 1500);
	assert(!std::strcmp(SampClientVersion(protocol), "0.3.DL-R1"));
	assert(!TryParseSampProtocol("invalid", protocol));
	return 0;
}
