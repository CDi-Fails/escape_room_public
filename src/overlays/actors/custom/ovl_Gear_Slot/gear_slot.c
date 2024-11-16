#include "gear_slot.h"
#include "assets_custom/objects/object_gear/object_gear.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void GearSlot_Init(Actor* thisx, PlayState* play);
void GearSlot_Destroy(Actor* thisx, PlayState* play);
void GearSlot_Update(Actor* thisx, PlayState* play);
void GearSlot_Draw(Actor* thisx, PlayState* play);

void GearSlot_IdleFull(GearSlot* this, PlayState* play);
void GearSlot_IdleEmpty(GearSlot* this, PlayState* play);

const ActorInit Gear_Slot_InitVars = {
    ACTOR_GEAR_SLOT,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GEAR,
    sizeof(GearSlot),
    GearSlot_Init,
    GearSlot_Destroy,
    GearSlot_Update,
    GearSlot_Draw,
};

#define GEAR_SLOT_GET_SWITCH(params) (params & 0x3F)
#define GEAR_SLOT_GET_TYPE(params) (params >> 8 & 0xF)
#define GEAR_SLOT_GET_LEADER(params) (params >> 12 & 0xF)

#define GEAR_SLOT_NUM 2

static u8 sNumSlotsFilled = 0;

void GearSlot_InitDyna(GearSlot* this, PlayState* play) {
    CollisionHeader* colHeader = NULL;

    DynaPolyActor_Init(&this->dyna, 0);
    CollisionHeader_GetVirtual(&gGearSlotCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
}

void GearSlot_Init(Actor* thisx, PlayState* play) {
    GearSlot* this = (GearSlot*)thisx;

    this->isLeader = GEAR_SLOT_GET_LEADER(this->dyna.actor.params);
    this->type = GEAR_SLOT_GET_TYPE(this->dyna.actor.params);

    if (Flags_GetSwitch(play, GEAR_SLOT_GET_SWITCH(this->dyna.actor.params))) {
        sNumSlotsFilled = GEAR_SLOT_NUM;
        this->type = GEAR_SLOT_TYPE_PREFILLED;
    }

    switch (this->type) {
        case GEAR_SLOT_TYPE_NEEDS_FILL:
            this->actionFunc = GearSlot_IdleEmpty;
            break;
        
        case GEAR_SLOT_TYPE_PREFILLED:
            this->gearLoaded = true;
            GearSlot_InitDyna(this, play);
            this->actionFunc = GearSlot_IdleFull;
            break;
    }
}

void GearSlot_Destroy(Actor* thisx, PlayState* play) {
    GearSlot* this = (GearSlot*)thisx;

    if (this->gearLoaded) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

#define GEAR_SLOT_ROT_STEP DEG_TO_BINANG(5.0f)

void GearSlot_IdleFull(GearSlot* this, PlayState* play) {
}

void GearSlot_IdleEmpty(GearSlot* this, PlayState* play) {
    if (this->gearLoaded) {
        sNumSlotsFilled++;
        GearSlot_InitDyna(this, play);
        this->actionFunc = GearSlot_IdleFull;
    }
}

void GearSlot_EndCutscene(GearSlot* this, PlayState* play) {
    Play_ReturnToMainCam(play, this->subCamId, 0);
    this->subCamId = SUB_CAM_ID_DONE;
    Cutscene_StopManual(play, &play->csCtx);
    Player_SetCsActionWithHaltedActors(play, &this->dyna.actor, PLAYER_CSACTION_END);
    Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
    this->actionFunc = GearSlot_IdleFull;
}

f32 GearSlot_EaseInOutCubic(f32 t) {
    if (t < 0.5) {
        return 4 * t * t * t;
    } else {
        return 1 - Math_PowF(-2 * t + 2, 3) / 2;
    }
}

#define GEAR_SLOT_CUTSCENE_LENGTH (8 * 20)
#define GEAR_SLOT_CUTSCENE_CAM_MOVE_TIME (5 * 20)

void GearSlot_Cutscene(GearSlot* this, PlayState* play) {
    static Vec3f camEyeFinal = { -2062.0f, -92.51f, -321.92f };
    static Vec3f camAtFinal = { -2068.9f, -90.24f, -315.05f };
    f32 interpFactor;

    if (DECR(this->timer) == 0) {
        this->actionFunc = GearSlot_EndCutscene;
    }

    if ((GEAR_SLOT_CUTSCENE_LENGTH - this->timer) < GEAR_SLOT_CUTSCENE_CAM_MOVE_TIME) {
        interpFactor = GearSlot_EaseInOutCubic((GEAR_SLOT_CUTSCENE_LENGTH - this->timer) / (f32)GEAR_SLOT_CUTSCENE_CAM_MOVE_TIME);
    } else {
        interpFactor = 1.0f;
    }

    this->subCamAt.x = this->initSubCamAt.x + interpFactor * (camAtFinal.x - this->initSubCamAt.x);
    this->subCamAt.y = this->initSubCamAt.y + interpFactor * (camAtFinal.y - this->initSubCamAt.y);
    this->subCamAt.z = this->initSubCamAt.z + interpFactor * (camAtFinal.z - this->initSubCamAt.z);

    this->subCamEye.x = this->initSubCamEye.x + interpFactor * (camEyeFinal.x - this->initSubCamEye.x);
    this->subCamEye.y = this->initSubCamEye.y + interpFactor * (camEyeFinal.y - this->initSubCamEye.y);
    this->subCamEye.z = this->initSubCamEye.z + interpFactor * (camEyeFinal.z - this->initSubCamEye.z);

    Play_SetCameraAtEye(play, this->subCamId, &this->subCamAt, &this->subCamEye);
}

void GearSlot_WaitForCs(GearSlot* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        this->subCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);

        this->initSubCamAt.x = this->subCamAt.x = -2271.9f;
        this->initSubCamAt.y = this->subCamAt.y = 67.25;
        this->initSubCamAt.z = this->subCamAt.z = -6.78f;

        this->initSubCamEye.x = this->subCamEye.x = -2263.0f;
        this->initSubCamEye.y = this->subCamEye.y = 71.82f;
        this->initSubCamEye.z = this->subCamEye.z = -6.78f;

        Play_SetCameraAtEye(play, this->subCamId, &this->subCamAt, &this->subCamEye);

        this->timer = GEAR_SLOT_CUTSCENE_LENGTH;
        this->actionFunc = GearSlot_Cutscene;
    }
}

