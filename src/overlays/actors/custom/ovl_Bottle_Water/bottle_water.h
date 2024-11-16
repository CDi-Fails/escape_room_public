#ifndef BOTTLEWATER_H
#define BOTTLEWATER_H

#include "ultra64.h"
#include "global.h"

struct BottleWater;

typedef void (*BottleWaterActionFunc)(struct BottleWater*, PlayState*);

typedef struct BottleWater {
    Actor actor;
    BottleWaterActionFunc actionFunc;
    ColliderCylinder collider;
    s16 timer;
    s16 alpha;
} BottleWater;

#endif
