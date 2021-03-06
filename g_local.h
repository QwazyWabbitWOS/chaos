// g_local.h -- local definitions for game module
#pragma once

/* MrG{DRGN} ty QW! */
//QW// Here's where I finally get smart and add some debugging stuff to the game.
// Don't forget to add null definitions for stuff not useable in Linux.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN	//non-MFC
#include <windows.h>
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
_CrtMemState startup1;	// memory diagnostics
#else
#define OutputDebugString	//not doing Windows
#endif

#include "q_shared.h"
#include "performance.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "game.h"
// MrG{DRGN}
#include "classindex.h"

//ZOID
#include "p_menu.h"
//ZOID

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"Chaos DM Lives v3.2c"

#ifndef _DEBUG
#define BUILD	"Release"
#else
#define BUILD	"Debug"
#endif

// protocol bytes that can be directly added to messages
#define	svc_muzzleflash		1
#define	svc_muzzleflash2	2
#define	svc_temp_entity		3
#define	svc_layout			4
#define	svc_inventory		5

//==================================================================

// view pitching times
#define DAMAGE_TIME		0.5F /* MrG{DRGN} explicit float, since this is used it calculating float variables */
#define	FALL_TIME		0.3F /* MrG{DRGN} explicit float, since this is used it calculating float variables */

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_DEATHMATCH	0x00000800
#define	SPAWNFLAG_NOT_COOP			0x00001000

// edict->flags
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	// implied immunity to drowning
#define FL_IMMUNE_LASER			0x00000004
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define FL_IMMUNE_SLIME			0x00000040
#define FL_IMMUNE_LAVA			0x00000080
#define	FL_PARTIALGROUND		0x00000100	// not all corners are valid
#define	FL_WATERJUMP			0x00000200	// player jumping out of water
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_POWER_ARMOR			0x00001000	// power armor (if any) is active
#define FL_RESPAWN				0x80000000	// used for item respawning

#define	FRAMETIME		0.1F

// memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME	765		// clear when unloading the dll
#define	TAG_LEVEL	766		// clear when loading a new level

#define MELEE_DISTANCE	80

#define BODY_QUEUE_SIZE		8

typedef enum damage_e
{
	DAMAGE_NO,
	DAMAGE_YES,			// will take damage if hit
	DAMAGE_AIM			// auto targeting recognizes this
} damage_t;

typedef enum weaponstate_e
{
	WEAPON_READY,
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING,
	WEAPON_STARTFIRE,
	WEAPON_ENDFIRE
} weaponstate_t;

typedef enum ammo_e
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS,
	AMMO_EXPLOSIVESHELLS,
	AMMO_ARROWS,
	AMMO_POISONARROWS,
	AMMO_EXPLOSIVEARROWS,
	AMMO_FLASHGRENADES,
	AMMO_LASERGRENADES,
	AMMO_POISONGRENADES,
	AMMO_PROXYMINES,
	AMMO_HOMING,
	AMMO_BUZZES,
	AMMO_VORTEX,
	AMMO_LTURRET,
	AMMO_RTURRET
} ammo_t;

//deadflag
#define DEAD_NO					0
#define DEAD_DYING				1
#define DEAD_DEAD				2
#define DEAD_RESPAWNABLE		3

//gib types
#define GIB_ORGANIC				0
#define GIB_METALLIC			1

// armor types
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4

// power armor types
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

// handedness values
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2

// game.serverflags values
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff

// noise types for PlayerNoise
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2

// edict->movetype values
typedef enum movetype_e
{
	MOVETYPE_NONE,			// never moves
	MOVETYPE_NOCLIP,		// origin and angles change with no interaction
	MOVETYPE_PUSH,			// no clip to world, push on box contact
	MOVETYPE_STOP,			// no clip to world, stops on box contact

	MOVETYPE_WALK,			// gravity
	MOVETYPE_STEP,			// gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS,			// gravity
	MOVETYPE_FLYMISSILE,	// extra size to monsters
	MOVETYPE_BOUNCE,
	MOVETYPE_FLYRICOCHET,	// MATTHIAS
	MOVETYPE_BOT			// MATTHIAS
} movetype_t;

typedef struct gitem_armor_s
{
	int		base_count;
	int		max_count;
	float	normal_protection;
	float	energy_protection;
	int		armor;
} gitem_armor_t;

//QW for metering multiple bots into game
typedef struct bot_queue_s
{
	int bot_skill;
	int team;
	char* name;
	char* model;
} bot_queue_t;

