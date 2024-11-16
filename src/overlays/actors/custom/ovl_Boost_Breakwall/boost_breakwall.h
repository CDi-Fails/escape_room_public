#ifndef BOOST_BREAKWALL_H
#define BOOST_BREAKWALL_H

#include "ultra64.h"
#include "global.h"

struct BoostBreakwall;

typedef void (*BoostBreakwallActionFunc)(struct BoostBreakwall*, PlayState*);

typedef struct BoostBreakwall {
    DynaPolyActor dyna;
    BoostBreakwallActionFunc actionFunc;
} BoostBreakwall;

#endif
