#ifndef REACTORFANS_H
#define REACTORFANS_H

#include "ultra64.h"
#include "global.h"

struct ReactorFans;

typedef void (*ReactorFansActionFunc)(struct ReactorFans*, PlayState*);

typedef struct {
    DynaPolyActor dyna;
    Vec3f relativeActorPos;
    ReactorFansActionFunc actionFunc;
    ColliderCylinder collider;
    s16 propellerRot;
    f32 power;
    f32 scale;
} ReactorFans;

#endif