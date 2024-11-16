#include "metal_cube.h"
#include "assets_custom/objects/object_reactor/object_reactor.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5 | ACTOR_FLAG_26)

void MetalCube_Init(Actor* thisx, PlayState* play);
void MetalCube_Destroy(Actor* thisx, PlayState* play);
void MetalCube_Update(Actor* thisx, PlayState* play);
void MetalCube_Draw(Actor* thisx, PlayState* play);

void MetalCube_SetupIdle(MetalCube* this);
void MetalCube_Idle(MetalCube* this, PlayState* play);
void MetalCube_SetupHeld(MetalCube* this);
void MetalCube_Held(MetalCube* this, PlayState* play);
void MetalCube_SetupThrown(MetalCube* this);
void MetalCube_Thrown(MetalCube* this, PlayState* play);

const ActorInit Metal_Cube_InitVars = {
    ACTOR_METAL_CUBE,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_REACTOR,
    sizeof(MetalCube),
    MetalCube_Init,
    MetalCube_Destroy,
    MetalCube_Update,
    MetalCube_Draw,
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
    { 32, 40, 0, { 0, 0, 0 } },
};

static CollisionCheckInfoInit sCCInfoInit = { 0, 32, 60, MASS_HEAVY };

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(uncullZoneForward, 1000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 60, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 1000, ICHAIN_STOP),
};

#define METALCUBE_GET_TYPE(params) (params & 0x3F)

void MetalCube_ApplyGravity(MetalCube* this) {
    this->actor.velocity.y += this->actor.gravity;
    if (this->actor.velocity.y < this->actor.minVelocityY) {
        this->actor.velocity.y = this->actor.minVelocityY;
    }
}

void MetalCube_InitCollider(Actor* thisx, PlayState* play) {
    MetalCube* this = (MetalCube*)thisx;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);
}

void MetalCube_Init(Actor* thisx, PlayState* play) {
    MetalCube* this = (MetalCube*)thisx;

    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->actor.gravity = -1.2f;
    this->actor.minVelocityY = -13.0f;
    MetalCube_InitCollider(&this->actor, play);
    CollisionCheck_SetInfo(&this->actor.colChkInfo, NULL, &sCCInfoInit);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 25.0f);
    MetalCube_SetupIdle(this);
}

void MetalCube_Destroy(Actor* thisx, PlayState* play) {
    MetalCube* this = (MetalCube*)thisx;
}

void MetalCube_SetupIdle(MetalCube* this) {
    this->actionFunc = MetalCube_Idle;
    this->actor.colChkInfo.mass = MASS_HEAVY;
}

void MetalCube_Idle(MetalCube* this, PlayState* play) {
    if (Actor_HasParent(&this->actor, play)) {
        MetalCube_SetupHeld(this);
    } else {
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

void MetalCube_SetupHeld(MetalCube* this) {
    this->actionFunc = MetalCube_Held;
    this->actor.room = -1;
    Actor_PlaySfx(&this->actor, NA_SE_PL_PULL_UP_BIGROCK);
}

void MetalCube_Held(MetalCube* this, PlayState* play) {
    if (Actor_HasNoParent(&this->actor, play)) {
        this->actor.room = play->roomCtx.curRoom.num;
        if (fabsf(this->actor.speed) < 0.1f) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_METALDOOR_STOP);
            MetalCube_SetupIdle(this);
            this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
        } else {
            MetalCube_SetupThrown(this);
            MetalCube_ApplyGravity(this);
            Actor_UpdatePos(&this->actor);
        }
    }
    Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
}

void MetalCube_SetupThrown(MetalCube* this) {
    this->actor.velocity.x = Math_SinS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.colChkInfo.mass = 255;
    this->actionFunc = MetalCube_Thrown;
}

void MetalCube_Thrown(MetalCube* this, PlayState* play) {
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Actor_PlaySfx(&this->actor, NA_SE_EV_METALDOOR_STOP);
        this->actor.speed = 0.0f;
        this->actor.colChkInfo.mass = MASS_HEAVY;
        MetalCube_SetupIdle(this);
        this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
    } else {
        MetalCube_ApplyGravity(this);
        Actor_UpdatePos(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

void MetalCube_Update(Actor* thisx, PlayState* play) {
    MetalCube* this = (MetalCube*)thisx;

    this->actionFunc(this, play);
}

void MetalCube_Draw(Actor* thisx, PlayState* play) {
    MetalCube* this = (MetalCube*)thisx;

    Gfx_DrawDListOpa(play, gMetalCubeDL);
}
