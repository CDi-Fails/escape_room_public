#include "water_shutter.h"
#include "assets_custom/objects/object_water_shutter/object_water_shutter.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void WaterShutter_Init(Actor* thisx, PlayState* play);
void WaterShutter_Destroy(Actor* thisx, PlayState* play);
void WaterShutter_Update(Actor* thisx, PlayState* play);
void WaterShutter_Draw(Actor* thisx, PlayState* play);

void WaterShutter_DoNothing(WaterShutter* this, PlayState* play);
void WaterShutter_IdleInactive(WaterShutter* this, PlayState* play);
void WaterShutter_StartCutscene(WaterShutter* this, PlayState* play);
void WaterShutter_EndCutscene(WaterShutter* this, PlayState* play);
void WaterShutter_Open(WaterShutter* this, PlayState* play);

const ActorInit Water_Shutter_InitVars = {
    ACTOR_WATER_SHUTTER,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_WATER_SHUTTER,
    sizeof(WaterShutter),
    WaterShutter_Init,
    WaterShutter_Destroy,
    WaterShutter_Update,
    WaterShutter_Draw,
};

#define WATER_SHUTTER_GET_SWITCH(params) (params & 0x3F)


void WaterShutter_Init(Actor* thisx, PlayState* play) {
    WaterShutter* this = (WaterShutter*)thisx;
    CollisionHeader* colHeader = NULL;

    this->subCamId = CAM_ID_NONE;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gWaterShutterDL_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    if (Flags_GetSwitch(play, WATER_SHUTTER_GET_SWITCH(this->dyna.actor.params))) {
        this->dyna.actor.world.pos.x += 1200.0f;
        this->actionFunc = WaterShutter_DoNothing;
    } else {
        this->actionFunc = WaterShutter_IdleInactive;
    }
}

void WaterShutter_Destroy(Actor* thisx, PlayState* play) {
    WaterShutter* this = (WaterShutter*)thisx;
}

void WaterShutter_DoNothing(WaterShutter* this, PlayState* play) {

}

void WaterShutter_StartCutscene(WaterShutter* this, PlayState* play) {
    Vec3f camAtWorld = { 3896.97f, -856.192f, 3988.97f };
    Vec3f camEyeWorld = { 3881.46f, -781.47f, 3924.35f};

    this->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

    Play_SetCameraAtEye(play, this->subCamId, &camAtWorld, &camEyeWorld);

    Cutscene_StartManual(play, &play->csCtx);
    this->actionFunc = WaterShutter_Open;
}

void WaterShutter_EndCutscene(WaterShutter* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        Play_ReturnToMainCam(play, this->subCamId, 0);
        this->subCamId = SUB_CAM_ID_DONE;
        Cutscene_StopManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, &this->dyna.actor, PLAYER_CSACTION_END);
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        this->actionFunc = WaterShutter_DoNothing;
    }
}

#define WATER_SHUTTER_OPEN_STEP 20.0f

void WaterShutter_Open(WaterShutter* this, PlayState* play) {
    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.x, this->dyna.actor.home.pos.x + 1200.0f, 1.0f, WATER_SHUTTER_OPEN_STEP, 0.0f) <= WATER_SHUTTER_OPEN_STEP) {
        Sfx_PlaySfxCentered(NA_SE_EV_METALDOOR_STOP);
        this->timer = 30;
        this->actionFunc = WaterShutter_EndCutscene;
    } else {
        Sfx_PlaySfxCentered(NA_SE_EV_METALDOOR_SLIDE - SFX_FLAG);
    }
}

void WaterShutter_IdleInactive(WaterShutter* this, PlayState* play) {
    if (Flags_GetSwitch(play, WATER_SHUTTER_GET_SWITCH(this->dyna.actor.params))) {
        this->actionFunc = WaterShutter_StartCutscene;
    }
}

void WaterShutter_Update(Actor* thisx, PlayState* play) {
    WaterShutter* this = (WaterShutter*)thisx;

    this->actionFunc(this, play);
}

void WaterShutter_Draw(Actor* thisx, PlayState* play) {
    WaterShutter* this = (WaterShutter*)thisx;

    Gfx_DrawDListOpa(play, gWaterShutterDL);
}
