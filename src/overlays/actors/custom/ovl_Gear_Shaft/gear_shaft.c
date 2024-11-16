#include "gear_shaft.h"
#include "assets_custom/objects/object_clock_tower/object_clock_tower.h"
#include "src/overlays/actors/custom/ovl_Chain_Platform/chain_platform.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void GearShaft_Init(Actor* thisx, PlayState* play);
void GearShaft_Destroy(Actor* thisx, PlayState* play);
void GearShaft_Update(Actor* thisx, PlayState* play);
void GearShaft_Draw(Actor* thisx, PlayState* play);

void GearShaft_IdleInactive(GearShaft* this, PlayState* play);
void GearShaft_Idle(GearShaft* this, PlayState* play);

const ActorInit Gear_Shaft_InitVars = {
    ACTOR_GEAR_SHAFT,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_CLOCK_TOWER,
    sizeof(GearShaft),
    GearShaft_Init,
    GearShaft_Destroy,
    GearShaft_Update,
    GearShaft_Draw,
};

#define GEAR_SHAFT_GET_SWITCH(params) (params & 0x3F)

void GearShaft_Init(Actor* thisx, PlayState* play) {
    GearShaft* this = (GearShaft*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gGearShaftHandleCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    if (Flags_GetSwitch(play, GEAR_SHAFT_GET_SWITCH(this->dyna.actor.params))) {
        // Do nothing
        this->actionFunc = GearShaft_IdleInactive;
    } else {
        this->actionFunc = GearShaft_Idle;
    }
}

void GearShaft_Destroy(Actor* thisx, PlayState* play) {
    GearShaft* this = (GearShaft*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void GearShaft_IdleInactive(GearShaft* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->dyna.unk_150 != 0.0f) {
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->dyna.unk_150 = 0.0f;
    }
}

#define GEAR_SHAFT_PIECE_ROT_STEP DEG_TO_RAD(3.5f)
#define GEAR_SHAFT_CS_WAIT_TIME 30

void GearShaft_Turn(GearShaft* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    u8 switchFlagSet = Flags_GetSwitch(play, GEAR_SHAFT_GET_SWITCH(this->dyna.actor.params));

    if (DECR(this->timer) == 0) {
        if (!switchFlagSet) {
            Flags_SetSwitch(play, GEAR_SHAFT_GET_SWITCH(this->dyna.actor.params));
        } else {
            ChainPlatform* chainPlatform = (ChainPlatform*)Actor_Find(&play->actorCtx, ACTOR_CHAIN_PLATFORM, ACTORCAT_BG);

            if (chainPlatform != NULL && chainPlatform->dyna.actor.world.pos.y == chainPlatform->dyna.actor.home.pos.y) {
                this->pieceRot = 0.0f;
                this->actionFunc = GearShaft_IdleInactive;
            }
        }
    }

    this->pieceRot += (GEAR_SHAFT_PIECE_ROT_STEP * this->turnDir);
    Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
}

void GearShaft_Idle(GearShaft* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s32 playerDirection;
    f32 forceDirection;

    if (this->dyna.unk_150 != 0.0f) {
        if (DECR(this->timer) == 0) {
            forceDirection = (this->dyna.unk_150 >= 0.0f) ? 1.0f : -1.0f;
            playerDirection = ((s16)(this->dyna.actor.yawTowardsPlayer - player->actor.shape.rot.y) > 0) ? -1 : 1;
            this->turnDir = playerDirection * forceDirection;
            Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BLOCK_BOUND);
            this->timer = GEAR_SHAFT_CS_WAIT_TIME;
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
            this->dyna.unk_150 = 0.0f;
            OnePointCutscene_Attention(play, &this->dyna.actor);
            this->actionFunc = GearShaft_Turn;
        }
        // Don't want to actually have anything being pushed
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->dyna.unk_150 = 0.0f;
    } else {
        this->timer = 20;
    }
}

void GearShaft_Update(Actor* thisx, PlayState* play) {
    GearShaft* this = (GearShaft*)thisx;
    Player* player = GET_PLAYER(play);

    if (ABS(this->dyna.actor.yDistToPlayer) > 50.0f && this->dyna.actor.xzDistToPlayer < 90.0f) {
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
        DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }

    if (player->actor.world.pos.y < -310.0f || player->actor.world.pos.y > 1340.0f) {
        return;
    }

    this->actionFunc(this, play);
}

void GearShaft_Draw(Actor* thisx, PlayState* play) {
    GearShaft* this = (GearShaft*)thisx;
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    OPEN_DISPS(gfxCtx, __FILE__, __LINE__);

    Matrix_Push();
    Matrix_RotateY(this->pieceRot, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Gfx_DrawDListOpa(play, gGearShaftHandleDL);
    Gfx_DrawDListOpa(play, gGearShaftCenterDL);
    Matrix_Pop();

    Matrix_Push();
    Matrix_Translate(0.0f, 8.25f * 10.0f * (1 / this->dyna.actor.scale.y), 0.0f, MTXMODE_APPLY);
    Matrix_RotateZ(this->pieceRot, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Gfx_DrawDListOpa(play, gGearShaftSideLongDL);
    Matrix_Pop();

    Matrix_Push();
    Matrix_Translate(0.0f, 8.25f * 10.0f * (1 / this->dyna.actor.scale.y), 0.0f, MTXMODE_APPLY);
    Matrix_RotateX(this->pieceRot, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Gfx_DrawDListOpa(play, gGearShaftSideShortDL);
    Matrix_Pop();

    CLOSE_DISPS(gfxCtx, __FILE__, __LINE__);
}
