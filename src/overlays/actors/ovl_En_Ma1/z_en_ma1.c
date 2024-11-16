/*
 * File: z_en_ma1.c
 * Overlay: En_Ma1
 * Description: Child Malon
 */

#include "z_en_ma1.h"
#include "assets/objects/object_ma1/object_ma1.h"

#define FLAGS (ACTOR_FLAG_0 | ACTOR_FLAG_3 | ACTOR_FLAG_4 | ACTOR_FLAG_5 | ACTOR_FLAG_25)

void EnMa1_Init(Actor* thisx, PlayState* play);
void EnMa1_Destroy(Actor* thisx, PlayState* play);
void EnMa1_Update(Actor* thisx, PlayState* play);
void EnMa1_Draw(Actor* thisx, PlayState* play);

void func_80AA0D88(EnMa1* this, PlayState* play);
void func_80AA0EA0(EnMa1* this, PlayState* play);
void func_80AA0EFC(EnMa1* this, PlayState* play);
void func_80AA0F44(EnMa1* this, PlayState* play);
void func_80AA106C(EnMa1* this, PlayState* play);
void func_80AA10EC(EnMa1* this, PlayState* play);
void func_80AA1150(EnMa1* this, PlayState* play);
void EnMa1_DoNothing(EnMa1* this, PlayState* play);

ActorInit En_Ma1_InitVars = {
    /**/ ACTOR_EN_MA1,
    /**/ ACTORCAT_NPC,
    /**/ FLAGS,
    /**/ OBJECT_MA1,
    /**/ sizeof(EnMa1),
    /**/ EnMa1_Init,
    /**/ EnMa1_Destroy,
    /**/ EnMa1_Update,
    /**/ EnMa1_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_NONE,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x00000000, 0x00, 0x00 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 18, 46, 0, { 0, 0, 0 } },
};

static CollisionCheckInfoInit2 sColChkInfoInit = { 0, 0, 0, 0, MASS_IMMOVABLE };

typedef enum {
    /* 0 */ ENMA1_ANIM_0,
    /* 1 */ ENMA1_ANIM_1,
    /* 2 */ ENMA1_ANIM_2,
    /* 3 */ ENMA1_ANIM_3
} EnMa1Animation;

static AnimationFrameCountInfo sAnimationInfo[] = {
    { &gMalonChildIdleAnim, 1.0f, ANIMMODE_LOOP, 0.0f },
    { &gMalonChildIdleAnim, 1.0f, ANIMMODE_LOOP, -10.0f },
    { &gMalonChildSingAnim, 1.0f, ANIMMODE_LOOP, 0.0f },
    { &gMalonChildSingAnim, 1.0f, ANIMMODE_LOOP, -10.0f },
};

static Vec3f D_80AA16B8 = { 800.0f, 0.0f, 0.0f };

static void* sMouthTextures[] = {
    gMalonChildNeutralMouthTex,
    gMalonChildSmilingMouthTex,
    gMalonChildTalkingMouthTex,
};

static void* sEyeTextures[] = {
    gMalonChildEyeOpenTex,
    gMalonChildEyeHalfTex,
    gMalonChildEyeClosedTex,
};

u16 EnMa1_GetTextId(PlayState* play, Actor* thisx) {
    u16 faceReaction = Text_GetFaceReaction(play, 0x17);

    if (faceReaction != 0) {
        return faceReaction;
    }
    if (CHECK_QUEST_ITEM(QUEST_SONG_EPONA)) {
        return 0x204A;
    }
    if (GET_EVENTCHKINF(EVENTCHKINF_16)) {
        return 0x2049;
    }
    if (GET_EVENTCHKINF(EVENTCHKINF_15)) {
        if (GET_INFTABLE(INFTABLE_85)) {
            return 0x2049;
        } else {
            return 0x2048;
        }
    }
    if (GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE)) {
        return 0x2047;
    }
    if (GET_EVENTCHKINF(EVENTCHKINF_12)) {
        return 0x2044;
    }
    if (GET_INFTABLE(INFTABLE_84)) {
        if (GET_INFTABLE(INFTABLE_8B)) {
            return 0x2043;
        } else {
            return 0x2042;
        }
    }
    return 0x2041;
}

