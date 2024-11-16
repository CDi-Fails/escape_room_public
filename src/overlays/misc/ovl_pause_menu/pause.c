#include "global.h"
#include "pause_helpers.h"
#include "assets_custom/textures/pause_menu_static/pause_menu_static.h"
#include "assets_custom/textures/icon_item_static/icon_item_static.h"
#include "assets/textures/parameter_static/parameter_static.h"

void Pause_Init(PlayState* play);
void Pause_Gameover(PlayState* play);
void Pause_Open(PlayState* play);
void Pause_Inventory(PlayState* play);
void Pause_SavePrompt(PlayState* play);
void Pause_Close(PlayState* play);
void Pause_ResumeGameplay(PlayState* play);

void Pause_ProcessInputs(PlayState* play);
void Pause_SetButtonStatus(PlayState* play);
void Pause_SetEquippedItems(PlayState* play);

void Pause_InitEquipAnimVtx(PlayState* play);
void Pause_SetEquipAnimVtx(PlayState* play);

s32 Pause_HasEquipment(ItemID item);

#define PAUSE_SEGMENT_ALLOC_SIZE(name) ((uintptr_t)_##name##SegmentRomEnd - (uintptr_t)_##name##SegmentRomStart)
#define PAUSE_SEGMENT_ALLOC_START(name) ((uintptr_t)_##name##SegmentRomStart)

#define PAUSE_ALPHA_FADE_RATE 40

#define PAUSE_FIRST_ITEM_SLOT_X 96
#define PAUSE_FIRST_EQUIP_SLOT_X 112
#define PAUSE_INVENTORY_SLOT_OFFSET(i) (32 * i)

#define PAUSE_HAS_BOTTLE_ITEM(item) (INV_CONTENT(item) >= ITEM_BOTTLE_EMPTY && INV_CONTENT(item) <= ITEM_BOTTLE_FISH)
#define PAUSE_HAS_ITEM(item) (item == ITEM_BOTTLE_EMPTY ? PAUSE_HAS_BOTTLE_ITEM(item) : INV_CONTENT(item) == item)

static PauseInventorySlot sItemSlots[] = {
    { .pos.x = PAUSE_FIRST_ITEM_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(0), .pos.y = 88, .item = ITEM_HOOKSHOT, false },
    { .pos.x = PAUSE_FIRST_ITEM_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(1), .pos.y = 88, .item = ITEM_BOMB, false },
    { .pos.x = PAUSE_FIRST_ITEM_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(2), .pos.y = 88, .item = ITEM_POCKET_EGG, false },
    { .pos.x = PAUSE_FIRST_ITEM_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(3), .pos.y = 88, .item = ITEM_BOTTLE_EMPTY, false },
};

static PauseInventorySlot sEquipmentSlots[] = {
    { .pos.x = PAUSE_FIRST_EQUIP_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(0), .pos.y = 146, .item = ITEM_TUNIC_GORON, false },
    { .pos.x = PAUSE_FIRST_EQUIP_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(1), .pos.y = 146, .item = ITEM_BOOTS_IRON, false },
    { .pos.x = PAUSE_FIRST_EQUIP_SLOT_X + PAUSE_INVENTORY_SLOT_OFFSET(2), .pos.y = 146, .item = ITEM_SHIELD_MIRROR, false },
};

static PauseSaveSlot sSaveSlots[] = {
    { .pos.x = 106, .pos.y = 106 },
    { .pos.x = 166, .pos.y = 106 },
};

static u8 sPrevButtonStatus[5];

static Color_RGB8 prim = { 223, 176, 128 };
static Color_RGB8 env = { 10, 9, 19 };

static void* sCursorTexs[] = {
    gNewPauseCursorTopLeftTex,
    gNewPauseCursorTopRightTex,
    gNewPauseCursorBottomLeftTex,
    gNewPauseCursorBottomRightTex,
};

static void* sItemTexs[] = {
    gItemIconHookshotTex,
    gItemIconBombTex,
    gBoosterGauntletsIconTex,
    gItemIconBottleEmptyTex,
};

static void* sItemNameTexs[] = {
    gHookshotNameTex,
    gBombsNameTex,
    gBoosterGauntletsNameTex,
    gEmptyBottleNameTex,
};

static void* sBottleItemTexs[] = {
    gItemIconBottleEmptyTex,
    gItemIconBottlePotionRedTex,
    gItemIconBottlePotionGreenTex,
    gItemIconBottlePotionBlueTex,
    gItemIconBottleFairyTex,
    gItemIconBottleWaterTex,
};

static void* sBottleItemNameTexs[] = {
    gEmptyBottleNameTex,
    gRedPotionNameTex,
    gGreenPotionNameTex,
    gBluePotionNameTex,
    gFairyNameTex,
    gWaterNameTex,
};

static void* sEquipmentTexs[] = {
    gHeatsinkBuckleIconTex,
    gZoraAmuletIconTex,
    gItemIconShieldMirrorTex,
};

static void* sEquipmentNameTexs[] = {
    gHeatsinkBuckleNameTex,
    gZoraAmuletNameTex,
    gMirrorShieldNameTex,
};

void* sPauseBgTextures[5][3] = {
    &gNewPauseBg00, &gNewPauseBg01, &gNewPauseBg02,
    &gNewPauseBg10, &gNewPauseBgEmpty, &gNewPauseBg12,
    &gNewPauseBg20, &gNewPauseBgEmpty, &gNewPauseBg22,
    &gNewPauseBg30, &gNewPauseBgEmpty, &gNewPauseBg32,
    &gNewPauseBg40, &gNewPauseBg41, &gNewPauseBg42,
};

void Pause_DrawCursor(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Color_RGB8 envTarget = { 0, 0, 0 };
    Color_RGB8 envInterp = { 0, 0, 0 };
    u8 x = sItemSlots[0].pos.x;
    u8 y = sItemSlots[0].pos.y;
    f32 glowPercent;

    // Don't draw if equipping equipment
    if (this->inventoryCursor.row == PAUSE_ROW_EQUIPMENT && this->equipTimer != 0) {
        return;
    }

    // Don't draw if equipping items
    if (this->equipAnimState != PAUSE_EQUIP_ANIM_STATE_OFF) {
        return;
    }

    // Wrap glow timer to keep smooth sine
    if (this->cursorGlowTimer > (2 * M_PI)) {
        this->cursorGlowTimer = 0.0f;
    }

    this->cursorGlowTimer += 0.125f;
    glowPercent = (sinf(this->cursorGlowTimer) + 1.0f) / 2.0f;

    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            x = sItemSlots[this->inventoryCursor.slotNum].pos.x - 2;
            y = sItemSlots[this->inventoryCursor.slotNum].pos.y - 2;
            break;

        case PAUSE_ROW_EQUIPMENT:
            x = sEquipmentSlots[this->inventoryCursor.slotNum].pos.x - 2;
            y = sEquipmentSlots[this->inventoryCursor.slotNum].pos.y - 2;
            break;
    }

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 160, 0, this->foregroundAlpha);
            envTarget.r = 255;
            envTarget.g = 160;
            envTarget.b = 0;
            break;
        
        case PAUSE_ROW_EQUIPMENT:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0, 100, 255, this->foregroundAlpha);
            envTarget.r = 0;
            envTarget.g = 100;
            envTarget.b = 255;
            break;
    }

    envInterp.r = (u8)LERP(0, envTarget.r, glowPercent);
    envInterp.g = (u8)LERP(0, envTarget.g, glowPercent);
    envInterp.b = (u8)LERP(0, envTarget.b, glowPercent);

    gDPSetEnvColor(POLY_OPA_DISP++, envInterp.r, envInterp.g, envInterp.b, 0);

    POLY_OPA_DISP = Pause_LoadTexIA4(POLY_OPA_DISP, gNewPauseCursorTopLeftTex, 16, 16);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x, y, 16, 16);
    POLY_OPA_DISP = Pause_LoadTexIA4(POLY_OPA_DISP, gNewPauseCursorTopRightTex, 16, 16);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x + 16, y, 16, 16);
    POLY_OPA_DISP = Pause_LoadTexIA4(POLY_OPA_DISP, gNewPauseCursorBottomLeftTex, 16, 16);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x, y + 16, 16, 16);
    POLY_OPA_DISP = Pause_LoadTexIA4(POLY_OPA_DISP, gNewPauseCursorBottomRightTex, 16, 16);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x + 16, y + 16, 16, 16);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_StartClose(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    this->inputDisabled = true;
    this->state = PAUSED_STATE_CLOSING;
    if (play->gameOverCtx.state == GAMEOVER_INACTIVE) {
        Pause_PlayOpenOrCloseSfx(PAUSE_MENU_CLOSE_SFX);
    }
}

