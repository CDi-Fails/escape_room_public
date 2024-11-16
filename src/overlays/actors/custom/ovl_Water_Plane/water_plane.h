#ifndef WATERPLANE_H
#define WATERPLANE_H

#include "ultra64.h"
#include "global.h"

struct WaterPlane;

typedef void (*WaterPlaneActionFunc)(struct WaterPlane*, PlayState*);

typedef enum {
    WATER_PLANE_TYPE_LOWER,
    WATER_PLANE_TYPE_RAISE_TWICE,
} WaterPlaneType;

typedef struct WaterPlane {
    Actor actor;
    WaterPlaneActionFunc actionFunc;
    WaterBox* waterbox;
    f32 prevWaterSurfaceY;
    f32 homeWaterSurfaceY;
    f32 moveDist;
    u8 type;
} WaterPlane;

#endif
