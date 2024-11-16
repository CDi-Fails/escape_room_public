#ifndef FIREPLACE_H
#define FIREPLACE_H

#include "ultra64.h"
#include "global.h"

struct Fireplace;

typedef void (*FireplaceActionFunc)(struct Fireplace*, PlayState*);

typedef enum {
    FIREPLACE_TYPE_BASE,
    FIREPLACE_TYPE_FIRE,
} FireplaceType;

typedef struct Fireplace {
    DynaPolyActor dyna;
    FireplaceActionFunc actionFunc;
    ColliderCylinder colliderFire;
    f32 flameScale;
    f32 slideAccel;
    s16 timer;
    s16 subCamId;
    u8 type;
} Fireplace;

#endif
