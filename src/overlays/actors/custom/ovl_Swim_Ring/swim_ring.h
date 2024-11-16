#ifndef SWIMRING_H
#define SWIMRING_H

#include "ultra64.h"
#include "global.h"

struct SwimRing;

typedef void (*SwimRingActionFunc)(struct SwimRing*, PlayState*);

typedef struct SwimRing {
    DynaPolyActor dyna;
    SwimRingActionFunc actionFunc;
    s16 timer;
    u8 swamThrough;
    u8 alpha;
} SwimRing;

#endif
