#ifndef C_ITEM_H
#define C_ITEM_H

#define LAYOUT_MAX_LENGTH              1400

qboolean	Jet_Active(edict_t* ent);
qboolean	Jet_AvoidGround(edict_t* ent);
void	Jet_BecomeExplosion(edict_t* ent, int damage);
void	Jet_ApplyJet(edict_t* ent, usercmd_t* ucmd);
void	Use_Jet(edict_t* ent, gitem_t* item);	//MATTHIAS
void	Use_Invisibility(edict_t* ent, gitem_t* item);	//MATTHIAS

#endif // !C_ITEM_H