// the bot queue instance
bot_queue_t bot_queue[MAX_CLIENTS];
//QW end bot queue instance

// gitem_t->flags
#define	IT_WEAPON		1		// use makes active weapon
#define	IT_AMMO			2
#define IT_ARMOR		4
#define IT_STAY_COOP	8
#define IT_KEY			16
#define IT_POWERUP		32
//ZOID
#define IT_TECH			64
//ZOID

typedef struct gitem_s
{
	unsigned int	classindex; /* MrG{DRGN} added */
	char* classname;	// spawning name
	qboolean(*pickup)(struct edict_s* ent, struct edict_s* other);
	void	(*use)(struct edict_s* ent, struct gitem_s* item);
	void	(*drop)(struct edict_s* ent, struct gitem_s* item);
	void	(*weaponthink)(struct edict_s* ent);
	char* pickup_sound;
	char* world_model;
	int		world_model_flags;
	char* view_model;

	// client side info
	char* icon;
	char* pickup_name;	// for printing on pickup
	int		count_width;		// number of digits to display by icon

	int		quantity;		// for ammo how much, for weapons how much is used per shot
	char* ammo;			// for weapons
	int		flags;			// IT_* flags

	void* info;
	int		tag;

	char* precaches;		// string of all models, sounds, and images this item will use
} gitem_t;

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct game_locals_s
{
	char		helpmessage1[512];
	char		helpmessage2[512];
	int			helpchanged;	// flash F1 icon if non 0, play sound
								// and increment only if 1, 2, or 3

	gclient_t* clients;		// [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char		spawnpoint[512];	// needed for coop respawns

	// store latched cvars here that we want to get at often
	int			maxclients;
	int			maxentities;

	// cross level triggers
	int			serverflags;

	// items
	int			num_items;

	qboolean	autosaved;
} game_locals_t;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct level_locals_s
{
	int			framenum;
	float		time;

	char		level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)
	char		mapname[MAX_QPATH];		// the server map name (base1, etc)
	char		nextmap[MAX_QPATH];		// go here when fraglimit is hit
	char		forcemap[MAX_QPATH];	// go here

	// intermission state
	float		intermissiontime;		// time the intermission was started
	char* changemap;
	int			exitintermission;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;

	edict_t* sight_client;	// changed once each frame for coop games

	edict_t* sight_entity;
	int			sight_entity_framenum;
	edict_t* sound_entity;
	int			sound_entity_framenum;
	edict_t* sound2_entity;
	int			sound2_entity_framenum;

	int			pic_health;

	int			total_secrets;
	int			found_secrets;

	int			total_goals;
	int			found_goals;

	int			total_monsters;
	int			killed_monsters;

	edict_t* current_entity;	// entity running from G_RunFrame
	int			body_que;			// dead bodies

	int			power_cubes;		// ugly necessity for coop
} level_locals_t;

// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct spawn_temp_s
{
	// world vars
	char* sky;
	float		skyrotate;
	vec3_t		skyaxis;
	char* nextmap;

	int			lip;
	int			distance;
	int			height;
	char* noise;
	float		pausetime;
	char* item;
	char* gravity;

	float		minyaw;
	float		maxyaw;
	float		minpitch;
	float		maxpitch;
} spawn_temp_t;

typedef struct move_info_s
{
	// fixed data
	vec3_t		start_origin;
	vec3_t		start_angles;
	vec3_t		end_origin;
	vec3_t		end_angles;

	int			sound_start;
	int			sound_middle;
	int			sound_end;

	float		accel;
	float		speed;
	float		decel;
	float		distance;

	float		wait;

	// state data
	int			state;
	vec3_t		dir;
	float		current_speed;
	float		move_speed;
	float		next_speed;
	float		remaining_distance;
	float		decel_distance;
	void		(*endfunc)(edict_t*);
} moveinfo_t;

typedef struct mframe_s
{
	void	(*aifunc)(edict_t* self, float dist);
	float	dist;
	void	(*thinkfunc)(edict_t* self);
} mframe_t;

typedef struct mmove_s
{
	int			firstframe;
	int			lastframe;
	mframe_t* frame;
	void		(*endfunc)(edict_t* self);
} mmove_t;

