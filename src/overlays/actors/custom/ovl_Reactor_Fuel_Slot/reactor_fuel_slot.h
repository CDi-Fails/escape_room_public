#ifndef REACTORFUELSLOT_H
#define REACTORFUELSLOT_H

#include "ultra64.h"
#include "global.h"

struct ReactorFuelSlot;

typedef void (*ReactorFuelSlotActionFunc)(struct ReactorFuelSlot*, PlayState*);

typedef enum {
    REACTOR_FUEL_SLOT_HEAVY,
    REACTOR_FUEL_SLOT_LIGHT,
} ReactorFuelSlotType;

typedef enum {
    REACTOR_FUEL_SLOT_UPDATE_MODE_ADD,
    REACTOR_FUEL_SLOT_UPDATE_MODE_REMOVE,
} ReactorFuelSlotUpdateMode;

typedef struct ReactorFuelSlot {
    Actor actor;
    ReactorFuelSlotActionFunc actionFunc;
    s8 loadedFuelType;
    u8 prevLoadedFuelType;
} ReactorFuelSlot;

#endif
