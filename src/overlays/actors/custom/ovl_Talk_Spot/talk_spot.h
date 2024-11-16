#ifndef TALKSPOT_H
#define TALKSPOT_H

#include "ultra64.h"
#include "global.h"

struct TalkSpot;

typedef void (*TalkSpotActionFunc)(struct TalkSpot*, PlayState*);

typedef struct TalkSpot {
    Actor actor;
    TalkSpotActionFunc actionFunc;
} TalkSpot;

#endif