#define PAUSE_CURSOR_START_MOVE_COOLDOWN_TIME 10
#define PAUSE_CURSOR_CONTINUE_MOVE_COOLDOWN_TIME 3

#define PAUSE_STICK_ACTIVE_THRESHOLD 30

void Pause_TransformStickSavePrompt(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Input* input = &play->state.input[0];

    if (input->rel.stick_x > PAUSE_STICK_ACTIVE_THRESHOLD) {
        this->saveCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_RIGHT;
    }
    if (input->rel.stick_x < -PAUSE_STICK_ACTIVE_THRESHOLD) {
        this->saveCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_LEFT;
    }
};

void Pause_TransformStickInventory(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Input* input = &play->state.input[0];

    if (input->rel.stick_x == 0 && input->rel.stick_y == 0) {
        this->inventoryCursor.cooldownTimer = 0;
    }

    if (this->inventoryCursor.cooldownTimer == 0) {
        if (input->rel.stick_x > PAUSE_STICK_ACTIVE_THRESHOLD) {
            this->inventoryCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_RIGHT;
        }
        if (input->rel.stick_x < -PAUSE_STICK_ACTIVE_THRESHOLD) {
            this->inventoryCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_LEFT;
        }
    }
    // Don't check cooldown for up/down movement
    if (input->rel.stick_y > PAUSE_STICK_ACTIVE_THRESHOLD) {
        this->inventoryCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_UP;
    }
    if (input->rel.stick_y < -PAUSE_STICK_ACTIVE_THRESHOLD) {
        this->inventoryCursor.moveDir |= PAUSE_CURSOR_MOVEDIR_DOWN;
    }
};

void Pause_ProcessInputs(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Input* input = &play->state.input[0];

    if (this->inputDisabled) {
        return;
    }

    switch (this->state) {
        case PAUSED_STATE_INVENTORY:
            Pause_TransformStickInventory(play);
            // Close pause menu
            if (CHECK_BTN_ALL(input->press.button, BTN_START)) {
                Pause_StartClose(play);
                return;
            }
            // Setup fade to save prompt, disable input during fade to save prompt
            if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
                this->inputDisabled = true;
                Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
                this->state = PAUSED_STATE_FADE_OUT_INVENTORY;
                return;
            }
            break;
        
        case PAUSED_STATE_SAVE_PROMPT:
            Pause_TransformStickSavePrompt(play);
            // Close pause menu
            if (CHECK_BTN_ALL(input->press.button, BTN_START) || CHECK_BTN_ALL(input->press.button, BTN_B)) {
                Pause_StartClose(play);
            }
            // "A" button pressed on yes/no
            if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                switch (this->saveCursor.slotNum) {
                    // Yes
                    case 0:
                        Sfx_PlaySfxCentered(NA_SE_SY_PIECE_OF_HEART);
                        Play_SaveSceneFlags(play);
                        gSaveContext.save.info.playerData.savedSceneId = play->sceneId;
                        Sram_WriteSave(&play->sramCtx);
                        Pause_StartClose(play);
                        break;

                    // No
                    case 1:
                        Pause_StartClose(play);
                        break;
                }
            }
            break;

        case PAUSED_STATE_GAMEOVER_SAVE_PROMPT:
            Pause_TransformStickSavePrompt(play);
            // "A" button pressed on yes/no
            if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                if (this->saveCursor.slotNum == 0) {
                    // "Yes", save
                    Sfx_PlaySfxCentered(NA_SE_SY_PIECE_OF_HEART);
                    Play_SaveSceneFlags(play);
                    gSaveContext.save.info.playerData.savedSceneId = play->sceneId;
                    Sram_WriteSave(&play->sramCtx);
                } else {
                    Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
                }

                this->state = PAUSED_STATE_GAMEOVER_CONTINUE_PROMPT;
            }
            break;

        case PAUSED_STATE_GAMEOVER_CONTINUE_PROMPT:
            Pause_TransformStickSavePrompt(play);
            // "A" button pressed on yes/no
            if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                switch (this->saveCursor.slotNum) {
                    case 0:
                        // "Yes", continue
                        Sfx_PlaySfxCentered(NA_SE_SY_PIECE_OF_HEART);
                        this->promptChoice = PAUSE_GAMEOVER_PROMPT_CONTINUE;
                        break;

                    case 1:
                        // "No", quit
                        Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
                        this->promptChoice = PAUSE_GAMEOVER_PROMPT_QUIT;
                        break;
                }
                
                Pause_StartClose(play);
            }
            break;

        default:
            break;
    }
}


void Pause_UpdateCursorSavePrompt(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    if (this->saveCursor.moveDir & PAUSE_CURSOR_MOVEDIR_LEFT && this->saveCursor.slotNum == 1) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->saveCursor.slotNum--;
    } else if (this->saveCursor.moveDir & PAUSE_CURSOR_MOVEDIR_RIGHT && this->saveCursor.slotNum == 0) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->saveCursor.slotNum++;
    }

    this->saveCursor.moveDir = 0;
}

