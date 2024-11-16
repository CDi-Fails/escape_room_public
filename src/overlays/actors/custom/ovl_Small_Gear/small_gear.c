#include "small_gear.h"
#include "assets_custom/objects/object_gear/object_gear.h"
#include "src/overlays/actors/custom/ovl_Gear_Slot/gear_slot.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void SmallGear_Init(Actor* thisx, PlayState* play);
void SmallGear_Destroy(Actor* thisx, PlayState* play);
void SmallGear_Update(Actor* thisx, PlayState* play);
void SmallGear_Draw(Actor* thisx, PlayState* play);

void SmallGear_SetupIdle(SmallGear* this);
void SmallGear_Idle(SmallGear* this, PlayState* play);
void SmallGear_SetupHeld(SmallGear* this);
void SmallGear_Held(SmallGear* this, PlayState* play);
void SmallGear_SetupThrown(SmallGear* this);
void SmallGear_Thrown(SmallGear* this, PlayState* play);

const ActorInit Small_Gear_InitVars = {
    ACTOR_SMALL_GEAR,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GEAR,
    sizeof(SmallGear),
    SmallGear_Init,
    SmallGear_Destroy,
    SmallGear_Update,
    SmallGear_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x00000002, 0x00, 0x01 },
        { 0x4FC00748, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_ON,
    },
    { 32, 15, 0, { 0, 0, 0 } },
};

static CollisionCheckInfoInit sCCInfoInit = { 0, 32, 15, MASS_HEAVY };

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(uncullZoneForward, 1000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 60, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 1000, ICHAIN_STOP),
};

void SmallGear_ApplyGravity(SmallGear* this) {
    this->actor.velocity.y += this->actor.gravity;
    if (this->actor.velocity.y < this->actor.minVelocityY) {
        this->actor.velocity.y = this->actor.minVelocityY;
    }
}

void SmallGear_InitCollider(Actor* thisx, PlayState* play) {
    SmallGear* this = (SmallGear*)thisx;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);
}

#define SMALL_GEAR_GET_SWITCH(params) (params & 0x3F)

void SmallGear_Init(Actor* thisx, PlayState* play) {
    SmallGear* this = (SmallGear*)thisx;

    if (Flags_GetSwitch(play, SMALL_GEAR_GET_SWITCH(this->actor.params))) {
        Actor_Kill(&this->actor);
    }

    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->actor.gravity = -1.2f;
    this->actor.minVelocityY = -13.0f;
    SmallGear_InitCollider(&this->actor, play);
    CollisionCheck_SetInfo(&this->actor.colChkInfo, NULL, &sCCInfoInit);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 65.0f);
    SmallGear_SetupIdle(this);
}

void SmallGear_Destroy(Actor* thisx, PlayState* play) {
    SmallGear* this = (SmallGear*)thisx;
}

void SmallGear_SetupIdle(SmallGear* this) {
    this->actionFunc = SmallGear_Idle;
    this->actor.colChkInfo.mass = MASS_HEAVY;
}

void SmallGear_Idle(SmallGear* this, PlayState* play) {
    GearSlot* gearSlotActor = (GearSlot*)Actor_FindNearby(play, &this->actor, ACTOR_GEAR_SLOT, ACTORCAT_PROP, 25.0f);

    if (Actor_HasParent(&this->actor, play)) {
        SmallGear_SetupHeld(this);
    } else {
        // We're only finding slots that are really close to the fuel anyway, so just gonna skip a distance check
        if (gearSlotActor != NULL && gearSlotActor->gearLoaded == false) {
            gearSlotActor->gearLoaded = true;
            Sfx_PlaySfxAtPos(&gearSlotActor->dyna.actor.projectedPos, NA_SE_EV_BLOCK_BOUND);
            Actor_Kill(&this->actor);
        }

        Actor_MoveXZGravity(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 0.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_2);
        if (!(this->collider.base.ocFlags1 & OC1_TYPE_PLAYER) && (this->actor.xzDistToPlayer > 28.0f)) {
            this->collider.base.ocFlags1 |= OC1_TYPE_PLAYER;
        }
        if (this->actor.xzDistToPlayer < 600.0f) {
            Collider_UpdateCylinder(&this->actor, &this->collider);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
            if (this->actor.xzDistToPlayer < 180.0f) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
            }
        }
        if (this->actor.xzDistToPlayer < 100.0f) {
            Actor_OfferCarry(&this->actor, play);
        }
    }
}

void SmallGear_SetupHeld(SmallGear* this) {
    this->actionFunc = SmallGear_Held;
    this->actor.room = -1;
    Actor_PlaySfx(&this->actor, NA_SE_PL_PULL_UP_POT);
}

void SmallGear_Held(SmallGear* this, PlayState* play) {
    if (Actor_HasNoParent(&this->actor, play)) {
        this->actor.room = play->roomCtx.curRoom.num;
        if (fabsf(this->actor.speed) < 0.1f) {
            Actor_PlaySfx(&this->actor, NA_SE_PL_PUT_DOWN_POT);
            SmallGear_SetupIdle(this);
            this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
        } else {
            SmallGear_SetupThrown(this);
            SmallGear_ApplyGravity(this);
            Actor_UpdatePos(&this->actor);
        }
    }
    Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 0.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_2);
}

void SmallGear_SetupThrown(SmallGear* this) {
    this->actor.velocity.x = Math_SinS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.colChkInfo.mass = 240;
    this->actionFunc = SmallGear_Thrown;
}

void SmallGear_Thrown(SmallGear* this, PlayState* play) {
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Actor_PlaySfx(&this->actor, NA_SE_PL_PUT_DOWN_POT);
        this->actor.speed = 0.0f;
        this->actor.colChkInfo.mass = MASS_HEAVY;
        SmallGear_SetupIdle(this);
        this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
    } else {
        SmallGear_ApplyGravity(this);
        Actor_UpdatePos(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 0.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_2);
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

void SmallGear_Update(Actor* thisx, PlayState* play) {
    SmallGear* this = (SmallGear*)thisx;

    this->actionFunc(this, play);
}

void SmallGear_Draw(Actor* thisx, PlayState* play) {
    SmallGear* this = (SmallGear*)thisx;

    Gfx_DrawDListOpa(play, gSmallGearDL);
}
