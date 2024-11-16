#ifndef REACTORRODS_H
#define REACTORRODS_H

#include "ultra64.h"
#include "global.h"

struct ReactorRods;

typedef void (*ReactorRodsActionFunc)(struct ReactorRods*, PlayState*);

typedef enum {
    REACTOR_RODS_CONTROL,
    REACTOR_RODS_RODS,
} ReactorRodsType;

typedef struct ReactorRods {
    DynaPolyActor dyna;
    ReactorRodsActionFunc actionFunc;
    s16 timer;
    s16 initTurnAngle;
    s16 turnAngle;
    s16 prevRotY;
    s16 playerPrevRotY;
    u8 type;
} ReactorRods;

#endif
