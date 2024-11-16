#include "air_vent.h"
#include "assets_custom/objects/object_air_vent/object_air_vent.h"
#include "assets/objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Kakera/z_eff_ss_kakera.h"

#define FLAGS (ACTOR_FLAG_4)

void AirVent_Init(Actor* thisx, PlayState* play);
void AirVent_Destroy(Actor* thisx, PlayState* play);
void AirVent_Update(Actor* thisx, PlayState* play);
void AirVent_Draw(Actor* thisx, PlayState* play);

void AirVent_DoNothing(AirVent* this, PlayState* play);
void AirVent_Explode(AirVent* this, PlayState* play);
void AirVent_Idle(AirVent* this, PlayState* play);

const ActorInit Air_Vent_InitVars = {
    ACTOR_AIR_VENT,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_AIR_VENT,
    sizeof(AirVent),
    AirVent_Init,
    AirVent_Destroy,
    AirVent_Update,
    AirVent_Draw,
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
    { 120, 150, -75, { 0 } },
};

#define AIR_VENT_GET_SWITCH(params) (params & 0x3F)

void AirVent_Init(Actor* thisx, PlayState* play) {
    AirVent* this = (AirVent*)thisx;
    CollisionHeader* colHeader = NULL;

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gAirVentCrackedDL_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    if (Flags_GetSwitch(play, AIR_VENT_GET_SWITCH(this->dyna.actor.params))) {
        Actor_Kill(&this->dyna.actor);
    } else {
        Collider_InitCylinder(play, &this->collider);
        Collider_SetCylinder(play, &this->collider, &this->dyna.actor, &sCylinderInit);
        Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
        this->actionFunc = AirVent_Idle;
    }
}

void AirVent_Destroy(Actor* thisx, PlayState* play) {
    AirVent* this = (AirVent*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void AirVent_DoNothing(AirVent* this, PlayState* play) {
}

static s16 sEffectScales[] = {
    10, 9, 9, 8, 7, 5, 4, 4, 3, 2,
};

void AirVent_SpawnExplodeEffects(AirVent* this, PlayState* play) {
    Vec3f pos;
    Vec3f velocity;
    Gfx* dlist;
    s16 arg5;
    s16 scale;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEffectScales); i++) {
        pos.x = ((Rand_ZeroOne() - 0.5f) * 10.0f) + this->dyna.actor.home.pos.x;
        pos.y = ((Rand_ZeroOne() * 5.0f) + this->dyna.actor.home.pos.y);
        pos.z = ((Rand_ZeroOne() - 0.5f) * 10.0f) + this->dyna.actor.home.pos.z;
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


void AirVent_Idle(AirVent* this, PlayState* play) {
    if (this->collider.base.acFlags & AC_HIT) {
        AirVent_SpawnExplodeEffects(this, play);
        Flags_SetSwitch(play, AIR_VENT_GET_SWITCH(this->dyna.actor.params));
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->dyna.actor.world.pos, 40, NA_SE_SY_TRE_BOX_APPEAR);
        Actor_Kill(&this->dyna.actor);
    }
    Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
}

void AirVent_Update(Actor* thisx, PlayState* play) {
    AirVent* this = (AirVent*)thisx;

    this->actionFunc(this, play);
}

void AirVent_Draw(Actor* thisx, PlayState* play) {
    AirVent* this = (AirVent*)thisx;

    Gfx_DrawDListOpa(play, gAirVentCrackedDL);
}