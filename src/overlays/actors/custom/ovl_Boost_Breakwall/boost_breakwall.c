#include "boost_breakwall.h"
#include "assets_custom/objects/object_boost_breakwall/object_boost_breakwall.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"
#include "assets/objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void BoostBreakwall_Init(Actor* thisx, PlayState* play);
void BoostBreakwall_Destroy(Actor* thisx, PlayState* play);
void BoostBreakwall_Update(Actor* thisx, PlayState* play);
void BoostBreakwall_Draw(Actor* thisx, PlayState* play);

const ActorInit Boost_Breakwall_InitVars = {
    ACTOR_BOOST_BREAKWALL,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_BOOST_BREAKWALL,
    sizeof(BoostBreakwall),
    BoostBreakwall_Init,
    BoostBreakwall_Destroy,
    BoostBreakwall_Update,
    BoostBreakwall_Draw,
};

#define BOOST_BREAKWALL_GET_SWITCH(params) (params & 0x3F)

void BoostBreakwall_Init(Actor* thisx, PlayState* play) {
    BoostBreakwall* this = (BoostBreakwall*)thisx;
    CollisionHeader* colHeader = NULL;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    if (Flags_GetSwitch(play, BOOST_BREAKWALL_GET_SWITCH(this->dyna.actor.params))) {
        Actor_Kill(&this->dyna.actor);
        return;
    }

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gBoostBreakwallDL_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
}

void BoostBreakwall_Destroy(Actor* thisx, PlayState* play) {
    BoostBreakwall* this = (BoostBreakwall*)thisx;

    if (!Flags_GetSwitch(play, BOOST_BREAKWALL_GET_SWITCH(this->dyna.actor.params))) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

static s16 sEffectScales[] = {
    30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 17, 16, 15, 14, 12, 10, 8, 7, 5, 3, 2,
};

void BoostBreakwall_Explode(BoostBreakwall* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f pos;
    Vec3f velocity;
    Gfx* dlist;
    s16 arg5;
    s16 scale;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEffectScales); i++) {
        pos.x = ((Rand_ZeroOne() - 0.5f) * 75.0f) + player->actor.world.pos.x;
        pos.y = ((Rand_ZeroOne() * 12.0f) + player->actor.world.pos.y);
        pos.z = ((Rand_ZeroOne() - 0.5f) * 75.0f) + player->actor.world.pos.z;
        velocity.x = (Rand_ZeroOne() - 0.5f) * 45.0f;
        velocity.y = (Rand_ZeroOne() * 16.0f) + 5.0f;
        velocity.z = (Rand_ZeroOne() - 0.5f) * 45.0f;
        scale = sEffectScales[i] * 0.5f;
        arg5 = (scale >= 16) ? 37 : 33;
        EffectSsKakera_Spawn(play, &pos, &velocity, &pos, -400, arg5, 10, 2, 0, scale, 1, 0, 80, KAKERA_COLOR_WHITE,
                             OBJECT_BOOST_BREAKWALL, gBoostBreakwallFragmentDL);
    }
    func_80033480(play, &player->actor.world.pos, 60.0f, 8, 100, 160, 1);
}

void BoostBreakwall_Update(Actor* thisx, PlayState* play) {
    BoostBreakwall* this = (BoostBreakwall*)thisx;
    Player* player = GET_PLAYER(play);
    f32 xDistToPlayer = ABS(this->dyna.actor.world.pos.x - player->actor.world.pos.x);

    if ((player->stateFlags3 & PLAYER_STATE3_USING_BOOSTERS && xDistToPlayer < 30.0f)) {
        BoostBreakwall_Explode(this, play);
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->dyna.actor.projectedPos, 50, NA_SE_EV_GRAVE_EXPLOSION);
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        player->speedXZ = 10.0f;
        Flags_SetSwitch(play, BOOST_BREAKWALL_GET_SWITCH(this->dyna.actor.params));
        Actor_Kill(&this->dyna.actor);
    }
}

void BoostBreakwall_Draw(Actor* thisx, PlayState* play) {
    BoostBreakwall* this = (BoostBreakwall*)thisx;

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    gDPPipeSync(POLY_OPA_DISP++);

    gSPSegment(POLY_OPA_DISP++, 0x08,
                Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, 0, 64, 64, 1, 0,
                                play->gameplayFrames, 64, 64));

    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                G_MTX_MODELVIEW | G_MTX_LOAD);

    Gfx_DrawDListOpa(play, gBoostBreakwallDL);

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}
