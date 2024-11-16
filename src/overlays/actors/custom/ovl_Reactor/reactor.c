#include "reactor.h"

#define FLAGS (ACTOR_FLAG_0 | ACTOR_FLAG_3 | ACTOR_FLAG_27 | ACTOR_FLAG_DRAW_FOCUS_INDICATOR)

void Reactor_Init(Actor* thisx, PlayState* play);
void Reactor_Destroy(Actor* thisx, PlayState* play);
void Reactor_Update(Actor* thisx, PlayState* play);

void Reactor_WaitForTalk(Reactor* this, PlayState* play);
void Reactor_ErrorMessage(Reactor* this, PlayState* play);
void Reactor_StartCutscene(Reactor* this, PlayState* play);
void Reactor_Cutscene(Reactor* this, PlayState* play);
void Reactor_EndCutscene(Reactor* this, PlayState* play);

const ActorInit Reactor_InitVars = {
    ACTOR_REACTOR,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(Reactor),
    Reactor_Init,
    Reactor_Destroy,
    Reactor_Update,
    NULL,
};

#define REACTOR_GET_SWITCH(params) (params & 0x3F)

#define REACTOR_TEXT_ASK_INPUT 0x71B3
#define REACTOR_TEXT_CUR_DIAGNOSTIC 0x71B4
#define REACTOR_TEXT_PREV_DIAGNOSTIC 0x71BB
#define REACTOR_TEXT_SUCCESS 0x71BA

static ReactorErrorInfo errorInfo[] = {
    { 3, 0x71B5 }, // REACTOR_SWITCH_RODS
    { 0x10, 0x71B6 }, // REACTOR_SWITCH_CIRCUIT
    { 0x12, 0x71B7 }, // REACTOR_SWITCH_FANS
    { 0x13, 0x71B8 }, // REACTOR_SWITCH_LEAK
    { 0x11, 0x71B9 }, // REACTOR_SWITCH_FUEL
};

static u8 sReactorStartSuccess = false;
static u8 sErrorList = 0;

void Reactor_Init(Actor* thisx, PlayState* play) {
    Reactor* this = (Reactor*)thisx;
    s32 error;

    if (Flags_GetSwitch(play, REACTOR_GET_SWITCH(this->actor.params))) {
        this->actor.textId = REACTOR_TEXT_SUCCESS;
    } else {
        for (error = REACTOR_ERROR_RODS; error < REACTOR_ERROR_MAX; error++) {
            if (!Flags_GetSwitch(play, errorInfo[error].switchFlag)) {
                sErrorList |= (1 << error);
            }
        }
        this->actor.textId = REACTOR_TEXT_ASK_INPUT;
    }

    this->subCamId = CAM_ID_NONE;
    this->actionFunc = Reactor_WaitForTalk;
}

void Reactor_Destroy(Actor* thisx, PlayState* play) {
    Reactor* this = (Reactor*)thisx;
}

void Reactor_FinishTalking(Reactor* this, PlayState* play) {
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = Reactor_WaitForTalk;
    }
}

s32 Reactor_TryStartup(Reactor* this, PlayState* play) {
    s32 error;

    for (error = REACTOR_ERROR_RODS; error < REACTOR_ERROR_MAX; error++) {
        if (!Flags_GetSwitch(play, errorInfo[error].switchFlag)) {
            sErrorList |= (1 << error);
        }
    }

    if (sErrorList) {
        return false;
    } else {
        return true;
    }
}

void Reactor_Cutscene(Reactor* this, PlayState* play) {
    // If at frame X, try startup
    if (DECR(this->timer) == 0) {
        // Reset error list, we're about to generate a new one
        sErrorList = 0;
        if (Reactor_TryStartup(this, play)) {
            // Success
            sReactorStartSuccess = true;
            this->actor.textId = REACTOR_TEXT_SUCCESS;
        } else {
            // Failure
            this->actor.textId = REACTOR_TEXT_CUR_DIAGNOSTIC;
            this->curError = 0;
        };

        Reactor_EndCutscene(this, play);
    }
}

