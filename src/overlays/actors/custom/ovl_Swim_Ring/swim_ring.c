#include "swim_ring.h"
#include "assets_custom/objects/object_swim_ring/object_swim_ring.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void SwimRing_Init(Actor* thisx, PlayState* play);
void SwimRing_Destroy(Actor* thisx, PlayState* play);
void SwimRing_Update(Actor* thisx, PlayState* play);
void SwimRing_Draw(Actor* thisx, PlayState* play);

const ActorInit Swim_Ring_InitVars = {
    ACTOR_SWIM_RING,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_SWIM_RING,
    sizeof(SwimRing),
    SwimRing_Init,
    SwimRing_Destroy,
    SwimRing_Update,
    SwimRing_Draw,
};

static u8 sNumRingsInRoom = 0;
static u8 sNumRingsSwamThrough = 0;

#define SWIM_RING_GET_SWITCH(params) (params & 0x3F)

void SwimRing_Init(Actor* thisx, PlayState* play) {
    SwimRing* this = (SwimRing*)thisx;
    CollisionHeader* colHeader = NULL;

    if (Flags_GetSwitch(play, SWIM_RING_GET_SWITCH(this->dyna.actor.params))) {
        Actor_Kill(&this->dyna.actor);
        return;
    }

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gSwimRingDL_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    this->alpha = 255;
    this->timer = 30;

    sNumRingsInRoom++;
}

void SwimRing_Destroy(Actor* thisx, PlayState* play) {
    SwimRing* this = (SwimRing*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

#define SWIM_RING_RADIUS 95.0f

void SwimRing_Update(Actor* thisx, PlayState* play) {
    SwimRing* this = (SwimRing*)thisx;
    Player* player = GET_PLAYER(play);
    Vec3f diff;
    f32 distXZ;

    if (this->swamThrough && this->alpha != 0) {
        this->alpha -= 10;
        if (this->alpha <= 10) {
            this->alpha = 0;
        }
    }

    if (this->alpha < 255) {
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    } else if (this->alpha == 0) {
        Actor_Kill(&this->dyna.actor);
        return;
    } else if (this->alpha == 255) {
        if (this->dyna.actor.xyzDistToPlayerSq > SQ(200.0f)) {
            DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
        } else if (play->colCtx.dyna.bgActorFlags[this->dyna.bgId] & BGACTOR_COLLISION_DISABLED) {
            DynaPoly_EnableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
        }
    }

    if (sNumRingsSwamThrough == sNumRingsInRoom) {
        if (!Flags_GetSwitch(play, SWIM_RING_GET_SWITCH(this->dyna.actor.params))) {
            if (DECR(this->timer) == 0) {
                Flags_SetSwitch(play, SWIM_RING_GET_SWITCH(this->dyna.actor.params));
            }
        }
        return;
    }

    if (this->dyna.actor.xyzDistToPlayerSq < SQ(SWIM_RING_RADIUS)) {
        // Player has successfully swam through the ring (or near it, whatever)
        if (!this->swamThrough) {
            sNumRingsSwamThrough++;
            // Don't play SFX if this is the last ring to swim through, OnePointCutscene plays it instead
            if (sNumRingsSwamThrough != sNumRingsInRoom) {
                Sfx_PlaySfxCentered(NA_SE_SY_TRE_BOX_APPEAR);
            } else {
                OnePointCutscene_AttentionSetSfx(play, &this->dyna.actor, NA_SE_SY_TRE_BOX_APPEAR);
            }
            this->swamThrough = true;
        }
    }
}

Gfx* SwimRing_SetXluRenderMode(GraphicsContext* gfxCtx) {
    Gfx* dList;
    Gfx* dListHead;

    dListHead = Graph_Alloc(gfxCtx, 2 * sizeof(Gfx));

    dList = dListHead;
    gDPSetRenderMode(dListHead++,
                     AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
                         GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA),
                     AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gSPEndDisplayList(dListHead++);

    return dList;
}

Gfx* SwimRing_EmptyDList(GraphicsContext* gfxCtx) {
    Gfx* dListHead;
    Gfx* dList;

    dList = Graph_Alloc(gfxCtx, sizeof(Gfx));

    dListHead = dList;
    gSPEndDisplayList(dListHead++);

    return dList;
}

void SwimRing_Draw(Actor* thisx, PlayState* play) {
    SwimRing* this = (SwimRing*)thisx;

    if (this->alpha == 0) {
        return;
    }

    if (this->alpha < 255) {
        OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

        gDPPipeSync(POLY_XLU_DISP++);

        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPSegment(POLY_XLU_DISP++, 0x08, SwimRing_SetXluRenderMode(play->state.gfxCtx));

        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, this->alpha);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gSPDisplayList(POLY_XLU_DISP++, gSwimRingDL);

        CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
    } else {
        OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

        gDPPipeSync(POLY_OPA_DISP++);

        gSPSegment(POLY_OPA_DISP++, 0x08, SwimRing_EmptyDList(play->state.gfxCtx));
        Gfx_SetupDL_25Opa(play->state.gfxCtx);

        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gSPDisplayList(POLY_OPA_DISP++, gSwimRingDL);

        CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    }
}
