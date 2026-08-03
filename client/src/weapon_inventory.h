#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct WeaponInventoryEntry
{
	std::uint8_t weaponId;
	std::uint16_t ammo;
};

constexpr std::size_t WeaponInventorySlotCount = 13;
using WeaponInventory =
	std::array<WeaponInventoryEntry, WeaponInventorySlotCount>;

template <typename Stream>
void WriteWeaponInventorySlots(
	Stream &stream,
	const WeaponInventory &inventory,
	bool includeEmptySlots)
{
	for(std::size_t slot = 0; slot < inventory.size(); ++slot)
	{
		const auto &entry = inventory[slot];
		if(!includeEmptySlots &&
			(entry.weaponId == 0 || entry.ammo == 0))
			continue;

		stream.Write(static_cast<std::uint8_t>(slot));
		stream.Write(entry.weaponId);
		stream.Write(entry.ammo);
	}
}