void Reactor_RelPosToWorldPos(Actor* actor, Vec3f* result, Vec3f* relPos) {
    f32 cosYaw = Math_CosS(actor->shape.rot.y);
    f32 sinYaw = Math_SinS(actor->shape.rot.y);
    f32 dx = relPos->x * cosYaw + relPos->z * sinYaw;
    f32 dz = relPos->x * sinYaw + relPos->z * cosYaw;

    result->x = actor->world.pos.x + dx;
    result->y = actor->world.pos.y + relPos->y;
    result->z = actor->world.pos.z + dz;
}

#define REACTOR_CUTSCENE_LENGTH 3 * 20

void Reactor_StartCutscene(Reactor* this, PlayState* play) {
    Vec3f camAtRel = { 0.0f, 30.0f, 0.0f };
    Vec3f camEyeRel = { -30.0f, 30.0f, 100.0f };
    Vec3f camAtWorld;
    Vec3f camEyeWorld;

    this->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

    Reactor_RelPosToWorldPos(&this->actor, &camAtWorld, &camAtRel);
    Reactor_RelPosToWorldPos(&this->actor, &camEyeWorld, &camEyeRel);

    Play_SetCameraAtEye(play, this->subCamId, &camAtWorld, &camEyeWorld);

    this->timer = REACTOR_CUTSCENE_LENGTH;
    this->actionFunc = Reactor_Cutscene;
}

void Reactor_Success(Reactor* this, PlayState* play) {
    if (Message_GetState(&play->msgCtx) == TEXT_STATE_DONE && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);
        Flags_SetSwitch(play, REACTOR_GET_SWITCH(this->actor.params));
        this->actionFunc = Reactor_WaitForTalk;
    }
}

void Reactor_EndCutscene(Reactor* this, PlayState* play) {
    Play_ReturnToMainCam(play, this->subCamId, 0);
    this->subCamId = SUB_CAM_ID_DONE;
    Cutscene_StopManual(play, &play->csCtx);
    Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
    if (sErrorList) {
        this->actionFunc = Reactor_ErrorMessage;
    } else {
        this->actionFunc = Reactor_Success;
    }
    Message_StartTextbox(play, this->actor.textId, NULL);
}

void Reactor_ErrorMessage(Reactor* this, PlayState* play) {
    s32 error;

    if (Message_GetState(&play->msgCtx) == TEXT_STATE_DONE && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);

        // Start checking from the current error
        for (error = this->curError; error < REACTOR_ERROR_MAX; error++) {
            if ((sErrorList >> error) & 1) {
                // Increment curError for the next call
                this->curError = error + 1;
                this->actor.textId = errorInfo[error].textId;
                Message_ContinueTextbox(play, this->actor.textId);
                return;
            }
        }

        // No more errors to display
        // Reset text for next talk
        this->actor.textId = REACTOR_TEXT_ASK_INPUT;
        this->actionFunc = Reactor_FinishTalking;
    }
}

void Reactor_Talk(Reactor* this, PlayState* play) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);

        switch (play->msgCtx.choiceIndex) {
            case 0:
                // Reactor start cutscene
                play->msgCtx.msgMode = MSGMODE_PAUSED;
                Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_WAIT);
                this->actionFunc = Reactor_StartCutscene;
                return;
                break;
            case 1:
                this->actor.textId = REACTOR_TEXT_PREV_DIAGNOSTIC;
                // Next frame, we want to start displaying error messages
                this->curError = 0;
                this->actionFunc = Reactor_ErrorMessage;
                break;
            case 2:
                this->actionFunc = Reactor_FinishTalking;
                return;
                break;
        }

        Message_ContinueTextbox(play, this->actor.textId);
    }

    // Once successfully started, repeat the same message over and over
    if (this->actor.textId == REACTOR_TEXT_SUCCESS) {
        this->actionFunc = Reactor_FinishTalking;
    }
}

void Reactor_WaitForTalk(Reactor* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (sReactorStartSuccess) {
        this->actor.textId = REACTOR_TEXT_SUCCESS;
    }

    if (this->actor.textId != 0) {
        if (Actor_TalkOfferAccepted(&this->actor, play)) {
            this->actionFunc = Reactor_Talk;
        } else {
            Actor_OfferTalk(&this->actor, play, 50.0f);
        }
    }
}

void Reactor_Update(Actor* thisx, PlayState* play) {
    Reactor* this = (Reactor*)thisx;
    
    this->actionFunc(this, play);
}
