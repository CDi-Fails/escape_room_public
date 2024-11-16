#ifndef METALCUBE_H
#define METALCUBE_H

#include "ultra64.h"
#include "global.h"

struct MetalCube;

typedef void (*MetalCubeActionFunc)(struct MetalCube*, PlayState*);

typedef struct MetalCube {
    Actor actor;
    MetalCubeActionFunc actionFunc;
    ColliderCylinder collider;
} MetalCube;

#endif
