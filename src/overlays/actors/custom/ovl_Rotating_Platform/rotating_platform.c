#include "rotating_platform.h"
#include "assets_custom/objects/object_rotating_platform/object_rotating_platform.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void RotatingPlatform_Init(Actor* thisx, PlayState* play);
void RotatingPlatform_Destroy(Actor* thisx, PlayState* play);
void RotatingPlatform_Update(Actor* thisx, PlayState* play);
void RotatingPlatform_Draw(Actor* thisx, PlayState* play);

void RotatingPlatform_Arms_IdleRaised(RotatingPlatform* this, PlayState* play);
void RotatingPlatform_Rotate(RotatingPlatform* this, PlayState* play);
void RotatingPlatform_Arms_Lower(RotatingPlatform* this, PlayState* play);
void RotatingPlatform_Raising_Raise(RotatingPlatform* this, PlayState* play);
void RotatingPlatform_Raising_IdleLowered(RotatingPlatform* this, PlayState* play);
void RotatingPlatform_DoNothing(RotatingPlatform* this, PlayState* play);

const ActorInit Rotating_Platform_InitVars = {
    ACTOR_ROTATING_PLATFORM,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_ROTATING_PLATFORM,
    sizeof(RotatingPlatform),
    RotatingPlatform_Init,
    RotatingPlatform_Destroy,
    RotatingPlatform_Update,
    RotatingPlatform_Draw,
};

#define ROTATING_PLATFORM_GET_SWITCH(params) (params & 0x3F)
#define ROTATING_PLATFORM_GET_TYPE(params) ((params >> 8) & 0xF)

#define ROTATING_PLATFORM_ARMS_RAISE_HEIGHT 250.0f
#define ROTATING_PLATFORM_RAISING_LOWER_HEIGHT 250.0f
#define ROTATING_PLATFORM_RAISE_LOWER_STEP 4.0f

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_TREE,
        AT_NONE,
        AC_NONE,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK5,
        { 0x00000000, 0x00, 0x00 },
        { 0x0FC0074A, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 60, 2440, -500, { 0, 0, 0 } },
};

void RotatingPlatform_Init(Actor* thisx, PlayState* play) {
    RotatingPlatform* this = (RotatingPlatform*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    this->type = ROTATING_PLATFORM_GET_TYPE(this->dyna.actor.params);
    this->cameraSetting = CAM_ID_NONE;

    switch (this->type) {
        case ROTATING_PLATFORM_TYPE_BASE:
            this->actionFunc = RotatingPlatform_Rotate;
            // Cylinder collider for only the base
            Collider_InitCylinder(play, &this->collider);
            Collider_SetCylinder(play, &this->collider, &this->dyna.actor, &sCylinderInit);
            Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
            break;

        case ROTATING_PLATFORM_TYPE_ARMS:
            this->actionFunc = RotatingPlatform_Rotate;
            break;

        case ROTATING_PLATFORM_TYPE_ARMS_SWITCH:
            if (Flags_GetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
                this->actionFunc = RotatingPlatform_Rotate;
            } else {
                // Switch isn't set, move arms up to raised position
                this->dyna.actor.world.pos.y += ROTATING_PLATFORM_ARMS_RAISE_HEIGHT;
                this->actionFunc = RotatingPlatform_Arms_IdleRaised;
            }
            break;
        
        case ROTATING_PLATFORM_TYPE_RAISING:
            if (Flags_GetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
                this->actionFunc = RotatingPlatform_DoNothing;
            } else {
                this->dyna.actor.world.pos.y -= ROTATING_PLATFORM_RAISING_LOWER_HEIGHT;
                this->actionFunc = RotatingPlatform_Raising_IdleLowered;
            }
            // Dynapoly for raising type
            DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
            CollisionHeader_GetVirtual(&gRaisingPlatformCol_collisionHeader, &colHeader);
            this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
            break;
    }

    // Set focus for cutscenes
    this->dyna.actor.focus.pos.y = this->dyna.actor.world.pos.y;

    // Dynapoly for both the arm types
    if (this->type == ROTATING_PLATFORM_TYPE_ARMS || this->type == ROTATING_PLATFORM_TYPE_ARMS_SWITCH) {
        DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS | DYNA_TRANSFORM_ROT_Y);
        CollisionHeader_GetVirtual(&gRotatingPlatformCol_collisionHeader, &colHeader);
        this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
    }
}

