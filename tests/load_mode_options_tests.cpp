#include "load_mode_options.h"

#include <cassert>
#include <string>

namespace
{
void Parse(LoadModeOptions &options, const char *name, const char *value)
{
	char executable[] = "raksamp-client";
	char *arguments[] = {
		executable,
		const_cast<char *>(name),
		const_cast<char *>(value)
	};
	int index = 1;
	std::string error;
	assert(ParseLoadModeOption(3, arguments, index, options, error) ==
		LoadOptionParseResult::Matched);
	assert(error.empty());
	assert(index == 2);
}
}

int main()
{
	LoadModeOptions options;
	Parse(options, "--load-clients", "1");
	Parse(options, "--load-duration", "30");
	Parse(options, "--load-connect-rate", "50");
	Parse(options, "--load-sync-rate", "10");
	Parse(options, "--load-ready-timeout", "240");
	Parse(options, "--load-anticheat-probe-clients", "1");
	Parse(options, "--load-index-offset", "99");
	Parse(options, "--load-account-prefix", "rp_load");
	Parse(options, "--load-character-first", "Load");
	Parse(options, "--load-password", "testpassword");
	Parse(options, "--load-start-file", "/tmp/raksamp-load.start");

	std::string error;
	assert(options.requested);
	assert(ValidateLoadModeOptions(options, error));
	assert(MakeLoadAccountName(options, 0) == "rp_load0100");
	assert(MakeLoadCharacterName(options, 0) == "Load_Aadv");
	assert(options.startFile == "/tmp/raksamp-load.start");

	options.clientCount = 2;
	assert(!ValidateLoadModeOptions(options, error));
	options.clientCount = 1;
	options.antiCheatProbeClients = 2;
	assert(!ValidateLoadModeOptions(options, error));
	options.antiCheatProbeClients = 0;
	options.indexOffset = 100;
	assert(!ValidateLoadModeOptions(options, error));
	options.indexOffset = 0;
	options.clientCount = 1;
	options.password.clear();
	assert(!ValidateLoadModeOptions(options, error));

	char executable[] = "raksamp-client";
	char unknown[] = "--other";
	char *unknownArguments[] = {executable, unknown};
	int index = 1;
	assert(ParseLoadModeOption(2, unknownArguments, index, options, error) ==
		LoadOptionParseResult::NotMatched);

	LoadModeOptions incomplete;
	Parse(incomplete, "--load-duration", "30");
	assert(!ValidateLoadModeOptions(incomplete, error));
	return 0;
}