s16 EnMa1_UpdateTalkState(PlayState* play, Actor* thisx) {
    s16 talkState = NPC_TALK_STATE_TALKING;

    switch (Message_GetState(&play->msgCtx)) {
        case TEXT_STATE_CLOSING:
            switch (thisx->textId) {
                case 0x2041:
                    SET_INFTABLE(INFTABLE_84);
                    SET_EVENTCHKINF(EVENTCHKINF_10);
                    talkState = NPC_TALK_STATE_IDLE;
                    break;
                case 0x2043:
                    talkState = NPC_TALK_STATE_TALKING;
                    break;
                case 0x2047:
                    SET_EVENTCHKINF(EVENTCHKINF_15);
                    talkState = NPC_TALK_STATE_IDLE;
                    break;
                case 0x2048:
                    SET_INFTABLE(INFTABLE_85);
                    talkState = NPC_TALK_STATE_IDLE;
                    break;
                case 0x2049:
                    SET_EVENTCHKINF(EVENTCHKINF_16);
                    talkState = NPC_TALK_STATE_IDLE;
                    break;
                case 0x2061:
                    talkState = NPC_TALK_STATE_ACTION;
                    break;
                default:
                    talkState = NPC_TALK_STATE_IDLE;
                    break;
            }
            break;
        case TEXT_STATE_CHOICE:
        case TEXT_STATE_EVENT:
            if (Message_ShouldAdvance(play)) {
                talkState = NPC_TALK_STATE_ACTION;
            }
            break;
        case TEXT_STATE_DONE:
            if (Message_ShouldAdvance(play)) {
                talkState = NPC_TALK_STATE_ITEM_GIVEN;
            }
            break;
        case TEXT_STATE_NONE:
        case TEXT_STATE_DONE_HAS_NEXT:
        case TEXT_STATE_DONE_FADING:
        case TEXT_STATE_SONG_DEMO_DONE:
        case TEXT_STATE_8:
        case TEXT_STATE_9:
            talkState = NPC_TALK_STATE_TALKING;
            break;
    }
    return talkState;
}

s32 func_80AA08C4(EnMa1* this, PlayState* play) {
    if ((this->actor.shape.rot.z == 3) && (gSaveContext.sceneLayer == 5)) {
        return 1;
    }
    if (!LINK_IS_CHILD) {
        return 0;
    }
    if (((play->sceneId == SCENE_MARKET_NIGHT) || (play->sceneId == SCENE_MARKET_DAY)) &&
        !GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE) && !GET_INFTABLE(INFTABLE_8B)) {
        return 1;
    }
    if ((play->sceneId == SCENE_HYRULE_CASTLE) && !GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE)) {
        if (GET_INFTABLE(INFTABLE_8B)) {
            return 1;
        } else {
            SET_INFTABLE(INFTABLE_8B);
            return 0;
        }
    }
    if ((play->sceneId == SCENE_LON_LON_BUILDINGS) && IS_NIGHT &&
        GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE)) {
        return 1;
    }
    if (play->sceneId != SCENE_LON_LON_RANCH) {
        return 0;
    }
    if ((this->actor.shape.rot.z == 3) && IS_DAY && GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE)) {
        return 1;
    }
    return 0;
}

void EnMa1_UpdateEyes(EnMa1* this) {
    if (DECR(this->blinkTimer) == 0) {
        this->eyeIndex++;
        if (this->eyeIndex >= 3) {
            this->blinkTimer = Rand_S16Offset(30, 30);
            this->eyeIndex = 0;
        }
    }
}

void EnMa1_ChangeAnim(EnMa1* this, s32 index) {
    f32 frameCount = Animation_GetLastFrame(sAnimationInfo[index].animation);

    Animation_Change(&this->skelAnime, sAnimationInfo[index].animation, 1.0f, 0.0f, frameCount,
                     sAnimationInfo[index].mode, sAnimationInfo[index].morphFrames);
}

void func_80AA0AF4(EnMa1* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s16 trackingMode;

    if ((this->interactInfo.talkState == NPC_TALK_STATE_IDLE) && (this->skelAnime.animation == &gMalonChildSingAnim)) {
        trackingMode = NPC_TRACKING_NONE;
    } else {
        trackingMode = NPC_TRACKING_PLAYER_AUTO_TURN;
    }

    this->interactInfo.trackPos = player->actor.world.pos;
    this->interactInfo.trackPos.y -= -10.0f;

    Npc_TrackPoint(&this->actor, &this->interactInfo, 0, trackingMode);
}

