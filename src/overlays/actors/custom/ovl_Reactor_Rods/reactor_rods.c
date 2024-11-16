#include "reactor_rods.h"
#include "assets_custom/objects/object_reactor/object_reactor.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void ReactorRods_Init(Actor* thisx, PlayState* play);
void ReactorRods_Destroy(Actor* thisx, PlayState* play);
void ReactorRods_Update(Actor* thisx, PlayState* play);
void ReactorRods_Draw(Actor* thisx, PlayState* play);

void ReactorRods_Control_Idle(ReactorRods* this, PlayState* play);
void ReactorRods_Control_Turn(ReactorRods* this, PlayState* play);
void ReactorRods_Rods_Idle(ReactorRods* this, PlayState* play);

const ActorInit Reactor_Rods_InitVars = {
    ACTOR_REACTOR_RODS,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_REACTOR,
    sizeof(ReactorRods),
    ReactorRods_Init,
    ReactorRods_Destroy,
    ReactorRods_Update,
    ReactorRods_Draw,
};

static s16 sTurnDir = 0;
static f32 sRodsDistToPlayer = 0;
static s16 sControlTurnRate = 0;

#define REACTOR_RODS_GET_SWITCH(params) (params & 0x3F)
#define REACTOR_RODS_GET_TYPE(params) (params >> 8 & 0xF)

void ReactorRods_Init(Actor* thisx, PlayState* play) {
    ReactorRods* this = (ReactorRods*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);
    this->type = REACTOR_RODS_GET_TYPE(this->dyna.actor.params);

    this->dyna.actor.uncullZoneForward = 2000.0f;
    this->dyna.actor.uncullZoneScale = 3000.0f;
    this->dyna.actor.uncullZoneDownward = 3000.0f;

    switch (this->type) {
        case REACTOR_RODS_CONTROL:
            DynaPolyActor_Init(&this->dyna, 0);
            CollisionHeader_GetVirtual(&gReactorRodControlCol_collisionHeader, &colHeader);
            this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
            this->actionFunc = ReactorRods_Control_Idle;
            break;
        
        case REACTOR_RODS_RODS:
            this->actionFunc = ReactorRods_Rods_Idle;
            break;
    }
}

