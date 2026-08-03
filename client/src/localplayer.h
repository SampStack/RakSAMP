/*
	Updated to 0.3.7 by P3ti
*/

void SendOnFootFullSyncData(ONFOOT_SYNC_DATA *pofSync, int sendDeathNoti,
	PLAYERID followPlayerID, bool force = false);
void SendInCarFullSyncData(INCAR_SYNC_DATA *picSync, int iUseCarPos,
	PLAYERID followPlayerID, bool force = false);
void SendPassengerFullSyncData(VEHICLEID vehicleID, bool force = false);
void SendAimSyncData(DWORD dwAmmoInClip, int iReloading, PLAYERID copyFromPlayer);
void SendUnoccupiedSyncData(UNOCCUPIED_SYNC_DATA *punocSync);
void SendSpectatorData(SPECTATOR_SYNC_DATA *pSpecData);
void SendBulletData(BULLET_SYNC_DATA *pBulletData);
void SendGiveTakeDamage(bool taking, PLAYERID otherPlayerId, float damage, DWORD weaponId, DWORD bodyPart);
void HandleIncomingBulletDamage(PLAYERID attackerId, const BULLET_SYNC_DATA *bulletData);
void ResetDamageEmulation();
void ResetWeaponInventory();
void SetWeaponInventoryEntry(DWORD weaponId, DWORD ammo);
void SendWeaponInventoryUpdate(bool includeEmptySlots = false);

void SendEnterVehicleNotification(VEHICLEID VehicleID, BOOL bPassenger);
void SendExitVehicleNotification(VEHICLEID VehicleID);
void SendWastedNotification(BYTE byteDeathReason, PLAYERID WhoWasResponsible);
void NotifyVehicleDeath(VEHICLEID VehicleID);
void SendDamageVehicle(WORD vehicleID, DWORD panel, DWORD door, BYTE lights, BYTE tires);
