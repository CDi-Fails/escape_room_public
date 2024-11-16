#ifndef SMALLGEAR_H
#define SMALLGEAR_H

#include "ultra64.h"
#include "global.h"

struct SmallGear;

typedef void (*SmallGearActionFunc)(struct SmallGear*, PlayState*);

typedef struct SmallGear {
    Actor actor;
    SmallGearActionFunc actionFunc;
    ColliderCylinder collider;
} SmallGear;

#endif
