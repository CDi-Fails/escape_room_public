#include "global.h"

void KaleidoSetup_Update(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = &play->state.input[0];

    if (!IS_PAUSED(pauseCtx) && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
        play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
        gSaveContext.save.cutsceneIndex < 0xFFF0 && gSaveContext.nextCutsceneIndex < 0xFFF0 && !Play_InCsMode(play) &&
        play->shootingGalleryStatus <= 1 && gSaveContext.magicState != MAGIC_STATE_STEP_CAPACITY &&
        gSaveContext.magicState != MAGIC_STATE_FILL &&
        (play->sceneId != SCENE_BOMBCHU_BOWLING_ALLEY || !Flags_GetSwitch(play, 0x38))) {

        if (CHECK_BTN_ALL(input->cur.button, BTN_L) && CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
            if (BREG(0)) {
                pauseCtx->debugState = 3;
            }
        } else if (CHECK_BTN_ALL(input->press.button, BTN_START)) {
            gSaveContext.prevHudVisibilityMode = gSaveContext.hudVisibilityMode;

            WREG(16) = -175;
            WREG(17) = 155;

            pauseCtx->inputDisabled = false;
            pauseCtx->state = PAUSED_STATE_WAIT_LETTERBOX;
        }

        if (pauseCtx->state == PAUSED_STATE_WAIT_LETTERBOX) {
            WREG(2) = -6240;
            R_UPDATE_RATE = 2;

            if (Letterbox_GetSizeTarget() != 0) {
                Letterbox_SetSizeTarget(0);
            }

            Pause_PlayOpenOrCloseSfx(PAUSE_MENU_OPEN_SFX);
        }
    }
}

void KaleidoSetup_Init(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;

    pauseCtx->state = PAUSED_STATE_OFF;
    pauseCtx->debugState = 0;
    pauseCtx->alpha = 0;
    pauseCtx->inputDisabled = false;
    pauseCtx->inventoryCursor.row = PAUSE_ROW_ITEMS;
    pauseCtx->inventoryCursor.slotNum = 0;
    pauseCtx->inventoryCursor.moveDir = 0;
    pauseCtx->inventoryCursor.cooldownTimer = 0;
    pauseCtx->saveCursor.slotNum = 0;
    pauseCtx->saveCursor.moveDir = 0;

    View_Init(&pauseCtx->view, play->state.gfxCtx);
}

void KaleidoSetup_Destroy(PlayState* play) {
}
