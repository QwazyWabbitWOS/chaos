#ifndef C_CAM_H
#define C_CAM_H

void CreateCamera(edict_t* ent);
void CamNext(edict_t* ent);
void CamPrev(edict_t* ent);
void CameraThink(edict_t* ent, usercmd_t* ucmd);

#endif
