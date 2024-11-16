#include "reactor_circuit.h"
#include "assets_custom/objects/object_reactor/object_reactor.h"

#define FLAGS 0

void ReactorCircuit_Init(Actor* thisx, PlayState* play);
void ReactorCircuit_Destroy(Actor* thisx, PlayState* play);
void ReactorCircuit_Update(Actor* thisx, PlayState* play);
void ReactorCircuit_Draw(Actor* thisx, PlayState* play);

void ReactorCircuit_Circuit_Idle(ReactorCircuit* this, PlayState* play);
void ReactorCircuit_Terminal_WaitForTalk(ReactorCircuit* this, PlayState* play);

const ActorInit Reactor_Circuit_InitVars = {
    ACTOR_REACTOR_CIRCUIT,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_REACTOR,
    sizeof(ReactorCircuit),
    ReactorCircuit_Init,
    ReactorCircuit_Destroy,
    ReactorCircuit_Update,
    ReactorCircuit_Draw,
};

#define REACTOR_CIRCUIT_GET_TYPE(params) (params & 0xF)
#define REACTOR_CIRCUIT_GET_ID(params) (params >> 4 & 0xF)

// TODO: Change to final switch flag
#define REACTOR_CIRCUIT_SWITCH_FLAG 0x10

#define REACTOR_CIRCUIT_SPIN_DELAY_TIME 20
#define REACTOR_CIRCUIT_SPIN_ANGLE 0x2000

#define REACTOR_CIRCUIT_CORRECT_ANGLE 0x2000 * 3

#define REACTOR_CIRCUIT_TEXT_ID 0x71BE

static u8 sCircuitSpinningFlags[3] = { false };
static u8 sCircuitCorrectAngleFlags[3] = { false };
static s16 sCircuitSpinTimers[3] = { 0 };

void ReactorCircuit_Init(Actor* thisx, PlayState* play) {
    ReactorCircuit* this = (ReactorCircuit*)thisx;

    Actor_SetScale(&this->actor, 0.1f);

    this->type = REACTOR_CIRCUIT_GET_TYPE(this->actor.params);
    this->id = REACTOR_CIRCUIT_GET_ID(this->actor.params);

    switch (this->type) {
        case REACTOR_CIRCUIT_TYPE_CIRCUIT:
            // Unalign the circuits if switch flag is not set
            if (!Flags_GetSwitch(play, REACTOR_CIRCUIT_SWITCH_FLAG)) {
                this->actor.shape.rot.y -= 0x2000 * this->id * 3;
            } else {
                this->actor.shape.rot.y = this->actor.home.rot.y + REACTOR_CIRCUIT_CORRECT_ANGLE;
                sCircuitCorrectAngleFlags[this->id] = true;
            }
            this->actor.flags |= (ACTOR_FLAG_4 | ACTOR_FLAG_5);
            this->actionFunc = ReactorCircuit_Circuit_Idle;
            break;

        case REACTOR_CIRCUIT_TYPE_TERMINAL:
            this->actor.flags |= (ACTOR_FLAG_0 | ACTOR_FLAG_3 | ACTOR_FLAG_DRAW_FOCUS_INDICATOR);
            this->actor.textId = REACTOR_CIRCUIT_TEXT_ID;
            this->actionFunc = ReactorCircuit_Terminal_WaitForTalk;
            break;
    }
}

void ReactorCircuit_Destroy(Actor* thisx, PlayState* play) {
    ReactorCircuit* this = (ReactorCircuit*)thisx;
}

void ReactorCircuit_Terminal_FinishTalking(ReactorCircuit* this, PlayState* play) {
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = ReactorCircuit_Terminal_WaitForTalk;
    }
}

void ReactorCircuit_Terminal_Talk(ReactorCircuit* this, PlayState* play) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);

        switch (play->msgCtx.choiceIndex) {
            case 0:
                // Toggle spinning circuit
                this->toggleSpin = true;
                this->actionFunc = ReactorCircuit_Terminal_FinishTalking;
                break;
            case 1:
                this->actionFunc = ReactorCircuit_Terminal_FinishTalking;
                break;
        }
    }
}

void ReactorCircuit_Terminal_WaitForTalk(ReactorCircuit* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->toggleSpin) {
        // If we need to toggle spinning, make sure the circuit isn't in the middle of rotating so it won't stop in-between valid rotations
        if (sCircuitSpinningFlags[this->id] == true) {
            if (sCircuitSpinTimers[this->id] != 0) {
                sCircuitSpinningFlags[this->id] = false;
                this->toggleSpin = false;
            }
        } else {
            sCircuitSpinningFlags[this->id] = true;
            this->toggleSpin = false;
        }
    }

    if (this->actor.textId != 0) {
        if (Actor_TalkOfferAccepted(&this->actor, play)) {
            this->actionFunc = ReactorCircuit_Terminal_Talk;
        } else {
            Actor_OfferTalk(&this->actor, play, 50.0f);
        }
    }
}

