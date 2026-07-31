#pragma once

#include <cstdint>

namespace raksamp::damage
{
	constexpr std::uint32_t TorsoBodyPart = 3;

	struct Vitals
	{
		float health;
		float armour;
	};

	bool TryGetWeaponDamage(std::uint32_t weaponId, float &damage);
	bool TryNormalizeTakenDamage(
		std::uint32_t weaponId,
		float reportedDamage,
		bool hasIssuer,
		float &damage);
	bool IsBulletWeapon(std::uint32_t weaponId);
	bool IsMeleeWeapon(std::uint32_t weaponId);
	bool IsGivenDamageWeapon(std::uint32_t weaponId);
	bool IsTakenDamageSource(std::uint32_t weaponId, bool hasIssuer);
	float GetWeaponRange(std::uint32_t weaponId);
	int GetWeaponSlot(std::uint32_t weaponId);
	std::uint32_t GetMinimumHitIntervalMs(std::uint32_t weaponId);
	bool IsFiniteScalar(float value);
	bool IsFiniteVector(const float value[3]);
	float DistanceSquared(const float lhs[3], const float rhs[3]);
	Vitals ApplyArmourFirst(Vitals current, float damage);
}
