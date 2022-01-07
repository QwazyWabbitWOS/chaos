#pragma once

#define	GRAPPLE_OFF			0
#define	GRAPPLE_INAIR		1
#define	GRAPPLE_ATTACHED	2		//attached without grow/shrink
#define	GRAPPLE_STARTSHRINK	3	//start with shrink

cvar_t* ban_grapple, * start_grapple;
void GrappleInitVars(void);

// MrG{DRGN} never in game, but always possessed!
gitem_t* it_grapple;

void ShutOff_Grapple(edict_t* ent);
void Grapple_Reset(edict_t* ent);
void Grapple_DrawCable(edict_t* ent);
void Grapple_Touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);
void Grapple_Think(edict_t* ent);
void Grapple_Fire(edict_t* ent);
void Cmd_Hook_f(edict_t* ent);
void Cmd_Grapple_f(edict_t* ent);
