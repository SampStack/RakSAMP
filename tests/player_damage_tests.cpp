#include "player_damage.h"

#include <cassert>
#include <cmath>
#include <limits>

namespace
{
	bool Near(float lhs, float rhs)
	{
		return std::fabs(lhs - rhs) < 0.001f;
	}
}

int main()
{
	float damage = 0.0f;
	assert(raksamp::damage::TryGetWeaponDamage(24, damage));
	assert(Near(damage, 46.2f));
	assert(raksamp::damage::TryGetWeaponDamage(31, damage));
	assert(Near(damage, 9.9f));
	assert(!raksamp::damage::TryGetWeaponDamage(19, damage));
	assert(!raksamp::damage::TryGetWeaponDamage(255, damage));
	assert(raksamp::damage::IsBulletWeapon(24));
	assert(raksamp::damage::IsBulletWeapon(38));
	assert(!raksamp::damage::IsBulletWeapon(0));
	assert(!raksamp::damage::IsBulletWeapon(35));
	assert(raksamp::damage::IsMeleeWeapon(0));
	assert(raksamp::damage::IsMeleeWeapon(15));
	assert(!raksamp::damage::IsMeleeWeapon(16));
	assert(raksamp::damage::IsGivenDamageWeapon(0));
	assert(raksamp::damage::IsGivenDamageWeapon(24));
	assert(!raksamp::damage::IsGivenDamageWeapon(51));
	assert(raksamp::damage::GetWeaponSlot(24) == 2);
	assert(raksamp::damage::GetWeaponSlot(35) == 7);
	assert(raksamp::damage::GetWeaponSlot(40) == 12);
	assert(raksamp::damage::GetWeaponSlot(51) == -1);
	assert(raksamp::damage::IsTakenDamageSource(51, true));
	assert(raksamp::damage::IsTakenDamageSource(54, false));
	assert(!raksamp::damage::IsTakenDamageSource(54, true));

	assert(raksamp::damage::TryNormalizeTakenDamage(51, 1.0f, true, damage));
	assert(Near(damage, 82.5f));
	assert(raksamp::damage::TryNormalizeTakenDamage(54, 200.0f, false, damage));
	assert(Near(damage, 100.0f));
	assert(!raksamp::damage::TryNormalizeTakenDamage(54, 10.0f, true, damage));

	auto armourOnly = raksamp::damage::ApplyArmourFirst({ 100.0f, 50.0f }, 46.2f);
	assert(Near(armourOnly.health, 100.0f));
	assert(Near(armourOnly.armour, 3.8f));

	auto spill = raksamp::damage::ApplyArmourFirst({ 100.0f, 10.0f }, 46.2f);
	assert(Near(spill.health, 63.8f));
	assert(Near(spill.armour, 0.0f));

	auto killed = raksamp::damage::ApplyArmourFirst({ 20.0f, 0.0f }, 46.2f);
	assert(Near(killed.health, 0.0f));
	assert(Near(killed.armour, 0.0f));

	auto invalid = raksamp::damage::ApplyArmourFirst(
		{ 100.0f, 25.0f }, std::numeric_limits<float>::quiet_NaN());
	assert(Near(invalid.health, 100.0f));
	assert(Near(invalid.armour, 25.0f));

	const float origin[3] = { 0.0f, 0.0f, 0.0f };
	const float target[3] = { 3.0f, 4.0f, 0.0f };
	assert(Near(raksamp::damage::DistanceSquared(origin, target), 25.0f));
	assert(raksamp::damage::IsFiniteScalar(1.0f));
	assert(!raksamp::damage::IsFiniteScalar(
		std::numeric_limits<float>::quiet_NaN()));
	assert(raksamp::damage::IsFiniteVector(target));

	return 0;
}