/*typedef struct
{
	mmove_t		*currentmove;
	int			aiflags;
	int			nextframe;
	float		scale;

	void		(*stand)(edict_t *self);
	void		(*idle)(edict_t *self);
	void		(*search)(edict_t *self);
	void		(*walk)(edict_t *self);
	void		(*run)(edict_t *self);
	void		(*dodge)(edict_t *self, edict_t *other, float eta);
	void		(*attack)(edict_t *self);
	void		(*melee)(edict_t *self);
	void		(*sight)(edict_t *self, edict_t *other);
	qboolean	(*checkattack)(edict_t *self);

	float		pausetime;
	float		attack_finished;

	vec3_t		saved_goal;
	float		search_time;
	float		trail_time;
	vec3_t		last_sighting;
	int			attack_state;
	int			lefty;
	float		idle_time;
	int			linkcount;

	int			power_armor_type;
	int			power_armor_power;
} monsterinfo_t;*/

extern	game_locals_t	game;
extern	level_locals_t	level;
extern	game_import_t	gi;
extern	game_export_t	globals;
extern	spawn_temp_t	st;

extern	int	vwep_index; //QW// The index for the vwep cached model offset
extern	int	sm_meat_index;
extern	int	snd_fry;

extern	int	jacket_armor_index;
extern	int	combat_armor_index;
extern	int	body_armor_index;

extern gitem_armor_t jacketarmor_info;
extern gitem_armor_t combatarmor_info;
extern gitem_armor_t bodyarmor_info;

// means of death

typedef enum mod_e {
	MOD_UNKNOWN,
	MOD_BLASTER,
	MOD_SHOTGUN,
	MOD_SSHOTGUN,
	MOD_MACHINEGUN,
	MOD_CHAINGUN,
	MOD_GRENADE,
	MOD_G_SPLASH,
	MOD_ROCKET,
	MOD_R_SPLASH,
	MOD_HYPERBLASTER,
	MOD_RAILGUN,
	MOD_BFG_LASER,
	MOD_BFG_BLAST,
	MOD_BFG_EFFECT,
	MOD_HANDGRENADE,
	MOD_HG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_HELD_GRENADE,
	MOD_EXPLOSIVE,
	MOD_BARREL,
	MOD_BOMB,
	MOD_EXIT,
	MOD_SPLASH,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_HIT,
	MOD_TARGET_BLASTER,
	MOD_GRAPPLE,
	MOD_ARROW,
	MOD_ESSHOTGUN,
	MOD_AIRFIST,
	MOD_PROXYMINE,
	MOD_HOMING,
	MOD_BUZZ,
	MOD_VORTEX,
	MOD_KAMIKAZE,
	MOD_JETWATER,
	MOD_SWORD,
	MOD_TURRET,
	MOD_CHAINSAW,
	MOD_PGRENADE,
	MOD_FGRENADE,
	MOD_PARROW,
	MOD_AK42,
	MOD_ESSHOT_SPLASH,
	MOD_EXARROW_SPLASH,
	MOD_EXARROW
}mod_t;
#define MOD_FRIENDLY_FIRE  0x8000000

extern	int	meansOfDeath;

extern	edict_t* g_edicts;

#define q_offsetof(t, m)	((size_t)&((t *)0)->m)
#define FOFS(x)		q_offsetof(edict_t, x)
#define STOFS(x)	q_offsetof(spawn_temp_t, x)
#define	LLOFS(x)	q_offsetof(level_locals_t, x)
#define	CLOFS(x)	q_offsetof(gclient_t, x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0F * (random() - 0.5F))

extern cvar_t* maxentities;
extern cvar_t* deathmatch;
extern cvar_t* coop;
extern cvar_t* dmflags;
extern cvar_t* skill;
extern cvar_t* fraglimit;
extern cvar_t* timelimit;
//ZOID
extern	cvar_t* capturelimit;
extern	cvar_t* instantweap;
//ZOID
extern cvar_t* password;
extern cvar_t* g_select_empty;
extern cvar_t* dedicated;
extern cvar_t* game_dir;
extern cvar_t* g_maplistfile;
extern cvar_t* g_maplistmode;

extern cvar_t* sv_gravity;
extern cvar_t* sv_maxvelocity;

extern cvar_t* gun_x, * gun_y, * gun_z;
extern cvar_t* sv_rollspeed;
extern cvar_t* sv_rollangle;

extern cvar_t* run_pitch;
extern cvar_t* run_roll;
extern cvar_t* bob_up;
extern cvar_t* bob_pitch;
extern cvar_t* bob_roll;

extern cvar_t* sv_cheats;
extern cvar_t* maxclients;

// MrG{DRGN}
extern cvar_t* flood_msgs;
extern cvar_t* flood_persecond;
extern cvar_t* flood_waitdelay;
extern cvar_t* filterban;

