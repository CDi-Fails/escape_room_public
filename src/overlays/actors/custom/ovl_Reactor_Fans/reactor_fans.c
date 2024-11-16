/*
 * Modified version of the fan actor from the HM Pack
 */

#include "reactor_fans.h"
#include "assets_custom/objects/object_fan/object_fan.h"
#include "assets_custom/objects/object_fan/gWindDL.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"
#include "assets/objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"

#define FLAGS ACTOR_FLAG_4 | ACTOR_FLAG_5
#define FAN_REACH 250.0f
#define FAN_RADIUS 114.0f
#define FAN_POWER 10.0f
#define FAN_THRESHOLD 0.5f

void ReactorFans_Init(Actor* thisx, PlayState* play);
void ReactorFans_Destroy(Actor* thisx, PlayState* play);
void ReactorFans_Update(Actor* thisx, PlayState* play);
void ReactorFans_Draw(Actor* thisx, PlayState* play);

ActorInit Reactor_Fans_InitVars = {
    ACTOR_REACTOR_FANS,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_FAN,
    sizeof(ReactorFans),
    (ActorFunc)ReactorFans_Init,
    (ActorFunc)ReactorFans_Destroy,
    (ActorFunc)ReactorFans_Update,
    (ActorFunc)ReactorFans_Draw,
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
    { 200, 400, -200, { 0 } },
};

static u8 sNumFansOn = 0;

#define FANS_GET_SWITCH_MAIN(params) (params & 0x3F)
#define FANS_GET_SWITCH_INDIVIDUAL(params) ((params >> 8) & 0x3F)

void ReactorFans_Init(Actor* thisx, PlayState* play) {
    ReactorFans* this = (ReactorFans*)thisx;
    this->scale = 1.7f;

    if (Flags_GetSwitch(play, FANS_GET_SWITCH_MAIN(this->dyna.actor.params))) {
        Flags_SetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params));
    }

    if (Flags_GetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params))) {
        sNumFansOn++;
        this->power = FAN_POWER;
    } else {
        CollisionHeader* colHeader = NULL;

        DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
        CollisionHeader_GetVirtual(&gFansRubbleDL_collisionHeader, &colHeader);
        this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

        Collider_InitCylinder(play, &this->collider);
        Collider_SetCylinder(play, &this->collider, &this->dyna.actor, &sCylinderInit);
        Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    }
}

void ReactorFans_Destroy(Actor* thisx, PlayState* play) {
    ReactorFans* this = (ReactorFans*)thisx;
    
    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void ReactorFans_SetRelativePos(Actor* target, ReactorFans* this) {
    MtxF rotationMatrix;
    Vec3f temp;

    Math_Vec3f_Diff(&target->world.pos, &this->dyna.actor.world.pos, &temp);
    SkinMatrix_SetRotateZYX(&rotationMatrix, -this->dyna.actor.world.rot.x, -this->dyna.actor.world.rot.y, -this->dyna.actor.world.rot.z);
    SkinMatrix_Vec3fMtxFMultXYZ(&rotationMatrix, &temp, &this->relativeActorPos);
}

u8 ReactorFans_ActorInRange(Actor* target, ReactorFans* this) {
    f32 dist;
    f32 reach = (FAN_REACH * (this->power / FAN_POWER)) * this->scale;

    ReactorFans_SetRelativePos(target, this);

    if (this->relativeActorPos.z < 0 || this->relativeActorPos.z > reach) {
        return 0;
    }

    dist = sqrtf(SQ(this->relativeActorPos.x) + SQ(this->relativeActorPos.y)); // target dist from intercept

    if (dist > (FAN_RADIUS * this->scale)) {
        return 0;
    }

    return 1;
}

void ReactorFans_PushPlayer(Player* player, ReactorFans* this, f32 distRatio, PlayState* play) {
    // Scale down power based on distance
    // XZ power is scaled up slightly to make Y power relatively weaker, helps accentuate effect of gravity
    f32 powerXZ = Math_CosS(this->dyna.actor.shape.rot.x) * this->power * distRatio * 1.25f;
    f32 powerY = -Math_SinS(this->dyna.actor.shape.rot.x) * this->power * distRatio;

    player->pushedYaw = this->dyna.actor.shape.rot.y;
    player->pushedSpeed = powerXZ;
    if (powerY > 0 || powerY <= player->actor.minVelocityY) {
        player->actor.velocity.y = powerY + (3.0f * Math_SinS(play->gameplayFrames * DEG_TO_BINANG(22.5f)));
    }
}


void ReactorFans_PushActor(Actor* actor, ReactorFans* this, PlayState* play) {
    f32 dist = sqrtf(SQ(this->relativeActorPos.x) + SQ(this->relativeActorPos.y) + SQ(this->relativeActorPos.z));
    f32 maxDist = FAN_REACH * this->scale;
    f32 scaledThreshold = maxDist * FAN_THRESHOLD;
    f32 distRatio;

    // Calculate the distance ratio based on the threshold
    if (dist <= scaledThreshold) {
        distRatio = 1.0f; // Full power within the threshold
    } else if (dist > scaledThreshold && dist < maxDist) {
        distRatio = 1.0f - ((dist - scaledThreshold) / (maxDist - scaledThreshold));
    } else {
        distRatio = 0.0f; // No power beyond max distance
    }

    if (actor->id == ACTOR_PLAYER) {
        ReactorFans_PushPlayer((Player*)actor, this, distRatio, play);
        return;
    }

    // XZ power is scaled up slightly to make Y power relatively weaker, helps accentuate effect of gravity
    f32 powerXZ = Math_CosS(this->dyna.actor.shape.rot.x) * this->power * distRatio * 1.25;
    f32 powerY = -Math_SinS(this->dyna.actor.shape.rot.x) * this->power * distRatio;
    
    if (powerXZ > 0) {
        actor->speed = powerXZ;
        actor->world.rot.y = this->dyna.actor.shape.rot.y;
    }
    if (powerY > 0 || powerY <= actor->minVelocityY) {
        actor->velocity.y = powerY + (3.0f * Math_SinS(play->gameplayFrames * DEG_TO_BINANG(22.5f)));
    }
}

static s16 sEffectScales[] = {
    30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 17, 16, 15, 14, 12, 10, 8, 7, 5, 3, 2,
};

void ReactorFans_Rubble_Explode(ReactorFans* this, PlayState* play) {
    Vec3f pos;
    Vec3f velocity;
    Gfx* dlist;
    s16 arg5;
    s16 scale;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEffectScales); i++) {
        pos.x = ((Rand_ZeroOne() - 0.5f) * 30.0f) + this->dyna.actor.home.pos.x;
        pos.y = ((Rand_ZeroOne() * 5.0f) + this->dyna.actor.home.pos.y) - 150.0f;
        pos.z = ((Rand_ZeroOne() - 0.5f) * 30.0f) + this->dyna.actor.home.pos.z;
        velocity.x = (Rand_ZeroOne() - 0.5f) * 45.0f;
        velocity.y = (Rand_ZeroOne() * 16.0f) + 5.0f;
        velocity.z = (Rand_ZeroOne() - 0.5f) * 45.0f;
        scale = sEffectScales[i] + 10;
        arg5 = (scale >= 16) ? 37 : 33;
        EffectSsKakera_Spawn(play, &pos, &velocity, &pos, -400, arg5, 10, 2, 0, scale, 1, 0, 80, KAKERA_COLOR_NONE,
                             OBJECT_GAMEPLAY_DANGEON_KEEP, gBrownFragmentDL);
    }
    func_80033480(play, &this->dyna.actor.world.pos, 60.0f, 8, 100, 160, 1);
}

