#pragma once

#include <cstddef>
#include <string>

struct LoadModeOptions
{
	bool requested = false;
	int clientCount = 0;
	int durationSeconds = 15;
	int connectRatePerSecond = 5;
	int syncRatePerSecond = 5;
	int readyTimeoutSeconds = 180;
	int antiCheatProbeClients = 0;
	int indexOffset = 0;
	bool directCharacter = false;
	std::string accountPrefix = "loadtest";
	std::string characterFirstName = "Load";
	std::string password;
	std::string startFile;
};

enum class LoadOptionParseResult
{
	NotMatched,
	Matched,
	Error
};

LoadOptionParseResult ParseLoadModeOption(
	int argc,
	char **argv,
	int &index,
	LoadModeOptions &options,
	std::string &error);

bool ValidateLoadModeOptions(const LoadModeOptions &options, std::string &error);
std::string MakeLoadAccountName(
	const LoadModeOptions &options,
	std::size_t zeroBasedIndex);
std::string MakeLoadCharacterName(
	const LoadModeOptions &options,
	std::size_t zeroBasedIndex);
