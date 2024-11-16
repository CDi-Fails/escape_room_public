#include "moving_platform.h"
#include "assets_custom/objects/object_clock_tower/object_clock_tower.h"

#define FLAGS (ACTOR_FLAG_4)

void MovingPlatform_Init(Actor* thisx, PlayState* play);
void MovingPlatform_Destroy(Actor* thisx, PlayState* play);
void MovingPlatform_Update(Actor* thisx, PlayState* play);
void MovingPlatform_Draw(Actor* thisx, PlayState* play);

void MovingPlatform_MoveOnTimer(MovingPlatform* this, PlayState* play);
void MovingPlatform_WaitToMove(MovingPlatform* this, PlayState* play);
void MovingPlatform_IdleInactive(MovingPlatform* this, PlayState* play);

const ActorInit Moving_Platform_InitVars = {
    ACTOR_MOVING_PLATFORM,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_CLOCK_TOWER,
    sizeof(MovingPlatform),
    MovingPlatform_Init,
    MovingPlatform_Destroy,
    MovingPlatform_Update,
    MovingPlatform_Draw,
};

#define MOVING_PLATFORM_GET_SWITCH(params) (params & 0x3F)
#define MOVING_PLATFORM_GET_TYPE(params) (params >> 8 & 0xF)

#define MOVING_PLATFORM_WAIT_TIME 30
#define MOVING_PLATFORM_MOVE_STEP_FAST 8.0f
#define MOVING_PLATFORM_MOVE_STEP_SLOW 6.0f

void MovingPlatform_Init(Actor* thisx, PlayState* play) {
    MovingPlatform* this = (MovingPlatform*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    this->type = MOVING_PLATFORM_GET_TYPE(this->dyna.actor.params);
    // Move up or forward first, depending on type
    this->moveDir = 1;

    this->cameraSetting = CAM_ID_NONE;

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);

    if (this->type == MOVING_PLATFORM_TYPE_STEP_ON) {
        this->dyna.actor.scale.x = 0.1f * 0.5f;
        this->dyna.actor.scale.z = 0.1f * 0.5f;
        this->dyna.actor.world.pos.z += 50.0f;
        CollisionHeader_GetVirtual(&gMovingPlatformUngrabCol_collisionHeader, &colHeader);
    } else {
        CollisionHeader_GetVirtual(&gMovingPlatformCol_collisionHeader, &colHeader);
    }

    if (Flags_GetSwitch(play, MOVING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        switch (this->type) {
            case MOVING_PLATFORM_TYPE_TIMER:
                this->timer = MOVING_PLATFORM_WAIT_TIME;
                this->actionFunc = MovingPlatform_MoveOnTimer;
                break;

            case MOVING_PLATFORM_TYPE_STEP_ON:
                this->actionFunc = MovingPlatform_WaitToMove;
                break;
        }
    } else {
        this->actionFunc = MovingPlatform_IdleInactive;
    }

    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
}

void MovingPlatform_Destroy(Actor* thisx, PlayState* play) {
    MovingPlatform* this = (MovingPlatform*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void MovingPlatform_IdleInactive(MovingPlatform* this, PlayState* play) {
    if (Flags_GetSwitch(play, MOVING_PLATFORM_GET_SWITCH(this->dyna.actor.params))) {
        switch (this->type) {
            case MOVING_PLATFORM_TYPE_TIMER:
                this->actionFunc = MovingPlatform_MoveOnTimer;
                break;
            
            case MOVING_PLATFORM_TYPE_STEP_ON:
                this->actionFunc = MovingPlatform_WaitToMove;
                break;
        }
    }
}

void MovingPlatform_MoveOnTimer(MovingPlatform* this, PlayState* play) {
    if (DECR(this->timer) != 0) {
        return;
    }
    
    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.y, this->dyna.actor.home.pos.y + (360.0f * this->moveDir), 1.0f, MOVING_PLATFORM_MOVE_STEP_FAST, 0.0f) < 0.01f) {
        this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y + (360.0f * this->moveDir);
        this->moveDir ^= 1;
        this->timer = MOVING_PLATFORM_WAIT_TIME;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_STOP);
    } else {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_MOVE - SFX_FLAG);
    }
}

void MovingPlatform_MoveToSpot(MovingPlatform* this, PlayState* play) {
    // All this to avoid exporting another display list-- man, I gotta get it together
    if (Math_SmoothStepToF(&this->dyna.actor.world.pos.z,
                           this->dyna.actor.home.pos.z - ((738.0f + 50.0f) * this->moveDir) - (50.0f * (this->moveDir - 1)), 1.0f,
                           MOVING_PLATFORM_MOVE_STEP_SLOW, 0.0f) < 0.01f) {
        this->dyna.actor.world.pos.z = this->dyna.actor.home.pos.z - ((738.0f + 50.0f) * this->moveDir) - (50.0f * (this->moveDir - 1));
        this->moveDir ^= 1;
        this->actionFunc = MovingPlatform_WaitToMove;
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_STOP);
    } else {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ELEVATOR_MOVE - SFX_FLAG);
    }
}

void MovingPlatform_WaitToMove(MovingPlatform* this, PlayState* play) {
    // If moveDir is forward, check if player is on top, then move to spot
    // Otherwise, check if player is not on top, then move to spot
    if (this->moveDir == 1 && this->dyna.interactFlags & DYNA_INTERACT_PLAYER_ON_TOP) {
        this->actionFunc = MovingPlatform_MoveToSpot;
    } else if (this->moveDir == 0 && !(this->dyna.interactFlags & DYNA_INTERACT_PLAYER_ON_TOP)) {
        this->actionFunc = MovingPlatform_MoveToSpot;
    }
}

void MovingPlatform_Update(Actor* thisx, PlayState* play) {
    MovingPlatform* this = (MovingPlatform*)thisx;
    Player* player = GET_PLAYER(play);

    if (ABS(this->dyna.actor.yDistToPlayer) > 600.0f) {
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
        DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }

    if (this->dyna.interactFlags & DYNA_INTERACT_PLAYER_ON_TOP && this->cameraSetting != CAM_SET_NORMAL3) {
        this->cameraSetting = play->cameraPtrs[CAM_ID_MAIN]->setting;
        Camera_RequestSetting(play->cameraPtrs[CAM_ID_MAIN], CAM_SET_NORMAL3);
    } else if (this->cameraSetting != CAM_ID_NONE) {
        Camera_RequestSetting(play->cameraPtrs[CAM_ID_MAIN], this->cameraSetting);
        this->cameraSetting = CAM_ID_NONE;
    }

    this->actionFunc(this, play);
}

void MovingPlatform_Draw(Actor* thisx, PlayState* play) {
    MovingPlatform* this = (MovingPlatform*)thisx;

    Gfx_DrawDListOpa(play, gMovingPlatformDL);
}
