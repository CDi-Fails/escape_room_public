#include "steam.h"

#define FLAGS (ACTOR_FLAG_4)

void Steam_Init(Actor* thisx, PlayState* play);
void Steam_Destroy(Actor* thisx, PlayState* play);
void Steam_Update(Actor* thisx, PlayState* play);
void Steam_Draw(Actor* thisx, PlayState* play);

void Steam_SmallTimedIdle(Steam* this, PlayState* play);
void Steam_LargeSwitchIdleOn(Steam* this, PlayState* play);

const ActorInit Steam_InitVars = {
    ACTOR_STEAM,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(Steam),
    Steam_Init,
    Steam_Destroy,
    Steam_Update,
    Steam_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x20000000, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NONE,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 20, 150, 0, { 0, 0, 0 } },
};

#define STEAM_GET_SWITCH(params) (params & 0x3F)
#define STEAM_TYPE(params) (params >> 8 & 0xF)

#define STEAM_TIMER_COOLDOWN_MAX 100
#define STEAM_TIMER_ON_MAX 100

void Steam_Init(Actor* thisx, PlayState* play) {
    Steam* this = (Steam*)thisx;

    this->type = STEAM_TYPE(this->actor.params);
    
    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);

    switch (this->type) {
        case STEAM_TYPE_SMALL_TIMED:
            this->timer = Rand_S16Offset(0, STEAM_TIMER_ON_MAX);
            this->actionFunc = Steam_SmallTimedIdle;
            break;
        
        case STEAM_TYPE_LARGE_SWITCH:
            if (Flags_GetSwitch(play, STEAM_GET_SWITCH(this->actor.params))) {
                Actor_Kill(&this->actor);
            } else {
                this->actor.flags |= ACTOR_FLAG_5;
                this->collider.dim.radius = 75.0f;
                this->actionFunc = Steam_LargeSwitchIdleOn;
            }
            break;
    }
}

void Steam_Destroy(Actor* thisx, PlayState* play) {
    Steam* this = (Steam*)thisx;
}

void Steam_LargeSwitchIdleOn(Steam* this, PlayState* play) {
    if (Flags_GetSwitch(play, STEAM_GET_SWITCH(this->actor.params))) {
        Actor_Kill(&this->actor);
        return;
    }

    if (this->collider.base.atFlags & AT_HIT) {
        this->collider.base.atFlags &= ~AT_HIT;
        func_8002F6D4(play, &this->actor, 2.0f, this->actor.yawTowardsPlayer, 0.0f, 4);
        Player_PlaySfx(GET_PLAYER(play), NA_SE_PL_BODY_HIT);
    }

    if (this->actor.xzDistToPlayer < 200.0f) {
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }

    Actor_PlaySfx(&this->actor, NA_SE_EN_VALVAISA_FIRE - SFX_FLAG);
}

void Steam_SmallTimedIdle(Steam* this, PlayState* play) {
    if (DECR(this->timer) == 0) {
        if (this->isOff) {
            this->timer = Rand_S16Offset(0, STEAM_TIMER_ON_MAX);
        } else {
            this->timer = Rand_S16Offset(0, STEAM_TIMER_COOLDOWN_MAX);
        }
        this->isOff ^= 1;
    }

    if (!this->isOff && this->actor.xzDistToPlayer < 200.0f) {
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

void Steam_Update(Actor* thisx, PlayState* play) {
    Steam* this = (Steam*)thisx;
    
    this->actionFunc(this, play);
}

void Steam_Draw(Actor* thisx, PlayState* play) {
    Steam* this = (Steam*)thisx;
    Vec3f velocity = { 0.0f, 10.0f, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };
    Color_RGBA8 prim = { 190, 170, 140, 255 };
    Color_RGBA8 env = { 130 + 40, 110 + 40, 80 + 40, 255 };
    s16 scale = 0;
    s16 life = 0;
    s16 scaleStep = 0;

    if (this->isOff) {
        return;
    }

    switch (this->type) {
        case STEAM_TYPE_SMALL_TIMED:
            life = 18;
            scale = 40;
            scaleStep = 45;
            break;

        case STEAM_TYPE_LARGE_SWITCH:
            velocity.y = 40.0f;
            life = 12;
            scale = 150;
            scaleStep = 500;
            break;
    }

    // Spawn dust
    func_8002836C(play, &this->actor.world.pos, &velocity, &accel, &prim, &env, scale, scaleStep, life);
}
