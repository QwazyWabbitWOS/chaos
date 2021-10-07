#ifndef C_ITEM_H
#define C_ITEM_H

#define	GRAPPLE_OFF			0
#define	GRAPPLE_INAIR		1
#define	GRAPPLE_ATTACHED	2		//attached without grow/shrink
#define	GRAPPLE_STARTSHRINK	3	//start with shrink
void Grapple_Reset(edict_t* ent);

#define SCANNER_UNIT                   32
#define SCANNER_RANGE                  64
#define SCANNER_UPDATE_FREQ            3
#define SAFE_STRCAT(org,add,maxlen)    if ((strlen(org) + strlen(add)) < maxlen)    strcat(org, add);
#define LAYOUT_MAX_LENGTH              1400

qboolean	Jet_Active(edict_t* ent);
qboolean	Jet_AvoidGround(edict_t* ent);
void	Jet_BecomeExplosion(edict_t* ent, int damage);
void	Jet_ApplyJet(edict_t* ent, usercmd_t* ucmd);
void	Use_Jet(edict_t* ent, gitem_t* item);	//MATTHIAS
void	ShowScanner(edict_t* ent, char* layout);
void	Toggle_Scanner(edict_t* ent);
void	Scanner_Think(edict_t* ent);
void	Use_Invisibility(edict_t* ent, gitem_t* item);	//MATTHIAS

#endif // !C_ITEM_H