void ReactorCircuit_Circuit_Spin(ReactorCircuit* this, PlayState* play) {
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
        return;
    }

    if (sCircuitSpinningFlags[this->id] != true) {
        this->actionFunc = ReactorCircuit_Circuit_Idle;
        return;
    }

    if (this->actor.shape.rot.y == this->actor.home.rot.y + REACTOR_CIRCUIT_CORRECT_ANGLE) {
        sCircuitCorrectAngleFlags[this->id] = true;
    } else {
        sCircuitCorrectAngleFlags[this->id] = false;
    }

    if (sCircuitCorrectAngleFlags[REACTOR_CIRCUIT_ID_UPPER] == true &&
        sCircuitCorrectAngleFlags[REACTOR_CIRCUIT_ID_CENTER] == true &&
        sCircuitCorrectAngleFlags[REACTOR_CIRCUIT_ID_LOWER] == true) {
        Flags_SetSwitch(play, REACTOR_CIRCUIT_SWITCH_FLAG);
    } else {
        Flags_UnsetSwitch(play, REACTOR_CIRCUIT_SWITCH_FLAG);
    }

    if (DECR(sCircuitSpinTimers[this->id]) == 0) {
        Actor_PlaySfx(&this->actor, NA_SE_EV_ELEVATOR_MOVE - SFX_FLAG);
        if (Math_StepToAngleS(&this->actor.shape.rot.y, this->prevRotY + REACTOR_CIRCUIT_SPIN_ANGLE, 0x200)) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_ELEVATOR_STOP);
            this->prevRotY = this->actor.shape.rot.y;
            sCircuitSpinTimers[this->id] = REACTOR_CIRCUIT_SPIN_DELAY_TIME;
        }
    }
}

void ReactorCircuit_Circuit_Idle(ReactorCircuit* this, PlayState* play) {
    if (sCircuitSpinningFlags[this->id] == true) {
        // Circuit piece alignment is activated, start spinning
        sCircuitSpinTimers[this->id] = REACTOR_CIRCUIT_SPIN_DELAY_TIME;
        this->prevRotY = this->actor.shape.rot.y;
        this->actionFunc = ReactorCircuit_Circuit_Spin;
    }
}

void ReactorCircuit_Update(Actor* thisx, PlayState* play) {
    ReactorCircuit* this = (ReactorCircuit*)thisx;

    this->actionFunc(this, play);
}

void ReactorCircuit_Draw(Actor* thisx, PlayState* play) {
    ReactorCircuit* this = (ReactorCircuit*)thisx;
    u8 animateBeam = false;

    if (this->type == REACTOR_CIRCUIT_TYPE_TERMINAL) {
        return;
    }

    // Animate beam for all pieces if all pieces are lined up
    if (Flags_GetSwitch(play, REACTOR_CIRCUIT_SWITCH_FLAG)) {
        animateBeam = true;
    }

    // Animate beam for the bottom two pieces if they're lined up, but the top isn't yet
    if (!animateBeam && (this->id == REACTOR_CIRCUIT_ID_LOWER || this->id == REACTOR_CIRCUIT_ID_CENTER)) {
        if (sCircuitCorrectAngleFlags[REACTOR_CIRCUIT_ID_LOWER] == true && sCircuitCorrectAngleFlags[REACTOR_CIRCUIT_ID_CENTER] == true) {
            animateBeam = true;
        }
    }

    OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    if (animateBeam) {
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 64, 0, 255);
        gSPSegment(POLY_OPA_DISP++, 0x08,
                   Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, (play->gameplayFrames * -4), 0, 32, 64, 1,
                                    (play->gameplayFrames * -1), 0, 32, 64));
    } else {
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 80, 80, 80, 255);
        gSPSegment(POLY_OPA_DISP++, 0x08, gEmptyDL);
    }

    switch (this->id) {
        case REACTOR_CIRCUIT_ID_UPPER:
            Gfx_DrawDListOpa(play, gReactorCircuitUpperBeamDL);
            Gfx_DrawDListOpa(play, gReactorCircuitUpperDL);
            break;

        case REACTOR_CIRCUIT_ID_CENTER:
            Gfx_DrawDListOpa(play, gReactorCircuitCenterBeamDL);
            Gfx_DrawDListOpa(play, gReactorCircuitCenterDL);
            break;

        case REACTOR_CIRCUIT_ID_LOWER:
            Gfx_DrawDListOpa(play, gReactorCircuitLowerBeamDL);
            Gfx_DrawDListOpa(play, gReactorCircuitLowerDL);
            break;
    }

    CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);
}
