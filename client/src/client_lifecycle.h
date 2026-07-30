#pragma once

inline bool ShouldAttemptAutomaticSpawn(bool spawned, bool spectating)
{
	return !spawned && !spectating;
}

inline bool ShouldSpawnAfterSpectatorExit(bool wasSpectating, bool spectating)
{
	return wasSpectating && !spectating;
}
