#include <cassert>
#include <cstdint>
#include <string>

#include "automation_protocol.h"

int main()
{
	AutomationCommand command;
	std::string error;
	assert(ParseAutomationCommand("/time", command, error) ==
		AutomationParseResult::PlainText);
	assert(ParseAutomationCommand(
		"{\"command\":\"send\",\"line\":\"/time\"}",
		command,
		error) == AutomationParseResult::Command);
	assert(command.command == "send");
	assert(command.line == "/time");
	assert(ParseAutomationCommand(
		"{\"command\":\"capabilities\"}", command, error) ==
		AutomationParseResult::Command);
	assert(ParseAutomationCommand(
		"{\"command\":\"send\",\"line\":\"\"}", command, error) ==
		AutomationParseResult::Error);
	assert(ParseAutomationCommand(
		"{\"command\":\"unknown\"}", command, error) ==
		AutomationParseResult::Error);
	assert(ParseAutomationCommand(
		"{\"command\":\"capabilities\",\"line\":\"ignored\"}", command, error) ==
		AutomationParseResult::Error);
	assert(ParseAutomationCommand(
		"{\"command\":\"send\",\"command\":\"capabilities\",\"line\":\"/time\"}",
		command, error) == AutomationParseResult::Error);

	const std::string complete = "{\"command\":\"send\",\"line\":\"/time\"}";
	for(std::size_t length = 1; length < complete.size(); ++length)
		assert(ParseAutomationCommand(complete.substr(0, length), command, error) !=
			AutomationParseResult::Command);

	std::uint32_t state = 0x5eed1234U;
	for(int sample = 0; sample < 2000; ++sample)
	{
		state = state * 1664525U + 1013904223U;
		std::string input(1 + state % 128, '\0');
		for(char &character : input)
		{
			state = state * 1664525U + 1013904223U;
			character = static_cast<char>(state & 0x7fU);
		}
		ParseAutomationCommand(input, command, error);
	}
	assert(EscapeAutomationJson("a\n\"b") == "a\\n\\\"b");
	return 0;
}