void Pause_UpdateCursorInventory(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    u8 maxItemSlots = sizeof(sItemSlots) / sizeof(sItemSlots[0]);
    u8 maxEquipmentSlots = sizeof(sEquipmentSlots) / sizeof(sEquipmentSlots[0]);

    if (this->inventoryCursor.moveDir == 0) {
        DECR(this->inventoryCursor.cooldownTimer);
        return;
    }

    // Only set up cooldown for left/right movement
    if (this->inventoryCursor.moveDir & (PAUSE_CURSOR_MOVEDIR_LEFT | PAUSE_CURSOR_MOVEDIR_RIGHT)) {
        if (this->inventoryCursor.moveDir == this->inventoryCursor.prevMoveDir) {
            this->inventoryCursor.cooldownTimer = PAUSE_CURSOR_CONTINUE_MOVE_COOLDOWN_TIME;
        } else {
            this->inventoryCursor.cooldownTimer = PAUSE_CURSOR_START_MOVE_COOLDOWN_TIME;
        }
    }

    // Handle vertical movement
    if (this->inventoryCursor.moveDir & PAUSE_CURSOR_MOVEDIR_UP && this->inventoryCursor.row == PAUSE_ROW_EQUIPMENT) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->inventoryCursor.row = PAUSE_ROW_ITEMS;
        Pause_SetButtonStatus(play);
        if (this->inventoryCursor.slotNum >= maxItemSlots) {
            this->inventoryCursor.slotNum = maxItemSlots - 1;
        }
    } else if (this->inventoryCursor.moveDir & PAUSE_CURSOR_MOVEDIR_DOWN && this->inventoryCursor.row == PAUSE_ROW_ITEMS) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->inventoryCursor.row = PAUSE_ROW_EQUIPMENT;
        Pause_SetButtonStatus(play);
        if (this->inventoryCursor.slotNum >= maxEquipmentSlots) {
            this->inventoryCursor.slotNum = maxEquipmentSlots - 1;
        }
    }

    // Handle horizontal movement
    u8 maxSlotsCurrentRow = (this->inventoryCursor.row == PAUSE_ROW_ITEMS) ? maxItemSlots : maxEquipmentSlots;

    if (this->inventoryCursor.moveDir & PAUSE_CURSOR_MOVEDIR_LEFT && this->inventoryCursor.slotNum > 0) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->inventoryCursor.slotNum--;
    } else if (this->inventoryCursor.moveDir & PAUSE_CURSOR_MOVEDIR_RIGHT && this->inventoryCursor.slotNum < maxSlotsCurrentRow - 1) {
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        this->inventoryCursor.slotNum++;
    }

    // Reset movement direction after moving
    this->inventoryCursor.prevMoveDir = this->inventoryCursor.moveDir;
    this->inventoryCursor.moveDir = 0;
}

void Pause_Update(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    switch (this->state) {
        case PAUSED_STATE_INIT:
            Pause_Init(play);
            break;

        case PAUSED_STATE_GAMEOVER_SAVE_PROMPT:
        case PAUSED_STATE_GAMEOVER_CONTINUE_PROMPT:
            Pause_ProcessInputs(play);
            Pause_UpdateCursorSavePrompt(play);
            Pause_Gameover(play);
            break;

        case PAUSED_STATE_OPENING:
            Pause_Open(play);
            break;

        case PAUSED_STATE_INVENTORY:
            Pause_ProcessInputs(play);
            Pause_UpdateCursorInventory(play);
            Pause_Inventory(play);
            break;

        case PAUSED_STATE_FADE_OUT_INVENTORY:
            this->foregroundAlpha = CLAMP_MIN(this->foregroundAlpha - (PAUSE_ALPHA_FADE_RATE * 2), 0);
            if (this->foregroundAlpha == 0) {
                this->state = PAUSED_STATE_FADE_IN_SAVE_PROMPT;
            }
            break;
        
        case PAUSED_STATE_FADE_IN_SAVE_PROMPT:
            this->foregroundAlpha = CLAMP_MAX(this->foregroundAlpha + (PAUSE_ALPHA_FADE_RATE * 2), 255);
            if (this->foregroundAlpha == 255) {
                gSaveContext.buttonStatus[0] = BTN_DISABLED; // B
                gSaveContext.buttonStatus[1] = BTN_DISABLED; // C-Left
                gSaveContext.buttonStatus[2] = BTN_DISABLED; // C-Down
                gSaveContext.buttonStatus[3] = BTN_DISABLED; // C-Left
                gSaveContext.buttonStatus[4] = BTN_ENABLED; // A
                gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
                Interface_LoadActionLabelB(play, DO_ACTION_SAVE);
                this->inputDisabled = false;
                this->state = PAUSED_STATE_SAVE_PROMPT;
            }
            break;

        case PAUSED_STATE_SAVE_PROMPT:
            Pause_ProcessInputs(play);
            Pause_UpdateCursorSavePrompt(play);
            Pause_SavePrompt(play);
            break;

        case PAUSED_STATE_CLOSING:
            Pause_Close(play);
            break;
        
        case PAUSED_STATE_RESUME_GAMEPLAY:
            Pause_ResumeGameplay(play);
            break;

        default:
            break;
    }
}

void Pause_SegmentAlloc(u8** segment, uintptr_t segStart, void** segAllocPtr, u32 size) {
    *segment = *segAllocPtr;
    DmaMgr_RequestSyncDebug(*segAllocPtr, segStart, size, __FILE__, __LINE__);
    *segAllocPtr += size;
}

void Pause_SetButtonStatus(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            gSaveContext.buttonStatus[0] = BTN_ENABLED; // B
            gSaveContext.buttonStatus[1] = BTN_ENABLED; // C-Left
            gSaveContext.buttonStatus[2] = BTN_ENABLED; // C-Down
            gSaveContext.buttonStatus[3] = BTN_ENABLED; // C-Left
            gSaveContext.buttonStatus[4] = BTN_DISABLED; // A
            break;
        
        case PAUSE_ROW_EQUIPMENT:
            gSaveContext.buttonStatus[0] = BTN_ENABLED; // B
            gSaveContext.buttonStatus[1] = BTN_DISABLED; // C-Left
            gSaveContext.buttonStatus[2] = BTN_DISABLED; // C-Down
            gSaveContext.buttonStatus[3] = BTN_DISABLED; // C-Left
            gSaveContext.buttonStatus[4] = BTN_ENABLED; // A
            break;
    }

    if (this->state != PAUSED_STATE_INIT) {
        gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
        Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
    }
}

void Pause_Init(PlayState* play) {
    void* segAllocPtr;
    PauseContext* this = &play->pauseCtx;

    segAllocPtr = (void*)ALIGN64((uintptr_t)play->objectCtx.spaceStart);

    Pause_SegmentAlloc(&this->iconItemSegment, PAUSE_SEGMENT_ALLOC_START(icon_item_static), &segAllocPtr, PAUSE_SEGMENT_ALLOC_SIZE(icon_item_static));
    Pause_SegmentAlloc(&this->pauseMenuSegment, PAUSE_SEGMENT_ALLOC_START(pause_menu_static), &segAllocPtr, PAUSE_SEGMENT_ALLOC_SIZE(pause_menu_static));
    // TODO: Allocate more segments? If needed?

    gSegments[8] = VIRTUAL_TO_PHYSICAL(this->iconItemSegment);
    gSegments[9] = VIRTUAL_TO_PHYSICAL(this->pauseMenuSegment);

    Pause_SetEquippedItems(play);

    Pause_InitEquipAnimVtx(play);

    this->equipTimer = 0;
    this->equipAnimTimer = 0.0f;
    this->equipAnimState = PAUSE_EQUIP_ANIM_STATE_OFF;
    this->cursorGlowTimer = 0;
    if (play->gameOverCtx.state != GAMEOVER_INACTIVE) {
        sPauseBgTextures[0][1] = &gPauseMenuBgGameOver;
        gSaveContext.save.info.playerData.deaths++;
        if (gSaveContext.save.info.playerData.deaths > 999) {
            gSaveContext.save.info.playerData.deaths = 999;
        }
    } else {
        sPrevButtonStatus[0] = gSaveContext.buttonStatus[0];
        sPrevButtonStatus[1] = gSaveContext.buttonStatus[1];
        sPrevButtonStatus[2] = gSaveContext.buttonStatus[2];
        sPrevButtonStatus[3] = gSaveContext.buttonStatus[3];
        sPrevButtonStatus[4] = gSaveContext.buttonStatus[4];

        Interface_SetDoAction(play, DO_ACTION_DECIDE);
        func_80084BF4(play, 1);
        Pause_SetButtonStatus(play);
        sPauseBgTextures[0][1] = &gNewPauseBg01;
    }
    this->state = PAUSED_STATE_OPENING;
}

