#ifndef STEAM_H
#define STEAM_H

#include "ultra64.h"
#include "global.h"

struct Steam;

typedef void (*SteamActionFunc)(struct Steam*, PlayState*);

typedef enum {
    STEAM_TYPE_SMALL_TIMED,
    STEAM_TYPE_LARGE_SWITCH,
} SteamType;

typedef struct Steam {
    Actor actor;
    SteamActionFunc actionFunc;
    ColliderCylinder collider;
    s16 timer;
    u8 isOff;
    u8 type;
} Steam;

#endif
