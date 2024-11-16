#ifndef DUMMY_H
#define DUMMY_H

#include "ultra64.h"
#include "global.h"

struct Dummy;

typedef void (*DummyActionFunc)(struct Dummy*, PlayState*);

typedef struct Dummy {
    Actor actor;
    DummyActionFunc actionFunc;
} Dummy;

#endif