qboolean	is_quad;     //MATTHIAS
byte		is_silenced;

#define world	(&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN		0x00000001
#define ITEM_NO_TOUCH			0x00000002
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM			0x00010000
#define	DROPPED_PLAYER_ITEM		0x00020000
#define ITEM_TARGETS_USED		0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN		2

typedef enum fieldtype_e {
	F_INT,
	F_FLOAT,
	F_LSTRING,			/* string on disk, pointer in memory, TAG_LEVEL */
	F_GSTRING,			/* string on disk, pointer in memory, TAG_GAME */
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,			/* index on disk, pointer in memory */
	F_ITEM,				/* index on disk, pointer in memory */
	F_CLIENT,			/* index on disk, pointer in memory */
	F_FUNCTION,
	F_IGNORE
} fieldtype_t;

typedef struct field_s
{
	char* name;
	int		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;

extern	field_t fields[];
extern	gitem_t	itemlist[];

//
// c_motd.c
//
void LoadMOTD(void);

//
// g_spawn.c
//
void SpawnEntities(char* mapname, char* entities, char* spawnpoint);

//
// g_save.c
//
void ReadGame(char* filename);
void WriteGame(char* filename, qboolean autosave);
void ReadLevel(char* filename);
void WriteLevel(char* filename);
void InitGame(void);

//
// g_ent.c
//

char* LoadEntFile(char* mapname, char* entities);

//
// g_cmds.c
//
char* ClientTeam(edict_t* ent);
qboolean OnSameTeam(edict_t* ent1, edict_t* ent2);
void SelectNextItem(edict_t* ent, int itflags);
void SelectPrevItem(edict_t* ent, int itflags);
void ValidateSelectedItem(edict_t* ent);
void Cmd_Give_f(edict_t* ent);
void Cmd_God_f(edict_t* ent);
void Cmd_Notarget_f(edict_t* ent);
void Cmd_Noclip_f(edict_t* ent);
void Cmd_Use_f(edict_t* ent);
void Cmd_Drop_f(edict_t* ent);
void Cmd_Inven_f(edict_t* ent);
void Cmd_InvUse_f(edict_t* ent);
void Cmd_LastWeap_f(edict_t* ent);
void Cmd_WeapPrev_f(edict_t* ent);
void Cmd_WeapNext_f(edict_t* ent);
void Cmd_WeapLast_f(edict_t* ent);
void Cmd_InvDrop_f(edict_t* ent);
void Cmd_Kill_f(edict_t* ent);
void Cmd_PutAway_f(edict_t* ent);
int PlayerSort(void const* a, void const* b);
void Cmd_Players_f(edict_t* ent);
void Cmd_Wave_f(edict_t* ent);
void Cmd_Say_f(edict_t* ent, qboolean team, qboolean arg0);
void Cmd_Help_f(edict_t* ent);
void ClientCommand(edict_t* ent);
qboolean CheckFlood(edict_t* ent);

//
// g_items.c
//
void PrecacheItem(gitem_t* it);
void InitItems(void);
void SetItemNames(void);
gitem_t* FindItem(char* pickup_name);
gitem_t* FindItemByClassname(char* classname);
gitem_t* FindItemByClassindex(unsigned int classindex);// MrG{DRGN}
#define	ITEM_INDEX(x) (int)((x)-itemlist)
edict_t* Drop_Item(edict_t* ent, gitem_t* item);
void SetRespawn(edict_t* ent, float delay);
void ChangeWeapon(edict_t* ent);
void SpawnItem(edict_t* ent, gitem_t* item);
void Think_Weapon(edict_t* ent);
int ArmorIndex(edict_t* ent);
int PowerArmorType(edict_t* ent);
gitem_t* GetItemByIndex(int index);
qboolean Add_Ammo(edict_t* ent, gitem_t* item, int count);
void Touch_Item(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);
void DoRespawn(edict_t* ent);
void Use_Weapon(edict_t* ent, gitem_t* item);
void Drop_Weapon(edict_t* ent, gitem_t* item);

//
// g_utils.c
//

#if defined _WIN32
//
// Define a noreturn wrapper for gi.error
//
__declspec(noreturn) void GameError(char* fmt, ...);
#else
__attribute__((noreturn)) void GameError(char* fmt, ...);
#endif

qboolean	KillBox(edict_t* ent);
void	G_ProjectSource(vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t* G_Find(edict_t* from, int fieldofs, char* match);
edict_t* findradius(edict_t* from, vec3_t org, float rad);
edict_t* G_PickTarget(char* targetname);
void	G_UseTargets(edict_t* ent, edict_t* activator);
void	G_SetMovedir(vec3_t angles, vec3_t movedir);

void	G_InitEdict(edict_t* e);
edict_t* G_Spawn(void);
void	G_FreeEdict(edict_t* ed);

void	G_TouchTriggers(edict_t* ent);
void	G_TouchSolids(edict_t* ent);

char* G_CopyString(const char* in);

float* tv(float x, float y, float z);
char* vtos(vec3_t v);

float vectoyaw(vec3_t vec);
void vectoangles(vec3_t value1, vec3_t angles);

//
// g_combat.c
//
qboolean OnSameTeam(edict_t* ent1, edict_t* ent2);
qboolean CanDamage(edict_t* target, edict_t* inflictor);
qboolean CheckTeamDamage(edict_t* target, edict_t* attacker);
void T_Damage(edict_t* target, edict_t* inflictor, edict_t* attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod);
void T_RadiusDamage(edict_t* inflictor, edict_t* attacker, float damage, edict_t* ignore, float radius, int mod);

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_ENERGY			0x00000004	// damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK		0x00000008	// do not affect velocity, just view angles
#define DAMAGE_BULLET			0x00000010  // damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION	0x00000020  // armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
#define DEFAULT_SHOTGUN_HSPREAD	1000
#define DEFAULT_SHOTGUN_VSPREAD	500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT	12
#define DEFAULT_SHOTGUN_COUNT	12
#define DEFAULT_SSHOTGUN_COUNT	20

//
// g_misc.c
//
void ThrowClientHead(edict_t* self, int damage);
void ThrowGib(edict_t* self, char* gibname, int damage, int type);
void BecomeExplosion1(edict_t* self);
void SP_misc_teleporter_dest(edict_t* ent);
int CountPlayers(void); // Return number of players

//
// g_ai.c
qboolean infront(edict_t* self, edict_t* other);
qboolean visible(edict_t* self, edict_t* other);

//
// g_weapon.c
//
void ThrowDebris(edict_t* self, char* modelname, float speed, vec3_t origin);
qboolean fire_hit(edict_t* self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_shotgun(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod);
void fire_blaster(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper); // MrG{DRGN} Prototype correction
void fire_grenade(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius);
void fire_grenade2(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held);
void fire_rocket(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_rail(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);
void rocket_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);
void Grenade_Touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);
void Grenade_Explode(edict_t* ent);

//
// p_client.c
//
void ShutOff(edict_t* ent);
void respawn(edict_t* self);
void SelectSpawnPoint(edict_t* ent, vec3_t origin, vec3_t angles);
void PutClientInServer(edict_t* ent);
void InitClientPersistent(gclient_t* client);
void InitClientResp(gclient_t* client);
void InitBodyQue(void);
void ClientBeginServerFrame(edict_t* ent);
void ClientBeginDeathmatch(edict_t* ent);
void TossClientWeapon(edict_t* self);
void ClientUserinfoChanged(edict_t* ent, char* userinfo);
qboolean ClientConnect(edict_t* ent, char* userinfo);
void CopyToBodyQue(edict_t* ent);
void ClientThink(edict_t* ent, usercmd_t* ucmd);
void AdjustPlayerList(edict_t* ent);
void ClientDisconnect(edict_t* ent);
void ClientObituary(edict_t* self, edict_t* inflictor, edict_t* attacker);
void Use_Plat(edict_t* ent, edict_t* other, edict_t* activator);
void trigger_elevator_use(edict_t* self, edict_t* other, edict_t* activator);
void door_use(edict_t* self, edict_t* other, edict_t* activator);
edict_t* SelectRandomDeathmatchSpawnPoint(void);
edict_t* SelectFarthestDeathmatchSpawnPoint(void);
float	PlayersRangeFromSpot(edict_t* spot);
trace_t	PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);

//
// g_player.c
//
void player_pain(edict_t* self, edict_t* other, float kick, int damage);
void player_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);

