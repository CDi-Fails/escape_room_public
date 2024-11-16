#include "bottle_water.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

#define FLAGS 0

void BottleWater_Init(Actor* thisx, PlayState* play);
void BottleWater_Destroy(Actor* thisx, PlayState* play);
void BottleWater_Update(Actor* thisx, PlayState* play);
void BottleWater_Draw(Actor* thisx, PlayState* play);

void BottleWater_DropWater(BottleWater* this, PlayState* play);

const ActorInit Bottle_Water_InitVars = {
    ACTOR_BOTTLE_WATER,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(BottleWater),
    BottleWater_Init,
    BottleWater_Destroy,
    BottleWater_Update,
    BottleWater_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_OTHER,
        AC_NONE,
        OC1_ON | OC1_TYPE_2,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x00 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 12, 60, 0, { 0, 0, 0 } },
};

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(uncullZoneForward, 1000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 400, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 1000, ICHAIN_STOP),
};

void BottleWater_InitDroppedWater(BottleWater* this, PlayState* play) {
    Actor_ProcessInitChain(&this->actor, sInitChain);
    Actor_SetScale(&this->actor, 0.00002f);
    this->actor.gravity = -0.5f;
    this->actor.minVelocityY = -4.0f;
    this->actor.shape.yOffset = 0.0f;
    this->actor.shape.rot.x = this->actor.shape.rot.y = this->actor.shape.rot.z = this->actor.world.rot.x =
        this->actor.world.rot.y = this->actor.world.rot.z = 0;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);

    this->collider.dim.radius = this->actor.scale.x * 4000.4f;
    this->collider.dim.height = this->actor.scale.y * 8000.2f;
    this->actor.colChkInfo.mass = 253;
    this->actionFunc = BottleWater_DropWater;
    this->timer = 100;
    this->alpha = 255;
}

void BottleWater_Init(Actor* thisx, PlayState* play) {
    BottleWater* this = (BottleWater*)thisx;

    switch (this->actor.params) {
        case 0:
            BottleWater_InitDroppedWater(this, play);
            break;
    }
}

void BottleWater_Destroy(Actor* thisx, PlayState* play) {
    BottleWater* this = (BottleWater*)thisx;
}

void BottleWater_DropWater(BottleWater* this, PlayState* play) {
    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Math_StepToF(&this->actor.scale.x, 0.0017f, 0.00008f);
        this->actor.scale.z = this->actor.scale.x;
        Math_StepToF(&this->actor.scale.y, 0.0017f, 0.00008f);
    } else {
        Math_StepToF(&this->actor.scale.x, 0.0035f, 0.00016f);
        this->actor.scale.z = this->actor.scale.x;
        Math_StepToF(&this->actor.scale.y, 0.0006f, 0.00016f);
    }

    Math_StepToS(&this->alpha, 0, 8);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Actor_PlaySfx(&this->actor, NA_SE_PL_WALK_WATER1);
        EffectSsGSplash_Spawn(play, &this->actor.world.pos, NULL, NULL, 0, 400);
    }

    Actor_MoveXZGravity(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 10.0f, this->actor.scale.x * 3500.0f, 0.0f,
                            UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_2);

    Collider_UpdateCylinder(&this->actor, &this->collider);
    this->collider.dim.radius = this->actor.scale.x * 4000.0f;
    this->collider.dim.height = this->actor.scale.y * 8000.0f;
    CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);

    if (this->timer <= 0) {
        Actor_Kill(&this->actor);
    }
}

void BottleWater_Update(Actor* thisx, PlayState* play) {
    BottleWater* this = (BottleWater*)thisx;

    DECR(this->timer);

    this->actionFunc(this, play);
}

void BottleWater_Draw(Actor* thisx, PlayState* play) {
    BottleWater* this = (BottleWater*)thisx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);

    gSPSegment(POLY_XLU_DISP++, 0x08, gEmptyDL);

    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 150, 210, 255, this->alpha * 0.75);

    gDPSetEnvColor(POLY_XLU_DISP++, 150, 210, 255, 0);

    Matrix_RotateY(BINANG_TO_RAD((s16)(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) - this->actor.shape.rot.y + 0x8000)),
                   MTXMODE_APPLY);

    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, gEffFire1DL);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}
