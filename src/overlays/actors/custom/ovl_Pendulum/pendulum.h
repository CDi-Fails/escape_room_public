#ifndef PENDULUM_H
#define PENDULUM_H

#include "ultra64.h"
#include "global.h"

struct Pendulum;

typedef void (*PendulumActionFunc)(struct Pendulum*, PlayState*);

typedef struct Pendulum {
    Actor actor;
    PendulumActionFunc actionFunc;
    ColliderCylinder collider;
    u8 type;
    f32 swingAngle;
    f32 angularAccel;
    f32 angularVel;
} Pendulum;

#endif