//
// g_svcmds.c
//
void	ServerCommand(void);

//
// p_view.c
//
void ClientEndServerFrame(edict_t* ent);

//
// p_hud.c
//
void Cmd_Score_f(edict_t* ent);
void BeginIntermission(edict_t* target);
void MoveClientToIntermission(edict_t* ent);
void G_SetStats(edict_t* ent);
void ValidateSelectedItem(edict_t* ent);
void DeathmatchScoreboard(edict_t* ent);
void DeathmatchScoreboardMessage(edict_t* ent, edict_t* killer);
void ClientBegin(edict_t* ent);

//
// p_weapon.c
//
void PlayerNoise(edict_t* who, vec3_t where, int type);
void P_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void Weapon_Generic(edict_t* ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int* pause_frames, int* fire_frames, void (*fire)(edict_t* ent));
qboolean	Pickup_Weapon(edict_t* ent, edict_t* other);
qboolean	Pickup_NoAmmoWeapon(edict_t* ent, edict_t* other);
qboolean	Pickup_Health(edict_t* ent, edict_t* other);
qboolean	Pickup_Ammo(edict_t* ent, edict_t* other);
void NoAmmoWeaponChange(edict_t* ent);
void Blaster_Fire(edict_t* ent, vec3_t g_offset, int damage, qboolean hyper, int effect);
void Weapon_SuperShotgun(edict_t* ent);
void Weapon_HyperBlaster(edict_t* ent);
void Weapon_RocketLauncher(edict_t* ent);
void Weapon_Grenade(edict_t* ent);
void Weapon_GrenadeLauncher(edict_t* ent);
void Weapon_Railgun(edict_t* ent);
void Weapon_BFG(edict_t* ent);
void weapon_grenade_fire(edict_t* ent, qboolean held);

