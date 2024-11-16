#ifndef REACTOR_H
#define REACTOR_H

#include "ultra64.h"
#include "global.h"

struct Reactor;

typedef void (*ReactorActionFunc)(struct Reactor*, PlayState*);

typedef enum {
    REACTOR_ERROR_RODS,
    REACTOR_ERROR_CIRCUIT,
    REACTOR_ERROR_FANS,
    REACTOR_ERROR_LEAK,
    REACTOR_ERROR_FUEL,
    REACTOR_ERROR_MAX
} ReactorErrors;

typedef struct {
    u8 switchFlag;
    u16 textId;
} ReactorErrorInfo;

typedef struct Reactor {
    Actor actor;
    ReactorActionFunc actionFunc;
    s16 timer;
    u8 errorList;
    u8 curError;
    s16 subCamId;
} Reactor;

#endif