void func_80AA0B74(EnMa1* this) {
    if (this->skelAnime.animation == &gMalonChildSingAnim) {
        if (this->interactInfo.talkState == NPC_TALK_STATE_IDLE) {
            if (this->isNotSinging) {
                // Turn on singing
                this->isNotSinging = false;
                Audio_ToggleMalonSinging(false);
            }
        } else {
            if (!this->isNotSinging) {
                // Turn off singing
                this->isNotSinging = true;
                Audio_ToggleMalonSinging(true);
            }
        }
    }
}

void EnMa1_Init(Actor* thisx, PlayState* play) {
    EnMa1* this = (EnMa1*)thisx;
    s32 pad;

    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 18.0f);
    SkelAnime_InitFlex(play, &this->skelAnime, &gMalonChildSkel, NULL, NULL, NULL, 0);
    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    CollisionCheck_SetInfo2(&this->actor.colChkInfo, DamageTable_Get(22), &sColChkInfoInit);

    if (!func_80AA08C4(this, play)) {
        Actor_Kill(&this->actor);
        return;
    }

    Actor_UpdateBgCheckInfo(play, &this->actor, 0.0f, 0.0f, 0.0f, UPDBGCHECKINFO_FLAG_2);
    Actor_SetScale(&this->actor, 0.01f);
    this->actor.targetMode = 6;
    this->interactInfo.talkState = NPC_TALK_STATE_IDLE;

    if (!GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE) || CHECK_QUEST_ITEM(QUEST_SONG_EPONA)) {
        this->actionFunc = func_80AA0D88;
        EnMa1_ChangeAnim(this, ENMA1_ANIM_2);
    } else {
        this->actionFunc = func_80AA0F44;
        EnMa1_ChangeAnim(this, ENMA1_ANIM_2);
    }
}

void EnMa1_Destroy(Actor* thisx, PlayState* play) {
    EnMa1* this = (EnMa1*)thisx;

    SkelAnime_Free(&this->skelAnime, play);
    Collider_DestroyCylinder(play, &this->collider);
}

void func_80AA0D88(EnMa1* this, PlayState* play) {
    if (this->interactInfo.talkState != NPC_TALK_STATE_IDLE) {
        if (this->skelAnime.animation != &gMalonChildIdleAnim) {
            EnMa1_ChangeAnim(this, ENMA1_ANIM_1);
        }
    } else {
        if (this->skelAnime.animation != &gMalonChildSingAnim) {
            EnMa1_ChangeAnim(this, ENMA1_ANIM_3);
        }
    }

    if ((play->sceneId == SCENE_HYRULE_CASTLE) && GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE)) {
        Actor_Kill(&this->actor);
    } else if (!GET_EVENTCHKINF(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE) || CHECK_QUEST_ITEM(QUEST_SONG_EPONA)) {
        if (this->interactInfo.talkState == NPC_TALK_STATE_ACTION) {
            this->actionFunc = func_80AA0EA0;
            play->msgCtx.stateTimer = 4;
            play->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
        }
    }
}

void func_80AA0EA0(EnMa1* this, PlayState* play) {
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = func_80AA0EFC;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_WEIRD_EGG, 120.0f, 10.0f);
    }
}

void func_80AA0EFC(EnMa1* this, PlayState* play) {
    if (this->interactInfo.talkState == NPC_TALK_STATE_ITEM_GIVEN) {
        this->interactInfo.talkState = NPC_TALK_STATE_IDLE;
        this->actionFunc = func_80AA0D88;
        SET_EVENTCHKINF(EVENTCHKINF_12);
        play->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
    }
}

void func_80AA0F44(EnMa1* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->interactInfo.talkState != NPC_TALK_STATE_IDLE) {
        if (this->skelAnime.animation != &gMalonChildIdleAnim) {
            EnMa1_ChangeAnim(this, ENMA1_ANIM_1);
        }
    } else {
        if (this->skelAnime.animation != &gMalonChildSingAnim) {
            EnMa1_ChangeAnim(this, ENMA1_ANIM_3);
        }
    }

    if (GET_EVENTCHKINF(EVENTCHKINF_16)) {
        if (player->stateFlags2 & PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR) {
            player->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR;
            player->ocarinaActor = &this->actor;
            this->actor.textId = 0x2061;
            Message_StartTextbox(play, this->actor.textId, NULL);
            this->interactInfo.talkState = NPC_TALK_STATE_TALKING;
            this->actor.flags |= ACTOR_FLAG_16;
            this->actionFunc = func_80AA106C;
        } else if (this->actor.xzDistToPlayer < 30.0f + (f32)this->collider.dim.radius) {
            player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
        }
    }
}