// also used in c_weapon.c
#define GRENADE_TIMER				3.0F
#define GRENADE_MINSPEED			400
#define GRENADE_MAXSPEED			800

//
// m_move.c
//
qboolean M_walkmove(edict_t* ent, float yaw, float dist);
/* MrG{DRGN} ghost prototype
void M_MoveToGoal(edict_t* ent, float dist);
*/
void M_ChangeYaw(edict_t* ent);

//
// g_phys.c
//
void G_RunEntity(edict_t* ent);
void M_CheckGround(edict_t* ent);

//
// g_main.c
//
void SaveClientData(void);
void FetchClientEntData(edict_t* ent);
void G_RunFrame(void);
void ExitLevel(void);

//
// g_svcmds.c
//
qboolean SVCmd_FilterPacket(char* from);

/* MrG{DRGN} include once here, rather than in multiple files */
#include "c_base.h"
#include "c_belt.h"
#include "c_cam.h"
#include "c_flashlight.h"
#include "c_grapple.h"
#include "c_invisibility.h"
#include "c_jetpack.h"
#include "c_maplist.h"
#include "c_scanner.h"
#include "c_teleporter.h"
#include "c_weapon.h"
#include "c_botai.h"
/* END*/
//============================================================================

// client_t->anim_priority
#define	ANIM_BASIC		0		// stand / run
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define ANIM_REVERSE	-1	//vwep

// client data that stays across multiple level loads
typedef struct client_persistent_s
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];
	int			hand;

	qboolean	connected;			// a loadgame will leave valid entities that
									// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int			health;
	int			max_health;
	qboolean	powerArmorActive;

	int			selected_item;
	int			inventory[MAX_ITEMS];

	// ammo capacities
	int			max_bullets; //MATTHIAS
	int			max_shells;
	int			max_rockets;
	int			max_grenades;
	int			max_cells;
	int			max_slugs;
	int			max_eshells;
	int			max_arrows;
	int			max_poisonarrows;
	int			max_explosivearrows;
	int			max_flashgrenades;
	int			max_lasergrenades;
	int			max_poisongrenades;
	int			max_proxymines;
	int			max_homing;
	int			max_buzzes;
	int			max_vortex;
	int			max_lturret;
	int			max_rturret;

	gitem_t* weapon;
	gitem_t* lastweapon;

	int			power_cubes;	// used for tracking the cubes in coop games
	int			score;			// for calculating total unit score in coop games
} client_persistent_t;

