#ifndef SEED_H
#define SEED_H

#include "ultra64.h"
#include "global.h"

struct Seed;

typedef void (*SeedActionFunc)(struct Seed*, PlayState*);

typedef struct Seed {
    Actor actor;
    SeedActionFunc actionFunc;
    ColliderCylinder collider;
    u8 plantType;
} Seed;

#endif
