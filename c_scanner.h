#ifndef C_SCANNER_H
#define C_SCANNER_H

#define SCANNER_UNIT                   32
#define SCANNER_RANGE                  64
#define SCANNER_UPDATE_FREQ            3
#define SAFE_STRCAT(org,add,maxlen)    if ((strlen(org) + strlen(add)) < maxlen)    strcat(org, add);

// MrG{DRGN} never in game, but always posessed!
gitem_t* it_scanner;

void ShowScanner(edict_t* ent, char* layout);
void Toggle_Scanner(edict_t* ent);
void Scanner_Think(edict_t* ent);

#endif
