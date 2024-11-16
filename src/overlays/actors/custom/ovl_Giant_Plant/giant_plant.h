#ifndef GIANTPLANT_H
#define GIANTPLANT_H

#include "ultra64.h"
#include "global.h"

struct GiantPlant;

typedef void (*GiantPlantActionFunc)(struct GiantPlant*, PlayState*);

typedef enum {
    GIANT_PLANT_TYPE_POT,
    GIANT_PLANT_TYPE_PLATFORM,
    GIANT_PLANT_TYPE_HOOKSHOT,
} GiantPlantTypes;

typedef struct GiantPlant {
    DynaPolyActor dyna;
    GiantPlantActionFunc actionFunc;
    ColliderCylinder collider;
    u8 id;
    u8 type;
    u8 timer;
    f32 scaleXZ;
    f32 scaleY;
    f32 budScale;
    u8 growthPhase;
} GiantPlant;

#endif
