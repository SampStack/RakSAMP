#pragma once

#include <string>

enum class AutomationParseResult
{
	PlainText,
	Command,
	Error
};

struct AutomationCommand
{
	std::string command;
	std::string line;
};

AutomationParseResult ParseAutomationCommand(
	const std::string &input,
	AutomationCommand &result,
	std::string &error);
std::string EscapeAutomationJson(const std::string &value);
void SetAutomationJsonl(bool enabled);
bool IsAutomationJsonl();
void EmitAutomationLog(const std::string &message);
void EmitAutomationCapabilities(const char *version, const char *protocol);
