#include "load_mode_options.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace
{
bool ParseInteger(const char *value, int &result)
{
	if(value == nullptr || value[0] == '\0')
		return false;

	char *end = nullptr;
	errno = 0;
	const long parsed = std::strtol(value, &end, 10);
	if(errno != 0 || end == value || *end != '\0' ||
		parsed < INT_MIN || parsed > INT_MAX)
		return false;

	result = static_cast<int>(parsed);
	return true;
}

bool IsAccountPrefix(const std::string &value)
{
	if(value.empty())
		return false;
	return std::all_of(value.begin(), value.end(), [](unsigned char character)
	{
		return std::isalnum(character) || character == '_';
	});
}

bool IsCharacterFirstName(const std::string &value)
{
	if(value.size() < 2 || value.size() > 11 ||
		!std::isupper(static_cast<unsigned char>(value.front())))
		return false;
	return std::all_of(value.begin() + 1, value.end(), [](unsigned char character)
	{
		return std::islower(character);
	});
}

std::string AlphaSuffix(std::size_t value)
{
	// Four letters cover 456,976 clients while keeping roleplay names short.
	std::string suffix(4, 'a');
	for(std::size_t position = suffix.size(); position > 0; --position)
	{
		suffix[position - 1] = static_cast<char>('a' + (value % 26));
		value /= 26;
	}
	suffix.front() = static_cast<char>(
		std::toupper(static_cast<unsigned char>(suffix.front())));
	return suffix;
}
}

LoadOptionParseResult ParseLoadModeOption(
	int argc,
	char **argv,
	int &index,
	LoadModeOptions &options,
	std::string &error)
{
	const std::string option = argv[index];
	if(option == "--load-direct-character")
	{
		options.requested = true;
		options.directCharacter = true;
		return LoadOptionParseResult::Matched;
	}
	const bool takesValue =
		option == "--load-clients" ||
		option == "--load-duration" ||
		option == "--load-connect-rate" ||
		option == "--load-sync-rate" ||
		option == "--load-ready-timeout" ||
		option == "--load-anticheat-probe-clients" ||
		option == "--load-index-offset" ||
		option == "--load-account-prefix" ||
		option == "--load-character-first" ||
		option == "--load-password" ||
		option == "--load-start-file";
	if(!takesValue)
		return LoadOptionParseResult::NotMatched;
	options.requested = true;

	if(index + 1 >= argc)
	{
		error = option + " requires a value";
		return LoadOptionParseResult::Error;
	}

	const char *value = argv[++index];
	int parsed = 0;
	if(option == "--load-clients" ||
		option == "--load-duration" ||
		option == "--load-connect-rate" ||
		option == "--load-sync-rate" ||
		option == "--load-ready-timeout" ||
		option == "--load-anticheat-probe-clients" ||
		option == "--load-index-offset")
	{
		if(!ParseInteger(value, parsed))
		{
			error = option + " requires an integer";
			return LoadOptionParseResult::Error;
		}
	}

	if(option == "--load-clients")
		options.clientCount = parsed;
	else if(option == "--load-duration")
		options.durationSeconds = parsed;
	else if(option == "--load-connect-rate")
		options.connectRatePerSecond = parsed;
	else if(option == "--load-sync-rate")
		options.syncRatePerSecond = parsed;
	else if(option == "--load-ready-timeout")
		options.readyTimeoutSeconds = parsed;
	else if(option == "--load-anticheat-probe-clients")
		options.antiCheatProbeClients = parsed;
	else if(option == "--load-index-offset")
		options.indexOffset = parsed;
	else if(option == "--load-account-prefix")
		options.accountPrefix = value;
	else if(option == "--load-character-first")
		options.characterFirstName = value;
	else if(option == "--load-start-file")
		options.startFile = value;
	else
		options.password = value;

	return LoadOptionParseResult::Matched;
}

bool ValidateLoadModeOptions(const LoadModeOptions &options, std::string &error)
{
	if(!options.requested)
		return true;
	if(options.clientCount != 1)
		error = "--load-clients must be 1; use process sharding for concurrency";
	else if(options.durationSeconds < 1 || options.durationSeconds > 3600)
		error = "--load-duration must be between 1 and 3600 seconds";
	else if(options.connectRatePerSecond < 1 || options.connectRatePerSecond > 100)
		error = "--load-connect-rate must be between 1 and 100 clients per second";
	else if(options.syncRatePerSecond < 1 || options.syncRatePerSecond > 30)
		error = "--load-sync-rate must be between 1 and 30 updates per second";
	else if(options.readyTimeoutSeconds < 5 || options.readyTimeoutSeconds > 900)
		error = "--load-ready-timeout must be between 5 and 900 seconds";
	else if(options.antiCheatProbeClients < 0 ||
		options.antiCheatProbeClients > options.clientCount)
		error = "--load-anticheat-probe-clients must be between 0 and --load-clients";
	else if(options.indexOffset < 0 ||
		options.indexOffset + options.clientCount > 100)
		error = "--load-index-offset plus --load-clients must be between 1 and 100";
	else if(options.password.empty() || options.password.size() > 255)
		error = "--load-password is required and must be at most 255 characters";
	else if(!IsAccountPrefix(options.accountPrefix))
		error = "--load-account-prefix may contain only letters, digits, and underscores";
	else if(!IsCharacterFirstName(options.characterFirstName))
		error = "--load-character-first must be 2-11 characters in Firstname format";
	else if(MakeLoadAccountName(options, options.clientCount - 1).size() > 24)
		error = "generated account names exceed SA-MP's 24-character limit";
	else if(MakeLoadCharacterName(options, options.clientCount - 1).size() > 24)
		error = "generated character names exceed the 24-character limit";
	else
		return true;

	return false;
}

std::string MakeLoadAccountName(
	const LoadModeOptions &options,
	std::size_t zeroBasedIndex)
{
	if(options.directCharacter)
		return MakeLoadCharacterName(options, zeroBasedIndex);

	const std::size_t effectiveIndex =
		zeroBasedIndex + static_cast<std::size_t>(options.indexOffset);
	const std::size_t largest = options.clientCount > 0
		? static_cast<std::size_t>(
			options.indexOffset + options.clientCount)
		: effectiveIndex + 1;
	const int width = std::max<int>(
		4,
		static_cast<int>(std::to_string(largest).size()));
	std::ostringstream stream;
	stream << options.accountPrefix << std::setw(width) << std::setfill('0')
		<< effectiveIndex + 1;
	return stream.str();
}

std::string MakeLoadCharacterName(
	const LoadModeOptions &options,
	std::size_t zeroBasedIndex)
{
	return options.characterFirstName + "_" + AlphaSuffix(
		zeroBasedIndex + static_cast<std::size_t>(options.indexOffset));
}