// client data that stays across deathmatch respawns
typedef struct client_respawn_s
{
	client_persistent_t	coop_respawn;	// what to set client->pers to on a respawn
	int			enterframe;			// level.framenum the client entered the game
	int			score;				// frags, etc

	qboolean	spectator;			// MrG{DRGM} client is a spectator
	//ZOID
	int			ctf_team;			// CTF team
	int			team; // MATHIAS Deathmatch Team
	int			ctf_state;
	float		ctf_lasthurtcarrier;
	float		ctf_lastreturnedflag;
	float		ctf_flagsince;
	float		ctf_lastfraggedcarrier;
	qboolean	id_state;
	float		lastidtime;
	qboolean	voted; // for elections
	qboolean	ready;
	qboolean	admin;
	struct ghost_s* ghost; // for ghost codes
	//ZOID
	vec3_t		cmd_angles;			// angles sent over in the last command
	int			game_helpchanged;
	int			helpchanged;
} client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s
{
	// known to server
	player_state_t	ps;				// communicated by server to clients
	int				ping;

	// private to game
	client_persistent_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	// for detecting out-of-pmove changes

	qboolean	showscores;			// set layout stat
	//ZOID
	qboolean	inmenu;				// in menu
	pmenuhnd_t* menu;				// current menu
	//ZOID
	qboolean	showinventory;		// set layout stat
	qboolean	showhelp;
	qboolean	showhelpicon;

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	qboolean	weapon_thunk;

	gitem_t* newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_parmor;		// damage absorbed by power armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponstate_t	weaponstate;

	float		b_respawntime;
	float		b_nextshot;
	float		b_nextwchange;
	edict_t* b_goalitem;
	edict_t* b_closeitem;
	edict_t* b_nopathitem;
	qboolean	b_duck;
	int			b_strafedir;
	int			b_rundir;
	float		b_strafechange;
	float		b_runchange;
	float		b_nextrandjump;
	float		b_nextroam;
	edict_t* b_activator;
	edict_t* grapple;
	int         grapple_state;

	float		b_waittime;
	float		b_pausetime;

	float		b_nodetime;
	int			b_botlevel;
	edict_t* b_target;
	float		b_lastdpath;

	int			b_path[100];
	int			b_currentnode;

	edict_t* flashlight;
	edict_t* teleporter;

	int			scanneractive;
	float		nextscannercell;
	int			beltactive;
	float		nextbeltcell;
	float		nextrandomsound;
	float		nextheartbeat;
	float		nextvomit;
	int			grenadesactive;
	int			flashlightactive;
	int			fakedeath;
	int			swordstate;
	float		kamikazetime;
	float       jet_framenum;
	float       jet_remaining;
	float       jet_next_think;
	int			invisible;

	float		BlindTime;
	float		BlindBase;
	float		PoisonTime;
	float		PoisonBase;

	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;		// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	float		next_drown_time;
	int			old_waterlevel;
	int			breather_sound;

	int			machinegun_shots;	// for weapon raising

	// animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;

	// powerup timers
	float		quad_framenum;
	float		invisible_framenum;	//MATTHIAS
	float		invincible_framenum;
	float		breather_framenum;
	float		enviro_framenum;

	qboolean	grenade_blew_up;
	float		grenade_time;
	int			silencer_shots;
	int			weapon_sound;

	float		pickup_msg_time;

	float		respawn_time;		// can respawn when time > this

	// MrG{DRGN}
	float		flood_locktill;		// locked from talking
	float		flood_when[10];		// when messages were said
	int			flood_whenhead;		// head pointer for when said

	//ZOID
	//void* ctf_grapple;		// entity of grapple
	//int			ctf_grapplestate;		// true if pulling
	//float		ctf_grapplereleasetime;	// time of grapple release
	float		ctf_regentime;		// regen tech
	float		ctf_techsndtime;
	float		ctf_lasttechmsg;
	edict_t* chase_target;
	qboolean	update_chase;
	float		menutime;			// time to update menu
	qboolean	menudirty;
	//ZOID
	int    camera;	//MATTHIAS Camera

	edict_t* pTarget;
};

struct edict_s
{
	entity_state_t	s;
	struct gclient_s* client;	// NULL if not a player
									// the server expects the first part
									// of gclient_s to be a player_state_t
									// but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf

	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t* owner;

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	int			movetype;
	int			flags;

	char* model;
	float		freetime;			// sv.time when the object was freed

	//
	// only used locally in game, not by server
	//
	char* message;
	unsigned	int	classindex; /* MrG{DRGN} added */
	char* classname;
	int		classtype;
	int			spawnflags;

	float		timestamp;

	float		angle;			// set in qe3, -1 = up, -2 = down
	char* target;
	char* targetname;
	char* killtarget;
	char* team;
	char* pathtarget;
	char* deathtarget;
	char* combattarget;
	edict_t* target_ent;

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2;

	vec3_t		velocity;
	vec3_t		avelocity;
	int			mass;
	float		air_finished;
	float		gravity;		// per entity gravity multiplier (1.0 is normal)
								// use for lowgrav artifact, flares