void Pause_Gameover(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    // I think this fixes that stupid no input after death bug?
    this->inputDisabled = false;
}

void Pause_Open(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    this->alpha = CLAMP_MAX(this->alpha + PAUSE_ALPHA_FADE_RATE, 255);
    this->foregroundAlpha = this->alpha;

    if (this->alpha >= 127 && play->gameOverCtx.state == GAMEOVER_INACTIVE) {
        interfaceCtx->startAlpha = CLAMP_MAX(interfaceCtx->startAlpha + (PAUSE_ALPHA_FADE_RATE * 2), 255);
    }

    if (this->alpha == 255) {
        Interface_LoadActionLabelB(play, DO_ACTION_SAVE);
        if (play->gameOverCtx.state == GAMEOVER_DEATH_MENU) {
            this->state = PAUSED_STATE_GAMEOVER_SAVE_PROMPT;
        } else {
            this->state = PAUSED_STATE_INVENTORY;
        }
    }
}

#define PAUSE_BTN_ITEM(button) (gSaveContext.save.info.equips.buttonItems[button])
#define PAUSE_BTN_SLOT(button) (gSaveContext.save.info.equips.cButtonSlots[button])

void Pause_SwapButtonItems(PlayState* play, u8 equipBtn, u8 swapBtn) {
    // Check if an item is on equipBtn...
    if (PAUSE_BTN_ITEM(equipBtn + 1) != ITEM_NONE) {
        // There's an item on equipBtn, we need to swap equipBtn and swapBtn
        // Put the item currently on equipBtn onto swapBtn to make space for our equip item
        PAUSE_BTN_ITEM(swapBtn + 1) = PAUSE_BTN_ITEM(equipBtn + 1);
        // Update the swapBtn slot so it holds the item that was on equipBtn before our equip item
        PAUSE_BTN_SLOT(swapBtn) = PAUSE_BTN_SLOT(equipBtn);
        // Refresh the item icon on swapBtn so it holds the item that was on equipBtn before our equip item
        Interface_LoadItemIcon2(play, swapBtn + 1);
    } else {
        // No item is on equipBtn, so there's nothing to swap
        // Put an empty item on swapBtn so it'll be empty when we swap our equip item to equipBtn
        PAUSE_BTN_ITEM(swapBtn + 1) = ITEM_NONE;
        PAUSE_BTN_SLOT(swapBtn) = SLOT_NONE;
    }
}

// Equips item onto equip target button, swaps items on buttons if equip item is already on another button
void Pause_UpdateItemButtons(PlayState* play, u8 equipBtn) {
    PauseContext* this = &play->pauseCtx;
    s8 otherBtn1 = -1;
    s8 otherBtn2 = -1;
    u8 i;

    // Set up which buttons we're going to be checking by looping through buttons,
    // and selecting every button that isn't the target button
    for (i = 0; i < 3; i++) {
        if (i != equipBtn) {
            if (otherBtn1 == -1) {
                otherBtn1 = i;
            } else {
                otherBtn2 = i;
                break;
            }
        }
    }

    // We want our equip item to go to equipBtn
    // If that equip item is on otherBtn1 already...
    if (this->equipTargetSlot == PAUSE_BTN_SLOT(otherBtn1)) {
        Pause_SwapButtonItems(play, equipBtn, otherBtn1);
    } else {
        // If equip item is on otherBtn2 already...
        if (this->equipTargetSlot == PAUSE_BTN_SLOT(otherBtn2)) {
            Pause_SwapButtonItems(play, equipBtn, otherBtn2);
        }
    }

    // Put our item onto equipBtn
    PAUSE_BTN_ITEM(equipBtn + 1) = this->equipTargetItem;
    PAUSE_BTN_SLOT(equipBtn) = this->equipTargetSlot;
    Interface_LoadItemIcon1(play, equipBtn + 1);
}

void Pause_SetEquippedItems(PlayState* play) {
    u8 row;
    u8 slotNum;
    u8 cBtn;
    u8 btnsNotEquipped = 0;

    for (row = PAUSE_ROW_ITEMS; row <= PAUSE_ROW_EQUIPMENT; row++) {
        switch (row) {
            case PAUSE_ROW_ITEMS:
                for (slotNum = 0, btnsNotEquipped = 0; slotNum < ARRAY_COUNT(sItemSlots); slotNum++, btnsNotEquipped = 0) {
                    for (cBtn = PAUSE_BTN_C_LEFT; cBtn <= PAUSE_BTN_C_RIGHT; cBtn++) {
                        if (sItemSlots[slotNum].item == ITEM_BOTTLE_EMPTY) {
                            u8 bottleItem;

                            // Check every bottle item
                            for (bottleItem = ITEM_BOTTLE_EMPTY; bottleItem <= ITEM_BOTTLE_FAIRY; bottleItem++) {
                                if (PAUSE_BTN_ITEM(cBtn + 1) == bottleItem) {
                                    sItemSlots[slotNum].isEquipped = true;
                                    break;
                                } else if (bottleItem == ITEM_BOTTLE_FAIRY) {
                                   btnsNotEquipped++;
                                }
                            }
                        }
                        // Handle everything that's not bottles
                        else {
                            if (PAUSE_BTN_ITEM(cBtn + 1) == sItemSlots[slotNum].item) {
                                sItemSlots[slotNum].isEquipped = true;
                                break;
                            } else {
                                btnsNotEquipped++;
                            }
                        }
                    }
                    if (btnsNotEquipped == 3) {
                        sItemSlots[slotNum].isEquipped = false;
                    }
                }
                break;

            case PAUSE_ROW_EQUIPMENT:
                for (slotNum = 0; slotNum < ARRAY_COUNT(sEquipmentSlots); slotNum++) {
                    switch (sEquipmentSlots[slotNum].item) {
                        case ITEM_TUNIC_GORON:
                            sEquipmentSlots[slotNum].isEquipped =
                                (CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC) == EQUIP_VALUE_TUNIC_HEATSINK);
                            break;

                        case ITEM_BOOTS_IRON:
                            sEquipmentSlots[slotNum].isEquipped =
                                (CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS) == EQUIP_VALUE_BOOTS_ZORA);
                            break;

                        case ITEM_SHIELD_MIRROR:
                            sEquipmentSlots[slotNum].isEquipped =
                                (CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_MIRROR);
                            break;

                        default:
                            break;
                    }
                }
                break;
        }
    }
}

