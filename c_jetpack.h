#ifndef C_JETPACK_H
#define C_JETPACK_H

cvar_t* ban_jetpack, * start_jetpack;
void JetpackInitVars(void);

gitem_t* it_jetpack;

void Use_Jet(edict_t* ent, gitem_t* item);
qboolean Jet_AvoidGround(edict_t* ent);
qboolean Jet_Active(edict_t* ent);
void Jet_BecomeExplosion(edict_t* ent, int damage);
void Jet_ApplyLifting(edict_t* ent);
void Jet_ApplySparks(edict_t* ent);
void Jet_ApplyRolling(edict_t* ent, const vec3_t right);
void Jet_ApplyJet(edict_t* ent, usercmd_t* ucmd);

#endif
