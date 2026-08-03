#pragma once

#include <cstdint>
#include <string>

struct _ONFOOT_SYNC_DATA;
struct _INCAR_SYNC_DATA;
struct _PASSENGER_SYNC_DATA;

struct AutomationKeyState
{
	uint16_t keys = 0;
	uint8_t additionalKey = 0;
};

enum class KeyCommandResult
{
	NotKeyCommand,
	Applied,
	Error
};

KeyCommandResult ApplyKeyCommand(
	const std::string &command,
	AutomationKeyState &state,
	std::string &error);

uint8_t PackWeaponAndAdditionalKey(uint8_t weapon, uint8_t additionalKey);

void ApplyAutomationKeyState(_ONFOOT_SYNC_DATA &sync, const AutomationKeyState &state);
void ApplyAutomationKeyState(_INCAR_SYNC_DATA &sync, const AutomationKeyState &state);
void ApplyAutomationKeyState(_PASSENGER_SYNC_DATA &sync, const AutomationKeyState &state);

const AutomationKeyState &GetAutomationKeyState();
void ResetAutomationKeyState();
