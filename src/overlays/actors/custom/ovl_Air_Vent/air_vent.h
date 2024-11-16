#ifndef AIRVENT_H
#define AIRVENT_H

#include "ultra64.h"
#include "global.h"

struct AirVent;

typedef void (*AirVentActionFunc)(struct AirVent*, PlayState*);

typedef struct AirVent {
    DynaPolyActor dyna;
    AirVentActionFunc actionFunc;
    ColliderCylinder collider;
} AirVent;

#endif
