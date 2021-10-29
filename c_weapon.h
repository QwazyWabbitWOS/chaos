#define		FLASH_RADIUS		400
#define		POISON_RADIUS		400

void pre_target_laser_think(edict_t* self);
void target_laser_on(edict_t* self);
void target_laser_off(edict_t* self);
void target_laser_think(edict_t* self);
int Valid_Target(edict_t* ent, edict_t* blip);
void Weapon_AK42(edict_t* ent);
void Weapon_FlashGrenade(edict_t* ent);
void Weapon_LaserGrenade(edict_t* ent);
void Weapon_PoisonGrenade(edict_t* ent);
void Weapon_ProxyMineLauncher(edict_t* ent);
void Weapon_FlashGrenadeLauncher(edict_t* ent);
void Weapon_PoisonGrenadeLauncher(edict_t* ent);
void Weapon_ExplosiveSuperShotgun(edict_t* ent);
void Weapon_Sword(edict_t* ent);
void Weapon_Chainsaw(edict_t* ent);
void Weapon_Crossbow(edict_t* ent);
void Weapon_ExplosiveCrossbow(edict_t* ent);
void Weapon_PoisonCrossbow(edict_t* ent);
void Weapon_Airfist(edict_t* ent);
void Weapon_HomingLauncher(edict_t* ent);
void Weapon_Buzzsaw(edict_t* ent);
void Weapon_Vortex(edict_t* ent);
void Weapon_LaserTurret(edict_t* ent);
void Weapon_RocketTurret(edict_t* ent);
void Turret_Die(edict_t* ent, edict_t* inflicter, edict_t* attacker, int damage, vec3_t point);
void Proxy_Die(edict_t* self, edict_t* inflicter, edict_t* attacker, int damage, vec3_t point);