void Pause_EquipItem(PlayState* play) {
    static s16 timer = 0;
    PauseContext* this = &play->pauseCtx;

    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            Pause_UpdateItemButtons(play, this->equipTargetCBtn);
            break;

        case PAUSE_ROW_EQUIPMENT:
            switch (sEquipmentSlots[this->inventoryCursor.slotNum].item) {
                case ITEM_TUNIC_GORON:
                    if (CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC) == EQUIP_VALUE_TUNIC_HEATSINK) {
                        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_KOKIRI);
                    } else {
                        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_HEATSINK);
                    }
                    sEquipmentSlots[this->inventoryCursor.slotNum].isEquipped ^= 1;
                    break;

                case ITEM_BOOTS_IRON:
                    if (CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS) == EQUIP_VALUE_BOOTS_ZORA) {
                        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_KOKIRI);
                    } else {
                        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_ZORA);
                    }
                    sEquipmentSlots[this->inventoryCursor.slotNum].isEquipped ^= 1;
                    break;

                case ITEM_SHIELD_MIRROR:
                    if (CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_MIRROR) {
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
                    } else {
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
                    }
                    sEquipmentSlots[this->inventoryCursor.slotNum].isEquipped ^= 1;
                    break;

                default:
                    break;
            }
            break;
    }
}

void Pause_SetEquipTargetCBtn(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Input* input = &play->state.input[0];

    if (CHECK_BTN_ALL(input->press.button, BTN_CLEFT)) {
        this->equipTargetCBtn = PAUSE_BTN_C_LEFT;
    } else if (CHECK_BTN_ALL(input->press.button, BTN_CDOWN)) {
        this->equipTargetCBtn = PAUSE_BTN_C_DOWN;
    } else if (CHECK_BTN_ALL(input->press.button, BTN_CRIGHT)) {
        this->equipTargetCBtn = PAUSE_BTN_C_RIGHT;
    }
}

void Pause_StartEquipItem(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    if (sItemSlots[this->inventoryCursor.slotNum].item == ITEM_BOTTLE_EMPTY) {
        this->equipTargetItem = INV_CONTENT(ITEM_BOTTLE_EMPTY);
    } else {
        this->equipTargetItem = sItemSlots[this->inventoryCursor.slotNum].item;
    }
    this->equipTargetSlot = SLOT(sItemSlots[this->inventoryCursor.slotNum].item);
    this->equipAnimX = sItemSlots[this->inventoryCursor.slotNum].pos.x;
    this->equipAnimY = sItemSlots[this->inventoryCursor.slotNum].pos.y;
    this->equipAnimTimer = 0.0f;
    this->equipAnimAlpha = 0;
    this->equipAnimState = PAUSE_EQUIP_ANIM_STATE_ANIMATE;
    this->inputDisabled = true;
    Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
}

#define PAUSE_ITEM_VTX_X(vtx) (this->itemVtx[vtx].v.ob[0])
#define PAUSE_ITEM_VTX_Y(vtx) (this->itemVtx[vtx].v.ob[1])
#define PAUSE_ITEM_VTX_Z(vtx) (this->itemVtx[vtx].v.ob[2])

#define PAUSE_ITEM_VTX_U(vtx) (this->itemVtx[vtx].v.tc[0])
#define PAUSE_ITEM_VTX_V(vtx) (this->itemVtx[vtx].v.tc[1])

#define PAUSE_ITEM_VTX_R(vtx) (this->itemVtx[vtx].v.cn[0])
#define PAUSE_ITEM_VTX_G(vtx) (this->itemVtx[vtx].v.cn[1])
#define PAUSE_ITEM_VTX_B(vtx) (this->itemVtx[vtx].v.cn[2])
#define PAUSE_ITEM_VTX_A(vtx) (this->itemVtx[vtx].v.cn[3])

#define PAUSE_ITEM_VTX_FLAG(vtx) (this->itemVtx[vtx].v.flag)

void Pause_InitEquipAnimVtx(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    this->itemVtx = Graph_Alloc(play->state.gfxCtx, sizeof(Vtx[4]));

    // Top left X, bottom left X
    PAUSE_ITEM_VTX_X(0) = PAUSE_ITEM_VTX_X(2) = 0;
    // Top right X, bottom right X
    PAUSE_ITEM_VTX_X(1) = PAUSE_ITEM_VTX_X(3) = 32;

    // Top left Y, top right Y
    PAUSE_ITEM_VTX_Y(0) = PAUSE_ITEM_VTX_Y(1) = 0;
    // Bottom left Y, bottom right Y
    PAUSE_ITEM_VTX_Y(2) = PAUSE_ITEM_VTX_Y(3) = 32;

    // Set depth
    PAUSE_ITEM_VTX_Z(0) = PAUSE_ITEM_VTX_Z(1) = PAUSE_ITEM_VTX_Z(2) = PAUSE_ITEM_VTX_Z(3) = 0;

    // Top left U, bottom left U
    PAUSE_ITEM_VTX_U(0) = PAUSE_ITEM_VTX_U(2) = 0;
    // Top right U, bottom right U
    PAUSE_ITEM_VTX_U(1) = PAUSE_ITEM_VTX_U(3) = 1 << 10;

    // Top left U, top right U
    PAUSE_ITEM_VTX_V(0) = PAUSE_ITEM_VTX_V(1) = 0;
    // Bottom left V, bottom right V
    PAUSE_ITEM_VTX_V(2) = PAUSE_ITEM_VTX_V(3) = 1 << 10;

    // Set vertex color, I think these aren't used but whatever
    PAUSE_ITEM_VTX_R(0) = PAUSE_ITEM_VTX_R(1) = PAUSE_ITEM_VTX_R(2) = PAUSE_ITEM_VTX_R(3) = 0;
    PAUSE_ITEM_VTX_G(0) = PAUSE_ITEM_VTX_G(1) = PAUSE_ITEM_VTX_G(2) = PAUSE_ITEM_VTX_G(3) = 0;
    PAUSE_ITEM_VTX_B(0) = PAUSE_ITEM_VTX_B(1) = PAUSE_ITEM_VTX_B(2) = PAUSE_ITEM_VTX_B(3) = 0;
    PAUSE_ITEM_VTX_A(0) = PAUSE_ITEM_VTX_A(1) = PAUSE_ITEM_VTX_A(2) = PAUSE_ITEM_VTX_A(3) = 255;

    // Zero out flag... whatever it is
    PAUSE_ITEM_VTX_FLAG(0) = PAUSE_ITEM_VTX_FLAG(1) = PAUSE_ITEM_VTX_FLAG(2) = PAUSE_ITEM_VTX_FLAG(3) = 0;
}

void Pause_SetView(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Vec3f eye;
    Vec3f lookAt;
    Vec3f up;

    lookAt.x = lookAt.y = lookAt.z = 0.0f;
    up.x = up.z = 0.0f;

    eye.x = 0.0f;
    eye.y = 0.0f;
    eye.z = 64.0f;

    up.y = 1.0f;

    View_LookAt(&this->view, &eye, &lookAt, &up);

    SET_FULLSCREEN_VIEWPORT(&this->view);
    View_Apply(&this->view, VIEW_ALL | VIEW_FORCE_VIEWING | VIEW_FORCE_VIEWPORT | VIEW_FORCE_PROJECTION_PERSPECTIVE);
}

