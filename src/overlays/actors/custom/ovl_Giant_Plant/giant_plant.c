#include "giant_plant.h"
#include "overlays/actors/custom/ovl_Seed/seed.h"
#include "overlays/actors/custom/ovl_Darkness/darkness.h"
#include "assets_custom/objects/object_giant_plant/object_giant_plant.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void GiantPlant_Init(Actor* thisx, PlayState* play);
void GiantPlant_Destroy(Actor* thisx, PlayState* play);
void GiantPlant_Update(Actor* thisx, PlayState* play);
void GiantPlant_Draw(Actor* thisx, PlayState* play);

void GiantPlant_Plant_StartGrow(GiantPlant* this, PlayState* play);
void GiantPlant_Plant_Grow(GiantPlant* this, PlayState* play);
void GiantPlant_Plant_IdleGrown(GiantPlant* this, PlayState* play);
void GiantPlant_Pot_Idle(GiantPlant* this, PlayState* play);

const ActorInit Giant_Plant_InitVars = {
    ACTOR_GIANT_PLANT,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_GIANT_PLANT,
    sizeof(GiantPlant),
    GiantPlant_Init,
    GiantPlant_Destroy,
    GiantPlant_Update,
    GiantPlant_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000000, 0x00, 0x00 },
        { DMG_EXPLOSIVE, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_NONE,
    },
    { 120, 450, 0, { 0 } },
};

static CollisionCheckInfoInit sColChkInfoInit = { 0, 12, 60, MASS_IMMOVABLE };

// PARAMS:
// 0x003F - Switch flag
// 0x0F00 - ID
// 0xF000 - Type (don't set manually)

#define GIANT_PLANT_GET_ID(params) ((params >> 8) & 0xF)
#define GIANT_PLANT_SET_ID(id) ((id & 0xF) << 8)
#define GIANT_PLANT_GET_TYPE(params) ((params >> 12) & 0xF)
#define GIANT_PLANT_SET_TYPE(type) ((type & 0xF) << 12)
#define GIANT_PLANT_GET_SWITCH(params) (params & 0x3F)
#define GIANT_PLANT_GET_PLANT_TYPE(id) (((gSaveContext.giantPlantTypes >> id) & 1) + 1)
#define GIANT_PLANT_SET_PLANT_TYPE(id, type) (gSaveContext.giantPlantTypes = (gSaveContext.giantPlantTypes & ~(1 << id)) | ((type - 1) << id))

void GiantPlant_Pot_Init(GiantPlant* this, PlayState* play) {
    CollisionHeader* colHeader = NULL;

    CollisionHeader_GetVirtual(&gPlantPotCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    this->actionFunc = GiantPlant_Pot_Idle;
}

void GiantPlant_Plant_Init(GiantPlant* this, PlayState* play) {
    CollisionHeader* colHeader = NULL;

    switch (this->type) {
        case GIANT_PLANT_TYPE_PLATFORM:
            CollisionHeader_GetVirtual(&gPlatformPlantCol_collisionHeader, &colHeader);
            break;
            
        case GIANT_PLANT_TYPE_HOOKSHOT:
            CollisionHeader_GetVirtual(&gHookshotPlantCol_collisionHeader, &colHeader);
            break;
    }

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->dyna.actor, &sCylinderInit);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    // If plant is spawned without the switch flag set yet, then it needs to grow first before it can idle
    if (Flags_GetSwitch(play, GIANT_PLANT_GET_SWITCH(this->dyna.actor.params))) {
        this->scaleXZ = this->scaleY = this->budScale = 1.0f;
        this->actionFunc = GiantPlant_Plant_IdleGrown;
    } else {
        this->timer = 30;
        OnePointCutscene_Attention(play, &this->dyna.actor);
        Flags_SetSwitch(play, GIANT_PLANT_GET_SWITCH(this->dyna.actor.params));
        this->actionFunc = GiantPlant_Plant_StartGrow;
    }
}

void GiantPlant_Init(Actor* thisx, PlayState* play) {
    GiantPlant* this = (GiantPlant*)thisx;

    Actor_SetScale(&this->dyna.actor, 0.02f);
    this->dyna.actor.uncullZoneDownward = 1000.0f;
    this->dyna.actor.uncullZoneForward = 4000.0f;

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);

    this->id = GIANT_PLANT_GET_ID(this->dyna.actor.params);
    this->type = GIANT_PLANT_GET_TYPE(this->dyna.actor.params);

    switch (this->type) {
        case GIANT_PLANT_TYPE_POT:
            GiantPlant_Pot_Init(this, play);
            // If switch flag is set on room load (pot init), spawn grown plant
            if (Flags_GetSwitch(play, GIANT_PLANT_GET_SWITCH(this->dyna.actor.params))) {
                Actor_Spawn(&play->actorCtx, play, ACTOR_GIANT_PLANT, this->dyna.actor.world.pos.x,
                            this->dyna.actor.world.pos.y, this->dyna.actor.world.pos.z, this->dyna.actor.shape.rot.x,
                            this->dyna.actor.shape.rot.y, this->dyna.actor.shape.rot.z,
                            GIANT_PLANT_SET_ID(this->id) | GIANT_PLANT_SET_TYPE(GIANT_PLANT_GET_PLANT_TYPE(this->id)) |
                                GIANT_PLANT_GET_SWITCH(this->dyna.actor.params));
            }
            break;

        case GIANT_PLANT_TYPE_PLATFORM:
        case GIANT_PLANT_TYPE_HOOKSHOT:
            GiantPlant_Plant_Init(this, play);
            break;
    }
}

