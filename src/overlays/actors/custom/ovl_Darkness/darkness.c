#include "darkness.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void Darkness_Init(Actor* thisx, PlayState* play);
void Darkness_Destroy(Actor* thisx, PlayState* play);
void Darkness_Update(Actor* thisx, PlayState* play);
void Darkness_Draw(Actor* thisx, PlayState* play);

void Darkness_FadeToDark(Darkness* this, PlayState* play);

const ActorInit Darkness_InitVars = {
    ACTOR_DARKNESS,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(Darkness),
    Darkness_Init,
    Darkness_Destroy,
    Darkness_Update,
    NULL,
};

#define DARKNESS_GET_SWITCH(params) (params & 0x3F)
#define DARKNESS_FADE_TIME 10
#define DARKNESS_FOG_NEAR 800.0f
#define DARKNESS_Z_FAR 800.0f

void Darkness_Init(Actor* thisx, PlayState* play) {
    Darkness* this = (Darkness*)thisx;
    
    if (!Flags_GetSwitch(play, DARKNESS_GET_SWITCH(this->actor.params))) {
        this->actionFunc = Darkness_FadeToDark;
    } else {
        Actor_Kill(&this->actor);
    }
}

void Darkness_Destroy(Actor* thisx, PlayState* play) {
    Darkness* this = (Darkness*)thisx;

    if (!Flags_GetSwitch(play, DARKNESS_GET_SWITCH(this->actor.params))) {
        Environment_AdjustLights2(play, 0.0f, DARKNESS_FOG_NEAR, 0.0f, 0.0f, DARKNESS_Z_FAR);
    }
}

void Darkness_EndCutscene(Darkness* this, PlayState* play) {
    if (DECR(this->csTimer) == 0) {
        Play_ReturnToMainCam(play, this->subCamId, 0);
        this->subCamId = SUB_CAM_ID_DONE;
        Cutscene_StopManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
        Actor_Kill(&this->actor);
    }
}

void Darkness_FadeToLight(Darkness* this, PlayState* play) {
    if (this->csTimer == 1) {
        Sfx_PlaySfxCentered(NA_SE_EV_TRE_BOX_FLASH);
    }
    if (DECR(this->csTimer) == 0) {
        this->fadeTimer++;
        Environment_AdjustLights2(play, 1.0f - (this->fadeTimer / DARKNESS_FADE_TIME), DARKNESS_FOG_NEAR, 0.0f, 0.0f, DARKNESS_Z_FAR);

        if (this->fadeTimer >= DARKNESS_FADE_TIME) {
            this->csTimer = 30;
            this->actionFunc = Darkness_EndCutscene;
        }
    }
}

void Darkness_WaitForCutscene(Darkness* this, PlayState* play) {
    if (DECR(this->csTimer) == 0) {
        Vec3f camAtWorld = { 5743.84f, 485.172f, -1223.68f };
        Vec3f camEyeWorld = { 5744.16f, 421.71f, -1146.4f };

        this->subCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

        Play_SetCameraAtEye(play, this->subCamId, &camAtWorld, &camEyeWorld);

        Cutscene_StartManual(play, &play->csCtx);
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_WAIT);

        this->csTimer = 15;
        this->actionFunc = Darkness_FadeToLight;
    }
}

void Darkness_IdleDark(Darkness* this, PlayState* play) {
    Environment_AdjustLights2(play, 1.0f, DARKNESS_FOG_NEAR, 0.0f, 0.0f, DARKNESS_Z_FAR);

    if (Flags_GetSwitch(play, DARKNESS_GET_SWITCH(this->actor.params))) {
        this->csTimer = 30;
        this->actionFunc = Darkness_WaitForCutscene;
    }
}

void Darkness_FadeToDark(Darkness* this, PlayState* play) {
    this->fadeTimer++;
    Environment_AdjustLights2(play, this->fadeTimer / DARKNESS_FADE_TIME, DARKNESS_FOG_NEAR, 0.0f, 0.0f, DARKNESS_Z_FAR);

    if (this->fadeTimer >= DARKNESS_FADE_TIME) {
        this->fadeTimer = 0;
        this->actionFunc = Darkness_IdleDark;
    }
}

void Darkness_Update(Actor* thisx, PlayState* play) {
    Darkness* this = (Darkness*)thisx;

    this->actionFunc(this, play);
}
