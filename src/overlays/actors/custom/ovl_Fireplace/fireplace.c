#include "fireplace.h"
#include "assets_custom/objects/object_clock_tower/object_clock_tower.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "quake.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void Fireplace_Init(Actor* thisx, PlayState* play);
void Fireplace_Destroy(Actor* thisx, PlayState* play);
void Fireplace_Update(Actor* thisx, PlayState* play);
void Fireplace_Draw(Actor* thisx, PlayState* play);

void Fireplace_Idle(Fireplace* this, PlayState* play);
void Fireplace_IdleFire(Fireplace* this, PlayState* play);
void Fireplace_DoNothing(Fireplace* this, PlayState* play);

const ActorInit Fireplace_InitVars = {
    ACTOR_FIREPLACE,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_CLOCK_TOWER,
    sizeof(Fireplace),
    Fireplace_Init,
    Fireplace_Destroy,
    Fireplace_Update,
    Fireplace_Draw,
};

static ColliderCylinderInit sCylinderInitFire = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x20000000, 0x01, 0x04 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NONE,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 45, 80, 0, { 0, 0, 0 } },
};

#define FIREPLACE_GET_SWITCH(params) (params & 0x3F)
#define FIREPLACE_GET_TYPE(params) ((params >> 8) & 0xF)

void Fireplace_Init(Actor* thisx, PlayState* play) {
    Fireplace* this = (Fireplace*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);
    this->type = FIREPLACE_GET_TYPE(this->dyna.actor.params);
    this->subCamId = CAM_ID_NONE;

    switch (this->type) {
        case FIREPLACE_TYPE_BASE:
            if (Flags_GetSwitch(play, FIREPLACE_GET_SWITCH(this->dyna.actor.params))) {
                // Move to moved position
                this->dyna.actor.world.pos.z += 200.0f;
                this->actionFunc = Fireplace_DoNothing;
            } else {
                this->actionFunc = Fireplace_Idle;
            }
            DynaPolyActor_Init(&this->dyna, 0);
            CollisionHeader_GetVirtual(&gFireplaceCol_collisionHeader, &colHeader);
            this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
            break;

        case FIREPLACE_TYPE_FIRE:
            if (Flags_GetSwitch(play, FIREPLACE_GET_SWITCH(this->dyna.actor.params))) {
                Actor_Kill(&this->dyna.actor);
            } else {
                Collider_InitCylinder(play, &this->colliderFire);
                Collider_SetCylinder(play, &this->colliderFire, &this->dyna.actor, &sCylinderInitFire);
                Collider_UpdateCylinder(&this->dyna.actor, &this->colliderFire);
                this->dyna.actor.colChkInfo.mass = MASS_IMMOVABLE;
                this->flameScale = 1.0f;
                this->actionFunc = Fireplace_IdleFire;
            }
            break;

        default:
            break;
    }
}

void Fireplace_Destroy(Actor* thisx, PlayState* play) {
    Fireplace* this = (Fireplace*)thisx;

    if (this->type == FIREPLACE_TYPE_BASE) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

void Fireplace_DoNothing(Fireplace* this, PlayState* play) {

}

void Fireplace_RelPosToWorldPos(Actor* actor, Vec3f* result, Vec3f* relPos) {
    f32 cosYaw = Math_CosS(actor->shape.rot.y);
    f32 sinYaw = Math_SinS(actor->shape.rot.y);
    f32 dx = relPos->x * cosYaw + relPos->z * sinYaw;
    f32 dz = relPos->x * sinYaw + relPos->z * cosYaw;

    result->x = actor->world.pos.x + dx;
    result->y = actor->world.pos.y + relPos->y;
    result->z = actor->world.pos.z + dz;
}

void Fireplace_EndCutscene(Fireplace* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        Play_ReturnToMainCam(play, this->subCamId, 0);
        this->subCamId = SUB_CAM_ID_DONE;
        Cutscene_StopManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, &this->dyna.actor, PLAYER_CSACTION_END);
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        this->actionFunc = Fireplace_DoNothing;
    }
}

#define FIREPLACE_MOVE_STEP 3.0f

