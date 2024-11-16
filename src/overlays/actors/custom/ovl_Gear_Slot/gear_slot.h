#ifndef GearSlot_H
#define GearSlot_H

#include "ultra64.h"
#include "global.h"

struct GearSlot;

typedef void (*GearSlotActionFunc)(struct GearSlot*, PlayState*);

typedef enum {
    GEAR_SLOT_TYPE_NEEDS_FILL,
    GEAR_SLOT_TYPE_PREFILLED,
} GearSlotType;

typedef struct GearSlot {
    DynaPolyActor dyna;
    GearSlotActionFunc actionFunc;
    ColliderCylinder collider;
    s16 timer;
    s16 subCamId;
    Vec3f subCamAt;
    Vec3f subCamEye;
    Vec3f initSubCamAt;
    Vec3f initSubCamEye;
    f32 camAccel;
    u8 type;
    u8 gearLoaded;
    u8 isLeader;
} GearSlot;

#endif
