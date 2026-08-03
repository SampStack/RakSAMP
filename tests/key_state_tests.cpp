#include <cassert>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include "native_compat.h"
#endif
#include "common.h"
#include "key_state.h"

int main()
{
	AutomationKeyState state;
	std::string error;

	assert(ApplyKeyCommand("hello", state, error) == KeyCommandResult::NotKeyCommand);
	assert(ApplyKeyCommand("!key down fire lalt", state, error) == KeyCommandResult::Applied);
	assert(state.keys == (KEY_FIRE | KEY_WALK));
	assert(state.additionalKey == 0);

	assert(ApplyKeyCommand("!key down y", state, error) == KeyCommandResult::Applied);
	assert(state.additionalKey == 1);
	assert(ApplyKeyCommand("!key down n", state, error) == KeyCommandResult::Error);
	assert(state.additionalKey == 1);
	assert(ApplyKeyCommand("!key up yes fire", state, error) == KeyCommandResult::Applied);
	assert(state.keys == KEY_WALK);
	assert(state.additionalKey == 0);

	assert(ApplyKeyCommand("!key down 0x4002", state, error) == KeyCommandResult::Applied);
	assert(state.keys == (KEY_WALK | KEY_CROUCH | KEY_ANALOG_RIGHT));
	assert(ApplyKeyCommand("!key up 16384 crouch walk", state, error) == KeyCommandResult::Applied);
	assert(state.keys == 0);

	assert(ApplyKeyCommand("!key down 0x30000", state, error) == KeyCommandResult::Error);
	assert(ApplyKeyCommand("!key sideways fire", state, error) == KeyCommandResult::Error);
	assert(ApplyKeyCommand("!key down keyboard_q", state, error) == KeyCommandResult::Error);

	state = { KEY_JUMP, 3 };
	ONFOOT_SYNC_DATA onFoot{};
	onFoot.byteCurrentWeapon = 24;
	ApplyAutomationKeyState(onFoot, state);
	assert(onFoot.wKeys == KEY_JUMP);
	assert(onFoot.byteCurrentWeapon == static_cast<BYTE>(24 | (3 << 6)));

	INCAR_SYNC_DATA driver{};
	driver.byteCurrentWeapon = 31;
	ApplyAutomationKeyState(driver, state);
	assert(driver.wKeys == KEY_JUMP);
	assert(driver.byteCurrentWeapon == static_cast<BYTE>(31 | (3 << 6)));

	PASSENGER_SYNC_DATA passenger{};
	passenger.byteSeatFlags = 5;
	passenger.byteCurrentWeapon = 46;
	ApplyAutomationKeyState(passenger, state);
	assert(passenger.byteSeatFlags == 5);
	assert(passenger.wKeys == KEY_JUMP);
	assert(passenger.byteCurrentWeapon == static_cast<BYTE>(46 | (3 << 6)));

	AutomationKeyState empty;
	ONFOOT_SYNC_DATA followed{};
	followed.wKeys = KEY_SPRINT;
	followed.byteCurrentWeapon = static_cast<BYTE>(22 | (2 << 6));
	ApplyAutomationKeyState(followed, empty);
	assert(followed.wKeys == KEY_SPRINT);
	assert(followed.byteCurrentWeapon == static_cast<BYTE>(22 | (2 << 6)));

	assert(PackWeaponAndAdditionalKey(0xFF, 0) == 0xFF);
	return 0;
}
