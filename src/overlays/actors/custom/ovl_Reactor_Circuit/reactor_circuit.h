#ifndef REACTORCIRCUIT_H
#define REACTORCIRCUIT_H

#include "ultra64.h"
#include "global.h"

struct ReactorCircuit;

typedef void (*ReactorCircuitActionFunc)(struct ReactorCircuit*, PlayState*);

typedef enum {
    REACTOR_CIRCUIT_TYPE_CIRCUIT,
    REACTOR_CIRCUIT_TYPE_TERMINAL,
} ReactorCircuitType;

typedef enum {
    REACTOR_CIRCUIT_ID_UPPER,
    REACTOR_CIRCUIT_ID_CENTER,
    REACTOR_CIRCUIT_ID_LOWER,
} ReactorCircuitId;

typedef struct ReactorCircuit {
    Actor actor;
    ReactorCircuitActionFunc actionFunc;
    s16 prevRotY;
    u8 toggleSpin;
    u8 type;
    u8 id;
} ReactorCircuit;

#endif