static s16 sCButtonPosX[] = { 228, 250, 272 };
static s16 sCButtonPosY[] = { 17, 33, 17 };

void Pause_SetEquipAnimVtx(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    s16 x = LERP16(this->equipAnimX, sCButtonPosX[this->equipTargetCBtn], this->equipAnimTimer);
    s16 y = LERP16(this->equipAnimY, sCButtonPosY[this->equipTargetCBtn], this->equipAnimTimer);

    // Top left X, bottom left X
    PAUSE_ITEM_VTX_X(0) = PAUSE_ITEM_VTX_X(2) = x;
    // Top right X, bottom right X
    PAUSE_ITEM_VTX_X(1) = PAUSE_ITEM_VTX_X(3) = x + 32;

    // Top left Y, top right Y
    PAUSE_ITEM_VTX_Y(0) = PAUSE_ITEM_VTX_Y(1) = y;
    // Bottom left Y, bottom right Y
    PAUSE_ITEM_VTX_Y(2) = PAUSE_ITEM_VTX_Y(3) = y + 32;
}

void Pause_DrawEquipAnimItem(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPPipeSync(POLY_OPA_DISP++);

    Gfx_SetupDL_42Opa(play->state.gfxCtx);

    // what
    Matrix_Translate(-160.0f, 120.0f, -120.0f, MTXMODE_NEW);
    Matrix_Scale(1.0f, -1.0f, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->equipAnimAlpha);

    gSPVertex(POLY_OPA_DISP++, &this->itemVtx[0], 4, 0);

    if (sItemSlots[this->inventoryCursor.slotNum].item == ITEM_BOTTLE_EMPTY) {
        POLY_OPA_DISP = Pause_LoadTexRGBA32(POLY_OPA_DISP, sBottleItemTexs[INV_CONTENT(ITEM_BOTTLE_EMPTY) - ITEM_BOTTLE_EMPTY], 32, 32);
    } else {
        POLY_OPA_DISP = Pause_LoadTexRGBA32(POLY_OPA_DISP, sItemTexs[this->inventoryCursor.slotNum], 32, 32);
    }

    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

    Gfx_SetupDL_39Opa(play->state.gfxCtx);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_EquipAnim(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    Pause_InitEquipAnimVtx(play);
    Pause_SetEquipAnimVtx(play);
    this->equipAnimTimer += 0.1f;
    if (this->equipAnimTimer >= 1.0f) {
        this->equipAnimTimer = 1.0f;
    }
    this->equipAnimAlpha = 255;
}

void Pause_Inventory(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Input* input = &play->state.input[0];

    switch (this->equipAnimState) {
        case PAUSE_EQUIP_ANIM_STATE_ANIMATE:
            // Animate the item moving to the C buttons
            // Item move animation is done
            Pause_EquipAnim(play);
            if (this->equipAnimTimer == 1.0f) {
                this->equipAnimState = PAUSE_EQUIP_ANIM_STATE_DONE;
            }
            break;
        
        case PAUSE_EQUIP_ANIM_STATE_DONE:
            // Actually equip the item on the C buttons
            Pause_EquipItem(play);
            // Finish equipping and re-enable input
            this->inputDisabled = false;
            this->equipAnimState = PAUSE_EQUIP_ANIM_STATE_OFF;
            break;
    }

    if (this->inventoryCursor.row == PAUSE_ROW_EQUIPMENT) {
        // Decrement equipTimer, if 0 then equipment finished equipping
        if (DECR(this->equipTimer) == 0) {
            this->inputDisabled = false;
        }
    }

    Pause_SetEquippedItems(play);

    if (this->inputDisabled) {
        return;
    }

    // Check input for equipping items or equipment
    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            if (PAUSE_HAS_ITEM(sItemSlots[this->inventoryCursor.slotNum].item)) {
                if (CHECK_BTN_ANY(input->press.button, BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT)) {
                    Pause_SetEquipTargetCBtn(play);
                    Pause_StartEquipItem(play);
                }
            }
            break;

        case PAUSE_ROW_EQUIPMENT:
            if (Pause_HasEquipment(sEquipmentSlots[this->inventoryCursor.slotNum].item)) {
                if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                    this->equipTimer = 20;
                    this->inputDisabled = true;
                    Pause_EquipItem(play);
                    Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
                }
            }
            break;
    }
}

void Pause_SavePrompt(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

}

void Pause_FinishGameover(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    // Stop gameplay and reload
    this->state = PAUSED_STATE_OFF;
    R_UPDATE_RATE = 3;
    R_PAUSE_BG_PRERENDER_STATE = PAUSE_BG_PRERENDER_OFF;
    func_800981B8(&play->objectCtx);
    func_800418D0(&play->colCtx, play);
    switch (this->promptChoice) {
        case PAUSE_GAMEOVER_PROMPT_CONTINUE:
            Play_TriggerRespawn(play);
            gSaveContext.respawnFlag = -2;
            gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.save.info.playerData.health = 6 * 16;
            SEQCMD_RESET_AUDIO_HEAP(0, 10);
            gSaveContext.healthAccumulator = 0;
            gSaveContext.magicState = MAGIC_STATE_IDLE;
            gSaveContext.prevMagicState = MAGIC_STATE_IDLE;
            gSaveContext.magicCapacity = 0;
            // Set the fill target to be the magic amount before game over
            gSaveContext.magicFillTarget = gSaveContext.save.info.playerData.magic;
            // Set `magicLevel` and `magic` to 0 so `magicCapacity` then `magic` grows from nothing
            // to respectively the full capacity and `magicFillTarget`
            gSaveContext.save.info.playerData.magicLevel = gSaveContext.save.info.playerData.magic = 0;
            break;

        case PAUSE_GAMEOVER_PROMPT_QUIT:
            play->state.running = false;
            SET_NEXT_GAMESTATE(&play->state, TitleSetup_Init, TitleSetupState);
            break;
    }
}

void Pause_Close(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    u8 gameOver = (play->gameOverCtx.state != GAMEOVER_INACTIVE);

    if (!gameOver) {
        this->alpha = CLAMP_MIN(this->alpha - PAUSE_ALPHA_FADE_RATE, 0);
        interfaceCtx->startAlpha = this->alpha;
    } else {
        this->alpha = CLAMP_MIN(this->alpha - (PAUSE_ALPHA_FADE_RATE / 2), 0);
    }
    this->foregroundAlpha = this->alpha;

    if (DECR(this->alpha) == 0) {
        if (gameOver) {
            // Stop gameplay and reload
            Pause_FinishGameover(play);
        } else {
            // Set slot num so next time player saves, their cursor starts on "Yes"
            this->saveCursor.slotNum = 0;
            this->state = PAUSED_STATE_RESUME_GAMEPLAY;
        }
    }
}

