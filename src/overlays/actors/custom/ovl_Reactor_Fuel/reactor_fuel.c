#include "reactor_fuel.h"
#include "overlays/actors/custom/ovl_Reactor_Fuel_Slot/reactor_fuel_slot.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"
#include "assets_custom/objects/object_reactor/object_reactor.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void ReactorFuel_Init(Actor* thisx, PlayState* play);
void ReactorFuel_Destroy(Actor* thisx, PlayState* play);
void ReactorFuel_Update(Actor* thisx, PlayState* play);
void ReactorFuel_Draw(Actor* thisx, PlayState* play);

void ReactorFuel_SetupIdle(ReactorFuel* this);
void ReactorFuel_Idle(ReactorFuel* this, PlayState* play);
void ReactorFuel_SetupHeld(ReactorFuel* this);
void ReactorFuel_Held(ReactorFuel* this, PlayState* play);
void ReactorFuel_SetupThrown(ReactorFuel* this);
void ReactorFuel_Thrown(ReactorFuel* this, PlayState* play);

const ActorInit Reactor_Fuel_InitVars = {
    ACTOR_REACTOR_FUEL,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_REACTOR,
    sizeof(ReactorFuel),
    ReactorFuel_Init,
    ReactorFuel_Destroy,
    ReactorFuel_Update,
    ReactorFuel_Draw,
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
    { 24, 40, 0, { 0, 0, 0 } },
};

static CollisionCheckInfoInit sCCInfoInit = { 0, 24, 40, MASS_HEAVY };

static InitChainEntry sInitChain[] = {
    ICHAIN_VEC3F_DIV1000(scale, 100, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneForward, 1000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 60, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 1000, ICHAIN_STOP),
};

#define REACTOR_FUEL_GET_TYPE(params) (params & 0x3F)

void ReactorFuel_ApplyGravity(ReactorFuel* this) {
    this->actor.velocity.y += this->actor.gravity;
    if (this->actor.velocity.y < this->actor.minVelocityY) {
        this->actor.velocity.y = this->actor.minVelocityY;
    }
}

void ReactorFuel_InitCollider(Actor* thisx, PlayState* play) {
    ReactorFuel* this = (ReactorFuel*)thisx;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);
}

void ReactorFuel_Init(Actor* thisx, PlayState* play) {
    ReactorFuel* this = (ReactorFuel*)thisx;

    this->fuelType = REACTOR_FUEL_GET_TYPE(this->actor.params);

    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->actor.gravity = -1.2f;
    this->actor.minVelocityY = -13.0f;
    ReactorFuel_InitCollider(&this->actor, play);
    CollisionCheck_SetInfo(&this->actor.colChkInfo, NULL, &sCCInfoInit);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 5.0f);
    ReactorFuel_SetupIdle(this);
}

void ReactorFuel_Destroy(Actor* thisx, PlayState* play) {
    ReactorFuel* this = (ReactorFuel*)thisx;
}

void ReactorFuel_SetupIdle(ReactorFuel* this) {
    this->actionFunc = ReactorFuel_Idle;
    this->actor.colChkInfo.mass = MASS_HEAVY;
}

void ReactorFuel_Idle(ReactorFuel* this, PlayState* play) {
    ReactorFuelSlot* fuelSlotActor = (ReactorFuelSlot*)Actor_FindNearby(play, &this->actor, ACTOR_REACTOR_FUEL_SLOT, ACTORCAT_PROP, 25.0f);

    if (Actor_HasParent(&this->actor, play)) {
        ReactorFuel_SetupHeld(this);
        if (fuelSlotActor != NULL) {
            fuelSlotActor->loadedFuelType = -1;
        }
    } else {
        // We're only finding slots that are really close to the fuel anyway, so just gonna skip a distance check
        if (fuelSlotActor != NULL) {
            fuelSlotActor->loadedFuelType = this->fuelType;
            this->actor.world.pos = fuelSlotActor->actor.world.pos;
            this->actor.shape.rot.y = fuelSlotActor->actor.shape.rot.y + 0x1555;
        }
        
        Actor_MoveXZGravity(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
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

void ReactorFuel_SetupHeld(ReactorFuel* this) {
    this->actionFunc = ReactorFuel_Held;
    this->actor.room = -1;
    if (this->fuelType == REACTOR_FUEL_TYPE_HEAVY) {
        Actor_PlaySfx(&this->actor, NA_SE_PL_PULL_UP_BIGROCK);
    } else {
        Actor_PlaySfx(&this->actor, NA_SE_PL_PULL_UP_ROCK);
    }
}

void ReactorFuel_Held(ReactorFuel* this, PlayState* play) {
    if (Actor_HasNoParent(&this->actor, play)) {
        this->actor.room = play->roomCtx.curRoom.num;
        if (fabsf(this->actor.speed) < 0.1f) {
            Actor_PlaySfx(&this->actor, NA_SE_PL_PUT_DOWN_POT);
            ReactorFuel_SetupIdle(this);
            this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
        } else {
            ReactorFuel_SetupThrown(this);
            ReactorFuel_ApplyGravity(this);
            Actor_UpdatePos(&this->actor);
        }
    }
    Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
}

void ReactorFuel_SetupThrown(ReactorFuel* this) {
    this->actor.velocity.x = Math_SinS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.colChkInfo.mass = 240;
    this->actionFunc = ReactorFuel_Thrown;
}

void ReactorFuel_Thrown(ReactorFuel* this, PlayState* play) {
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Actor_PlaySfx(&this->actor, NA_SE_PL_PUT_DOWN_POT);
        this->actor.speed = 0.0f;
        this->actor.colChkInfo.mass = MASS_HEAVY;
        ReactorFuel_SetupIdle(this);
        this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
    } else {
        ReactorFuel_ApplyGravity(this);
        Actor_UpdatePos(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

void ReactorFuel_Update(Actor* thisx, PlayState* play) {
    ReactorFuel* this = (ReactorFuel*)thisx;

    this->actionFunc(this, play);
}

void ReactorFuel_Draw(Actor* thisx, PlayState* play) {
    ReactorFuel* this = (ReactorFuel*)thisx;

    Gfx_DrawDListOpa(play, gReactorFuelDL);
}
