#include "client_lifecycle.h"

#include <cassert>

int main()
{
	assert(ShouldAttemptAutomaticSpawn(false, false));
	assert(!ShouldAttemptAutomaticSpawn(false, true));
	assert(!ShouldAttemptAutomaticSpawn(true, false));
	assert(!ShouldAttemptAutomaticSpawn(true, true));

	assert(!ShouldSpawnAfterSpectatorExit(false, false));
	assert(!ShouldSpawnAfterSpectatorExit(false, true));
	assert(!ShouldSpawnAfterSpectatorExit(true, true));
	assert(ShouldSpawnAfterSpectatorExit(true, false));

	// Hospital recovery is another spectator cycle after the initial spawn.
	assert(ShouldSpawnAfterSpectatorExit(true, false));
	return 0;
}
