#ifndef ROTATINGPLATFORM_H
#define ROTATINGPLATFORM_H

#include "ultra64.h"
#include "global.h"

struct RotatingPlatform;

typedef void (*RotatingPlatformActionFunc)(struct RotatingPlatform*, PlayState*);

typedef enum {
    ROTATING_PLATFORM_TYPE_BASE,
    ROTATING_PLATFORM_TYPE_ARMS,
    ROTATING_PLATFORM_TYPE_ARMS_SWITCH,
    ROTATING_PLATFORM_TYPE_RAISING,
} RotatingPlatformType;

typedef struct RotatingPlatform {
    DynaPolyActor dyna;
    RotatingPlatformActionFunc actionFunc;
    ColliderCylinder collider;
    s16 cameraSetting;
    u8 type;
} RotatingPlatform;

#endif
