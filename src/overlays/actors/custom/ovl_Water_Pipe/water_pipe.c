#include "water_pipe.h"
#include "assets_custom/objects/object_water_pipe/object_water_pipe.h"
#include "assets/objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"

#define FLAGS (ACTOR_FLAG_4)

void WaterPipe_Init(Actor* thisx, PlayState* play);
void WaterPipe_Destroy(Actor* thisx, PlayState* play);
void WaterPipe_Update(Actor* thisx, PlayState* play);
void WaterPipe_Draw(Actor* thisx, PlayState* play);

void WaterPipe_DoNothing(WaterPipe* this, PlayState* play);
void WaterPipe_Explode(WaterPipe* this, PlayState* play);
void WaterPipe_Idle(WaterPipe* this, PlayState* play);

const ActorInit Water_Pipe_InitVars = {
    ACTOR_WATER_PIPE,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_WATER_PIPE,
    sizeof(WaterPipe),
    WaterPipe_Init,
    WaterPipe_Destroy,
    WaterPipe_Update,
    WaterPipe_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000000, 0x00, 0x00 },
        { DMG_EXPLOSIVE, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_NONE,
    },
    { 225, 150, -100, { 0, 0, 0 } },
};

#define WATER_PIPE_GET_SWITCH(params) (params & 0x3F)

void WaterPipe_Init(Actor* thisx, PlayState* play) {
    WaterPipe* this = (WaterPipe*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    if (Flags_GetSwitch(play, WATER_PIPE_GET_SWITCH(this->dyna.actor.params))) {
        this->alpha = 0;
        this->actionFunc = WaterPipe_DoNothing;
    } else {
        Collider_InitCylinder(play, &this->collider);
        Collider_SetCylinder(play, &this->collider, &this->dyna.actor, &sCylinderInit);
        Collider_UpdateCylinder(&this->dyna.actor, &this->collider);

        // Waterfall not yet spawned
        this->alpha = -1;
        this->timer = 30;

        this->actionFunc = WaterPipe_Idle;
    }

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gWaterPipeCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
}

void WaterPipe_Destroy(Actor* thisx, PlayState* play) {
    WaterPipe* this = (WaterPipe*)thisx;
}

void WaterPipe_DoNothing(WaterPipe* this, PlayState* play) {
}

#define WATER_PIPE_WATERFALL_LIFETIME (8 * 20)

static s16 sEffectScales[] = {
    10, 9, 9, 8, 7, 5, 4, 4, 3, 2,
};

void WaterPipe_SpawnExplodeEffects(WaterPipe* this, PlayState* play) {
    Vec3f pos;
    Vec3f velocity;
    Gfx* dlist;
    s16 arg5;
    s16 scale;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEffectScales); i++) {
        pos.x = ((Rand_ZeroOne() - 0.5f) * 10.0f) + this->dyna.actor.home.pos.x;
        pos.y = ((Rand_ZeroOne() * 5.0f) + this->dyna.actor.home.pos.y) - 20.0f;
        pos.z = ((Rand_ZeroOne() - 0.5f) * 10.0f) + this->dyna.actor.home.pos.z - 130.0f;;
        velocity.x = (Rand_ZeroOne() - 0.5f) * 10.0f;
        velocity.y = (Rand_ZeroOne() * 16.0f) + 5.0f;
        velocity.z = (Rand_ZeroOne() - 0.5f) * 10.0f;
        scale = sEffectScales[i] + 10;
        arg5 = (scale >= 16) ? 37 : 33;
        EffectSsKakera_Spawn(play, &pos, &velocity, &pos, -400, arg5, 10, 2, 0, scale, 1, 0, 80, KAKERA_COLOR_NONE,
                             OBJECT_GAMEPLAY_DANGEON_KEEP, gBrownFragmentDL);
    }
    func_80033480(play, &this->dyna.actor.world.pos, 60.0f, 8, 100, 160, 1);
}

f32 WaterPipe_EaseOutCubic(f32 t) {
    return 1 - Math_PowF(1 - t, 5);
}

void WaterPipe_Explode(WaterPipe* this, PlayState* play) {
    f32 interpFactor = WaterPipe_EaseOutCubic(this->timer / (f32)WATER_PIPE_WATERFALL_LIFETIME);

    if (DECR(this->timer) == 0) {
        this->alpha = 0;
        this->actionFunc = WaterPipe_DoNothing;
    }
    this->alpha = 255 * interpFactor;
    Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_WATER_WALL - SFX_FLAG);
}

void WaterPipe_Idle(WaterPipe* this, PlayState* play) {
    if (this->collider.base.acFlags & AC_HIT) {
        this->alpha = 0;
        this->timer = WATER_PIPE_WATERFALL_LIFETIME;
        OnePointCutscene_Attention(play, &this->dyna.actor);
        WaterPipe_SpawnExplodeEffects(this, play);
        Flags_SetSwitch(play, WATER_PIPE_GET_SWITCH(this->dyna.actor.params));
        this->actionFunc = WaterPipe_Explode;
    }
    Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
}

void WaterPipe_Update(Actor* thisx, PlayState* play) {
    WaterPipe* this = (WaterPipe*)thisx;

    this->actionFunc(this, play);
}

void WaterPipe_Draw(Actor* thisx, PlayState* play) {
    WaterPipe* this = (WaterPipe*)thisx;

    Gfx_DrawDListOpa(play, gWaterPipeDL);

    if (!Flags_GetSwitch(play, WATER_PIPE_GET_SWITCH(this->dyna.actor.params))) {
        Gfx_DrawDListOpa(play, gWaterPipeCrackDL);   
    }

    if (this->alpha > 0) {
        OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

        gDPPipeSync(POLY_XLU_DISP++);

        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, (play->gameplayFrames * 6) % 256, 64, 64, 1, 0,
                                    (play->gameplayFrames * 6) % 256, 64, 64));

        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, this->alpha);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gSPDisplayList(POLY_XLU_DISP++, gWaterPipeWaterfallDL);

        CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
    }
}
