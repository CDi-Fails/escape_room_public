#include "talk_spot.h"
#include "terminal.h"

#define FLAGS (ACTOR_FLAG_0 | ACTOR_FLAG_3 | ACTOR_FLAG_27 | ACTOR_FLAG_DRAW_FOCUS_INDICATOR)

void TalkSpot_Init(Actor* thisx, PlayState* play);
void TalkSpot_Destroy(Actor* thisx, PlayState* play);
void TalkSpot_Update(Actor* thisx, PlayState* play);
void TalkSpot_Draw(Actor* thisx, PlayState* play);

void TalkSpot_WaitForTalk(TalkSpot* this, PlayState* play);

const ActorInit Talk_Spot_InitVars = {
    ACTOR_TALK_SPOT,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(TalkSpot),
    TalkSpot_Init,
    TalkSpot_Destroy,
    TalkSpot_Update,
    NULL,
};

#define TALK_SPOT_GET_TEXT_ID(params) (params)

void TalkSpot_Init(Actor* thisx, PlayState* play) {
    TalkSpot* this = (TalkSpot*)thisx;

    this->actor.textId = TALK_SPOT_GET_TEXT_ID(this->actor.params);
    this->actionFunc = TalkSpot_WaitForTalk;
}

void TalkSpot_Destroy(Actor* thisx, PlayState* play) {
    TalkSpot* this = (TalkSpot*)thisx;
}

void TalkSpot_Talk(TalkSpot* this, PlayState* play) {
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = TalkSpot_WaitForTalk;
    }
}

void TalkSpot_WaitForTalk(TalkSpot* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->actor.textId != 0) {
        if (Actor_TalkOfferAccepted(&this->actor, play)) {
            this->actionFunc = TalkSpot_Talk;
        } else {
            Actor_OfferTalk(&this->actor, play, 50.0f);
        }
    }
}

void TalkSpot_Update(Actor* thisx, PlayState* play) {
    TalkSpot* this = (TalkSpot*)thisx;

    this->actionFunc(this, play);
}
