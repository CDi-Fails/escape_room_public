#include "water_plane.h"
#include "assets_custom/objects/object_water_plane/object_water_plane.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void WaterPlane_Init(Actor* thisx, PlayState* play);
void WaterPlane_Destroy(Actor* thisx, PlayState* play);
void WaterPlane_Update(Actor* thisx, PlayState* play);
void WaterPlane_Draw(Actor* thisx, PlayState* play);

void WaterPlane_Once_IdleRaised(WaterPlane* this, PlayState* play);
void WaterPlane_DoNothing(WaterPlane* this, PlayState* play);
void WaterPlane_Twice_IdleLowered(WaterPlane* this, PlayState* play);
void WaterPlane_Twice_IdleHalfRaised(WaterPlane* this, PlayState* play);
void WaterPlane_Twice_IdleFullRaised(WaterPlane* this, PlayState* play);

const ActorInit Water_Plane_InitVars = {
    ACTOR_WATER_PLANE,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_WATER_PLANE,
    sizeof(WaterPlane),
    WaterPlane_Init,
    WaterPlane_Destroy,
    WaterPlane_Update,
    WaterPlane_Draw,
};

#define WATER_PLANE_GET_SWITCH(params) (params & 0x3F)
#define WATER_PLANE_GET_SWITCH_2(params) ((params >> 8) & 0x3F)
#define WATER_PLANE_GET_TYPE(params) ((params >> 15) & 0x1)

void WaterPlane_Init(Actor* thisx, PlayState* play) {
    WaterPlane* this = (WaterPlane*)thisx;

    // Find waterbox below the actor and store its current height and a pointer to it 
    WaterBox_GetSurface1(play, &play->colCtx, this->actor.world.pos.x, this->actor.world.pos.z, &this->homeWaterSurfaceY,
                         &this->waterbox);
    this->prevWaterSurfaceY = this->homeWaterSurfaceY;

    this->type = WATER_PLANE_GET_TYPE(this->actor.params);

    switch (this->type) {
        case WATER_PLANE_TYPE_LOWER:
            if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH(this->actor.params))) {
                this->waterbox->ySurface = this->prevWaterSurfaceY - 65.0f;
                this->actor.world.pos.y = this->waterbox->ySurface;
                this->actionFunc = WaterPlane_DoNothing;
            } else {
                this->actionFunc = WaterPlane_Once_IdleRaised;
            }
            break;

        case WATER_PLANE_TYPE_RAISE_TWICE:
            Actor_SetScale(&this->actor, 0.1f);
            if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH_2(this->actor.params))) {
                this->waterbox->ySurface = this->prevWaterSurfaceY + 400.0f;
                this->actor.world.pos.y = this->waterbox->ySurface;
                this->actionFunc = WaterPlane_DoNothing;
            } else if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH(this->actor.params))) {
                Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                this->waterbox->ySurface = this->prevWaterSurfaceY + 200.0f;
                this->actor.world.pos.y = this->waterbox->ySurface;
                this->actionFunc = WaterPlane_Twice_IdleHalfRaised;
            } else {
                this->actionFunc = WaterPlane_Twice_IdleLowered;
            }
            break;
    }
}

void WaterPlane_Destroy(Actor* thisx, PlayState* play) {
    WaterPlane* this = (WaterPlane*)thisx;

    // Prevents us accidentally adding a bunch of height to the waterbox permanently, but also lets us keep it at the
    // right height when saves are loaded
    this->waterbox->ySurface = this->homeWaterSurfaceY;
}

void WaterPlane_Twice_Raise(WaterPlane* this, PlayState* play) {
    u8 isRaised = Math_StepToS(&this->waterbox->ySurface, this->prevWaterSurfaceY + this->moveDist, 1);

    this->actor.world.pos.y = this->waterbox->ySurface;
    
    if (isRaised) {
        if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH_2(this->actor.params))) {
            this->actionFunc = WaterPlane_DoNothing;
        } else if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH(this->actor.params))) {
            this->prevWaterSurfaceY = this->waterbox->ySurface;
            this->actionFunc = WaterPlane_Twice_IdleHalfRaised;
        }
    } else {
        func_8002F948(&this->actor, NA_SE_EV_WATER_LEVEL_DOWN - SFX_FLAG);
    }
}

void WaterPlane_DoNothing(WaterPlane* this, PlayState* play) {

}

void WaterPlane_Twice_IdleHalfRaised(WaterPlane* this, PlayState* play) {
    if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH_2(this->actor.params))) {
        this->moveDist = 200.0f;
        this->actionFunc = WaterPlane_Twice_Raise;
    }
}

void WaterPlane_Twice_IdleLowered(WaterPlane* this, PlayState* play) {
    if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH(this->actor.params))) {
        OnePointCutscene_Attention(play, &this->actor);
        this->moveDist = 200.0f;
        this->actionFunc = WaterPlane_Twice_Raise;
    }
}

void WaterPlane_Once_Lower(WaterPlane* this, PlayState* play) {
    u8 isLowered = Math_StepToS(&this->waterbox->ySurface, this->prevWaterSurfaceY - this->moveDist, 1);

    this->actor.world.pos.y = this->waterbox->ySurface;
    
    if (isLowered) {
        this->actionFunc = WaterPlane_DoNothing;
    } else {
        func_8002F948(&this->actor, NA_SE_EV_WATER_LEVEL_DOWN - SFX_FLAG);
    }
}

void WaterPlane_Once_IdleRaised(WaterPlane* this, PlayState* play) {
    if (Flags_GetSwitch(play, WATER_PLANE_GET_SWITCH(this->actor.params))) {
        OnePointCutscene_Attention(play, &this->actor);
        this->moveDist = 65.0f;
        this->actionFunc = WaterPlane_Once_Lower;
    }
}

void WaterPlane_Update(Actor* thisx, PlayState* play) {
    WaterPlane* this = (WaterPlane*)thisx;

    this->actionFunc(this, play);
}

void WaterPlane_Draw(Actor* thisx, PlayState* play) {
    WaterPlane* this = (WaterPlane*)thisx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPPipeSync(POLY_XLU_DISP++);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x09,
                Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, play->gameplayFrames, 32, 32, 1, 0,
                                play->gameplayFrames, 32, 32));

    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                G_MTX_MODELVIEW | G_MTX_LOAD);

    switch (this->type) {
        case WATER_PLANE_TYPE_LOWER:
            Gfx_DrawDListXlu(play, gWaterPlaneDL);
            break;
        
        case WATER_PLANE_TYPE_RAISE_TWICE:
            Gfx_DrawDListXlu(play, gWaterPlaneLargeDL);
            break;
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

}