	edict_t* goalentity;
	edict_t* movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	float		nextthink;
	void		(*prethink) (edict_t* ent);
	void		(*think)(edict_t* self);
	void		(*blocked)(edict_t* self, edict_t* other);	//move to moveinfo?
	void		(*touch)(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf);
	void		(*use)(edict_t* self, edict_t* other, edict_t* activator);
	void		(*pain)(edict_t* self, edict_t* other, float kick, int damage);
	void		(*die)(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);

	float		touch_debounce_time;		// are all these legit?  do we need more/less of them?
	float		pain_debounce_time;
	float		damage_debounce_time;
	float		fly_sound_debounce_time;	//move to clientinfo
	float		last_move_time;

	int			health;
	int			max_health;
	int			deadflag;

	float		powerarmor_time;

	char* map;			// target_changelevel

	int			viewheight;		// height above origin where eyesight is determined
	int			takedamage;
	int			dmg;
	int			radius_dmg;
	float		dmg_radius;
	int			sounds;			//make this a spawntemp var?
	int			count;

	edict_t* chain;
	edict_t* enemy;
	edict_t* other;	//MATTHIAS	General Purpose ent pointer
	edict_t* next_listitem;	//MATTHIAS item_list pointer
	edict_t* activator;
	edict_t* groundentity;
	edict_t* lastgroundentity;
	int			groundentity_linkcount;
	edict_t* teamchain;
	edict_t* teammaster;

	edict_t* mynoise;		// can go in client only
	edict_t* mynoise2;

	int			noise_index;
	float		volume;
	float		attenuation;

	// timing variables
	float		wait;
	float		delay;			// before firing targets
	float		random;

	int			watertype;
	int			waterlevel;

	vec3_t		move_origin;
	vec3_t		move_angles;

	// move this to clientinfo?
	int			style;			// also used as areaportal number

	gitem_t* item;			// for bonus items

	// common data blocks
	moveinfo_t		moveinfo;
	//monsterinfo_t	monsterinfo;

	//MATTHIAS
	float		avoidtime;
	vec3_t      spawnorigin;
	qboolean	bot_player;  // MrG{DRGN} this is a lot easier than checking the classname to track if we're dealing wiith a bot or not!
};

//MATTHIAS
void	bprint_botsafe(int printlevel, char* fmt, ...);
void	cprint_botsafe(edict_t* ent, int printlevel, char* fmt, ...);
void	nprintf(int printlevel, char* fmt, ...);
void	stuffcmd(edict_t* ent, char* s);
qboolean visible2(vec3_t spot1, vec3_t spot2);
edict_t* CreateTargetChangeLevel(char* map);
void	EndDMLevel(void);

int		numbots;
int		numnodes;
int		numred;	//size of the red team
int		numblue;	//size of the blue team
int		numturrets;
int		vortexstate;

char	motd[570];
int		numplayers;
int		path_buffer[100];				// used to exchange path infos between functions
										// dirty but fast way of doing this
int		first_pathnode;
edict_t* players[MAX_CLIENTS];
// edict_t *turrets[2];	// maximum of 3 turrets
edict_t* turrets[4];	// maximum of 3 turrets FWP - Source of turret crash, need 3 elements for 3 turrets :)
edict_t* weapon_list;
edict_t* health_list;
edict_t* powerup_list;
edict_t* ammo_list;
edict_t* vortex_pointer;	//pointer to the vortex if one is currently active
extern cvar_t* node_debug;
extern cvar_t* lightsoff;
extern cvar_t* botchat;
extern cvar_t* blindtime;
extern cvar_t* poisontime;
extern cvar_t* lasertime;
extern cvar_t* proxytime;
extern cvar_t* defence_turret_ammo;
extern cvar_t* rocket_turret_ammo;
extern cvar_t* dntg;
extern cvar_t* lasermine_health;
extern cvar_t* ex_arrow_damage;
extern cvar_t* ex_arrow_radius;
extern cvar_t* start_invulnerable_time;
extern cvar_t* developer;

/*
* MrG{DRGN} Chaos DM Lives cvars
*/
extern cvar_t* drop_tech;	// MrG{DRGN} tech drop prevention
extern cvar_t* allow_flagdrop;	// MrG{DRGN} allow flag dropping
extern cvar_t* weapon_kick; // MrG{DRGN} kickable weapons toggle

int		red_base, blue_base;	//node at red/blue flag

// ZOID
#include "g_ctf.h"
// ZOID
/* End of g_local.h */
