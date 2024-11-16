#ifndef WATERPIPE_H
#define WATERPIPE_H

#include "ultra64.h"
#include "global.h"

struct WaterPipe;

typedef void (*WaterPipeActionFunc)(struct WaterPipe*, PlayState*);

typedef struct WaterPipe {
    DynaPolyActor dyna;
    WaterPipeActionFunc actionFunc;
    ColliderCylinder collider;
    s16 timer;
    s16 alpha;
} WaterPipe;

#endif
