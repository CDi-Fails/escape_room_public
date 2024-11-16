#include "bookshelf_door.h"
#include "assets_custom/objects/object_bookshelf_door/object_bookshelf_door.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void BookshelfDoor_Init(Actor* thisx, PlayState* play);
void BookshelfDoor_Destroy(Actor* thisx, PlayState* play);
void BookshelfDoor_Update(Actor* thisx, PlayState* play);
void BookshelfDoor_Draw(Actor* thisx, PlayState* play);

void BookshelfDoor_DoNothing(BookshelfDoor* this, PlayState* play);
void BookshelfDoor_Idle(BookshelfDoor* this, PlayState* play);
void BookshelfDoor_IdleInactive(BookshelfDoor* this, PlayState* play);
void BookshelfDoor_Turn(BookshelfDoor* this, PlayState* play);

const ActorInit Bookshelf_Door_InitVars = {
    ACTOR_BOOKSHELF_DOOR,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_BOOKSHELF_DOOR,
    sizeof(BookshelfDoor),
    BookshelfDoor_Init,
    BookshelfDoor_Destroy,
    BookshelfDoor_Update,
    BookshelfDoor_Draw,
};

#define BOOKSHELF_DOOR_GET_SWITCH(params) (params & 0x3F)

void BookshelfDoor_Init(Actor* thisx, PlayState* play) {
    BookshelfDoor* this = (BookshelfDoor*)thisx;
    CollisionHeader* colHeader = NULL;

    DynaPolyActor_Init(&this->dyna, DYNA_TRANSFORM_POS);
    CollisionHeader_GetVirtual(&gBookshelfDoorCol_collisionHeader, &colHeader);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &this->dyna.actor, colHeader);

    if (Flags_GetSwitch(play, BOOKSHELF_DOOR_GET_SWITCH(this->dyna.actor.params))) {
        this->dyna.actor.shape.rot.y = this->dyna.actor.home.rot.y - DEG_TO_BINANG(90.0f);
        this->actionFunc = BookshelfDoor_IdleInactive;
    } else {
        this->dyna.actor.shape.rot.y -= 0x600;
        this->actionFunc = BookshelfDoor_Idle;
    }
}

void BookshelfDoor_Destroy(Actor* thisx, PlayState* play) {
    BookshelfDoor* this = (BookshelfDoor*)thisx;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void BookshelfDoor_IdleInactive(BookshelfDoor* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->dyna.unk_150 != 0.0f) {
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->dyna.unk_150 = 0.0f;
    }
}

void BookshelfDoor_Turn(BookshelfDoor* this, PlayState* play) {

    if (Math_StepToAngleS(&this->dyna.actor.shape.rot.y, this->dyna.actor.home.rot.y - DEG_TO_BINANG(90.0f), 0x200)) {
        this->actionFunc = BookshelfDoor_IdleInactive;
    }

    Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
}

void BookshelfDoor_Idle(BookshelfDoor* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->dyna.unk_150 > 0.0f) {
        if (DECR(this->timer) == 0) {
            Actor_PlaySfx(&this->dyna.actor, NA_SE_EV_BLOCK_BOUND);
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
            this->dyna.unk_150 = 0.0f;
            OnePointCutscene_Attention(play, &this->dyna.actor);
            Flags_SetSwitch(play, BOOKSHELF_DOOR_GET_SWITCH(this->dyna.actor.params));
            this->actionFunc = BookshelfDoor_Turn;
        }
        // Don't want to actually have anything being pushed
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->dyna.unk_150 = 0.0f;
    } else {
        this->timer = 20;
    }

    if (this->dyna.unk_150 < 0.0f) {
        // Don't want to actually have anything being pushed
        player->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        this->dyna.unk_150 = 0.0f;
    }
}

void BookshelfDoor_Update(Actor* thisx, PlayState* play) {
    BookshelfDoor* this = (BookshelfDoor*)thisx;

    this->actionFunc(this, play);
}

void BookshelfDoor_Draw(Actor* thisx, PlayState* play) {
    BookshelfDoor* this = (BookshelfDoor*)thisx;

    Gfx_DrawDListOpa(play, gBookshelfDoorDL);
}