void ReactorRods_Destroy(Actor* thisx, PlayState* play) {
    ReactorRods* this = (ReactorRods*)thisx;

    if (this->type == REACTOR_RODS_CONTROL) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

// 1 degree
#define REACTOR_RODS_TURN_RATE 0xB6
// 15 degrees
#define REACTOR_RODS_TURN_ANGLE_MAX 0xAAA

#define REACTOR_RODS_HEIGHT_RAISED_MAX 100.0f

void ReactorRods_Rods_Idle(ReactorRods* this, PlayState* play) {
    CollisionHeader* colHeader;
    Vec3f correctPos;

    sRodsDistToPlayer = this->dyna.actor.xzDistToPlayer;

    correctPos = this->dyna.actor.home.pos;
    correctPos.y += 70.0f;

    if (Actor_WorldDistXYZToPoint(&this->dyna.actor, &correctPos) <= 0.1f) {
        Flags_SetSwitch(play, REACTOR_RODS_GET_SWITCH(this->dyna.actor.params));
    } else {
        Flags_UnsetSwitch(play, REACTOR_RODS_GET_SWITCH(this->dyna.actor.params));
    }

    if (sControlTurnRate == REACTOR_RODS_TURN_RATE) {
        if (sTurnDir > 0) {
            // Pushing forward
            if (this->dyna.actor.world.pos.y < (this->dyna.actor.home.pos.y + REACTOR_RODS_HEIGHT_RAISED_MAX)) {
                this->dyna.actor.world.pos.y += 1.0f;
                func_8002F974(&this->dyna.actor, NA_SE_EV_IRON_DOOR_CLOSE - SFX_FLAG);
            }
        } else {
            // Pushing backward
            if (this->dyna.actor.world.pos.y > this->dyna.actor.home.pos.y) {
                this->dyna.actor.world.pos.y -= 1.0f;
                func_8002F974(&this->dyna.actor, NA_SE_EV_IRON_DOOR_CLOSE - SFX_FLAG);
            }
        }
    }
}

void ReactorRods_Control_Turn(ReactorRods* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 turnPercent;
    s16 turnAngle;
    u8 turnFinished;

    sControlTurnRate = REACTOR_RODS_TURN_RATE;

    // Adjust turn angle towards target, check if turn is finished
    turnFinished = Math_StepToS(&this->turnAngle, REACTOR_RODS_TURN_ANGLE_MAX, sControlTurnRate);
    turnAngle = this->turnAngle * sTurnDir;
    turnPercent = BINANG_TO_DEG(turnAngle) / BINANG_TO_DEG(REACTOR_RODS_TURN_ANGLE_MAX);
    this->dyna.actor.shape.rot.y = (this->prevRotY + DEG_TO_BINANG(turnPercent * BINANG_TO_DEG(REACTOR_RODS_TURN_ANGLE_MAX)));
    player->actor.shape.rot.y = (this->playerPrevRotY + DEG_TO_BINANG(turnPercent * BINANG_TO_DEG(REACTOR_RODS_TURN_ANGLE_MAX)));
    
    if ((player->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) && (sRodsDistToPlayer > 0.0f)) {
        player->actor.world.pos.x =
            this->dyna.actor.home.pos.x +
            (Math_SinS(this->dyna.actor.shape.rot.y - this->initTurnAngle) * sRodsDistToPlayer);
        player->actor.world.pos.z =
            this->dyna.actor.home.pos.z +
            (Math_CosS(this->dyna.actor.shape.rot.y - this->initTurnAngle) * sRodsDistToPlayer);
    } else {
        sRodsDistToPlayer = 0.0f;
    }
    
    if (turnFinished) {
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->playerPrevRotY = player->actor.shape.rot.y;
        this->prevRotY += turnAngle;
        sControlTurnRate = 0;
        this->turnAngle = 0;
        this->timer = 5;
        this->actionFunc = ReactorRods_Control_Idle;
        this->dyna.unk_150 = 0.0f;
    }

    func_8002F974(&this->dyna.actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
}

void ReactorRods_Control_Idle(ReactorRods* this, PlayState* play) {
    CollisionHeader* colHeader;
    Player* player = GET_PLAYER(play);
    s32 playerDirection;
    f32 forceDirection;

    if (this->dyna.unk_150 != 0.0f) {
        if (this->timer == 0) {
            this->initTurnAngle = this->dyna.actor.shape.rot.y - this->dyna.actor.yawTowardsPlayer;
            forceDirection = (this->dyna.unk_150 >= 0.0f) ? 1.0f : -1.0f;
            playerDirection = ((s16)(this->dyna.actor.yawTowardsPlayer - player->actor.shape.rot.y) > 0) ? -1 : 1;
            sTurnDir = playerDirection * forceDirection;
            this->playerPrevRotY = player->actor.shape.rot.y;
            this->actionFunc = ReactorRods_Control_Turn;
        } else {
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
            this->dyna.unk_150 = 0.0f;
            DECR(this->timer);
        }
    } else {
        this->timer = 0;
    }
}

void ReactorRods_Update(Actor* thisx, PlayState* play) {
    ReactorRods* this = (ReactorRods*)thisx;

    this->actionFunc(this, play);
}

void ReactorRods_Draw(Actor* thisx, PlayState* play) {
    ReactorRods* this = (ReactorRods*)thisx;

    switch (this->type) {
        case REACTOR_RODS_CONTROL:
            Gfx_DrawDListOpa(play, gReactorRodControlDL);
            break;
        
        case REACTOR_RODS_RODS: 
            Gfx_DrawDListOpa(play, gReactorRodsDL);
            break;
    }
}
