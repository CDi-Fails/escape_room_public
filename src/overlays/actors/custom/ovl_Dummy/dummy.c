#include "dummy.h"

#define FLAGS 0

void Dummy_Init(Actor* thisx, PlayState* play);
void Dummy_Destroy(Actor* thisx, PlayState* play);
void Dummy_Update(Actor* thisx, PlayState* play);
void Dummy_Draw(Actor* thisx, PlayState* play);

const ActorInit Dummy_InitVars = {
    ACTOR_DUMMY,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(Dummy),
    Dummy_Init,
    Dummy_Destroy,
    Dummy_Update,
    Dummy_Draw,
};

void Dummy_Init(Actor* thisx, PlayState* play) {
    Dummy* this = (Dummy*)thisx;
}

void Dummy_Destroy(Actor* thisx, PlayState* play) {
    Dummy* this = (Dummy*)thisx;
}

void Dummy_Update(Actor* thisx, PlayState* play) {
    Dummy* this = (Dummy*)thisx;
}

void Dummy_Draw(Actor* thisx, PlayState* play) {
    Dummy* this = (Dummy*)thisx;

    Actor_DrawGreenCube(&this->actor, play);
}
