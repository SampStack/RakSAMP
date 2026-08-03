#include "player_damage.h"

#include "finite_value.h"

#include <algorithm>
#include <cmath>

namespace raksamp::damage
{
	bool IsBulletWeapon(std::uint32_t weaponId)
	{
		return (weaponId >= 22 && weaponId <= 34) || weaponId == 38;
	}

	bool IsMeleeWeapon(std::uint32_t weaponId)
	{
		return weaponId <= 15;
	}

	bool IsGivenDamageWeapon(std::uint32_t weaponId)
	{
		return IsMeleeWeapon(weaponId) || IsBulletWeapon(weaponId);
	}

	bool IsTakenDamageSource(std::uint32_t weaponId, bool hasIssuer)
	{
		return weaponId == 37 || weaponId == 49 ||
			weaponId == 50 || weaponId == 51 ||
			(!hasIssuer && (weaponId == 53 || weaponId == 54));
	}

	bool TryGetWeaponDamage(std::uint32_t weaponId, float &damage)
	{
		if(weaponId <= 18)
			damage = 5.0f;
		else
		{
			switch(weaponId)
			{
				case 22: damage = 8.25f; break;
				case 23: damage = 13.2f; break;
				case 24: damage = 46.2f; break;
				case 25:
				case 26:
				case 27: damage = 30.0f; break;
				case 28:
				case 32: damage = 6.6f; break;
				case 29: damage = 8.25f; break;
				case 30:
				case 31: damage = 9.9f; break;
				case 33: damage = 24.8f; break;
				case 34: damage = 41.3f; break;
				case 35:
				case 36:
				case 37:
				case 39:
				case 40:
				case 41:
				case 42: damage = 5.0f; break;
				case 38: damage = 46.2f; break;
				default: return false;
			}
		}

		return true;
	}

	bool TryNormalizeTakenDamage(
		std::uint32_t weaponId,
		float reportedDamage,
		bool hasIssuer,
		float &damage)
	{
		if(!raksamp::numeric::IsFinite(reportedDamage) || reportedDamage <= 0.0f ||
			!IsTakenDamageSource(weaponId, hasIssuer))
			return false;

		switch(weaponId)
		{
			case 37: damage = std::min(reportedDamage, 5.0f); break;
			case 49: damage = std::min(reportedDamage, 35.0f); break;
			case 50: damage = std::min(reportedDamage, 50.0f); break;
			case 51: damage = 82.5f; break;
			case 53: damage = std::min(reportedDamage, 10.0f); break;
			case 54: damage = std::min(reportedDamage, 100.0f); break;
			default: return false;
		}

		return true;
	}

	float GetWeaponRange(std::uint32_t weaponId)
	{
		if(weaponId <= 18)
			return 4.0f;
		if(weaponId >= 25 && weaponId <= 27)
			return 65.0f;
		if(weaponId == 33)
			return 250.0f;
		if(weaponId == 34)
			return 350.0f;
		if(weaponId == 30 || weaponId == 31 || weaponId == 38)
			return 200.0f;
		if(weaponId >= 22 && weaponId <= 32)
			return 125.0f;
		return 50.0f;
	}

	int GetWeaponSlot(std::uint32_t weaponId)
	{
		if(weaponId == 1) return 0;
		if(weaponId >= 2 && weaponId <= 9) return 1;
		if(weaponId >= 22 && weaponId <= 24) return 2;
		if(weaponId >= 25 && weaponId <= 27) return 3;
		if(weaponId == 28 || weaponId == 29 || weaponId == 32) return 4;
		if(weaponId == 30 || weaponId == 31) return 5;
		if(weaponId == 33 || weaponId == 34) return 6;
		if(weaponId >= 35 && weaponId <= 38) return 7;
		if((weaponId >= 16 && weaponId <= 18) || weaponId == 39) return 8;
		if(weaponId >= 41 && weaponId <= 43) return 9;
		if(weaponId >= 10 && weaponId <= 15) return 10;
		if(weaponId >= 44 && weaponId <= 46) return 11;
		if(weaponId == 40) return 12;
		return -1;
	}

	std::uint32_t GetMinimumHitIntervalMs(std::uint32_t weaponId)
	{
		switch(weaponId)
		{
			case 24: return 500;
			case 25:
			case 26:
			case 27:
			case 33:
			case 34: return 250;
			case 22:
			case 23: return 150;
			default: return 35;
		}
	}

	bool IsFiniteScalar(float value)
	{
		return raksamp::numeric::IsFinite(value);
	}

	bool IsFiniteVector(const float value[3])
	{
		return value != nullptr &&
			raksamp::numeric::IsFinite(value[0]) &&
			raksamp::numeric::IsFinite(value[1]) &&
			raksamp::numeric::IsFinite(value[2]);
	}

	float DistanceSquared(const float lhs[3], const float rhs[3])
	{
		const float x = lhs[0] - rhs[0];
		const float y = lhs[1] - rhs[1];
		const float z = lhs[2] - rhs[2];
		return x * x + y * y + z * z;
	}

	Vitals ApplyArmourFirst(Vitals current, float damage)
	{
		current.health = std::clamp(current.health, 0.0f, 100.0f);
		current.armour = std::clamp(current.armour, 0.0f, 100.0f);

		if(!raksamp::numeric::IsFinite(damage) || damage <= 0.0f)
			return current;

		const float armourDamage = std::min(current.armour, damage);
		current.armour -= armourDamage;
		current.health = std::max(0.0f, current.health - (damage - armourDamage));
		return current;
	}
}