void Pause_ResumeGameplay(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    Player* player = GET_PLAYER(play);

    // Restore object and collision spaces
    func_800981B8(&play->objectCtx);
    func_800418D0(&play->colCtx, play);

    // Restore button status and HUD visibility
    gSaveContext.buttonStatus[0] = sPrevButtonStatus[0];
    gSaveContext.buttonStatus[1] = sPrevButtonStatus[1];
    gSaveContext.buttonStatus[2] = sPrevButtonStatus[2];
    gSaveContext.buttonStatus[3] = sPrevButtonStatus[3];
    gSaveContext.buttonStatus[4] = sPrevButtonStatus[4];
    gSaveContext.hudVisibilityMode = 0;
    Interface_ChangeHudVisibilityMode(gSaveContext.prevHudVisibilityMode);

    // Restore B button icon
    play->interfaceCtx.unk_1FC = 0;
    play->interfaceCtx.doActionOnB = 0;

    player->talkActor = NULL;
    Player_SetEquipmentData(play, player);

    R_UPDATE_RATE = 3;
    R_PAUSE_BG_PRERENDER_STATE = PAUSE_BG_PRERENDER_OFF;
    this->state = PAUSED_STATE_OFF;
}

#define PAUSE_ITEM_HEADER_X 112
#define PAUSE_ITEM_HEADER_Y 64

#define PAUSE_EQUIPMENT_HEADER_X 112
#define PAUSE_EQUIPMENT_HEADER_Y 122

void Pause_DrawInventoryHeaders(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, prim.r, prim.g, prim.b, this->foregroundAlpha);
    gDPSetEnvColor(POLY_OPA_DISP++, env.r, env.g, env.b, 0);

    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseItemsHeader, 96, 24);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_ITEM_HEADER_X, PAUSE_ITEM_HEADER_Y, 96, 24);

    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseEquipmentHeader, 96, 24);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_EQUIPMENT_HEADER_X, PAUSE_EQUIPMENT_HEADER_Y, 96, 24);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

#define PAUSE_ITEM_NAME_BG_X 96
#define PAUSE_ITEM_NAME_BG_Y 196

void Pause_DrawItemNameBackground(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, prim.r, prim.g, prim.b, this->foregroundAlpha);
    gDPSetEnvColor(POLY_OPA_DISP++, env.r, env.g, env.b, 0);

    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseItemNameBgTex, 128, 24);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_ITEM_NAME_BG_X, PAUSE_ITEM_NAME_BG_Y, 128, 24);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_DrawItemName(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    void* texture = NULL;

    switch (this->inventoryCursor.row) {
        case PAUSE_ROW_ITEMS:
            if (!(PAUSE_HAS_ITEM(sItemSlots[this->inventoryCursor.slotNum].item))) {
                return;
            }
            if (sItemSlots[this->inventoryCursor.slotNum].item == ITEM_BOTTLE_EMPTY) {
                texture = sBottleItemNameTexs[INV_CONTENT(ITEM_BOTTLE_EMPTY) - ITEM_BOTTLE_EMPTY];
            } else {
                texture = sItemNameTexs[this->inventoryCursor.slotNum];
            }
            break;
        
        case PAUSE_ROW_EQUIPMENT:
            if (!(Pause_HasEquipment(sEquipmentSlots[this->inventoryCursor.slotNum].item))) {
                return;
            }
            texture = sEquipmentNameTexs[this->inventoryCursor.slotNum];
            break;
    }

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 21, 18, 25, 0);

    if (texture != NULL) {
        POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, texture, 128, 16);
        POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_ITEM_NAME_BG_X, PAUSE_ITEM_NAME_BG_Y + 3, 128, 16);
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

s32 Pause_HasEquipment(ItemID item) {
    switch (item) {
        case ITEM_TUNIC_GORON:
            return gSaveContext.save.info.inventory.equipment & OWNED_EQUIP_FLAG(EQUIP_TYPE_TUNIC, ITEM_TUNIC_GORON - ITEM_TUNIC_KOKIRI);

        case ITEM_BOOTS_IRON:
            return gSaveContext.save.info.inventory.equipment & OWNED_EQUIP_FLAG(EQUIP_TYPE_BOOTS, ITEM_BOOTS_IRON - ITEM_BOOTS_KOKIRI);
        
        case ITEM_SHIELD_MIRROR:
            return gSaveContext.save.info.inventory.equipment & OWNED_EQUIP_FLAG(EQUIP_TYPE_SHIELD, ITEM_SHIELD_MIRROR - ITEM_SHIELD_DEKU);
        
        default:
            break;
    }
    return false;
}

void Pause_DrawAmmoCount(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    u8 slotNum;

    for (slotNum = 0; slotNum < ARRAY_COUNT(sItemSlots); slotNum++) {
        s16 item = sItemSlots[slotNum].item;
        s16 x = sItemSlots[slotNum].pos.x;
        s16 y = sItemSlots[slotNum].pos.y;
        s16 ammo = AMMO(item);

        if (item != ITEM_BOMB) {
            continue;
        } else if (item == ITEM_BOMB && !(PAUSE_HAS_ITEM(ITEM_BOMB))) {
            continue;
        }

        OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

        gDPPipeSync(POLY_OPA_DISP++);

        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);

        if (ammo == 0) {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 130, 130, 130, this->foregroundAlpha);
        } else if ((item == ITEM_BOMB && ammo == CUR_CAPACITY(UPG_BOMB_BAG))) {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 120, 255, 0, this->foregroundAlpha);
        }

        // Draw the 1s place digit
        POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, ((u8*)gAmmoDigit0Tex + ((8 * 8) * (ammo % 10))), 8, 8);
        POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x + 6, y + 24, 8, 8);

        // Draw the 10s place digit if needed
        if (ammo >= 10) {
            // Remove the last digit
            ammo = ammo / 10;
            POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, ((u8*)gAmmoDigit0Tex + ((8 * 8) * ammo)), 8, 8);
            POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x, y + 24, 8, 8);
        }

        CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
    }
}

