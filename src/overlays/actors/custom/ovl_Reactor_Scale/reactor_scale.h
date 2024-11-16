#ifndef REACTORSCALE_H
#define REACTORSCALE_H

#include "ultra64.h"
#include "global.h"

struct ReactorScale;

typedef void (*ReactorScaleActionFunc)(struct ReactorScale*, PlayState*);

typedef struct ReactorScale {
    DynaPolyActor dyna;
    ReactorScaleActionFunc actionFunc;
    f32 totalOffsetY;
} ReactorScale;

#endif
