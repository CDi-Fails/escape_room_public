#include "reactor_fuel_slot.h"

#define FLAGS (ACTOR_FLAG_4)

void ReactorFuelSlot_Init(Actor* thisx, PlayState* play);
void ReactorFuelSlot_Destroy(Actor* thisx, PlayState* play);
void ReactorFuelSlot_Update(Actor* thisx, PlayState* play);
void ReactorFuelSlot_Draw(Actor* thisx, PlayState* play);

void ReactorFuelSlot_IdleFull(ReactorFuelSlot* this, PlayState* play);
void ReactorFuelSlot_IdleEmpty(ReactorFuelSlot* this, PlayState* play);

const ActorInit Reactor_Fuel_Slot_InitVars = {
    ACTOR_REACTOR_FUEL_SLOT,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(ReactorFuelSlot),
    ReactorFuelSlot_Init,
    ReactorFuelSlot_Destroy,
    ReactorFuelSlot_Update,
    NULL,
};

#define REACTOR_FUEL_SLOT_GET_SWITCH(params) (params & 0x3F)

static u8 sNumSlotsFilledHeavy = 0;
static u8 sNumSlotsFilledLight = 0;

void ReactorFuelSlot_Init(Actor* thisx, PlayState* play) {
    ReactorFuelSlot* this = (ReactorFuelSlot*)thisx;

    this->loadedFuelType = -1;
    this->actionFunc = ReactorFuelSlot_IdleEmpty;
}

void ReactorFuelSlot_Destroy(Actor* thisx, PlayState* play) {
    ReactorFuelSlot* this = (ReactorFuelSlot*)thisx;
}

void ReactorFuelSlot_UpdateSlot(ReactorFuelSlot* this, PlayState* play, u8 fuelType, u8 mode) {
    switch (fuelType) {
        case REACTOR_FUEL_SLOT_HEAVY:
            switch (mode) {
                case REACTOR_FUEL_SLOT_UPDATE_MODE_ADD:
                    sNumSlotsFilledHeavy += 1;
                    break;

                case REACTOR_FUEL_SLOT_UPDATE_MODE_REMOVE:
                    sNumSlotsFilledHeavy -= 1;
                    break;
            }
            break;

        case REACTOR_FUEL_SLOT_LIGHT:
            switch (mode) {
                case REACTOR_FUEL_SLOT_UPDATE_MODE_ADD:
                    sNumSlotsFilledLight += 1;
                    break;

                case REACTOR_FUEL_SLOT_UPDATE_MODE_REMOVE:
                    sNumSlotsFilledLight -= 1;
                    break;
            }
            break;
    }

}

void ReactorFuelSlot_IdleFull(ReactorFuelSlot* this, PlayState* play) {
    if (this->loadedFuelType == -1) {
        ReactorFuelSlot_UpdateSlot(this, play, this->prevLoadedFuelType, REACTOR_FUEL_SLOT_UPDATE_MODE_REMOVE);
        this->actionFunc = ReactorFuelSlot_IdleEmpty;
    }
}

void ReactorFuelSlot_IdleEmpty(ReactorFuelSlot* this, PlayState* play) {
    if (this->loadedFuelType != -1) {
        ReactorFuelSlot_UpdateSlot(this, play, this->loadedFuelType, REACTOR_FUEL_SLOT_UPDATE_MODE_ADD);
        this->prevLoadedFuelType = this->loadedFuelType;
        this->actionFunc = ReactorFuelSlot_IdleFull;
    }
}

#define REACTOR_FUEL_SLOT_NUM 3

void ReactorFuelSlot_Update(Actor* thisx, PlayState* play) {
    ReactorFuelSlot* this = (ReactorFuelSlot*)thisx;
    u8 switchOn = Flags_GetSwitch(play, REACTOR_FUEL_SLOT_GET_SWITCH(this->actor.params));

    if (sNumSlotsFilledHeavy == 2 && sNumSlotsFilledLight == 1) {
        if (!switchOn) {
            Flags_SetSwitch(play, REACTOR_FUEL_SLOT_GET_SWITCH(this->actor.params));
        }
    } else if (Flags_GetSwitch(play, REACTOR_FUEL_SLOT_GET_SWITCH(this->actor.params))) {
        Flags_UnsetSwitch(play, REACTOR_FUEL_SLOT_GET_SWITCH(this->actor.params));
    }

    this->actionFunc(this, play);
}
