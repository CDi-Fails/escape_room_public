#ifndef DARKNESS_H
#define DARKNESS_H

#include "ultra64.h"
#include "global.h"

struct Darkness;

typedef void (*DarknessActionFunc)(struct Darkness*, PlayState*);

typedef struct Darkness {
    Actor actor;
    DarknessActionFunc actionFunc;
    f32 fadeTimer;
    s16 subCamId;
    u8 csTimer;
} Darkness;

#endif