void GiantPlant_Destroy(Actor* thisx, PlayState* play) {
    GiantPlant* this = (GiantPlant*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

static Vec3f sUnitDirections[] = {
    { 0.0f, 0.7071f, 0.7071f },
    { 0.7071f, 0.7071f, 0.0f },
    { 0.0f, 0.7071f, -0.7071f },
    { -0.7071f, 0.7071f, 0.0f },
};

static s16 sFragmentScales[] = { 108, 102, 96, 84, 66, 55, 42, 38 };

void GiantPlant_Plant_Explode(GiantPlant* this, PlayState* play) {
    Vec3f burstOrigin;
    Vec3f burstVelocity;
    Vec3f burstAcceleration;
    s16 scale;
    s32 i;

    for (i = 0; i < 20; i++) {
        burstOrigin = this->dyna.actor.world.pos;
        burstOrigin.y += (i >= 8) ? (Rand_ZeroOne() * 300.0f) + 50.0f : 50.0f;

        burstAcceleration.x = burstAcceleration.y = burstAcceleration.z = 0.0f;

        scale = Rand_ZeroOne() * 4.0f + 2.0f;
        Gfx* dList = (i % 4 == 0) ? gPlantFragmentLeafDL : gPlantFragmentStemDL;
        
        burstVelocity.x = (Rand_ZeroOne() - 0.5f) * 40.0f;
        burstVelocity.y = Rand_ZeroOne() * 50.0f + 2.0f;
        burstVelocity.z = (Rand_ZeroOne() - 0.5f) * 40.0f;

        EffectSsKakera_Spawn(play, &burstOrigin, &burstVelocity, &burstAcceleration,
                             (dList == gPlantFragmentLeafDL) ? -100 : -500, // gravity
                             (dList == gPlantFragmentLeafDL) ? 32 : 48,     // arg5 (rotation speed)
                             0,                                             // arg6
                             4,                                             // arg7
                             (dList == gPlantFragmentLeafDL) ? 16 : 0,      // arg8
                             scale,                                         // scale
                             (dList == gPlantFragmentLeafDL) ? 2 : 0,       // arg10
                             0,                                             // arg11
                             60,                                            // life
                             KAKERA_COLOR_NONE,                             // color index
                             OBJECT_GIANT_PLANT,                            // objId
                             dList);                                        // display list
    }
}

void GiantPlant_Plant_IdleGrown(GiantPlant* this, PlayState* play) {
    // If hit with a bomb
    if (this->collider.base.acFlags & AC_HIT) {
        Flags_UnsetSwitch(play, GIANT_PLANT_GET_SWITCH(this->dyna.actor.params));
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->dyna.actor.world.pos, 40, NA_SE_EV_BOX_BREAK);
        GiantPlant_Plant_Explode(this, play);
        Actor_Kill(&this->dyna.actor);
    }

    Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
}

void GiantPlant_Plant_StartGrow(GiantPlant* this, PlayState* play) {
    Vec3f pos;

    Math_Vec3f_Copy(&pos, &this->dyna.actor.world.pos);
    pos.y += 20.0f;

    func_80033480(play, &pos, 70.0f, 6, 50, 50, 1);

    if (DECR(this->timer) == 0) {
        this->timer = 5;
        this->growthPhase = 0;
        Actor_SetFocus(&this->dyna.actor, 250.0f);
        OnePointCutscene_Attention(play, &this->dyna.actor);
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_COME_UP_DEKU_JR);
        this->actionFunc = GiantPlant_Plant_Grow;
    } else {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_EARTHQUAKE - SFX_FLAG);
    }
}

