#ifndef WATERSHUTTER_H
#define WATERSHUTTER_H

#include "ultra64.h"
#include "global.h"

struct WaterShutter;

typedef void (*WaterShutterActionFunc)(struct WaterShutter*, PlayState*);

typedef struct WaterShutter {
    DynaPolyActor dyna;
    WaterShutterActionFunc actionFunc;
    s16 subCamId;
    s16 timer;
} WaterShutter;

#endif