void GearSlot_Update(Actor* thisx, PlayState* play) {
    GearSlot* this = (GearSlot*)thisx;
    Player* player = GET_PLAYER(play);

    if (ABS(this->dyna.actor.yDistToPlayer) > 600.0f) {
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
        DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }

    if (player->actor.world.pos.y < -310.0f || player->actor.world.pos.y > 1340.0f) {
        return;
    }

    if (sNumSlotsFilled == GEAR_SLOT_NUM) {
        if (this->isLeader) {
            if (!Flags_GetSwitch(play, GEAR_SLOT_GET_SWITCH(this->dyna.actor.params))) {
                OnePointCutscene_AttentionSetSfx(play, &this->dyna.actor, NA_SE_SY_TRE_BOX_APPEAR);
                Flags_SetSwitch(play, GEAR_SLOT_GET_SWITCH(this->dyna.actor.params));
                this->timer = 30;
                this->actionFunc = GearSlot_WaitForCs;
            }
            Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_WOOD_GEAR - SFX_FLAG);
        }
        this->dyna.actor.shape.rot.y += GEAR_SLOT_ROT_STEP;
    }

    this->actionFunc(this, play);
}

void GearSlot_Draw(Actor* thisx, PlayState* play) {
    GearSlot* this = (GearSlot*)thisx;
    
    if (this->gearLoaded) {
        Gfx_DrawDListOpa(play, gSmallGearDL);
    }
}