void func_80AA106C(EnMa1* this, PlayState* play) {
    GET_PLAYER(play)->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
    if (this->interactInfo.talkState == NPC_TALK_STATE_ACTION) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_MALON);
        Message_StartOcarina(play, OCARINA_ACTION_TEACH_EPONA);
        this->actor.flags &= ~ACTOR_FLAG_16;
        this->actionFunc = func_80AA10EC;
    }
}

void func_80AA10EC(EnMa1* this, PlayState* play) {
    GET_PLAYER(play)->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
    if (Message_GetState(&play->msgCtx) == TEXT_STATE_SONG_DEMO_DONE) {
        Message_StartOcarina(play, OCARINA_ACTION_PLAYBACK_EPONA);
        this->actionFunc = func_80AA1150;
    }
}

void func_80AA1150(EnMa1* this, PlayState* play) {
    GET_PLAYER(play)->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
    if (play->msgCtx.ocarinaMode == OCARINA_MODE_03) {
        play->nextEntranceIndex = ENTR_LON_LON_RANCH_0;
        gSaveContext.nextCutsceneIndex = 0xFFF1;
        play->transitionType = TRANS_TYPE_CIRCLE(TCA_WAVE, TCC_WHITE, TCS_FAST);
        play->transitionTrigger = TRANS_TRIGGER_START;
        this->actionFunc = EnMa1_DoNothing;
    }
}

void EnMa1_DoNothing(EnMa1* this, PlayState* play) {
}

void EnMa1_Update(Actor* thisx, PlayState* play) {
    EnMa1* this = (EnMa1*)thisx;
    s32 pad;

    Collider_UpdateCylinder(&this->actor, &this->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    SkelAnime_Update(&this->skelAnime);
    EnMa1_UpdateEyes(this);
    this->actionFunc(this, play);
    if (this->actionFunc != EnMa1_DoNothing) {
        Npc_UpdateTalking(play, &this->actor, &this->interactInfo.talkState, (f32)this->collider.dim.radius + 30.0f,
                          EnMa1_GetTextId, EnMa1_UpdateTalkState);
    }
    func_80AA0B74(this);
    func_80AA0AF4(this, play);
}

s32 EnMa1_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    EnMa1* this = (EnMa1*)thisx;
    Vec3s limbRot;

    if ((limbIndex == 2) || (limbIndex == 5)) {
        *dList = NULL;
    }
    if (limbIndex == 15) {
        Matrix_Translate(1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        limbRot = this->interactInfo.headRot;
        Matrix_RotateX(BINANG_TO_RAD_ALT(limbRot.y), MTXMODE_APPLY);
        Matrix_RotateZ(BINANG_TO_RAD_ALT(limbRot.x), MTXMODE_APPLY);
        Matrix_Translate(-1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
    }
    if (limbIndex == 8) {
        limbRot = this->interactInfo.torsoRot;
        Matrix_RotateX(BINANG_TO_RAD_ALT(-limbRot.y), MTXMODE_APPLY);
        Matrix_RotateZ(BINANG_TO_RAD_ALT(-limbRot.x), MTXMODE_APPLY);
    }
    return false;
}

void EnMa1_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    EnMa1* this = (EnMa1*)thisx;
    Vec3f vec = D_80AA16B8;

    if (limbIndex == 15) {
        Matrix_MultVec3f(&vec, &this->actor.focus.pos);
    }
}

void EnMa1_Draw(Actor* thisx, PlayState* play) {
    EnMa1* this = (EnMa1*)thisx;
    Camera* activeCam;
    f32 distFromCamEye;
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx, "../z_en_ma1.c", 1226);

    activeCam = GET_ACTIVE_CAM(play);
    distFromCamEye = Math_Vec3f_DistXZ(&this->actor.world.pos, &activeCam->eye);
    Audio_UpdateMalonSinging(distFromCamEye, NA_BGM_LONLON);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x09, SEGMENTED_TO_VIRTUAL(sMouthTextures[this->mouthIndex]));
    gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sEyeTextures[this->eyeIndex]));

    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                          EnMa1_OverrideLimbDraw, EnMa1_PostLimbDraw, this);

    CLOSE_DISPS(play->state.gfxCtx, "../z_en_ma1.c", 1261);
}