#ifdef _WIN32
#include <windows.h>
#else
#include "native_compat.h"
#endif
#include "../../common/common.h"
#include "key_state.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <unordered_map>

namespace
{
constexpr uint32_t Yes = 1u << 16;
constexpr uint32_t No = 1u << 17;
constexpr uint32_t CtrlBack = 1u << 18;
constexpr uint32_t AdditionalMask = Yes | No | CtrlBack;
constexpr uint32_t SupportedMask = 0xFFFFu | AdditionalMask;

std::string Lower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
	{
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

bool TryNamedKey(const std::string &token, uint32_t &mask)
{
	static const std::unordered_map<std::string, uint32_t> keys =
	{
		{ "action", KEY_ACTION },
		{ "crouch", KEY_CROUCH },
		{ "horn", KEY_CROUCH },
		{ "h", KEY_CROUCH },
		{ "fire", KEY_FIRE },
		{ "sprint", KEY_SPRINT },
		{ "secondaryattack", KEY_SECONDARY_ATTACK },
		{ "secondary_attack", KEY_SECONDARY_ATTACK },
		{ "jump", KEY_JUMP },
		{ "lookright", KEY_LOOK_RIGHT },
		{ "look_right", KEY_LOOK_RIGHT },
		{ "handbrake", KEY_HANDBRAKE },
		{ "lookleft", KEY_LOOK_LEFT },
		{ "look_left", KEY_LOOK_LEFT },
		{ "submission", KEY_SUBMISSION },
		{ "lookbehind", KEY_LOOK_BEHIND },
		{ "look_behind", KEY_LOOK_BEHIND },
		{ "walk", KEY_WALK },
		{ "lalt", KEY_WALK },
		{ "analogup", KEY_ANALOG_UP },
		{ "analog_up", KEY_ANALOG_UP },
		{ "analogdown", KEY_ANALOG_DOWN },
		{ "analog_down", KEY_ANALOG_DOWN },
		{ "analogleft", KEY_ANALOG_LEFT },
		{ "analog_left", KEY_ANALOG_LEFT },
		{ "analogright", KEY_ANALOG_RIGHT },
		{ "analog_right", KEY_ANALOG_RIGHT },
		{ "yes", Yes },
		{ "y", Yes },
		{ "no", No },
		{ "n", No },
		{ "ctrlback", CtrlBack },
		{ "ctrl_back", CtrlBack }
	};

	const auto found = keys.find(Lower(token));
	if(found == keys.end())
		return false;
	mask = found->second;
	return true;
}

bool TryRawKey(const std::string &token, uint32_t &mask)
{
	if(token.empty() || token[0] == '-')
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long parsed = std::strtoul(token.c_str(), &end, 0);
	if(errno != 0 || end == token.c_str() || *end != '\0' || parsed > SupportedMask)
		return false;
	mask = static_cast<uint32_t>(parsed);
	return true;
}

uint8_t AdditionalValue(uint32_t mask)
{
	if(mask & Yes) return 1;
	if(mask & No) return 2;
	if(mask & CtrlBack) return 3;
	return 0;
}

int AdditionalCount(uint32_t mask)
{
	int count = 0;
	if(mask & Yes) ++count;
	if(mask & No) ++count;
	if(mask & CtrlBack) ++count;
	return count;
}

uint32_t ToMask(const AutomationKeyState &state)
{
	uint32_t result = state.keys;
	switch(state.additionalKey)
	{
	case 1: result |= Yes; break;
	case 2: result |= No; break;
	case 3: result |= CtrlBack; break;
	}
	return result;
}
}

KeyCommandResult ApplyKeyCommand(
	const std::string &command,
	AutomationKeyState &state,
	std::string &error)
{
	if(command.rfind("!key", 0) != 0)
		return KeyCommandResult::NotKeyCommand;

	std::istringstream input(command);
	std::string commandName;
	std::string direction;
	input >> commandName >> direction;
	if(commandName != "!key" || (direction != "down" && direction != "up"))
	{
		error = "Usage: !key <down|up> <action|mask> [action|mask ...]";
		return KeyCommandResult::Error;
	}

	uint32_t requested = 0;
	std::string token;
	while(input >> token)
	{
		uint32_t value = 0;
		if(!TryNamedKey(token, value) && !TryRawKey(token, value))
		{
			error = "Unknown key action '" + token + "'.";
			return KeyCommandResult::Error;
		}
		requested |= value;
	}
	if(requested == 0)
	{
		error = "At least one non-zero key action or mask is required.";
		return KeyCommandResult::Error;
	}
	if(AdditionalCount(requested) > 1)
	{
		error = "Yes, No, and CtrlBack are mutually exclusive protocol keys.";
		return KeyCommandResult::Error;
	}

	uint32_t next = ToMask(state);
	if(direction == "down")
	{
		const uint32_t heldAdditional = next & AdditionalMask;
		const uint32_t requestedAdditional = requested & AdditionalMask;
		if(heldAdditional != 0 && requestedAdditional != 0 &&
			heldAdditional != requestedAdditional)
		{
			error = "Release the held additional key before pressing another.";
			return KeyCommandResult::Error;
		}
		next |= requested;
	}
	else
	{
		next &= ~requested;
	}

	state.keys = static_cast<uint16_t>(next & 0xFFFFu);
	state.additionalKey = AdditionalValue(next & AdditionalMask);
	return KeyCommandResult::Applied;
}

uint8_t PackWeaponAndAdditionalKey(uint8_t weapon, uint8_t additionalKey)
{
	if(additionalKey == 0)
		return weapon;
	return static_cast<uint8_t>((weapon & 0x3Fu) | ((additionalKey & 0x03u) << 6));
}

void ApplyAutomationKeyState(ONFOOT_SYNC_DATA &sync, const AutomationKeyState &state)
{
	sync.wKeys |= state.keys;
	sync.byteCurrentWeapon = PackWeaponAndAdditionalKey(
		sync.byteCurrentWeapon,
		state.additionalKey);
}

void ApplyAutomationKeyState(INCAR_SYNC_DATA &sync, const AutomationKeyState &state)
{
	sync.wKeys |= state.keys;
	sync.byteCurrentWeapon = PackWeaponAndAdditionalKey(
		sync.byteCurrentWeapon,
		state.additionalKey);
}

void ApplyAutomationKeyState(PASSENGER_SYNC_DATA &sync, const AutomationKeyState &state)
{
	sync.wKeys |= state.keys;
	sync.byteCurrentWeapon = PackWeaponAndAdditionalKey(
		sync.byteCurrentWeapon,
		state.additionalKey);
}