void Pause_DrawSlotIcon(PlayState* play, u8 slotNum, u8 row) {
    PauseContext* this = &play->pauseCtx;
    void* texture = NULL;
    u8 iconSize;
    u8 x = sItemSlots[0].pos.x;
    u8 y = sItemSlots[0].pos.y;
    u8 isEquipped = false;

    switch (row) {
        case PAUSE_ROW_ITEMS:
            if (sItemSlots[slotNum].item == ITEM_BOTTLE_EMPTY) {
                texture = PAUSE_HAS_BOTTLE_ITEM(sItemSlots[slotNum].item) ? sBottleItemTexs[INV_CONTENT(ITEM_BOTTLE_EMPTY) - ITEM_BOTTLE_EMPTY] : gNewPauseItemBg;
            } else {
                texture = PAUSE_HAS_ITEM(sItemSlots[slotNum].item) ? sItemTexs[slotNum] : gNewPauseItemBg;
            }
            isEquipped = sItemSlots[slotNum].isEquipped;
            x = sItemSlots[slotNum].pos.x;
            y = sItemSlots[slotNum].pos.y;
            break;
        
        case PAUSE_ROW_EQUIPMENT:
            texture = Pause_HasEquipment(sEquipmentSlots[slotNum].item) ? sEquipmentTexs[slotNum] : gNewPauseItemBg;
            isEquipped = sEquipmentSlots[slotNum].isEquipped;
            x = sEquipmentSlots[slotNum].pos.x;
            y = sEquipmentSlots[slotNum].pos.y;
            break;
    }

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // Draw equipped item border
    if (isEquipped) {
        POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gEquippedItemOutlineTex, 32, 32);
        POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x - 2, y - 2, 32, 32);
    }

    // Enlargen icon if selected by inventoryCursor
    if (this->inventoryCursor.row == row && this->inventoryCursor.slotNum == slotNum &&
        (this->equipAnimState != PAUSE_EQUIP_ANIM_STATE_ANIMATE) && (this->equipTimer == 0.0f)) {
        iconSize = 32;
        x -= 2;
        y -= 2;
    } else {
        iconSize = 28;
    }

    if (texture != NULL) {
        if (texture == gNewPauseItemBg) {
            gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                              PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, prim.r, prim.g, prim.b, this->foregroundAlpha);
            gDPSetEnvColor(POLY_OPA_DISP++, env.r, env.g, env.b, 0);
            POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, texture, 32, 32);
        } else {
            POLY_OPA_DISP = Pause_LoadTexRGBA32(POLY_OPA_DISP, texture, 32, 32);
        }
        POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, x, y, iconSize, iconSize);
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_DrawInventory(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    u8 slotNum;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    Pause_DrawInventoryHeaders(play);
    Pause_DrawItemNameBackground(play);
    Pause_DrawItemName(play);

    // Draw items
    for (slotNum = 0; slotNum < ARRAY_COUNT(sItemSlots); slotNum++) {
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 0);
        Pause_DrawSlotIcon(play, slotNum, PAUSE_ROW_ITEMS);
    }

    // Draw equipment
    for (slotNum = 0; slotNum < ARRAY_COUNT(sEquipmentSlots); slotNum++) {
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 0);
        Pause_DrawSlotIcon(play, slotNum, PAUSE_ROW_EQUIPMENT);
    }

    if (this->equipAnimState == PAUSE_EQUIP_ANIM_STATE_ANIMATE) {
        Pause_DrawEquipAnimItem(play);
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

#define PAUSE_SAVE_PROMPT_X 96
#define PAUSE_SAVE_PROMPT_Y 83

void Pause_DrawSavePrompt(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // Save prompt
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseSavePrompt, 128, 24);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_SAVE_PROMPT_X, PAUSE_SAVE_PROMPT_Y, 128, 24);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0, 100, 255, this->foregroundAlpha);

    // Prompt cursor
    POLY_OPA_DISP = Pause_LoadTexI4(POLY_OPA_DISP, &gNewPausePromptCursorTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[this->saveCursor.slotNum].pos.x, sSaveSlots[this->saveCursor.slotNum].pos.y, 48, 48);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);

    // "Yes"
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseYesTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[0].pos.x, sSaveSlots[0].pos.y, 48, 48);

    // "No"
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseNoTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[1].pos.x, sSaveSlots[1].pos.y, 48, 48);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_DrawContinuePrompt(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // Continue prompt
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gPauseContinuePrompt, 128, 24);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_SAVE_PROMPT_X, PAUSE_SAVE_PROMPT_Y, 128, 24);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0, 100, 255, this->foregroundAlpha);

    // Prompt cursor
    POLY_OPA_DISP = Pause_LoadTexI4(POLY_OPA_DISP, &gNewPausePromptCursorTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[this->saveCursor.slotNum].pos.x, sSaveSlots[this->saveCursor.slotNum].pos.y, 48, 48);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->foregroundAlpha);

    // "Yes"
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseYesTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[0].pos.x, sSaveSlots[0].pos.y, 48, 48);

    // "No"
    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, &gNewPauseNoTex, 48, 48);
    POLY_OPA_DISP = Pause_DrawTexRect(POLY_OPA_DISP, sSaveSlots[1].pos.x, sSaveSlots[1].pos.y, 48, 48);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_DrawGameOver(PlayState* play) {
    PauseContext* this = &play->pauseCtx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    switch (this->state) {
        case PAUSED_STATE_GAMEOVER_SAVE_PROMPT:
            Pause_DrawSavePrompt(play);
            break;
        
        case PAUSED_STATE_GAMEOVER_CONTINUE_PROMPT:
            Pause_DrawContinuePrompt(play);
            break;
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

#define PAUSE_BG_TILE_WIDTH 80
#define PAUSE_BG_TILE_HEIGHT 32
#define PAUSE_BG_START_X 40
#define PAUSE_BG_START_Y 40

void Pause_DrawBackgroundTileRow(PlayState* play, u8 row, u8 tile) {
    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    POLY_OPA_DISP = Pause_LoadTexIA8(POLY_OPA_DISP, sPauseBgTextures[row][tile], PAUSE_BG_TILE_WIDTH, PAUSE_BG_TILE_HEIGHT);
    POLY_OPA_DISP =
        Pause_DrawTexRect(POLY_OPA_DISP, PAUSE_BG_START_X + (tile * PAUSE_BG_TILE_WIDTH),
                          PAUSE_BG_START_Y + (row * PAUSE_BG_TILE_HEIGHT), PAUSE_BG_TILE_WIDTH, PAUSE_BG_TILE_HEIGHT);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_DrawBackground(PlayState* play) {
    PauseContext* this = &play->pauseCtx;
    u8 row;
    u8 tile;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

    for (row = 0; row < ARRAY_COUNT(sPauseBgTextures); row++) {
        for (tile = 0; tile < ARRAY_COUNT(sPauseBgTextures[row]); tile++) {
            Pause_DrawBackgroundTileRow(play, row, tile);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}

void Pause_Draw(PlayState* play) {
    Input* input = &play->state.input[0];
    PauseContext* this = &play->pauseCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (this->alpha == 0) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // View setup
    Pause_SetView(play);

    // Segment setup
    gSPSegment(POLY_OPA_DISP++, 0x02, interfaceCtx->parameterSegment);
    gSPSegment(POLY_OPA_DISP++, 0x07, this->playerSegment);
    gSPSegment(POLY_OPA_DISP++, 0x09, this->pauseMenuSegment);
    gSPSegment(POLY_OPA_DISP++, 0x08, this->iconItemSegment);

    // Material setup
    Gfx_SetupDL_39Opa(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, prim.r, prim.g, prim.b, this->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, env.r, env.g, env.b, 0);

    // Draw background first
    Pause_DrawBackground(play);

    // Draw foreground next
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, prim.r, prim.g, prim.b, this->foregroundAlpha);

    if (play->gameOverCtx.state == GAMEOVER_INACTIVE) {
        if (this->debugState == 0) {
            switch (this->state) {
                case PAUSED_STATE_OPENING:
                case PAUSED_STATE_INVENTORY:
                case PAUSED_STATE_FADE_OUT_INVENTORY:
                    Pause_DrawInventory(play);
                    Pause_DrawCursor(play);
                    Pause_DrawAmmoCount(play);
                    break;
                
                case PAUSED_STATE_FADE_IN_SAVE_PROMPT:
                case PAUSED_STATE_SAVE_PROMPT:
                    Pause_DrawSavePrompt(play);
                    break;

                default:
                    break;
            }
        }
    } else {
        // Gameover screen active
        switch (this->state) {
            case PAUSED_STATE_OPENING:
            case PAUSED_STATE_GAMEOVER_SAVE_PROMPT:
            case PAUSED_STATE_GAMEOVER_CONTINUE_PROMPT:
                Pause_DrawGameOver(play);
        }
    }


    if ((this->debugState == 1) || (this->debugState == 2)) {
        // KaleidoScope_DrawDebugEditor(play);
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}