void ReactorFans_Update(Actor* thisx, PlayState* play) {
    ReactorFans* this = (ReactorFans*)thisx;
    Actor* actor = &GET_PLAYER(play)->actor;
    Actor* explosive;

    if (!Flags_GetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params))) {
        if (this->collider.base.acFlags & AC_HIT) {
            sNumFansOn++;
            Flags_SetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params));
            SfxSource_PlaySfxAtFixedWorldPos(play, &this->dyna.actor.world.pos, 80, NA_SE_EV_WALL_BROKEN);
            ReactorFans_Rubble_Explode(this, play);
        }
        Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
    }

    if (sNumFansOn == 2) {
        Flags_SetSwitch(play, FANS_GET_SWITCH_MAIN(this->dyna.actor.params));
    }

    if (this->power != 0) {
        if (ReactorFans_ActorInRange(actor, this)) {
            ReactorFans_PushActor(actor, this, play);
        }

        explosive = play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].head;
        while (explosive != NULL) {
            if (ReactorFans_ActorInRange(explosive, this)) {
                ReactorFans_PushActor(explosive, this, play);
            }
            explosive = explosive->next;
        }

        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_WIND_TRAP - SFX_FLAG);
    }

    if (Flags_GetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params))) {
        Math_SmoothStepToF(&this->power, FAN_POWER, 1.0f, 0.5f, 0.0f);
    } else {
        Math_SmoothStepToF(&this->power, 0.0f, 1.0f, 0.5f, 0.0f);
    }

    this->propellerRot += DEG_TO_BINANG((this->power / FAN_POWER) * 45.0f);
}

void ReactorFans_Draw(Actor* thisx, PlayState* play) {
    ReactorFans* this = (ReactorFans*)thisx;
    MtxF curMatrix;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    Matrix_Scale(this->scale, this->scale, this->scale, MTXMODE_APPLY);

    if (!Flags_GetSwitch(play, FANS_GET_SWITCH_INDIVIDUAL(this->dyna.actor.params))) {
        Gfx_DrawDListOpa(play, gFansRubbleDL);
    }

    Matrix_Get(&curMatrix);
    Matrix_RotateZYX(0, 0, this->propellerRot, MTXMODE_APPLY);

    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gSPDisplayList(POLY_OPA_DISP++, gFanDL);

    Matrix_Put(&curMatrix);

    Matrix_Scale(0.9f, 0.9f, 1.2f * this->power, MTXMODE_APPLY);

    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gSPSegment(POLY_XLU_DISP++, 0x08,
                Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, (play->gameplayFrames * 10) % 128, 64, 16,
                                1, 0, (play->gameplayFrames * 5) % 128, 64, 64));

    gSPDisplayList(POLY_XLU_DISP++, gWindDL);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}





