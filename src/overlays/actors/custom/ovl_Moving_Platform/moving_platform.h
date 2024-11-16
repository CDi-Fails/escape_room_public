#ifndef MOVINGPLATFORM_H
#define MOVINGPLATFORM_H

#include "ultra64.h"
#include "global.h"

struct MovingPlatform;

typedef void (*MovingPlatformActionFunc)(struct MovingPlatform*, PlayState*);

typedef enum {
    MOVING_PLATFORM_TYPE_TIMER,
    MOVING_PLATFORM_TYPE_STEP_ON
} MovingPlatformType;

typedef struct MovingPlatform {
    DynaPolyActor dyna;
    MovingPlatformActionFunc actionFunc;
    s16 cameraSetting;
    s16 moveDir;
    u8 type;
    u8 timer;
} MovingPlatform;

#endif
