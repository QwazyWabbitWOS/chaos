#ifndef C_TELEPORTER_H
#define C_TELEPORTER_H

void Shutoff_teleporter(edict_t* ent);

// MrG{DRGN} never in game, but always posessed!
gitem_t* it_teleporter;

void Teleport_Think(edict_t* ent);
void Teleport(edict_t* ent);
void Cmd_Teleport_f(edict_t* ent);

#endif





