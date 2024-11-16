#include "seed.h"
#include "overlays/actors/custom/ovl_Giant_Plant/giant_plant.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"
#include "assets_custom/objects/object_seed/object_seed.h"
#include "assets/objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void Seed_Init(Actor* thisx, PlayState* play);
void Seed_Destroy(Actor* thisx, PlayState* play);
void Seed_Update(Actor* thisx, PlayState* play);
void Seed_Draw(Actor* thisx, PlayState* play);

void Seed_SetupIdle(Seed* this);
void Seed_Idle(Seed* this, PlayState* play);
void Seed_SetupHeld(Seed* this);
void Seed_Held(Seed* this, PlayState* play);
void Seed_SetupThrown(Seed* this);
void Seed_Thrown(Seed* this, PlayState* play);

const ActorInit Seed_InitVars = {
    ACTOR_SEED,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_SEED,
    sizeof(Seed),
    Seed_Init,
    Seed_Destroy,
    Seed_Update,
    Seed_Draw,
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
    ICHAIN_VEC3F_DIV1000(scale, 15, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneForward, 1000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 60, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 1000, ICHAIN_STOP),
};

#define SEED_GET_TYPE(params) (params & 0x3F)

void Seed_ApplyGravity(Seed* this) {
    this->actor.velocity.y += this->actor.gravity;
    if (this->actor.velocity.y < this->actor.minVelocityY) {
        this->actor.velocity.y = this->actor.minVelocityY;
    }
}

void Seed_InitCollider(Actor* thisx, PlayState* play) {
    Seed* this = (Seed*)thisx;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);
}

void Seed_Init(Actor* thisx, PlayState* play) {
    Seed* this = (Seed*)thisx;

    this->plantType = SEED_GET_TYPE(this->actor.params);

    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->actor.gravity = -1.2f;
    this->actor.minVelocityY = -13.0f;
    Seed_InitCollider(&this->actor, play);
    CollisionCheck_SetInfo(&this->actor.colChkInfo, NULL, &sCCInfoInit);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 25.0f);
    Seed_SetupIdle(this);
}

void Seed_Destroy(Actor* thisx, PlayState* play) {
    Seed* this = (Seed*)thisx;
}

void Seed_WaterBreak(Seed* this, PlayState* play) {
    s16 angle;
    s32 i;
    Vec3f* breakPos = &this->actor.world.pos;
    Vec3f pos;
    Vec3f velocity;

    pos = *breakPos;
    pos.y += this->actor.yDistToWater;
    EffectSsGSplash_Spawn(play, &pos, NULL, NULL, 0, 500);

    for (i = 0, angle = 0; i < 12; i++, angle += 0x4E20) {
        f32 sn = Math_SinS(angle);
        f32 cs = Math_CosS(angle);
        f32 temp_rand;
        s16 phi_s0;

        pos.x = sn * 16.0f;
        pos.y = (Rand_ZeroOne() * 5.0f) + 2.0f;
        pos.z = cs * 16.0f;
        velocity.x = pos.x * 0.18f;
        velocity.y = (Rand_ZeroOne() * 4.0f) + 2.0f;
        velocity.z = pos.z * 0.18f;
        pos.x += breakPos->x;
        pos.y += breakPos->y;
        pos.z += breakPos->z;
        temp_rand = Rand_ZeroOne();
        phi_s0 = (temp_rand < 0.2f) ? 0x40 : 0x20;
        EffectSsKakera_Spawn(play, &pos, &velocity, breakPos, -180, phi_s0, 30, 30, 0, (Rand_ZeroOne() * 5.0f) + 2.0f,
                             0, 32, 20, KAKERA_COLOR_NONE, OBJECT_GAMEPLAY_DANGEON_KEEP, gBrownFragmentDL);
    }
}

void Seed_SetupIdle(Seed* this) {
    this->actionFunc = Seed_Idle;
    this->actor.colChkInfo.mass = MASS_HEAVY;
}

void Seed_Idle(Seed* this, PlayState* play) {
    if (Actor_HasParent(&this->actor, play)) {
        Seed_SetupHeld(this);
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_WATER) && (this->actor.yDistToWater > 19.0f)) {
        Seed_WaterBreak(this, play);
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 20, NA_SE_EV_WOODBOX_BREAK);
        Actor_Kill(&this->actor);
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

void Seed_SetupHeld(Seed* this) {
    this->actionFunc = Seed_Held;
    this->actor.room = -1;
    Actor_PlaySfx(&this->actor, NA_SE_PL_PULL_UP_WOODBOX);
}

void Seed_Held(Seed* this, PlayState* play) {
    if (Actor_HasNoParent(&this->actor, play)) {
        this->actor.room = play->roomCtx.curRoom.num;
        if (fabsf(this->actor.speed) < 0.1f) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_PUT_DOWN_WOODBOX);
            Seed_SetupIdle(this);
            this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
        } else {
            Seed_SetupThrown(this);
            Seed_ApplyGravity(this);
            Actor_UpdatePos(&this->actor);
        }
    }
    Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
}

void Seed_SetupThrown(Seed* this) {
    this->actor.velocity.x = Math_SinS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.y) * this->actor.speed;
    this->actor.colChkInfo.mass = 240;
    this->actionFunc = Seed_Thrown;
}

void Seed_Thrown(Seed* this, PlayState* play) {
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Actor_PlaySfx(&this->actor, NA_SE_EV_PUT_DOWN_WOODBOX);
        this->actor.speed = 0.0f;
        this->actor.colChkInfo.mass = MASS_HEAVY;
        Seed_SetupIdle(this);
        this->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
    }
    if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER_TOUCH) {
        Seed_WaterBreak(this, play);
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 20, NA_SE_EV_WOODBOX_BREAK);
        Actor_Kill(&this->actor);
    } else {
        Seed_ApplyGravity(this);
        Actor_UpdatePos(&this->actor);
        Actor_UpdateBgCheckInfo(play, &this->actor, 19.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

void Seed_Update(Actor* thisx, PlayState* play) {
    Seed* this = (Seed*)thisx;

    this->actionFunc(this, play);
}

void Seed_Draw(Actor* thisx, PlayState* play) {
    Seed* this = (Seed*)thisx;

    switch (this->plantType) {
        case GIANT_PLANT_TYPE_PLATFORM:
            Gfx_DrawDListOpa(play, gPlatformSeedDL);
            break;

        case GIANT_PLANT_TYPE_HOOKSHOT:
            Gfx_DrawDListOpa(play, gHookshotSeedDL);
            break;  
    }
}
