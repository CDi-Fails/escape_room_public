#include "pendulum.h"
#include "assets_custom/objects/object_clock_tower/object_clock_tower.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void Pendulum_Init(Actor* thisx, PlayState* play);
void Pendulum_Destroy(Actor* thisx, PlayState* play);
void Pendulum_Update(Actor* thisx, PlayState* play);
void Pendulum_Draw(Actor* thisx, PlayState* play);

void Pendulum_Swing(Pendulum* this, PlayState* play);
void Pendulum_IdleInactive(Pendulum* this, PlayState* play);

const ActorInit Pendulum_InitVars = {
    ACTOR_PENDULUM,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_CLOCK_TOWER,
    sizeof(Pendulum),
    Pendulum_Init,
    Pendulum_Destroy,
    Pendulum_Update,
    Pendulum_Draw,
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
        { 0x00000000, 0x00, 0x01 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_ON,
    },
    { 70, 200, -100, { 0, 0, 0 } },
};

#define PENDULUM_GET_SWITCH(params) (params & 0x3F)
#define PENDULUM_GET_TYPE(params) (params >> 8 & 0xF)

void Pendulum_Init(Actor* thisx, PlayState* play) {
    Pendulum* this = (Pendulum*)thisx;

    Actor_SetScale(&this->actor, 0.1f);

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->actor, &this->collider);

    this->type = PENDULUM_GET_TYPE(this->actor.params);

    if (Flags_GetSwitch(play, PENDULUM_GET_SWITCH(this->actor.params))) {
        switch (this->type) {
            case 0:
                this->swingAngle = 45.0f;
                break;
            
            case 1:
                this->swingAngle = -45.0f;
                break;
        }
        this->actionFunc = Pendulum_Swing;
    } else {
        this->actionFunc = Pendulum_IdleInactive;
    }
}

void Pendulum_Destroy(Actor* thisx, PlayState* play) {
    Pendulum* this = (Pendulum*)thisx;
}

#define PENDULUM_GRAVITY_CONSTANT 150.0f
#define PENDULUM_RADIUS 320.0f

void Pendulum_Swing(Pendulum* this, PlayState* play) {
    this->angularAccel = (-1.0f * PENDULUM_GRAVITY_CONSTANT / PENDULUM_RADIUS) * Math_SinF(DEG_TO_RAD(this->swingAngle));
    this->angularVel += this->angularAccel;
    this->swingAngle += this->angularVel;

    this->actor.shape.rot.z = DEG_TO_BINANG(this->swingAngle);
    
    if ((this->swingAngle >= 44.9f || this->swingAngle <= -44.9f) && this->type == 1) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_STALTU_DOWN_SET);
    }

    if (this->type == 1) {
        func_800F436C(&this->actor.projectedPos, NA_SE_EV_WOOD_GEAR - SFX_FLAG,
                    ((this->actor.world.rot.y - 0x80) * (1.0f / 0x380)) + 1.0f);
    }

    if (this->actor.xzDistToPlayer < 600.0f && this->actor.yDistToPlayer < 0) {
        this->collider.dim.pos.x = this->actor.world.pos.x + (PENDULUM_RADIUS * Math_SinF(DEG_TO_RAD(this->swingAngle)));
        this->collider.dim.pos.y = this->actor.world.pos.y - (PENDULUM_RADIUS * Math_CosF(DEG_TO_RAD(this->swingAngle)));
        this->collider.dim.pos.z = this->actor.world.pos.z;
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    }
}

void Pendulum_IdleInactive(Pendulum* this, PlayState* play) {
    if (this->actor.xzDistToPlayer < 600.0f) {
        this->collider.dim.pos.x = this->actor.world.pos.x;
        this->collider.dim.pos.y = this->actor.world.pos.y - PENDULUM_RADIUS;
        this->collider.dim.pos.z = this->actor.world.pos.z;
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    }

    if (Flags_GetSwitch(play, PENDULUM_GET_SWITCH(this->actor.params))) {
        switch (this->type) {
            case 0:
                this->swingAngle = 45.0f;
                break;

            case 1:
                this->swingAngle = -45.0f;
                break;
        }
        this->actionFunc = Pendulum_Swing;
    }
}

void Pendulum_Update(Actor* thisx, PlayState* play) {
    Pendulum* this = (Pendulum*)thisx;
    Player* player = GET_PLAYER(play);

    if (player->actor.world.pos.y < -310.0f || player->actor.world.pos.y > 1340.0f) {
        return;
    }

    this->actionFunc(this, play);
}

void Pendulum_Draw(Actor* thisx, PlayState* play) {
    Pendulum* this = (Pendulum*)thisx;

    Gfx_DrawDListOpa(play, gPendulumDL);
}
