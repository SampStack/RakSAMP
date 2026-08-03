#include "automation_protocol.h"

#include <cctype>
#include <cstdio>

namespace
{
bool jsonl = false;

void SkipWhitespace(const std::string &input, std::size_t &position)
{
	while(position < input.size() &&
		std::isspace(static_cast<unsigned char>(input[position])))
		++position;
}

bool ParseString(
	const std::string &input,
	std::size_t &position,
	std::string &value)
{
	if(position >= input.size() || input[position++] != '"')
		return false;
	std::string parsed;
	while(position < input.size())
	{
		const unsigned char character =
			static_cast<unsigned char>(input[position++]);
		if(character == '"')
		{
			value = std::move(parsed);
			return true;
		}
		if(character < 0x20)
			return false;
		if(character != '\\')
		{
			parsed.push_back(static_cast<char>(character));
			continue;
		}
		if(position >= input.size())
			return false;
		switch(input[position++])
		{
			case '"': parsed.push_back('"'); break;
			case '\\': parsed.push_back('\\'); break;
			case '/': parsed.push_back('/'); break;
			case 'b': parsed.push_back('\b'); break;
			case 'f': parsed.push_back('\f'); break;
			case 'n': parsed.push_back('\n'); break;
			case 'r': parsed.push_back('\r'); break;
			case 't': parsed.push_back('\t'); break;
			default: return false;
		}
	}
	return false;
}
}

AutomationParseResult ParseAutomationCommand(
	const std::string &input,
	AutomationCommand &result,
	std::string &error)
{
	result = {};
	error.clear();
	std::size_t position = 0;
	SkipWhitespace(input, position);
	if(position >= input.size() || input[position] != '{')
		return AutomationParseResult::PlainText;
	++position;
	bool first = true;
	bool hasCommand = false;
	bool hasLine = false;
	while(true)
	{
		SkipWhitespace(input, position);
		if(position < input.size() && input[position] == '}')
		{
			++position;
			break;
		}
		if(!first)
		{
			if(position >= input.size() || input[position++] != ',')
			{
				error = "expected a comma between JSON properties";
				return AutomationParseResult::Error;
			}
			SkipWhitespace(input, position);
		}
		first = false;
		std::string key;
		std::string value;
		if(!ParseString(input, position, key))
		{
			error = "expected a JSON property name";
			return AutomationParseResult::Error;
		}
		SkipWhitespace(input, position);
		if(position >= input.size() || input[position++] != ':')
		{
			error = "expected a colon after JSON property name";
			return AutomationParseResult::Error;
		}
		SkipWhitespace(input, position);
		if(!ParseString(input, position, value))
		{
			error = "automation properties must contain string values";
			return AutomationParseResult::Error;
		}
		if(key == "command" && !hasCommand)
		{
			result.command = value;
			hasCommand = true;
		}
		else if(key == "line" && !hasLine)
		{
			result.line = value;
			hasLine = true;
		}
		else
		{
			error = "unknown or duplicate automation property '" + key + "'";
			return AutomationParseResult::Error;
		}
	}
	SkipWhitespace(input, position);
	if(position != input.size())
	{
		error = "unexpected data after JSON command";
		return AutomationParseResult::Error;
	}
	if(result.command == "send")
	{
		if(result.line.empty() || result.line.size() > 511)
		{
			error = "send requires a line of 1-511 bytes";
			return AutomationParseResult::Error;
		}
	}
	else if(result.command != "capabilities")
	{
		error = "command must be send or capabilities";
		return AutomationParseResult::Error;
	}
	else if(hasLine)
	{
		error = "capabilities does not accept a line";
		return AutomationParseResult::Error;
	}
	return AutomationParseResult::Command;
}

std::string EscapeAutomationJson(const std::string &value)
{
	std::string escaped;
	escaped.reserve(value.size() + 16);
	for(const unsigned char character : value)
	{
		switch(character)
		{
			case '"': escaped += "\\\""; break;
			case '\\': escaped += "\\\\"; break;
			case '\b': escaped += "\\b"; break;
			case '\f': escaped += "\\f"; break;
			case '\n': escaped += "\\n"; break;
			case '\r': escaped += "\\r"; break;
			case '\t': escaped += "\\t"; break;
			default:
				if(character >= 0x20)
					escaped.push_back(static_cast<char>(character));
				break;
		}
	}
	return escaped;
}

void SetAutomationJsonl(bool enabled) { jsonl = enabled; }
bool IsAutomationJsonl() { return jsonl; }

void EmitAutomationLog(const std::string &message)
{
	if(!jsonl)
		return;
	std::printf("{\"event\":\"log\",\"message\":\"%s\"}\n",
		EscapeAutomationJson(message).c_str());
	std::fflush(stdout);
}

void EmitAutomationCapabilities(const char *version, const char *protocol)
{
	std::printf(
		"{\"event\":\"capabilities\",\"schema\":1,"
		"\"version\":\"%s\",\"protocol\":\"%s\","
		"\"features\":[\"stdin-send\",\"dialogs\",\"textdraws\","
		"\"weapons\",\"damage\",\"vehicles\",\"spectator\"],"
		"\"limitations\":[\"rendering\",\"physics\",\"asset-storage\"]}\n",
		version == nullptr ? "unknown" : version,
		protocol == nullptr ? "unknown" : protocol);
	std::fflush(stdout);
}
