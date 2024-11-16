#include "reactor_scale.h"
#include "assets_custom/objects/object_reactor/object_reactor.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void ReactorScale_Init(Actor* thisx, PlayState* play);
void ReactorScale_Destroy(Actor* thisx, PlayState* play);
void ReactorScale_Update(Actor* thisx, PlayState* play);
void ReactorScale_Draw(Actor* thisx, PlayState* play);

const ActorInit Reactor_Scale_InitVars = {
    ACTOR_REACTOR_SCALE,
    ACTORCAT_BG,
    FLAGS,
    OBJECT_REACTOR,
    sizeof(ReactorScale),
    (ActorFunc)ReactorScale_Init,
    (ActorFunc)ReactorScale_Destroy,
    (ActorFunc)ReactorScale_Update,
    (ActorFunc)ReactorScale_Draw,
};

void ReactorScale_Init(Actor* thisx, PlayState* play) {
    ReactorScale* this = (ReactorScale*)thisx;
    CollisionHeader* colHeader = NULL;

    this->dyna.actor.uncullZoneScale = 1000.0f;
    this->dyna.actor.uncullZoneDownward = 2000.0f;
    this->dyna.actor.uncullZoneForward = 2000.0f;

    Actor_SetScale(&this->dyna.actor, 0.1f);

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gReactorScaleCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);
}

void ReactorScale_Destroy(Actor* thisx, PlayState* play) {
    ReactorScale* this = (ReactorScale*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

#define FUEL_LIGHT_WEIGHT_OFFSET 16.0f
#define FUEL_HEAVY_WEIGHT_OFFSET 64.0f
#define PLAYER_WEIGHT_OFFSET 32.0f

void ReactorScale_Update(Actor* thisx, PlayState* play) {
    ReactorScale* this = (ReactorScale*)thisx;
    Player* player = GET_PLAYER(play);
    Actor* actor = NULL;
    f32 offsetY = 0.0f;

    actor = play->actorCtx.actorLists[ACTORCAT_PROP].head;

    while (actor != NULL) {
        if (actor->floorBgId != this->dyna.bgId || !(actor->bgCheckFlags & BGCHECKFLAG_GROUND)) {
            actor = actor->next;
            continue;
        }

        if (actor->id == ACTOR_REACTOR_FUEL) {
            switch (actor->params) {
                case 0:
                    offsetY += FUEL_HEAVY_WEIGHT_OFFSET;
                    break;

                case 1:
                    offsetY += FUEL_LIGHT_WEIGHT_OFFSET;
                    break;
            }
        }

        actor = actor->next;
    }

    if (this->dyna.interactFlags & DYNA_INTERACT_PLAYER_ON_TOP) {
        if (player->actor.child != NULL && player->actor.child->id == ACTOR_REACTOR_FUEL) {
            switch (player->actor.child->params) {
                case 0:
                    offsetY += FUEL_HEAVY_WEIGHT_OFFSET;
                    break;

                case 1:
                    offsetY += FUEL_LIGHT_WEIGHT_OFFSET;
                    break;
            }
        }
        offsetY += PLAYER_WEIGHT_OFFSET;
    }

    if (offsetY > 168.0f) {
        offsetY = 168.0f;
    }

    Math_SmoothStepToF(&this->totalOffsetY, offsetY, 1.0f, 1.0f, 0.0f);
    
    if (this->totalOffsetY != offsetY) {
        Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BRIDGE_CLOSE - SFX_FLAG);
    }

    this->dyna.actor.world.pos.y = this->dyna.actor.home.pos.y - this->totalOffsetY;
}

void ReactorScale_Draw(Actor* thisx, PlayState* play) {
    ReactorScale* this = (ReactorScale*)thisx;

    Gfx_DrawDListOpa(play, gReactorScaleDL);
}
