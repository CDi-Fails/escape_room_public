#ifndef GEARSHAFT_H
#define GEARSHAFT_H

#include "ultra64.h"
#include "global.h"

struct GearShaft;

typedef void (*GearShaftActionFunc)(struct GearShaft*, PlayState*);

typedef struct GearShaft {
    DynaPolyActor dyna;
    GearShaftActionFunc actionFunc;
    s16 timer;
    f32 pieceRot;
    s16 turnDir;
} GearShaft;

#endif
