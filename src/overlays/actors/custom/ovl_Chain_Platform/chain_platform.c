#include "chain_platform.h"
#include "assets_custom/objects/object_clock_tower/object_clock_tower.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void ChainPlatform_Init(Actor* thisx, PlayState* play);
void ChainPlatform_Destroy(Actor* thisx, PlayState* play);
void ChainPlatform_Update(Actor* thisx, PlayState* play);
void ChainPlatform_Draw(Actor* thisx, PlayState* play);

void ChainPlatform_IdleDown(ChainPlatform* this, PlayState* play);
void ChainPlatform_IdleUp(ChainPlatform* this, PlayState* play);

const ActorInit Chain_Platform_InitVars = {
    ACTOR_CHAIN_PLATFORM,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_CLOCK_TOWER,
    sizeof(ChainPlatform),
    ChainPlatform_Init,
    ChainPlatform_Destroy,
    ChainPlatform_Update,
    ChainPlatform_Draw,
};

#define CHAIN_PLATFORM_GET_SWITCH(params) (params & 0x3F)
#define CHAIN_PLATFORM_LOWER_TIME (10 * 20)
#define CHAIN_PLATFORM_LOWER_DISTANCE 160.0f
#define CHAIN_PLATFORM_LOWER_SPEED 4.0f

void ChainPlatform_Init(Actor* thisx, PlayState* play) {
    ChainPlatform* this = (ChainPlatform*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gChainPlatformCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    this->subCamId = CAM_ID_NONE;

    if (Flags_GetSwitch(play, CHAIN_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        this->actionFunc = ChainPlatform_IdleDown;
    } else {
        this->dyna.actor.world.pos.y += CHAIN_PLATFORM_LOWER_DISTANCE;
        this->actionFunc = ChainPlatform_IdleUp;
    }

    Actor_SpawnAsChild(&play->actorCtx, &this->dyna.actor, play, ACTOR_EN_BOX, this->dyna.actor.world.pos.x,
                       this->dyna.actor.world.pos.y, this->dyna.actor.world.pos.z, 0, DEG_TO_BINANG(90.0f), 0, 0x0100);
}

void ChainPlatform_Destroy(Actor* thisx, PlayState* play) {
    ChainPlatform* this = (ChainPlatform*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void ChainPlatform_IdleDown(ChainPlatform* this, PlayState* play) {
}

void ChainPlatform_EndCutscene(ChainPlatform* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        Play_ReturnToMainCam(play, this->subCamId, 0);
        this->subCamId = SUB_CAM_ID_DONE;
        Cutscene_StopManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, &this->dyna.actor, PLAYER_CSACTION_END);
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        this->actionFunc = ChainPlatform_IdleDown;
    }
}

void ChainPlatform_LowerDown(ChainPlatform* this, PlayState* play) {
    f32 distFromBottom;

    Math_SmoothStepToF(&this->dyna.actor.velocity.y, 4.0f, 0.1f, 1.0f, 0.0f);
    distFromBottom = Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y, 0.1f, this->dyna.actor.velocity.y, 0.2f);
    Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BRIDGE_CLOSE - SFX_FLAG);

    if (fabsf(distFromBottom) < 0.001f) {
        this->timer = 30;
        this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BRIDGE_CLOSE_STOP);
        this->actionFunc = ChainPlatform_EndCutscene;
    }
}

void ChainPlatform_RelPosToWorldPos(Actor* actor, Vec3f* result, Vec3f* relPos) {
    f32 cosYaw = Math_CosS(actor->shape.rot.y);
    f32 sinYaw = Math_SinS(actor->shape.rot.y);
    f32 dx = relPos->x * cosYaw + relPos->z * sinYaw;
    f32 dz = relPos->x * sinYaw + relPos->z * cosYaw;

    result->x = actor->world.pos.x + dx;
    result->y = actor->world.pos.y + relPos->y;
    result->z = actor->world.pos.z + dz;
}

void ChainPlatform_StartCutscene(ChainPlatform* this, PlayState* play) {
    Vec3f camAtRel = { -120.0f, 0.0f, 0.0f };
    Vec3f camEyeRel = { 39.96f, -CHAIN_PLATFORM_LOWER_DISTANCE + 45.07f, 640.71f };
    Vec3f camAtWorld;
    Vec3f camEyeWorld;

    this->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

    ChainPlatform_RelPosToWorldPos(&this->dyna.actor, &camAtWorld, &camAtRel);
    ChainPlatform_RelPosToWorldPos(&this->dyna.actor, &camEyeWorld, &camEyeRel);

    Play_SetCameraAtEye(play, this->subCamId, &camAtWorld, &camEyeWorld);

    this->actionFunc = ChainPlatform_LowerDown;
}

void ChainPlatform_IdleUp(ChainPlatform* this, PlayState* play) {
    if (Flags_GetSwitch(play, CHAIN_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        Cutscene_StartManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_WAIT);
        this->actionFunc = ChainPlatform_StartCutscene;
    }
}

void ChainPlatform_Update(Actor* thisx, PlayState* play) {
    ChainPlatform* this = (ChainPlatform*)thisx;

    if (this->dyna.actor.child != NULL) {
        this->dyna.actor.child->world.pos = this->dyna.actor.world.pos;
    }

    if (ABS(this->dyna.actor.yDistToPlayer) > 600.0f) {
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
        DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }

    this->actionFunc(this, play);
}

void ChainPlatform_Draw(Actor* thisx, PlayState* play) {
    ChainPlatform* this = (ChainPlatform*)thisx;

    Gfx_DrawDListOpa(play, gChainPlatformDL);
    Gfx_DrawDListXlu(play, gChainPlatformLightDL);
}
