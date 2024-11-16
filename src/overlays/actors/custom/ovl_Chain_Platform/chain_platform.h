#ifndef CHAINPLATFORM_H
#define CHAINPLATFORM_H

#include "ultra64.h"
#include "global.h"

struct ChainPlatform;

typedef void (*ChainPlatformActionFunc)(struct ChainPlatform*, PlayState*);

typedef struct ChainPlatform {
    DynaPolyActor dyna;
    ChainPlatformActionFunc actionFunc;
    s16 subCamId;
    s16 timer;
} ChainPlatform;

#endif
