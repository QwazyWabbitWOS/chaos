#define	AK42_FREQ				0.6f
#define	SHOTGUN_FREQ			1
#define	SUPERSHOTGUN_FREQ		1
#define	CROSSBOW_FREQ			1
#define	CHAINSAW_FREQ			0
#define	AIRFIST_FREQ			1
#define GRENADE_FREQ			0.5f
#define	GRENADELAUNCHER_FREQ	0.9f
#define	ROCKETLAUNCHER_FREQ		0.8f
#define	HOMINGLAUNCHER_FREQ		0.8f
#define	HYPERBLASTER_FREQ		0
#define	RAILGUN_FREQ			1.5f
#define	BUZZSAW_FREQ			1
#define BFG_FREQ				2.8f
#define VORTEX_FREQ				1
#define SWORD_FREQ				0
#define	CHAINSAW_FREQ			0
#define TURRET_FREQ				3

#define	RUN_SPEED				300
#define STRAFE_SPEED			200
#define	IDEAL_ENEMY_DIST		200
#define	MELEE_DIST				80
#define	CHANGEWEAPON_DELAY		0.8f
#define	SIGHT_FIRE_DELAY		0.5f

#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

#define	BOTCHAT_INSULT			0
#define	BOTCHAT_KILL			1
#define	BOTCHAT_TEAMKILL		2
#define	BOTCHAT_SELFKILL		3

#define	NUM_CHATSECTIONS		4
#define	MAX_LINES_PER_SECTION		64

char* chat_text[NUM_CHATSECTIONS][MAX_LINES_PER_SECTION];
int		chat_linecount[NUM_CHATSECTIONS];

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2
//Bot functions
void SVCmd_addbots_f(void);
void SVCmd_killbot_f(char* name);

int Riding_Plat(edict_t* ent);
edict_t* Bot_FindBestWeapon(edict_t* ent);
edict_t* Bot_FindBestPowerup(edict_t* ent);
edict_t* Bot_FindBestHealth(edict_t* ent);
edict_t* Bot_FindBestAmmo(edict_t* ent);
qboolean visible_node(vec3_t spot1, vec3_t spot2);

int Bot_CalcPath(edict_t* ent, vec3_t target, vec3_t source);
int Bot_FindPath(edict_t* ent, vec3_t target, vec3_t source);
void CTFBotJoinTeam(edict_t* ent, int desired_team);
void Bot_Create(int accuracy_level, int team, char* name, char* skin);
void Bot_Spawn(edict_t* ent);
void Bot_Respawn(edict_t* ent);
void Bot_Wave(edict_t* ent, int i, float time);
void Bot_Say(edict_t* ent, qboolean team, char* fmt, ...);
void bot_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);
void Bot_Think(edict_t* ent);
void Bot_Aim(edict_t* ent, vec3_t target, vec3_t angles);
void Bot_Attack(edict_t* ent, usercmd_t* cmd, vec3_t angles, vec3_t target);
void bot_pain(edict_t* ent, edict_t* other, float kickback, int damage);
void Bot_Move(edict_t* ent, usercmd_t* cmd);
void Bot_Strafe(edict_t* ent, usercmd_t* cmd, int strafedir, short speed, vec3_t angles);
void Bot_Jump(edict_t* ent, usercmd_t* cmd);
void Bot_FindEnemy(edict_t* self);
qboolean Bot_CanReachSpotDirectly(edict_t* ent, vec3_t target);
void BotClearGlobals(void);

edict_t* Bot_FindCloseItem(edict_t* ent);
edict_t* Bot_FindItem(edict_t* ent);

void Bot_FindActivator(edict_t* ent);
float Bot_Fire_Freq(edict_t* ent);
qboolean Bot_CanPickupItem(edict_t* ent, edict_t* eitem);
void Bot_ProjectileAvoidance(edict_t* self, usercmd_t* cmd, vec3_t angles);
void Bot_BestCloseWeapon(edict_t* self);
void Bot_BestMidWeapon(edict_t* self);
void Bot_BestFarWeapon(edict_t* self);
int Bot_CanPickupArmor(edict_t* self, edict_t* ent);
qboolean Bot_CanPickupAmmo(edict_t* ent, edict_t* eitem);
qboolean  Bot_NeedsHealth(edict_t* ent, edict_t* eitem);
qboolean SaveMoveDir(edict_t* self, short forwardmove, short sidemove, vec3_t angles);
qboolean CheckFall(edict_t* self, short forwardmove, short sidemove, vec3_t angles);

qboolean Node_FallMove(edict_t* self, short forwardmove, short sidemove, vec3_t angles);
qboolean Node_LavaMove(edict_t* self, short forwardmove, short sidemove, vec3_t angles);

char* Get_RandomBotSkin(void);
void Load_BotChat(void);
