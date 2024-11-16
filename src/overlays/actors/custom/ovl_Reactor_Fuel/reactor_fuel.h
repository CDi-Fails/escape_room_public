#ifndef REACTORFUEL_H
#define REACTORFUEL_H

#include "ultra64.h"
#include "global.h"

struct ReactorFuel;

typedef void (*ReactorFuelActionFunc)(struct ReactorFuel*, PlayState*);

typedef enum {
    REACTOR_FUEL_TYPE_HEAVY,
    REACTOR_FUEL_TYPE_LIGHT,
} ReactorFuelType;

typedef struct ReactorFuel {
    Actor actor;
    ReactorFuelActionFunc actionFunc;
    ColliderCylinder collider;
    u8 fuelType;
} ReactorFuel;

#endif