void Fireplace_Move(Fireplace* this, PlayState* play) {
    Vec3f dustPos = this->dyna.actor.world.pos;

    if (DECR(this->timer) != 0) {
        if (this->timer == 25) {
            s16 quakeIndex = Quake_Request(Play_GetCamera(play, this->subCamId), QUAKE_TYPE_3);

            Rumble_Override(0.0f, 180, 20, 100);
            Quake_SetSpeed(quakeIndex, 20000);
            Quake_SetPerturbations(quakeIndex, 2, 0, 0, 0);
            Quake_SetDuration(quakeIndex, 10);
            dustPos.z += 100.0f;
            func_80033480(play, &dustPos, 100.0f, 19, 75, 75, 1);
            Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_STONE_BOUND);
        }
        Sfx_PlaySfxCentered(NA_SE_EV_EARTHQUAKE - SFX_FLAG);
        return;
    }

    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.z, this->dyna.actor.home.pos.z + 200.0f, 1.0f, FIREPLACE_MOVE_STEP * this->slideAccel, 0.0f) <= FIREPLACE_MOVE_STEP) {
        this->timer = 30;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BLOCK_BOUND);
        this->actionFunc = Fireplace_EndCutscene;
    }

    this->slideAccel += 0.025f;
    this->slideAccel = CLAMP_MAX(this->slideAccel, 1.0f);

    dustPos.z -= 75.0f;

    func_80033480(play, &dustPos, 80.0f, 6, 75, 75, 1);
    Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
}

void Fireplace_WaitForCs(Fireplace* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        Vec3f camAtRel = { 0.0f, 80.0f, 12.0f };
        Vec3f camEyeRel = { -284.0f, 40.0f, 313.0f };
        Vec3f camAtWorld;
        Vec3f camEyeWorld;

        this->subCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

        Fireplace_RelPosToWorldPos(&this->dyna.actor, &camAtWorld, &camAtRel);
        Fireplace_RelPosToWorldPos(&this->dyna.actor, &camEyeWorld, &camEyeRel);

        Play_SetCameraAtEye(play, this->subCamId, &camAtWorld, &camEyeWorld);

        Cutscene_StartManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_WAIT);

        this->timer = 30;
        this->actionFunc = Fireplace_Move;
    }
}

void Fireplace_Idle(Fireplace* this, PlayState* play) {
    if (Flags_GetSwitch(play, FIREPLACE_GET_SWITCH(this->dyna.actor.params))) {
        this->actionFunc = Fireplace_WaitForCs;
        this->timer = 30;
    }
}

void Fireplace_ExtinguishFire(Fireplace* this, PlayState* play) {
    if (this->flameScale <= 0.01f) {
        this->flameScale = 0.0f;
        Flags_SetSwitch(play, FIREPLACE_GET_SWITCH(this->dyna.actor.params));
        Actor_Kill(&this->dyna.actor);
    }

    this->flameScale -= 0.1f;
}

void Fireplace_IdleFire(Fireplace* this, PlayState* play) {
    if (Actor_FindNearby(play, &this->dyna.actor, ACTOR_BOTTLE_WATER, ACTORCAT_ITEMACTION, 100.0f) != NULL) {
        OnePointCutscene_Attention(play, &this->dyna.actor);
        this->actionFunc = Fireplace_ExtinguishFire;
    }

    Collider_UpdateCylinder(&this->dyna.actor, &this->colliderFire);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderFire.base);
    CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderFire.base);
}

void Fireplace_Update(Actor* thisx, PlayState* play) {
    Fireplace* this = (Fireplace*)thisx;

    if (this->type == FIREPLACE_TYPE_BASE) {        
        if (ABS(this->dyna.actor.yDistToPlayer) > 220.0f) {
            DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
        } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
            DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
        }
    }

    this->actionFunc(this, play);
}

void Fireplace_Draw(Actor* thisx, PlayState* play) {
    Fireplace* this = (Fireplace*)thisx;

    switch (this->type) {
        case FIREPLACE_TYPE_BASE:
            Gfx_DrawDListOpa(play, gFireplaceDL);
            break;
        
        case FIREPLACE_TYPE_FIRE:
            if (this->flameScale > 0.0f) {
                f32 scale;

                OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

                Gfx_SetupDL_25Xlu(play->state.gfxCtx);

                gSPSegment(POLY_XLU_DISP++, 0x08,
                           Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, 0, 0x20, 0x40, 1, 0,
                                            (play->gameplayFrames * -20) & 0x1FF, 0x20, 0x80));
                gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 0, 255 * this->flameScale);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 0, 0, 0);

                Matrix_RotateY(BINANG_TO_RAD(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play))), MTXMODE_APPLY);
                scale = this->flameScale * 0.05f;
                Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, gEffFire1DL);

                CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
            }
            break;
    }
}
