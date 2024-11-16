#ifndef BOOKSHELFDOOR_H
#define BOOKSHELFDOOR_H

#include "ultra64.h"
#include "global.h"

struct BookshelfDoor;

typedef void (*BookshelfDoorActionFunc)(struct BookshelfDoor*, PlayState*);

typedef struct BookshelfDoor {
    DynaPolyActor dyna;
    BookshelfDoorActionFunc actionFunc;
    s16 timer;
} BookshelfDoor;

#endif