void GiantPlant_Plant_Grow(GiantPlant* this, PlayState* play) {
    switch (this->growthPhase) {
        case 0:
            Math_SmoothStepToF(&this->scaleXZ, 0.45f, 1.0f, 0.09f, 0.0f);
            Math_SmoothStepToF(&this->scaleY, 1.0f, 1.0f, 0.2f, 0.0f);
            break;

        case 1:
            Math_SmoothStepToF(&this->scaleXZ, 0.85f, 1.0f, 0.13f, 0.0f);
            Math_SmoothStepToF(&this->scaleY, 1.2f, 1.0f, 0.067f, 0.0f);
            break;

        case 2:
            Math_SmoothStepToF(&this->scaleXZ, 1.0f, 1.0f, 0.0375f, 0.0f);
            Math_SmoothStepToF(&this->scaleY, 1.0f, 1.0f, 0.05f, 0.0f);
            break;
        
        case 3:
            Math_SmoothStepToF(&this->budScale, 1.0f, 1.0f, 0.125f, 0.0f);
            break;

    }

    if (DECR(this->timer) == 0) {
        switch (this->growthPhase) {
            case 0:
                this->timer = 3;
                this->growthPhase++;
                break;
            
            case 1:
                this->timer = 4;
                this->growthPhase++;
                break;
            
            case 2:
                switch (this->type) {
                    case GIANT_PLANT_TYPE_HOOKSHOT:
                        this->timer = 8;
                        this->scaleXZ = this->scaleY = 1.0f;
                        this->growthPhase++;
                        Actor_PlaySfx(&this->dyna.actor, NA_SE_EN_NUTS_UP);
                        break;

                    case GIANT_PLANT_TYPE_PLATFORM:
                        this->scaleXZ = this->scaleY = this->budScale = 1.0f;
                        this->actionFunc = GiantPlant_Plant_IdleGrown;
                        break;
                    
                    default:
                        break;
                }
                break;
            
            case 3:
                this->budScale = 1.0f;
                this->actionFunc = GiantPlant_Plant_IdleGrown;

        }
    }
}

void GiantPlant_Pot_Idle(GiantPlant* this, PlayState* play) {
    // If plant is not already grown, check for seed
    if (!Flags_GetSwitch(play, GIANT_PLANT_GET_SWITCH(this->dyna.actor.params))) {
        Seed* seedActor = (Seed*)Actor_FindNearby(play, &this->dyna.actor, ACTOR_SEED, ACTORCAT_PROP, 85.0f);
        Darkness* darkness = (Darkness*)Actor_Find(&play->actorCtx, ACTOR_DARKNESS, ACTORCAT_PROP);

        if (darkness != NULL) {
            return;
        }

        if (seedActor != NULL && (seedActor->actor.world.pos.y - this->dyna.actor.world.pos.y) >= 50.0f &&
            (seedActor->actor.world.pos.y - this->dyna.actor.world.pos.y) <= 52.0f) {
            Actor_Kill(&seedActor->actor);
            // Set plant type in save so spawned plant actor can determine what to grow into
            GIANT_PLANT_SET_PLANT_TYPE(this->id, seedActor->plantType);
            // Spawn plant actor
            Actor_Spawn(&play->actorCtx, play, ACTOR_GIANT_PLANT, this->dyna.actor.world.pos.x,
                        this->dyna.actor.world.pos.y, this->dyna.actor.world.pos.z, this->dyna.actor.shape.rot.x,
                        this->dyna.actor.shape.rot.y, this->dyna.actor.shape.rot.z,
                        GIANT_PLANT_SET_ID(this->id) | GIANT_PLANT_SET_TYPE(seedActor->plantType) |
                            GIANT_PLANT_GET_SWITCH(this->dyna.actor.params));
        }
    }
}

void GiantPlant_Update(Actor* thisx, PlayState* play) {
    GiantPlant* this = (GiantPlant*)thisx;

    this->actionFunc(this, play);
}

void GiantPlant_Draw(Actor* thisx, PlayState* play) {
    GiantPlant* this = (GiantPlant*)thisx;
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    OPEN_DISPS(gfxCtx, __FILE__, __LINE__);
    
    switch (this->type) {
        case GIANT_PLANT_TYPE_POT:
            Gfx_DrawDListOpa(play, gPlantPotDL);
            break;

        case GIANT_PLANT_TYPE_PLATFORM:
            Matrix_Translate(0.0f, 5.0f * 10.0f * (1 / this->dyna.actor.scale.y), 0.0f, MTXMODE_APPLY);
            Matrix_Scale(this->scaleXZ, this->scaleY, this->scaleXZ, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            Gfx_DrawDListOpa(play, gPlatformPlantDL);
            break;
        
        case GIANT_PLANT_TYPE_HOOKSHOT:
            // Draw base
            Matrix_Push();
            Matrix_Translate(0.0f, 5.0f * 10.0f * (1 / this->dyna.actor.scale.y), 0.0f, MTXMODE_APPLY);
            Matrix_Scale(this->scaleXZ, this->scaleY, this->scaleXZ, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            Gfx_DrawDListOpa(play, gHookshotPlantBaseDL);
            Matrix_Pop();

            // Draw bud
            Matrix_Translate(-11.7675f * 10.0f * (1 / this->dyna.actor.scale.x), 42.9460f * 10.0f * (1 / this->dyna.actor.scale.y), 8.6669f * 10.0f * (1 / this->dyna.actor.scale.z), MTXMODE_APPLY);
            Matrix_Scale(this->scaleXZ * this->budScale, this->scaleY * this->budScale, this->scaleXZ * this->budScale, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            Gfx_DrawDListOpa(play, gHookshotPlantBudDL);
            break;
    }

    CLOSE_DISPS(gfxCtx, __FILE__, __LINE__);
}