void RotatingPlatform_Destroy(Actor* thisx, PlayState* play) {
    RotatingPlatform* this = (RotatingPlatform*)thisx;

    if (this->type != ROTATING_PLATFORM_TYPE_BASE) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

void RotatingPlatform_Raising_Raise(RotatingPlatform* this, PlayState* play) {
    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y, 1.0f, ROTATING_PLATFORM_RAISE_LOWER_STEP, 0.0f) <= ROTATING_PLATFORM_RAISE_LOWER_STEP) {
        this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_STOP);
        Flags_SetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params));
        this->actionFunc = RotatingPlatform_DoNothing;
    } else {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BUYOSTAND_RISING - SFX_FLAG);
    }
}

void RotatingPlatform_Raising_IdleLowered(RotatingPlatform* this, PlayState* play) {
    if (Flags_GetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        this->actionFunc = RotatingPlatform_Raising_Raise;
    }
}

void RotatingPlatform_DoNothing(RotatingPlatform* this, PlayState* play) {

}

#define ROTATING_PLATFORM_ROTATE_STEP DEG_TO_BINANG(1.0f)

void RotatingPlatform_Rotate(RotatingPlatform* this, PlayState* play) {
    if (this->type == ROTATING_PLATFORM_TYPE_BASE) {
        if (this->dyna.actor.xzDistToPlayer < 600.0f) {
            Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
            if (this->dyna.actor.xzDistToPlayer < 180.0f) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
            }
        }
        if (ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params) != 0) {
            if (!Flags_GetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
                return;
            }
        }
    }

    this->dyna.actor.shape.rot.y += ROTATING_PLATFORM_ROTATE_STEP;
    func_800F436C(&this->dyna.actor.projectedPos, NA_SE_EV_WOOD_GEAR - SFX_FLAG,
                  ((this->dyna.actor.world.rot.y - 0x80) * (1.0f / 0x380)) + 1.0f);
}

void RotatingPlatform_Arms_Lower(RotatingPlatform* this, PlayState* play) {
    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y, 1.0f, ROTATING_PLATFORM_RAISE_LOWER_STEP, 0.0f) <= ROTATING_PLATFORM_RAISE_LOWER_STEP) {
        this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_STOP);
        Flags_SetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params));
        this->actionFunc = RotatingPlatform_Rotate;
    } else {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BUYOSTAND_FALL - SFX_FLAG);
    }
}

void RotatingPlatform_Arms_IdleRaised(RotatingPlatform* this, PlayState* play) {
    if (Flags_GetSwitch(play, ROTATING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        OnePointCutscene_Attention(play, &this->dyna.actor);
        this->actionFunc = RotatingPlatform_Arms_Lower;
    }
}

void RotatingPlatform_Update(Actor* thisx, PlayState* play) {
    RotatingPlatform* this = (RotatingPlatform*)thisx;

    if (this->dyna.interactFlags & DYNA_INTERACT_PLAYER_ON_TOP && this->cameraSetting != CAM_SET_NORMAL3) {
        this->cameraSetting = play->cameraPtrs[CAM_ID_MAIN]->setting;
        Camera_RequestSetting(play->cameraPtrs[CAM_ID_MAIN], CAM_SET_NORMAL3);
    } else if (this->cameraSetting != CAM_ID_NONE) {
        Camera_RequestSetting(play->cameraPtrs[CAM_ID_MAIN], this->cameraSetting);
        this->cameraSetting = CAM_ID_NONE;
    }

    this->actionFunc(this, play);
}

void RotatingPlatform_Draw(Actor* thisx, PlayState* play) {
    RotatingPlatform* this = (RotatingPlatform*)thisx;

    switch (this->type) {
        case ROTATING_PLATFORM_TYPE_BASE:
            Gfx_DrawDListOpa(play, gRotatingPlatformBaseDL);
            break;
        
        case ROTATING_PLATFORM_TYPE_ARMS:
        case ROTATING_PLATFORM_TYPE_ARMS_SWITCH:
            Gfx_DrawDListOpa(play, gRotatingPlatformArmsDL);
            break;
        
        case ROTATING_PLATFORM_TYPE_RAISING:
            Gfx_DrawDListOpa(play, gRaisingPlatformDL);
    }
}
