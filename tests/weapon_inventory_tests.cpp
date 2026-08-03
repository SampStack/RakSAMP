#include <cassert>
#include <cstdint>
#include <vector>

#include "weapon_inventory.h"

namespace
{
	struct RecordingStream
	{
		std::vector<std::uint32_t> values;

		template <typename Value>
		void Write(Value value)
		{
			values.push_back(static_cast<std::uint32_t>(value));
		}
	};
}

int main()
{
	WeaponInventory inventory = {};
	inventory[5] = { 31, 65535 };

	RecordingStream incremental;
	WriteWeaponInventorySlots(incremental, inventory, false);
	assert((incremental.values ==
		std::vector<std::uint32_t>{ 5, 31, 65535 }));

	inventory = {};
	RecordingStream reset;
	WriteWeaponInventorySlots(reset, inventory, true);
	assert(reset.values.size() == WeaponInventorySlotCount * 3);
	for(std::size_t slot = 0; slot < WeaponInventorySlotCount; ++slot)
	{
		assert(reset.values[slot * 3] == slot);
		assert(reset.values[slot * 3 + 1] == 0);
		assert(reset.values[slot * 3 + 2] == 0);
	}

	return 0;
}
