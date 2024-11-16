/*
 * File: z_player.c
 * Overlay: ovl_player_actor
 * Description: Link
 */

#include "ultra64.h"
#include "global.h"
#include "quake.h"

#include "overlays/actors/ovl_Bg_Heavy_Block/z_bg_heavy_block.h"
#include "overlays/actors/ovl_Demo_Kankyo/z_demo_kankyo.h"
#include "overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "overlays/actors/ovl_En_Boom/z_en_boom.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_Fish/z_en_fish.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "overlays/actors/ovl_En_Insect/z_en_insect.h"
#include "overlays/effects/ovl_Effect_Ss_Fhg_Flash/z_eff_ss_fhg_flash.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "assets/objects/object_link_child/object_link_child.h"

#include "assets_custom/misc/link_animetion/link_anims_custom.h"

// Some player animations are played at this reduced speed, for reasons yet unclear.
// This is called "adjusted" for now.
#define PLAYER_ANIM_ADJUSTED_SPEED (2.0f / 3.0f)

typedef struct {
    /* 0x00 */ u8 itemId;
    /* 0x01 */ u8 field; // various bit-packed data
    /* 0x02 */ s8 gi;    // defines the draw id and chest opening animation
    /* 0x03 */ u8 textId;
    /* 0x04 */ u16 objectId;
} GetItemEntry; // size = 0x06

#define GET_ITEM(itemId, objectId, drawId, textId, field, chestAnim) \
    { itemId, field, (chestAnim != CHEST_ANIM_SHORT ? 1 : -1) * (drawId + 1), textId, objectId }

#define CHEST_ANIM_SHORT 0
#define CHEST_ANIM_LONG 1

#define GET_ITEM_NONE \
    { ITEM_NONE, 0, 0, 0, OBJECT_INVALID }

typedef struct {
    /* 0x00 */ u8 itemId;
    /* 0x02 */ s16 actorId;
} ExplosiveInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ u8 itemId;
    /* 0x03 */ u8 itemAction;
    /* 0x04 */ u8 textId;
} BottleCatchInfo; // size = 0x06

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ s16 actorParams;
} BottleDropInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s8 damage;
    /* 0x01 */ u8 rumbleStrength;
    /* 0x02 */ u8 rumbleDuration;
    /* 0x03 */ u8 rumbleDecreaseRate;
    /* 0x04 */ u16 sfxId;
} FallImpactInfo; // size = 0x06

typedef struct {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ s16 yaw;
} SpecialRespawnInfo; // size = 0x10

typedef enum {
    /* 1 */ ANIMSFX_TYPE_1 = 1,
    /* 2 */ ANIMSFX_TYPE_2,
    /* 3 */ ANIMSFX_TYPE_3,
    /* 4 */ ANIMSFX_TYPE_4,
    /* 5 */ ANIMSFX_TYPE_5,
    /* 6 */ ANIMSFX_TYPE_6,
    /* 7 */ ANIMSFX_TYPE_7,
    /* 8 */ ANIMSFX_TYPE_8,
    /* 9 */ ANIMSFX_TYPE_9
} AnimSfxType;

#define ANIMSFX_SHIFT_TYPE(type) ((type) << 11)

#define ANIMSFX_DATA(type, frame) ((ANIMSFX_SHIFT_TYPE(type) | ((frame) & 0x7FF)))

#define ANIMSFX_GET_TYPE(data) ((data) & 0x7800)
#define ANIMSFX_GET_FRAME(data) ((data) & 0x7FF)

typedef struct {
    /* 0x00 */ u16 sfxId;
    /* 0x02 */ s16 data;
} AnimSfxEntry; // size = 0x04

typedef struct {
    /* 0x00 */ u16 swordSfx;
    /* 0x02 */ s16 voiceSfx;
} SwordPedestalSfx; // size = 0x04

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ u8 changeFrame;
} ItemChangeInfo; // size = 0x08

typedef struct {
    /* 0x00 */ LinkAnimationHeader* bottleSwingAnim;
    /* 0x04 */ LinkAnimationHeader* bottleCatchAnim;
    /* 0x08 */ u8 unk_08;
    /* 0x09 */ u8 unk_09;
} BottleSwingAnimInfo; // size = 0x0C

typedef struct {
    /* 0x00 */ LinkAnimationHeader* startAnim;
    /* 0x04 */ LinkAnimationHeader* endAnim;
    /* 0x08 */ LinkAnimationHeader* fightEndAnim;
    /* 0x0C */ u8 startFrame;
    /* 0x0D */ u8 endFrame;
} MeleeAttackAnimInfo; // size = 0x10

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ f32 riderOffsetX;
    /* 0x04 */ f32 riderOffsetZ;
} HorseMountAnimInfo; // size = 0x0C

typedef struct {
    /* 0x00 */ s8 type;
    /* 0x04 */ union {
        void* ptr;
        void (*func)(PlayState*, Player*, CsCmdActorCue*);
    };
} CutsceneModeEntry; // size = 0x08

void Player_InitItemAction(PlayState* play, Player* this, s8 itemAction);

void Player_InitDefaultIA(PlayState* play, Player* this);
void Player_InitHammerIA(PlayState* play, Player* this);
void Player_InitBowOrSlingshotIA(PlayState* play, Player* this);
void Player_InitDekuStickIA(PlayState* play, Player* this);
void Player_InitExplosiveIA(PlayState* play, Player* this);
void Player_InitHookshotIA(PlayState* play, Player* this);
void Player_InitBoomerangIA(PlayState* play, Player* this);

void Player_InitBoostersIA(PlayState* play, Player* this);

s32 Player_SetupAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 flags);

s32 Player_UpperAction_ChangeHeldItem(Player* this, PlayState* play);
s32 Player_UpperAction_Wait(Player* this, PlayState* play);
s32 Player_UpperAction_Sword(Player* this, PlayState* play);
s32 Player_UpperAction_StandingDefense(Player* this, PlayState* play);
s32 Player_UpperAction_FinishDefense(Player* this, PlayState* play);
s32 Player_UpperAction_HoldFpsItem(Player* this, PlayState* play);
s32 Player_UpperAction_ReadyFpsItemToShoot(Player* this, PlayState* play);
s32 Player_UpperAction_AimFpsItem(Player* this, PlayState* play);
s32 Player_UpperAction_FinishAimFpsItem(Player* this, PlayState* play);
s32 Player_UpperAction_CarryActor(Player* this, PlayState* play);
s32 Player_UpperAction_CarryBoomerang(Player* this, PlayState* play);
s32 Player_UpperAction_StartAimBoomerang(Player* this, PlayState* play);
s32 Player_UpperAction_AimBoomerang(Player* this, PlayState* play);
s32 Player_UpperAction_ThrowBoomerang(Player* this, PlayState* play);
s32 Player_UpperAction_WaitForBoomerang(Player* this, PlayState* play);
s32 Player_UpperAction_CatchBoomerang(Player* this, PlayState* play);

s32 Player_UpperAction_WearBoosters(Player* this, PlayState* play);

void Player_UseItem(PlayState* play, Player* this, s32 item);
void Player_SetupContextualStandStill(Player* this, PlayState* play);
s32 Player_TryThrowDekuNut(PlayState* play, Player* this);
void Player_Spawn_StandStill(PlayState* play, Player* this);
void Player_Spawn_SlowWalk(PlayState* play, Player* this);
void Player_Spawn_MoveWithEntranceSpeed(PlayState* play, Player* this);
void Player_Spawn_NoUpdateOrDraw(PlayState* play, Player* this);
void Player_Spawn_FromBlueWarp(PlayState* play, Player* this);
void Player_Spawn_FromTimeTravel(PlayState* play, Player* this);
void Player_Spawn_OpeningDoor(PlayState* play, Player* this);
void Player_Spawn_ExitingGrotto(PlayState* play, Player* this);
void Player_Spawn_WithKnockback(PlayState* play, Player* this);
void Player_Spawn_FromWarpSong(PlayState* play, Player* this);
void Player_Spawn_FromFW(PlayState* play, Player* this);
void Player_UpdateCommon(Player* this, PlayState* play, Input* input);
s32 Player_CheckNoDebugModeCombo(Player* this, PlayState* play);
void Player_BowStringPhysics(Player* this);
void Player_UpdateBunnyEars(Player* this);
void Player_Cutscene_AnimPlaybackType0(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType1(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType13(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType2(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType3(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType4(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType5(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType6(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType7(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType8(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType9(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType14(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType15(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType10(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType11(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType16(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType12(PlayState* play, Player* this, void* anim);
void Player_Cutscene_AnimPlaybackType17(PlayState* play, Player* this, void* arg2);
void Player_Cutscene_InitSwimIdle(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_SurfaceFromDive(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Idle(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_TurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitIdle(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Wait(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_TurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitEnterWarp(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_EnterWarp(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitFightStance(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_FightStance(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Unk3Update(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Unk4Update(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitSwordPedestal(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_SwordPedestal(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitWarpToSages(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_WarpToSages(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_KnockedToGround(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitPlayOcarina(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_DrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_CloseEyes(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_OpenEyes(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitGetItemInWater(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitSleeping(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Sleeping(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitSleepingRestless(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Awaken(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_GetOffBed(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitBlownBackward(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_BlownBackward(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_RaisedByWarp(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitIdle3(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Idle2(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitStop(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitSetDraw(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InspectGroundCarefully(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_StartPassOcarina(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_DrawSwordChild(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_TurnAroundSlowly(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_DesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_StepBackCautiously(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitSpinAttackIdle(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_SpinAttackIdle(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InspectWeapon(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitDoNothing(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_DoNothing(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_KnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_InitGetSwordBack(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_SwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_GetSwordBack(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_GanonKillCombo(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Finish(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_SetPosAndYaw(PlayState* play, Player* this, CsCmdActorCue* cue);
void Player_Cutscene_Unk6Update(PlayState* play, Player* this, CsCmdActorCue* cue);
int Player_IsDroppingFish(PlayState* play);
s32 Player_StartFishing(PlayState* play);
s32 Player_TryRestrainedByEnemy(PlayState* play, Player* this);
s32 Player_TryCsAction(PlayState* play, Actor* actor, s32 csAction);
void Player_SetupStandStillMorph(Player* this, PlayState* play);
s32 Player_InflictDamageAndCheckForDeath(PlayState* play, s32 damage);
void Player_StartTalkToActor(PlayState* play, Actor* actor);

void Player_SetupRolling(Player* this, PlayState* play);

void Player_Action_BattleTargetStandStill(Player* this, PlayState* play);
void Player_Action_CalmTargetStandStill(Player* this, PlayState* play);
void Player_Action_StandStill(Player* this, PlayState* play);
void Player_Action_ZParallelSidewalk(Player* this, PlayState* play);
void Player_Action_ZParallelBackwalk(Player* this, PlayState* play);
void Player_Action_BreakZParallelBackwalk(Player* this, PlayState* play);
void Player_Action_FinishBrakeZParallelBackwalk(Player* this, PlayState* play);
void Player_Action_BattleTargetSidewalk(Player* this, PlayState* play);
void Player_Action_Turn(Player* this, PlayState* play);
void Player_Action_Run(Player* this, PlayState* play);
void Player_Action_TargetRun(Player* this, PlayState* play);
void Player_Action_PlantMagicBean(Player* this, PlayState* play);
void Player_Action_BattleTargetBackwalk(Player* this, PlayState* play);
void Player_Action_FinishBattleTargetBackwalk(Player* this, PlayState* play);
void Player_Action_AimShieldCrouched(Player* this, PlayState* play);
void Player_Action_DeflectAttackWithShield(Player* this, PlayState* play);
void Player_Action_RecoverFromMinorDamage(Player* this, PlayState* play);
void Player_Action_Knockback(Player* this, PlayState* play);
void Player_Action_DownFromKnockback(Player* this, PlayState* play);
void Player_Action_GetUpFromKnockback(Player* this, PlayState* play);
void Player_Action_Die(Player* this, PlayState* play);
void Player_Action_Midair(Player* this, PlayState* play);
void Player_Action_Rolling(Player* this, PlayState* play);
void Player_Action_FallingDive(Player* this, PlayState* play);
void Player_Action_JumpSlash(Player* this, PlayState* play);
void Player_Action_ChargeSpinAttack(Player* this, PlayState* play);
void Player_Action_WalkChargingSpinAttack(Player* this, PlayState* play);
void Player_Action_SidewalkChargingSpinAttack(Player* this, PlayState* play);
void Player_Action_JumpUpToLedge(Player* this, PlayState* play);
void Player_Action_MiniCs(Player* this, PlayState* play);
void Player_Action_MiniCsMovement(Player* this, PlayState* play);
void Player_Action_OpenDoor(Player* this, PlayState* play);
void Player_Action_LiftActor(Player* this, PlayState* play);
void Player_Action_ThrowStonePillar(Player* this, PlayState* play);
void Player_Action_LiftSilverBoulder(Player* this, PlayState* play);
void Player_Action_LiftHeavyFuel(Player* this, PlayState* play);
void Player_Action_ThrowSilverBoulder(Player* this, PlayState* play);
void Player_Action_FailToLiftActor(Player* this, PlayState* play);
void Player_Action_PutDownActor(Player* this, PlayState* play);
void Player_Action_StartThrowActor(Player* this, PlayState* play);
void Player_Action_FirstPersonAiming(Player* this, PlayState* play);
void Player_Action_TalkToActor(Player* this, PlayState* play);
void Player_Action_GrabPushPullWall(Player* this, PlayState* play);
void Player_Action_PushWall(Player* this, PlayState* play);
void Player_Action_PullWall(Player* this, PlayState* play);
void Player_Action_GrabLedge(Player* this, PlayState* play);
void Player_Action_ClimbOntoLedge(Player* this, PlayState* play);
void Player_Action_ClimbingWallOrDownLedge(Player* this, PlayState* play);
void Player_Action_EndClimb(Player* this, PlayState* play);
void Player_Action_Crawling(Player* this, PlayState* play);
void Player_Action_ExitCrawlspace(Player* this, PlayState* play);
void Player_Action_RideHorse(Player* this, PlayState* play);
void Player_Action_DismountHorse(Player* this, PlayState* play);
void Player_Action_SwimIdle(Player* this, PlayState* play);
void Player_Action_SpawnSwimming(Player* this, PlayState* play);
void Player_Action_Swim(Player* this, PlayState* play);
void Player_Action_ZTargetSwim(Player* this, PlayState* play);
void Player_Action_Dive(Player* this, PlayState* play);
void Player_Action_SurfaceFromDive(Player* this, PlayState* play);
void Player_Action_DamagedSwim(Player* this, PlayState* play);
void Player_Action_Drown(Player* this, PlayState* play);
void Player_Action_PlayOcarina(Player* this, PlayState* play);
void Player_Action_ThrowDekuNut(Player* this, PlayState* play);
void Player_Action_GetItem(Player* this, PlayState* play);
void Player_Action_FinishTimeTravel(Player* this, PlayState* play);
void Player_Action_DrinkFromBottle(Player* this, PlayState* play);
void Player_Action_SwingBottle(Player* this, PlayState* play);
void Player_Action_HealWithFairy(Player* this, PlayState* play);
void Player_Action_DropItemFromBottle(Player* this, PlayState* play);
void Player_Action_PresentExchangeItem(Player* this, PlayState* play);
void Player_Action_SlipOnSlope(Player* this, PlayState* play);
void Player_Action_SetDrawAndStartCutsceneAfterTimer(Player* this, PlayState* play);
void Player_Action_AppearFromWarpSong(Player* this, PlayState* play);
void Player_Action_DescendFromBlueWarp(Player* this, PlayState* play);
void Player_Action_EnterGrotto(Player* this, PlayState* play);
void Player_Action_TryOpenDoorFromSpawn(Player* this, PlayState* play);
void Player_Action_JumpFromGrotto(Player* this, PlayState* play);
void Player_Action_ShootingGalleryPlay(Player* this, PlayState* play);
void Player_Action_FrozenInIce(Player* this, PlayState* play);
void Player_Action_StartElectricShock(Player* this, PlayState* play);
void Player_Action_MeleeWeaponAttack(Player* this, PlayState* play);
void Player_Action_MeleeWeaponRebound(Player* this, PlayState* play);
void Player_Action_ChooseFWOption(Player* this, PlayState* play);
void Player_Action_AppearFromFW(Player* this, PlayState* play);
void Player_Action_MagicSpell(Player* this, PlayState* play);
void Player_Action_PulledByHookshot(Player* this, PlayState* play);
void Player_Action_CastFishingRod(Player* this, PlayState* play);
void Player_Action_ReleaseCaughtFish(Player* this, PlayState* play);
void Player_Action_CsAction(Player* this, PlayState* play);

void Player_RequestQuake(PlayState* play, s32 speed, s32 y, s32 duration);
void Player_SetVerticalWaterSpeed(Player* this);
s32 Player_TryStartZoraSwim(PlayState* play, Player* this);
void Player_SetupZoraSwim(PlayState* play, Player* this);
void Player_ReturnToStandStill(Player* this, PlayState* play);
void Player_SpawnBoosterEffects(PlayState* play, Player* this);

void Player_Action_ZoraSwim(Player* this, PlayState* play);
void Player_Action_ZoraSwimJump(Player* this, PlayState* play);
void Player_Action_StartBoosterJump(Player* this, PlayState* play);
void Player_Action_BoosterJump(Player* this, PlayState* play);
void Player_Action_FinishBoosterJump(Player* this, PlayState* play);

// .bss part 1
static s32 sPrevSkelAnimeMoveFlags;
static s32 sCurrentMask;
static Vec3f sInteractWallCheckResult;
static Input* sControlInput;

// .data

static u8 sUpperBodyLimbCopyMap[PLAYER_LIMB_MAX] = {
    false, // PLAYER_LIMB_NONE
    false, // PLAYER_LIMB_ROOT
    false, // PLAYER_LIMB_WAIST
    false, // PLAYER_LIMB_LOWER
    false, // PLAYER_LIMB_R_THIGH
    false, // PLAYER_LIMB_R_SHIN
    false, // PLAYER_LIMB_R_FOOT
    false, // PLAYER_LIMB_L_THIGH
    false, // PLAYER_LIMB_L_SHIN
    false, // PLAYER_LIMB_L_FOOT
    true,  // PLAYER_LIMB_UPPER
    true,  // PLAYER_LIMB_HEAD
    true,  // PLAYER_LIMB_HAT
    true,  // PLAYER_LIMB_COLLAR
    true,  // PLAYER_LIMB_L_SHOULDER
    true,  // PLAYER_LIMB_L_FOREARM
    true,  // PLAYER_LIMB_L_HAND
    true,  // PLAYER_LIMB_R_SHOULDER
    true,  // PLAYER_LIMB_R_FOREARM
    true,  // PLAYER_LIMB_R_HAND
    true,  // PLAYER_LIMB_SHEATH
    true   // PLAYER_LIMB_TORSO
};

static PlayerAgeProperties sAgeProperties[] = {
    {
        56.0f,            // ceilingCheckHeight
        90.0f,            // unk_04
        1.0f,             // unk_08
        111.0f,           // unk_0C
        70.0f,            // unk_10
        79.4f,            // unk_14
        59.0f,            // unk_18
        41.0f,            // unk_1C
        19.0f,            // unk_20
        36.0f,            // unk_24
        44.8f,            // unk_28
        56.0f,            // unk_2C
        68.0f,            // unk_30
        70.0f,            // unk_34
        18.0f,            // wallCheckRadius
        15.0f,            // unk_3C
        70.0f,            // unk_40
        { 9, 4671, 359 }, // unk_44
        {
            { 8, 4694, 380 },
            { 9, 6122, 359 },
            { 8, 4694, 380 },
            { 9, 6122, 359 },
        }, // unk_4A
        {
            { 9, 6122, 359 },
            { 9, 7693, 380 },
            { 9, 6122, 359 },
            { 9, 7693, 380 },
        }, // unk_62
        {
            { 8, 4694, 380 },
            { 9, 6122, 359 },
        }, // unk_7A
        {
            { -1592, 4694, 380 },
            { -1591, 6122, 359 },
        },                                     // unk_86
        0,                                     // unk_92
        0x80,                                  // unk_94
        &gPlayerAnim_link_demo_Tbox_open,      // unk_98
        &gPlayerAnim_link_demo_back_to_past,   // unk_9C
        &gPlayerAnim_link_demo_return_to_past, // unk_A0
        &gPlayerAnim_link_normal_climb_startA, // unk_A4
        &gPlayerAnim_link_normal_climb_startB, // unk_A8
        { &gPlayerAnim_link_normal_climb_upL, &gPlayerAnim_link_normal_climb_upR, &gPlayerAnim_link_normal_Fclimb_upL,
          &gPlayerAnim_link_normal_Fclimb_upR },                                          // unk_AC
        { &gPlayerAnim_link_normal_Fclimb_sideL, &gPlayerAnim_link_normal_Fclimb_sideR }, // unk_BC
        { &gPlayerAnim_link_normal_climb_endAL, &gPlayerAnim_link_normal_climb_endAR },   // unk_C4
        { &gPlayerAnim_link_normal_climb_endBR, &gPlayerAnim_link_normal_climb_endBL },   // unk_CC
    },
    {
        40.0f,                   // ceilingCheckHeight
        60.0f,                   // unk_04
        11.0f / 17.0f,           // unk_08
        71.0f,                   // unk_0C
        50.0f,                   // unk_10
        47.0f,                   // unk_14
        39.0f,                   // unk_18
        27.0f,                   // unk_1C
        19.0f,                   // unk_20
        22.0f,                   // unk_24
        29.6f,                   // unk_28
        32.0f,                   // unk_2C
        48.0f,                   // unk_30
        70.0f * (11.0f / 17.0f), // unk_34
        14.0f,                   // wallCheckRadius
        12.0f,                   // unk_3C
        55.0f,                   // unk_40
        { -24, 3565, 876 },      // unk_44
        {
            { -24, 3474, 862 },
            { -24, 4977, 937 },
            { 8, 4694, 380 },
            { 9, 6122, 359 },
        }, // unk_4A
        {
            { -24, 4977, 937 },
            { -24, 6495, 937 },
            { 9, 6122, 359 },
            { 9, 7693, 380 },
        }, // unk_62
        {
            { 8, 4694, 380 },
            { 9, 6122, 359 },
        }, // unk_7A
        {
            { -1592, 4694, 380 },
            { -1591, 6122, 359 },
        },                                        // unk_86
        0x20,                                     // unk_92
        0,                                        // unk_94
        &gPlayerAnim_clink_demo_Tbox_open,        // unk_98
        &gPlayerAnim_clink_demo_goto_future,      // unk_9C
        &gPlayerAnim_clink_demo_return_to_future, // unk_A0
        &gPlayerAnim_clink_normal_climb_startA,   // unk_A4
        &gPlayerAnim_clink_normal_climb_startB,   // unk_A8
        { &gPlayerAnim_clink_normal_climb_upL, &gPlayerAnim_clink_normal_climb_upR, &gPlayerAnim_link_normal_Fclimb_upL,
          &gPlayerAnim_link_normal_Fclimb_upR },                                          // unk_AC
        { &gPlayerAnim_link_normal_Fclimb_sideL, &gPlayerAnim_link_normal_Fclimb_sideR }, // unk_BC
        { &gPlayerAnim_clink_normal_climb_endAL, &gPlayerAnim_clink_normal_climb_endAR }, // unk_C4
        { &gPlayerAnim_clink_normal_climb_endBR, &gPlayerAnim_clink_normal_climb_endBL }, // unk_CC
    },
};

static u32 sDebugModeFlag = false;
static f32 sControlStickMagnitude = 0.0f;
static s16 sControlStickAngle = 0;
static s16 sCameraOffsetControlStickAngle = 0;
static s32 sUpperBodyBusy = 0;
static s32 sFloorType = FLOOR_TYPE_NONE;
static f32 sWaterSpeedScale = 1.0f;
static f32 sInvertedWaterSpeedScale = 1.0f;
static u32 sTouchedWallFlags = 0;
static u32 sConveyorSpeed = CONVEYOR_SPEED_DISABLED;
static s16 sIsFloorConveyor = false;
static s16 sConveyorYaw = 0;
static f32 sYDistToFloor = 0.0f;
static s32 sPrevFloorProperty = FLOOR_PROPERTY_0; // floor property from the previous frame
static s32 sShapeYawToTouchedWall = 0;
static s32 sWorldYawToTouchedWall = 0;
static s16 sFloorShapePitch = 0;
static s32 sUseHeldItem = false; // When true, the current held item is used. Is reset to false every frame.
static s32 sHeldItemButtonIsHeldDown = false; // Indicates if the button for the current held item is held down.

static u16 sInterruptableSfx[] = {
    NA_SE_VO_LI_SWEAT,
    NA_SE_VO_LI_SNEEZE,
    NA_SE_VO_LI_RELAX,
    NA_SE_VO_LI_FALL_L,
};

static GetItemEntry sGetItemTable[] = {
    // GI_BOMBS_5
    GET_ITEM(ITEM_BOMBS_5, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_5
    GET_ITEM(ITEM_DEKU_NUTS_5, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_10
    GET_ITEM(ITEM_BOMBCHU, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOW
    GET_ITEM(ITEM_BOW, OBJECT_GI_BOW, GID_BOW, 0x31, 0x80, CHEST_ANIM_LONG),
    // GI_SLINGSHOT
    GET_ITEM(ITEM_SLINGSHOT, OBJECT_GI_PACHINKO, GID_SLINGSHOT, 0x30, 0x80, CHEST_ANIM_LONG),
    // GI_BOOMERANG
    GET_ITEM(ITEM_BOOMERANG, OBJECT_GI_BOOMERANG, GID_BOOMERANG, 0x35, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICKS_1
    GET_ITEM(ITEM_DEKU_STICK, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_HOOKSHOT
    GET_ITEM(ITEM_HOOKSHOT, OBJECT_GI_HOOKSHOT, GID_HOOKSHOT, 0x36, 0x80, CHEST_ANIM_LONG),
    // GI_LONGSHOT
    GET_ITEM(ITEM_LONGSHOT, OBJECT_GI_HOOKSHOT, GID_LONGSHOT, 0x4F, 0x80, CHEST_ANIM_LONG),
    // GI_LENS_OF_TRUTH
    GET_ITEM(ITEM_LENS_OF_TRUTH, OBJECT_GI_GLASSES, GID_LENS_OF_TRUTH, 0x39, 0x80, CHEST_ANIM_LONG),
    // GI_ZELDAS_LETTER
    GET_ITEM(ITEM_ZELDAS_LETTER, OBJECT_GI_LETTER, GID_ZELDAS_LETTER, 0x69, 0x80, CHEST_ANIM_LONG),
    // GI_OCARINA_OF_TIME
    GET_ITEM(ITEM_OCARINA_OF_TIME, OBJECT_GI_OCARINA, GID_OCARINA_OF_TIME, 0x3A, 0x80, CHEST_ANIM_LONG),
    // GI_HAMMER
    GET_ITEM(ITEM_HAMMER, OBJECT_GI_HAMMER, GID_HAMMER, 0x38, 0x80, CHEST_ANIM_LONG),
    // GI_COJIRO
    GET_ITEM(ITEM_COJIRO, OBJECT_GI_NIWATORI, GID_COJIRO, 0x02, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_EMPTY
    GET_ITEM(ITEM_BOTTLE_EMPTY, OBJECT_GI_BOTTLE, GID_BOTTLE_EMPTY, 0x42, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_RED
    GET_ITEM(ITEM_BOTTLE_POTION_RED, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_RED, 0x43, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_GREEN
    GET_ITEM(ITEM_BOTTLE_POTION_GREEN, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_GREEN, 0x44, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_BLUE
    GET_ITEM(ITEM_BOTTLE_POTION_BLUE, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_BLUE, 0x45, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_FAIRY
    GET_ITEM(ITEM_BOTTLE_FAIRY, OBJECT_GI_BOTTLE, GID_BOTTLE_EMPTY, 0x46, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_MILK_FULL
    GET_ITEM(ITEM_BOTTLE_MILK_FULL, OBJECT_GI_MILK, GID_BOTTLE_MILK_FULL, 0x98, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_RUTOS_LETTER
    GET_ITEM(ITEM_BOTTLE_RUTOS_LETTER, OBJECT_GI_BOTTLE_LETTER, GID_BOTTLE_RUTOS_LETTER, 0x99, 0x80, CHEST_ANIM_LONG),
    // GI_MAGIC_BEAN
    GET_ITEM(ITEM_MAGIC_BEAN, OBJECT_GI_BEAN, GID_MAGIC_BEAN, 0x48, 0x80, CHEST_ANIM_SHORT),
    // GI_MASK_SKULL
    GET_ITEM(ITEM_MASK_SKULL, OBJECT_GI_SKJ_MASK, GID_MASK_SKULL, 0x10, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_SPOOKY
    GET_ITEM(ITEM_MASK_SPOOKY, OBJECT_GI_REDEAD_MASK, GID_MASK_SPOOKY, 0x11, 0x80, CHEST_ANIM_LONG),
    // GI_CHICKEN
    GET_ITEM(ITEM_CHICKEN, OBJECT_GI_NIWATORI, GID_CUCCO, 0x48, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_KEATON
    GET_ITEM(ITEM_MASK_KEATON, OBJECT_GI_KI_TAN_MASK, GID_MASK_KEATON, 0x12, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_BUNNY_HOOD
    GET_ITEM(ITEM_MASK_BUNNY_HOOD, OBJECT_GI_RABIT_MASK, GID_MASK_BUNNY_HOOD, 0x13, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_TRUTH
    GET_ITEM(ITEM_MASK_TRUTH, OBJECT_GI_TRUTH_MASK, GID_MASK_TRUTH, 0x17, 0x80, CHEST_ANIM_LONG),
    // GI_POCKET_EGG
    GET_ITEM(ITEM_POCKET_EGG, OBJECT_GI_BOOSTERS, GID_EGG, 0x01, 0x80, CHEST_ANIM_LONG),
    // GI_POCKET_CUCCO
    GET_ITEM(ITEM_POCKET_CUCCO, OBJECT_GI_NIWATORI, GID_CUCCO, 0x48, 0x80, CHEST_ANIM_LONG),
    // GI_ODD_MUSHROOM
    GET_ITEM(ITEM_ODD_MUSHROOM, OBJECT_GI_MUSHROOM, GID_ODD_MUSHROOM, 0x03, 0x80, CHEST_ANIM_LONG),
    // GI_ODD_POTION
    GET_ITEM(ITEM_ODD_POTION, OBJECT_GI_POWDER, GID_ODD_POTION, 0x04, 0x80, CHEST_ANIM_LONG),
    // GI_POACHERS_SAW
    GET_ITEM(ITEM_POACHERS_SAW, OBJECT_GI_SAW, GID_POACHERS_SAW, 0x05, 0x80, CHEST_ANIM_LONG),
    // GI_BROKEN_GORONS_SWORD
    GET_ITEM(ITEM_BROKEN_GORONS_SWORD, OBJECT_GI_BROKENSWORD, GID_BROKEN_GORONS_SWORD, 0x08, 0x80, CHEST_ANIM_LONG),
    // GI_PRESCRIPTION
    GET_ITEM(ITEM_PRESCRIPTION, OBJECT_GI_PRESCRIPTION, GID_PRESCRIPTION, 0x09, 0x80, CHEST_ANIM_LONG),
    // GI_EYEBALL_FROG
    GET_ITEM(ITEM_EYEBALL_FROG, OBJECT_GI_FROG, GID_EYEBALL_FROG, 0x0D, 0x80, CHEST_ANIM_LONG),
    // GI_EYE_DROPS
    GET_ITEM(ITEM_EYE_DROPS, OBJECT_GI_EYE_LOTION, GID_EYE_DROPS, 0x0E, 0x80, CHEST_ANIM_LONG),
    // GI_CLAIM_CHECK
    GET_ITEM(ITEM_CLAIM_CHECK, OBJECT_GI_TICKETSTONE, GID_CLAIM_CHECK, 0x0A, 0x80, CHEST_ANIM_LONG),
    // GI_SWORD_KOKIRI
    GET_ITEM(ITEM_SWORD_KOKIRI, OBJECT_GI_SWORD_1, GID_SWORD_KOKIRI, 0xA4, 0x80, CHEST_ANIM_LONG),
    // GI_SWORD_KNIFE
    GET_ITEM(ITEM_SWORD_BIGGORON, OBJECT_GI_LONGSWORD, GID_SWORD_BIGGORON, 0x4B, 0x80, CHEST_ANIM_LONG),
    // GI_SHIELD_DEKU
    GET_ITEM(ITEM_SHIELD_DEKU, OBJECT_GI_SHIELD_1, GID_SHIELD_DEKU, 0x4C, 0xA0, CHEST_ANIM_SHORT),
    // GI_SHIELD_HYLIAN
    GET_ITEM(ITEM_SHIELD_HYLIAN, OBJECT_GI_SHIELD_2, GID_SHIELD_HYLIAN, 0x4D, 0xA0, CHEST_ANIM_SHORT),
    // GI_SHIELD_MIRROR
    GET_ITEM(ITEM_SHIELD_MIRROR, OBJECT_GI_MIRROR_SHIELD, GID_SHIELD_MIRROR, 0x4E, 0x80, CHEST_ANIM_LONG),
    // GI_TUNIC_GORON
    GET_ITEM(ITEM_TUNIC_GORON, OBJECT_GI_HEATGEAR, GID_TUNIC_GORON, 0x50, 0xA0, CHEST_ANIM_LONG),
    // GI_TUNIC_ZORA
    GET_ITEM(ITEM_TUNIC_ZORA, OBJECT_GI_CLOTHES, GID_TUNIC_ZORA, 0x51, 0xA0, CHEST_ANIM_LONG),
    // GI_BOOTS_IRON
    GET_ITEM(ITEM_BOOTS_IRON, OBJECT_GI_ZORA_APPARATUS, GID_BOOTS_IRON, 0x53, 0x80, CHEST_ANIM_LONG),
    // GI_BOOTS_HOVER
    GET_ITEM(ITEM_BOOTS_HOVER, OBJECT_GI_HOVERBOOTS, GID_BOOTS_HOVER, 0x54, 0x80, CHEST_ANIM_LONG),
    // GI_QUIVER_40
    GET_ITEM(ITEM_QUIVER_40, OBJECT_GI_ARROWCASE, GID_QUIVER_40, 0x56, 0x80, CHEST_ANIM_LONG),
    // GI_QUIVER_50
    GET_ITEM(ITEM_QUIVER_50, OBJECT_GI_ARROWCASE, GID_QUIVER_50, 0x57, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_20
    GET_ITEM(ITEM_BOMB_BAG_20, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_20, 0x58, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_30
    GET_ITEM(ITEM_BOMB_BAG_30, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_30, 0x59, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_40
    GET_ITEM(ITEM_BOMB_BAG_40, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_40, 0x5A, 0x80, CHEST_ANIM_LONG),
    // GI_SILVER_GAUNTLETS
    GET_ITEM(ITEM_STRENGTH_SILVER_GAUNTLETS, OBJECT_GI_GLOVES, GID_SILVER_GAUNTLETS, 0x5B, 0x80, CHEST_ANIM_LONG),
    // GI_GOLD_GAUNTLETS
    GET_ITEM(ITEM_STRENGTH_GOLD_GAUNTLETS, OBJECT_GI_GLOVES, GID_GOLD_GAUNTLETS, 0x5C, 0x80, CHEST_ANIM_LONG),
    // GI_SCALE_SILVER
    GET_ITEM(ITEM_SCALE_SILVER, OBJECT_GI_SCALE, GID_SCALE_SILVER, 0xCD, 0x80, CHEST_ANIM_LONG),
    // GI_SCALE_GOLDEN
    GET_ITEM(ITEM_SCALE_GOLDEN, OBJECT_GI_SCALE, GID_SCALE_GOLDEN, 0xCE, 0x80, CHEST_ANIM_LONG),
    // GI_STONE_OF_AGONY
    GET_ITEM(ITEM_STONE_OF_AGONY, OBJECT_GI_MAP, GID_STONE_OF_AGONY, 0x68, 0x80, CHEST_ANIM_LONG),
    // GI_GERUDOS_CARD
    GET_ITEM(ITEM_GERUDOS_CARD, OBJECT_GI_GERUDO, GID_GERUDOS_CARD, 0x7B, 0x80, CHEST_ANIM_LONG),
    // GI_OCARINA_FAIRY
    GET_ITEM(ITEM_OCARINA_FAIRY, OBJECT_GI_OCARINA_0, GID_OCARINA_FAIRY, 0x3A, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_SEEDS_5
    GET_ITEM(ITEM_DEKU_SEEDS, OBJECT_GI_SEED, GID_DEKU_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    // GI_HEART_CONTAINER
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    // GI_HEART_PIECE
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xC2, 0x80, CHEST_ANIM_LONG),
    // GI_BOSS_KEY
    GET_ITEM(ITEM_DUNGEON_BOSS_KEY, OBJECT_GI_BOSSKEY, GID_BOSS_KEY, 0xC7, 0x80, CHEST_ANIM_LONG),
    // GI_COMPASS
    GET_ITEM(ITEM_DUNGEON_COMPASS, OBJECT_GI_COMPASS, GID_COMPASS, 0x67, 0x80, CHEST_ANIM_LONG),
    // GI_DUNGEON_MAP
    GET_ITEM(ITEM_DUNGEON_MAP, OBJECT_GI_MAP, GID_DUNGEON_MAP, 0x66, 0x80, CHEST_ANIM_LONG),
    // GI_SMALL_KEY
    GET_ITEM(ITEM_SMALL_KEY, OBJECT_GI_KEY, GID_SMALL_KEY, 0x60, 0x80, CHEST_ANIM_SHORT),
    // GI_MAGIC_JAR_SMALL
    GET_ITEM(ITEM_MAGIC_JAR_SMALL, OBJECT_GI_MAGICPOT, GID_MAGIC_JAR_SMALL, 0x52, 0x6F, CHEST_ANIM_SHORT),
    // GI_MAGIC_JAR_LARGE
    GET_ITEM(ITEM_MAGIC_JAR_BIG, OBJECT_GI_MAGICPOT, GID_MAGIC_JAR_LARGE, 0x52, 0x6E, CHEST_ANIM_SHORT),
    // GI_WALLET_ADULT
    GET_ITEM(ITEM_ADULTS_WALLET, OBJECT_GI_PURSE, GID_WALLET_ADULT, 0x5E, 0x80, CHEST_ANIM_LONG),
    // GI_WALLET_GIANT
    GET_ITEM(ITEM_GIANTS_WALLET, OBJECT_GI_PURSE, GID_WALLET_GIANT, 0x5F, 0x80, CHEST_ANIM_LONG),
    // GI_WEIRD_EGG
    GET_ITEM(ITEM_WEIRD_EGG, OBJECT_GI_EGG, GID_EGG, 0x9A, 0x80, CHEST_ANIM_LONG),
    // GI_RECOVERY_HEART
    GET_ITEM(ITEM_RECOVERY_HEART, OBJECT_GI_HEART, GID_RECOVERY_HEART, 0x55, 0x80, CHEST_ANIM_LONG),
    // GI_ARROWS_5
    GET_ITEM(ITEM_ARROWS_5, OBJECT_GI_ARROW, GID_ARROWS_5, 0xE6, 0x48, CHEST_ANIM_SHORT),
    // GI_ARROWS_10
    GET_ITEM(ITEM_ARROWS_10, OBJECT_GI_ARROW, GID_ARROWS_10, 0xE6, 0x49, CHEST_ANIM_SHORT),
    // GI_ARROWS_30
    GET_ITEM(ITEM_ARROWS_30, OBJECT_GI_ARROW, GID_ARROWS_30, 0xE6, 0x4A, CHEST_ANIM_SHORT),
    // GI_RUPEE_GREEN
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0x6F, 0x00, CHEST_ANIM_SHORT),
    // GI_RUPEE_BLUE
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xCC, 0x01, CHEST_ANIM_SHORT),
    // GI_RUPEE_RED
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF0, 0x02, CHEST_ANIM_SHORT),
    // GI_HEART_CONTAINER_2
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    // GI_MILK
    GET_ITEM(ITEM_MILK, OBJECT_GI_MILK, GID_BOTTLE_MILK_FULL, 0x98, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_GORON
    GET_ITEM(ITEM_MASK_GORON, OBJECT_GI_GOLONMASK, GID_MASK_GORON, 0x14, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_ZORA
    GET_ITEM(ITEM_MASK_ZORA, OBJECT_GI_ZORAMASK, GID_MASK_ZORA, 0x15, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_GERUDO
    GET_ITEM(ITEM_MASK_GERUDO, OBJECT_GI_GERUDOMASK, GID_MASK_GERUDO, 0x16, 0x80, CHEST_ANIM_LONG),
    // GI_GORONS_BRACELET
    GET_ITEM(ITEM_STRENGTH_GORONS_BRACELET, OBJECT_GI_BRACELET, GID_GORONS_BRACELET, 0x79, 0x80, CHEST_ANIM_LONG),
    // GI_RUPEE_PURPLE
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF1, 0x14, CHEST_ANIM_SHORT),
    // GI_RUPEE_GOLD
    GET_ITEM(ITEM_RUPEE_GOLD, OBJECT_GI_RUPY, GID_RUPEE_GOLD, 0xF2, 0x13, CHEST_ANIM_SHORT),
    // GI_SWORD_BIGGORON
    GET_ITEM(ITEM_SWORD_BIGGORON, OBJECT_GI_LONGSWORD, GID_SWORD_BIGGORON, 0x0C, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_FIRE
    GET_ITEM(ITEM_ARROW_FIRE, OBJECT_GI_M_ARROW, GID_ARROW_FIRE, 0x70, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_ICE
    GET_ITEM(ITEM_ARROW_ICE, OBJECT_GI_M_ARROW, GID_ARROW_ICE, 0x71, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_LIGHT
    GET_ITEM(ITEM_ARROW_LIGHT, OBJECT_GI_M_ARROW, GID_ARROW_LIGHT, 0x72, 0x80, CHEST_ANIM_LONG),
    // GI_SKULL_TOKEN
    GET_ITEM(ITEM_SKULL_TOKEN, OBJECT_GI_SUTARU, GID_SKULL_TOKEN, 0xB4, 0x80, CHEST_ANIM_SHORT),
    // GI_DINS_FIRE
    GET_ITEM(ITEM_DINS_FIRE, OBJECT_GI_GODDESS, GID_DINS_FIRE, 0xAD, 0x80, CHEST_ANIM_LONG),
    // GI_FARORES_WIND
    GET_ITEM(ITEM_FARORES_WIND, OBJECT_GI_GODDESS, GID_FARORES_WIND, 0xAE, 0x80, CHEST_ANIM_LONG),
    // GI_NAYRUS_LOVE
    GET_ITEM(ITEM_NAYRUS_LOVE, OBJECT_GI_GODDESS, GID_NAYRUS_LOVE, 0xAF, 0x80, CHEST_ANIM_LONG),
    // GI_BULLET_BAG_30
    GET_ITEM(ITEM_BULLET_BAG_30, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    // GI_BULLET_BAG_40
    GET_ITEM(ITEM_BULLET_BAG_40, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICKS_5
    GET_ITEM(ITEM_DEKU_STICKS_5, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_DEKU_STICKS_10
    GET_ITEM(ITEM_DEKU_STICKS_10, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_5_2
    GET_ITEM(ITEM_DEKU_NUTS_5, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_10
    GET_ITEM(ITEM_DEKU_NUTS_10, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_BOMBS_1
    GET_ITEM(ITEM_BOMB, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_10
    GET_ITEM(ITEM_BOMBS_10, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_20
    GET_ITEM(ITEM_BOMBS_20, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_30
    GET_ITEM(ITEM_BOMBS_30, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_DEKU_SEEDS_30
    GET_ITEM(ITEM_DEKU_SEEDS_30, OBJECT_GI_SEED, GID_DEKU_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_5
    GET_ITEM(ITEM_BOMBCHUS_5, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_20
    GET_ITEM(ITEM_BOMBCHUS_20, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOTTLE_FISH
    GET_ITEM(ITEM_BOTTLE_FISH, OBJECT_GI_FISH, GID_FISH, 0x47, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BUGS
    GET_ITEM(ITEM_BOTTLE_BUG, OBJECT_GI_INSECT, GID_BUG, 0x7A, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BLUE_FIRE
    GET_ITEM(ITEM_BOTTLE_BLUE_FIRE, OBJECT_GI_FIRE, GID_BLUE_FIRE, 0x5D, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POE
    GET_ITEM(ITEM_BOTTLE_POE, OBJECT_GI_GHOST, GID_POE, 0x97, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BIG_POE
    GET_ITEM(ITEM_BOTTLE_BIG_POE, OBJECT_GI_GHOST, GID_BIG_POE, 0xF9, 0x80, CHEST_ANIM_LONG),
    // GI_DOOR_KEY
    GET_ITEM(ITEM_SMALL_KEY, OBJECT_GI_KEY, GID_SMALL_KEY, 0xF3, 0x80, CHEST_ANIM_SHORT),
    // GI_RUPEE_GREEN_LOSE
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0xF4, 0x00, CHEST_ANIM_SHORT),
    // GI_RUPEE_BLUE_LOSE
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xF5, 0x01, CHEST_ANIM_SHORT),
    // GI_RUPEE_RED_LOSE
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF6, 0x02, CHEST_ANIM_SHORT),
    // GI_RUPEE_PURPLE_LOSE
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF7, 0x14, CHEST_ANIM_SHORT),
    // GI_HEART_PIECE_WIN
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xFA, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICK_UPGRADE_20
    GET_ITEM(ITEM_DEKU_STICK_UPGRADE_20, OBJECT_GI_STICK, GID_DEKU_STICK, 0x90, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_STICK_UPGRADE_30
    GET_ITEM(ITEM_DEKU_STICK_UPGRADE_30, OBJECT_GI_STICK, GID_DEKU_STICK, 0x91, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_NUT_UPGRADE_30
    GET_ITEM(ITEM_DEKU_NUT_UPGRADE_30, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0xA7, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_NUT_UPGRADE_40
    GET_ITEM(ITEM_DEKU_NUT_UPGRADE_40, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0xA8, 0x80, CHEST_ANIM_SHORT),
    // GI_BULLET_BAG_50
    GET_ITEM(ITEM_BULLET_BAG_50, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG_50, 0x6C, 0x80, CHEST_ANIM_LONG),
    // GI_ICE_TRAP
    GET_ITEM_NONE,
    // GI_ZORA_AMULET
    GET_ITEM(ITEM_BOOTS_IRON, OBJECT_GI_CLOTHES, GID_TUNIC_ZORA, 0x6C, 0x80, CHEST_ANIM_LONG),
};

#define GET_PLAYER_ANIM(group, type) sPlayerAnimations[group * PLAYER_ANIMTYPE_MAX + type]

static LinkAnimationHeader* sPlayerAnimations[PLAYER_ANIMGROUP_MAX * PLAYER_ANIMTYPE_MAX] = {
    /* PLAYER_ANIMGROUP_wait */
    &gPlayerAnim_link_normal_wait_free,
    &gPlayerAnim_link_normal_wait,
    &gPlayerAnim_link_normal_wait,
    &gPlayerAnim_link_fighter_wait_long,
    &gPlayerAnim_link_normal_wait_free,
    &gPlayerAnim_link_normal_wait_free,
    /* PLAYER_ANIMGROUP_walk */
    &gPlayerAnim_link_normal_walk_free,
    &gPlayerAnim_link_normal_walk,
    &gPlayerAnim_link_normal_walk,
    &gPlayerAnim_link_fighter_walk_long,
    &gPlayerAnim_link_normal_walk_free,
    &gPlayerAnim_link_normal_walk_free,
    /* PLAYER_ANIMGROUP_run */
    &gPlayerAnim_link_normal_run_free,
    &gPlayerAnim_link_fighter_run,
    &gPlayerAnim_link_normal_run,
    &gPlayerAnim_link_fighter_run_long,
    &gPlayerAnim_link_normal_run_free,
    &gPlayerAnim_link_normal_run_free,
    /* PLAYER_ANIMGROUP_damage_run */
    &gPlayerAnim_link_normal_damage_run_free,
    &gPlayerAnim_link_fighter_damage_run,
    &gPlayerAnim_link_normal_damage_run_free,
    &gPlayerAnim_link_fighter_damage_run_long,
    &gPlayerAnim_link_normal_damage_run_free,
    &gPlayerAnim_link_normal_damage_run_free,
    /* PLAYER_ANIMGROUP_heavy_run */
    &gPlayerAnim_link_normal_heavy_run_free,
    &gPlayerAnim_link_normal_heavy_run,
    &gPlayerAnim_link_normal_heavy_run_free,
    &gPlayerAnim_link_fighter_heavy_run_long,
    &gPlayerAnim_link_normal_heavy_run_free,
    &gPlayerAnim_link_normal_heavy_run_free,
    /* PLAYER_ANIMGROUP_waitL */
    &gPlayerAnim_link_normal_waitL_free,
    &gPlayerAnim_link_anchor_waitL,
    &gPlayerAnim_link_anchor_waitL,
    &gPlayerAnim_link_fighter_waitL_long,
    &gPlayerAnim_link_normal_waitL_free,
    &gPlayerAnim_link_normal_waitL_free,
    /* PLAYER_ANIMGROUP_waitR */
    &gPlayerAnim_link_normal_waitR_free,
    &gPlayerAnim_link_anchor_waitR,
    &gPlayerAnim_link_anchor_waitR,
    &gPlayerAnim_link_fighter_waitR_long,
    &gPlayerAnim_link_normal_waitR_free,
    &gPlayerAnim_link_normal_waitR_free,
    /* PLAYER_ANIMGROUP_wait2waitR */
    &gPlayerAnim_link_fighter_wait2waitR_long,
    &gPlayerAnim_link_normal_wait2waitR,
    &gPlayerAnim_link_normal_wait2waitR,
    &gPlayerAnim_link_fighter_wait2waitR_long,
    &gPlayerAnim_link_fighter_wait2waitR_long,
    &gPlayerAnim_link_fighter_wait2waitR_long,
    /* PLAYER_ANIMGROUP_normal2fighter */
    &gPlayerAnim_link_normal_normal2fighter_free,
    &gPlayerAnim_link_fighter_normal2fighter,
    &gPlayerAnim_link_fighter_normal2fighter,
    &gPlayerAnim_link_normal_normal2fighter_free,
    &gPlayerAnim_link_normal_normal2fighter_free,
    &gPlayerAnim_link_normal_normal2fighter_free,
    /* PLAYER_ANIMGROUP_doorA_free */
    &gPlayerAnim_link_demo_doorA_link_free,
    &gPlayerAnim_link_demo_doorA_link,
    &gPlayerAnim_link_demo_doorA_link,
    &gPlayerAnim_link_demo_doorA_link_free,
    &gPlayerAnim_link_demo_doorA_link_free,
    &gPlayerAnim_link_demo_doorA_link_free,
    /* PLAYER_ANIMGROUP_doorA */
    &gPlayerAnim_clink_demo_doorA_link,
    &gPlayerAnim_clink_demo_doorA_link,
    &gPlayerAnim_clink_demo_doorA_link,
    &gPlayerAnim_clink_demo_doorA_link,
    &gPlayerAnim_clink_demo_doorA_link,
    &gPlayerAnim_clink_demo_doorA_link,
    /* PLAYER_ANIMGROUP_doorB_free */
    &gPlayerAnim_link_demo_doorB_link_free,
    &gPlayerAnim_link_demo_doorB_link,
    &gPlayerAnim_link_demo_doorB_link,
    &gPlayerAnim_link_demo_doorB_link_free,
    &gPlayerAnim_link_demo_doorB_link_free,
    &gPlayerAnim_link_demo_doorB_link_free,
    /* PLAYER_ANIMGROUP_doorB */
    &gPlayerAnim_clink_demo_doorB_link,
    &gPlayerAnim_clink_demo_doorB_link,
    &gPlayerAnim_clink_demo_doorB_link,
    &gPlayerAnim_clink_demo_doorB_link,
    &gPlayerAnim_clink_demo_doorB_link,
    &gPlayerAnim_clink_demo_doorB_link,
    /* PLAYER_ANIMGROUP_carryB */
    &gPlayerAnim_link_normal_carryB_free,
    &gPlayerAnim_link_normal_carryB,
    &gPlayerAnim_link_normal_carryB,
    &gPlayerAnim_link_normal_carryB_free,
    &gPlayerAnim_link_normal_carryB_free,
    &gPlayerAnim_link_normal_carryB_free,
    /* PLAYER_ANIMGROUP_landing */
    &gPlayerAnim_link_normal_landing_free,
    &gPlayerAnim_link_normal_landing,
    &gPlayerAnim_link_normal_landing,
    &gPlayerAnim_link_normal_landing_free,
    &gPlayerAnim_link_normal_landing_free,
    &gPlayerAnim_link_normal_landing_free,
    /* PLAYER_ANIMGROUP_short_landing */
    &gPlayerAnim_link_normal_short_landing_free,
    &gPlayerAnim_link_normal_short_landing,
    &gPlayerAnim_link_normal_short_landing,
    &gPlayerAnim_link_normal_short_landing_free,
    &gPlayerAnim_link_normal_short_landing_free,
    &gPlayerAnim_link_normal_short_landing_free,
    /* PLAYER_ANIMGROUP_landing_roll */
    &gPlayerAnim_link_normal_landing_roll_free,
    &gPlayerAnim_link_normal_landing_roll,
    &gPlayerAnim_link_normal_landing_roll,
    &gPlayerAnim_link_fighter_landing_roll_long,
    &gPlayerAnim_link_normal_landing_roll_free,
    &gPlayerAnim_link_normal_landing_roll_free,
    /* PLAYER_ANIMGROUP_hip_down */
    &gPlayerAnim_link_normal_hip_down_free,
    &gPlayerAnim_link_normal_hip_down,
    &gPlayerAnim_link_normal_hip_down,
    &gPlayerAnim_link_normal_hip_down_long,
    &gPlayerAnim_link_normal_hip_down_free,
    &gPlayerAnim_link_normal_hip_down_free,
    /* PLAYER_ANIMGROUP_walk_endL */
    &gPlayerAnim_link_normal_walk_endL_free,
    &gPlayerAnim_link_normal_walk_endL,
    &gPlayerAnim_link_normal_walk_endL,
    &gPlayerAnim_link_fighter_walk_endL_long,
    &gPlayerAnim_link_normal_walk_endL_free,
    &gPlayerAnim_link_normal_walk_endL_free,
    /* PLAYER_ANIMGROUP_walk_endR */
    &gPlayerAnim_link_normal_walk_endR_free,
    &gPlayerAnim_link_normal_walk_endR,
    &gPlayerAnim_link_normal_walk_endR,
    &gPlayerAnim_link_fighter_walk_endR_long,
    &gPlayerAnim_link_normal_walk_endR_free,
    &gPlayerAnim_link_normal_walk_endR_free,
    /* PLAYER_ANIMGROUP_defense */
    &gPlayerAnim_link_normal_defense_free,
    &gPlayerAnim_link_normal_defense,
    &gPlayerAnim_link_normal_defense,
    &gPlayerAnim_link_normal_defense_free,
    &gPlayerAnim_link_bow_defense,
    &gPlayerAnim_link_normal_defense_free,
    /* PLAYER_ANIMGROUP_defense_wait */
    &gPlayerAnim_link_normal_defense_wait_free,
    &gPlayerAnim_link_normal_defense_wait,
    &gPlayerAnim_link_normal_defense_wait,
    &gPlayerAnim_link_normal_defense_wait_free,
    &gPlayerAnim_link_bow_defense_wait,
    &gPlayerAnim_link_normal_defense_wait_free,
    /* PLAYER_ANIMGROUP_defense_end */
    &gPlayerAnim_link_normal_defense_end_free,
    &gPlayerAnim_link_normal_defense_end,
    &gPlayerAnim_link_normal_defense_end,
    &gPlayerAnim_link_normal_defense_end_free,
    &gPlayerAnim_link_normal_defense_end_free,
    &gPlayerAnim_link_normal_defense_end_free,
    /* PLAYER_ANIMGROUP_side_walk */
    &gPlayerAnim_link_normal_side_walk_free,
    &gPlayerAnim_link_normal_side_walk,
    &gPlayerAnim_link_normal_side_walk,
    &gPlayerAnim_link_fighter_side_walk_long,
    &gPlayerAnim_link_normal_side_walk_free,
    &gPlayerAnim_link_normal_side_walk_free,
    /* PLAYER_ANIMGROUP_side_walkL */
    &gPlayerAnim_link_normal_side_walkL_free,
    &gPlayerAnim_link_anchor_side_walkL,
    &gPlayerAnim_link_anchor_side_walkL,
    &gPlayerAnim_link_fighter_side_walkL_long,
    &gPlayerAnim_link_normal_side_walkL_free,
    &gPlayerAnim_link_normal_side_walkL_free,
    /* PLAYER_ANIMGROUP_side_walkR */
    &gPlayerAnim_link_normal_side_walkR_free,
    &gPlayerAnim_link_anchor_side_walkR,
    &gPlayerAnim_link_anchor_side_walkR,
    &gPlayerAnim_link_fighter_side_walkR_long,
    &gPlayerAnim_link_normal_side_walkR_free,
    &gPlayerAnim_link_normal_side_walkR_free,
    /* PLAYER_ANIMGROUP_45_turn */
    &gPlayerAnim_link_normal_45_turn_free,
    &gPlayerAnim_link_normal_45_turn,
    &gPlayerAnim_link_normal_45_turn,
    &gPlayerAnim_link_normal_45_turn_free,
    &gPlayerAnim_link_normal_45_turn_free,
    &gPlayerAnim_link_normal_45_turn_free,
    /* PLAYER_ANIMGROUP_waitL2wait */
    &gPlayerAnim_link_fighter_waitL2wait_long,
    &gPlayerAnim_link_normal_waitL2wait,
    &gPlayerAnim_link_normal_waitL2wait,
    &gPlayerAnim_link_fighter_waitL2wait_long,
    &gPlayerAnim_link_fighter_waitL2wait_long,
    &gPlayerAnim_link_fighter_waitL2wait_long,
    /* PLAYER_ANIMGROUP_waitR2wait */
    &gPlayerAnim_link_fighter_waitR2wait_long,
    &gPlayerAnim_link_normal_waitR2wait,
    &gPlayerAnim_link_normal_waitR2wait,
    &gPlayerAnim_link_fighter_waitR2wait_long,
    &gPlayerAnim_link_fighter_waitR2wait_long,
    &gPlayerAnim_link_fighter_waitR2wait_long,
    /* PLAYER_ANIMGROUP_throw */
    &gPlayerAnim_link_normal_throw_free,
    &gPlayerAnim_link_normal_throw,
    &gPlayerAnim_link_normal_throw,
    &gPlayerAnim_link_normal_throw_free,
    &gPlayerAnim_link_normal_throw_free,
    &gPlayerAnim_link_normal_throw_free,
    /* PLAYER_ANIMGROUP_put */
    &gPlayerAnim_link_normal_put_free,
    &gPlayerAnim_link_normal_put,
    &gPlayerAnim_link_normal_put,
    &gPlayerAnim_link_normal_put_free,
    &gPlayerAnim_link_normal_put_free,
    &gPlayerAnim_link_normal_put_free,
    /* PLAYER_ANIMGROUP_back_walk */
    &gPlayerAnim_link_normal_back_walk,
    &gPlayerAnim_link_normal_back_walk,
    &gPlayerAnim_link_normal_back_walk,
    &gPlayerAnim_link_normal_back_walk,
    &gPlayerAnim_link_normal_back_walk,
    &gPlayerAnim_link_normal_back_walk,
    /* PLAYER_ANIMGROUP_check */
    &gPlayerAnim_link_normal_check_free,
    &gPlayerAnim_link_normal_check,
    &gPlayerAnim_link_normal_check,
    &gPlayerAnim_link_normal_check_free,
    &gPlayerAnim_link_normal_check_free,
    &gPlayerAnim_link_normal_check_free,
    /* PLAYER_ANIMGROUP_check_wait */
    &gPlayerAnim_link_normal_check_wait_free,
    &gPlayerAnim_link_normal_check_wait,
    &gPlayerAnim_link_normal_check_wait,
    &gPlayerAnim_link_normal_check_wait_free,
    &gPlayerAnim_link_normal_check_wait_free,
    &gPlayerAnim_link_normal_check_wait_free,
    /* PLAYER_ANIMGROUP_check_end */
    &gPlayerAnim_link_normal_check_end_free,
    &gPlayerAnim_link_normal_check_end,
    &gPlayerAnim_link_normal_check_end,
    &gPlayerAnim_link_normal_check_end_free,
    &gPlayerAnim_link_normal_check_end_free,
    &gPlayerAnim_link_normal_check_end_free,
    /* PLAYER_ANIMGROUP_pull_start */
    &gPlayerAnim_link_normal_pull_start_free,
    &gPlayerAnim_link_normal_pull_start,
    &gPlayerAnim_link_normal_pull_start,
    &gPlayerAnim_link_normal_pull_start_free,
    &gPlayerAnim_link_normal_pull_start_free,
    &gPlayerAnim_link_normal_pull_start_free,
    /* PLAYER_ANIMGROUP_pulling */
    &gPlayerAnim_link_normal_pulling_free,
    &gPlayerAnim_link_normal_pulling,
    &gPlayerAnim_link_normal_pulling,
    &gPlayerAnim_link_normal_pulling_free,
    &gPlayerAnim_link_normal_pulling_free,
    &gPlayerAnim_link_normal_pulling_free,
    /* PLAYER_ANIMGROUP_pull_end */
    &gPlayerAnim_link_normal_pull_end_free,
    &gPlayerAnim_link_normal_pull_end,
    &gPlayerAnim_link_normal_pull_end,
    &gPlayerAnim_link_normal_pull_end_free,
    &gPlayerAnim_link_normal_pull_end_free,
    &gPlayerAnim_link_normal_pull_end_free,
    /* PLAYER_ANIMGROUP_fall_up */
    &gPlayerAnim_link_normal_fall_up_free,
    &gPlayerAnim_link_normal_fall_up,
    &gPlayerAnim_link_normal_fall_up,
    &gPlayerAnim_link_normal_fall_up_free,
    &gPlayerAnim_link_normal_fall_up_free,
    &gPlayerAnim_link_normal_fall_up_free,
    /* PLAYER_ANIMGROUP_jump_climb_hold */
    &gPlayerAnim_link_normal_jump_climb_hold_free,
    &gPlayerAnim_link_normal_jump_climb_hold,
    &gPlayerAnim_link_normal_jump_climb_hold,
    &gPlayerAnim_link_normal_jump_climb_hold_free,
    &gPlayerAnim_link_normal_jump_climb_hold_free,
    &gPlayerAnim_link_normal_jump_climb_hold_free,
    /* PLAYER_ANIMGROUP_jump_climb_wait */
    &gPlayerAnim_link_normal_jump_climb_wait_free,
    &gPlayerAnim_link_normal_jump_climb_wait,
    &gPlayerAnim_link_normal_jump_climb_wait,
    &gPlayerAnim_link_normal_jump_climb_wait_free,
    &gPlayerAnim_link_normal_jump_climb_wait_free,
    &gPlayerAnim_link_normal_jump_climb_wait_free,
    /* PLAYER_ANIMGROUP_jump_climb_up */
    &gPlayerAnim_link_normal_jump_climb_up_free,
    &gPlayerAnim_link_normal_jump_climb_up,
    &gPlayerAnim_link_normal_jump_climb_up,
    &gPlayerAnim_link_normal_jump_climb_up_free,
    &gPlayerAnim_link_normal_jump_climb_up_free,
    &gPlayerAnim_link_normal_jump_climb_up_free,
    /* PLAYER_ANIMGROUP_down_slope_slip_end */
    &gPlayerAnim_link_normal_down_slope_slip_end_free,
    &gPlayerAnim_link_normal_down_slope_slip_end,
    &gPlayerAnim_link_normal_down_slope_slip_end,
    &gPlayerAnim_link_normal_down_slope_slip_end_long,
    &gPlayerAnim_link_normal_down_slope_slip_end_free,
    &gPlayerAnim_link_normal_down_slope_slip_end_free,
    /* PLAYER_ANIMGROUP_up_slope_slip_end */
    &gPlayerAnim_link_normal_up_slope_slip_end_free,
    &gPlayerAnim_link_normal_up_slope_slip_end,
    &gPlayerAnim_link_normal_up_slope_slip_end,
    &gPlayerAnim_link_normal_up_slope_slip_end_long,
    &gPlayerAnim_link_normal_up_slope_slip_end_free,
    &gPlayerAnim_link_normal_up_slope_slip_end_free,
    /* PLAYER_ANIMGROUP_nwait */
    &gPlayerAnim_sude_nwait,
    &gPlayerAnim_lkt_nwait,
    &gPlayerAnim_lkt_nwait,
    &gPlayerAnim_sude_nwait,
    &gPlayerAnim_sude_nwait,
    &gPlayerAnim_sude_nwait,
};

static LinkAnimationHeader* sManualJumpAnims[][3] = {
    { &gPlayerAnim_link_fighter_front_jump, &gPlayerAnim_link_fighter_front_jump_end,
      &gPlayerAnim_link_fighter_front_jump_endR },
    { &gPlayerAnim_link_fighter_Lside_jump, &gPlayerAnim_link_fighter_Lside_jump_end,
      &gPlayerAnim_link_fighter_Lside_jump_endL },
    { &gPlayerAnim_link_fighter_backturn_jump, &gPlayerAnim_link_fighter_backturn_jump_end,
      &gPlayerAnim_link_fighter_backturn_jump_endR },
    { &gPlayerAnim_link_fighter_Rside_jump, &gPlayerAnim_link_fighter_Rside_jump_end,
      &gPlayerAnim_link_fighter_Rside_jump_endR },
};

static LinkAnimationHeader* sIdleAnims[][2] = {
    { &gPlayerAnim_link_normal_wait_typeA_20f, &gPlayerAnim_link_normal_waitF_typeA_20f },
    { &gPlayerAnim_link_normal_wait_typeC_20f, &gPlayerAnim_link_normal_waitF_typeC_20f },
    { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f },
    { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_heat1_20f, &gPlayerAnim_link_waitF_heat1_20f },
    { &gPlayerAnim_link_wait_heat2_20f, &gPlayerAnim_link_waitF_heat2_20f },
    { &gPlayerAnim_link_wait_itemD1_20f, &gPlayerAnim_link_wait_itemD1_20f },
    { &gPlayerAnim_link_wait_itemA_20f, &gPlayerAnim_link_waitF_itemA_20f },
    { &gPlayerAnim_link_wait_itemB_20f, &gPlayerAnim_link_waitF_itemB_20f },
    { &gPlayerAnim_link_wait_itemC_20f, &gPlayerAnim_link_wait_itemC_20f },
    { &gPlayerAnim_link_wait_itemD2_20f, &gPlayerAnim_link_wait_itemD2_20f }
};

static AnimSfxEntry sIdleSneezeAnimSfx[] = {
    { NA_SE_VO_LI_SNEEZE, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 8) },
};

static AnimSfxEntry sIdleSweatAnimSfx[] = {
    { NA_SE_VO_LI_SWEAT, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 18) },
};

static AnimSfxEntry D_80853DF4[] = {
    { NA_SE_VO_LI_BREATH_REST, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 13) },
};

static AnimSfxEntry D_80853DF8[] = {
    { NA_SE_VO_LI_BREATH_REST, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 10) },
};

static AnimSfxEntry D_80853DFC[] = {
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 44) },  { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 48) },
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 52) },  { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 56) },
    { NA_SE_PL_CALM_HIT, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 60) },
};

static AnimSfxEntry D_80853E10[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 25) }, { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 30) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 44) }, { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 48) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 52) }, { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_8, 56) },
};

static AnimSfxEntry D_80853E28[] = {
    { NA_SE_IT_SHIELD_POSTURE, ANIMSFX_DATA(ANIMSFX_TYPE_1, 16) },
    { NA_SE_IT_SHIELD_POSTURE, ANIMSFX_DATA(ANIMSFX_TYPE_1, 20) },
    { NA_SE_IT_SHIELD_POSTURE, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 70) },
};

static AnimSfxEntry D_80853E34[] = {
    { NA_SE_IT_HAMMER_SWING, ANIMSFX_DATA(ANIMSFX_TYPE_1, 10) },
    { NA_SE_VO_LI_AUTO_JUMP, ANIMSFX_DATA(ANIMSFX_TYPE_4, 10) },
    { NA_SE_IT_SWORD_SWING, ANIMSFX_DATA(ANIMSFX_TYPE_1, 22) },
    { NA_SE_VO_LI_SWORD_N, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 22) },
};

static AnimSfxEntry D_80853E44[] = {
    { NA_SE_IT_SWORD_SWING, ANIMSFX_DATA(ANIMSFX_TYPE_1, 39) },
    { NA_SE_VO_LI_SWORD_N, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 39) },
};

static AnimSfxEntry D_80853E4C[] = {
    { NA_SE_VO_LI_RELAX, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 20) },
};

static AnimSfxEntry* sIdleAnimSfx[] = {
    sIdleSneezeAnimSfx, sIdleSweatAnimSfx, D_80853DF4, D_80853DF8, D_80853DFC, D_80853E10,
    D_80853E28,         D_80853E34,        D_80853E44, D_80853E4C, NULL,
};

static u8 sIdleAnimOffsetToAnimSfx[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 10, 10, 10, 10, 10, 10, 3, 3, 4, 4, 8, 8, 5, 5, 6, 6, 7, 7, 9, 9, 0,
};

// Used to map item IDs to item actions
static s8 sItemActions[] = {
    PLAYER_IA_DEKU_STICK,          // ITEM_DEKU_STICK
    PLAYER_IA_DEKU_NUT,            // ITEM_DEKU_NUT
    PLAYER_IA_BOMB,                // ITEM_BOMB
    PLAYER_IA_BOW,                 // ITEM_BOW
    PLAYER_IA_BOW_FIRE,            // ITEM_ARROW_FIRE
    PLAYER_IA_DINS_FIRE,           // ITEM_DINS_FIRE
    PLAYER_IA_SLINGSHOT,           // ITEM_SLINGSHOT
    PLAYER_IA_OCARINA_FAIRY,       // ITEM_OCARINA_FAIRY
    PLAYER_IA_OCARINA_OF_TIME,     // ITEM_OCARINA_OF_TIME
    PLAYER_IA_BOMBCHU,             // ITEM_BOMBCHU
    PLAYER_IA_HOOKSHOT,            // ITEM_HOOKSHOT
    PLAYER_IA_LONGSHOT,            // ITEM_LONGSHOT
    PLAYER_IA_BOW_ICE,             // ITEM_ARROW_ICE
    PLAYER_IA_FARORES_WIND,        // ITEM_FARORES_WIND
    PLAYER_IA_BOOMERANG,           // ITEM_BOOMERANG
    PLAYER_IA_LENS_OF_TRUTH,       // ITEM_LENS_OF_TRUTH
    PLAYER_IA_MAGIC_BEAN,          // ITEM_MAGIC_BEAN
    PLAYER_IA_HAMMER,              // ITEM_HAMMER
    PLAYER_IA_BOW_LIGHT,           // ITEM_ARROW_LIGHT
    PLAYER_IA_NAYRUS_LOVE,         // ITEM_NAYRUS_LOVE
    PLAYER_IA_BOTTLE,              // ITEM_BOTTLE_EMPTY
    PLAYER_IA_BOTTLE_POTION_RED,   // ITEM_BOTTLE_POTION_RED
    PLAYER_IA_BOTTLE_POTION_GREEN, // ITEM_BOTTLE_POTION_GREEN
    PLAYER_IA_BOTTLE_POTION_BLUE,  // ITEM_BOTTLE_POTION_BLUE
    PLAYER_IA_BOTTLE_FAIRY,        // ITEM_BOTTLE_FAIRY
    PLAYER_IA_BOTTLE_FISH,         // ITEM_BOTTLE_FISH
    PLAYER_IA_BOTTLE_MILK_FULL,    // ITEM_BOTTLE_MILK_FULL
    PLAYER_IA_BOTTLE_RUTOS_LETTER, // ITEM_BOTTLE_RUTOS_LETTER
    PLAYER_IA_BOTTLE_FIRE,         // ITEM_BOTTLE_BLUE_FIRE
    PLAYER_IA_BOTTLE_BUG,          // ITEM_BOTTLE_BUG
    PLAYER_IA_BOTTLE_BIG_POE,      // ITEM_BOTTLE_BIG_POE
    PLAYER_IA_BOTTLE_MILK_HALF,    // ITEM_BOTTLE_MILK_HALF
    PLAYER_IA_BOTTLE_POE,          // ITEM_BOTTLE_POE
    PLAYER_IA_WEIRD_EGG,           // ITEM_WEIRD_EGG
    PLAYER_IA_CHICKEN,             // ITEM_CHICKEN
    PLAYER_IA_ZELDAS_LETTER,       // ITEM_ZELDAS_LETTER
    PLAYER_IA_MASK_KEATON,         // ITEM_MASK_KEATON
    PLAYER_IA_MASK_SKULL,          // ITEM_MASK_SKULL
    PLAYER_IA_MASK_SPOOKY,         // ITEM_MASK_SPOOKY
    PLAYER_IA_MASK_BUNNY_HOOD,     // ITEM_MASK_BUNNY_HOOD
    PLAYER_IA_MASK_GORON,          // ITEM_MASK_GORON
    PLAYER_IA_MASK_ZORA,           // ITEM_MASK_ZORA
    PLAYER_IA_MASK_GERUDO,         // ITEM_MASK_GERUDO
    PLAYER_IA_MASK_TRUTH,          // ITEM_MASK_TRUTH
    PLAYER_IA_SWORD_MASTER,        // ITEM_SOLD_OUT
    PLAYER_IA_BOOSTERS,            // ITEM_POCKET_EGG
    PLAYER_IA_POCKET_CUCCO,        // ITEM_POCKET_CUCCO
    PLAYER_IA_COJIRO,              // ITEM_COJIRO
    PLAYER_IA_ODD_MUSHROOM,        // ITEM_ODD_MUSHROOM
    PLAYER_IA_ODD_POTION,          // ITEM_ODD_POTION
    PLAYER_IA_POACHERS_SAW,        // ITEM_POACHERS_SAW
    PLAYER_IA_BROKEN_GORONS_SWORD, // ITEM_BROKEN_GORONS_SWORD
    PLAYER_IA_PRESCRIPTION,        // ITEM_PRESCRIPTION
    PLAYER_IA_FROG,                // ITEM_EYEBALL_FROG
    PLAYER_IA_EYEDROPS,            // ITEM_EYE_DROPS
    PLAYER_IA_CLAIM_CHECK,         // ITEM_CLAIM_CHECK
    PLAYER_IA_BOW_FIRE,            // ITEM_BOW_FIRE
    PLAYER_IA_BOW_ICE,             // ITEM_BOW_ICE
    PLAYER_IA_BOW_LIGHT,           // ITEM_BOW_LIGHT
    PLAYER_IA_SWORD_KOKIRI,        // ITEM_SWORD_KOKIRI
    PLAYER_IA_SWORD_MASTER,        // ITEM_SWORD_MASTER
    PLAYER_IA_SWORD_BIGGORON,      // ITEM_SWORD_BIGGORON
};

static s32 (*sItemActionUpdateFuncs[])(Player* this, PlayState* play) = {
    Player_UpperAction_Wait,           // PLAYER_IA_NONE
    Player_UpperAction_Wait,           // PLAYER_IA_SWORD_CS
    Player_UpperAction_Wait,           // PLAYER_IA_FISHING_POLE
    Player_UpperAction_Sword,          // PLAYER_IA_SWORD_MASTER
    Player_UpperAction_Sword,          // PLAYER_IA_SWORD_KOKIRI
    Player_UpperAction_Sword,          // PLAYER_IA_SWORD_BIGGORON
    Player_UpperAction_Wait,           // PLAYER_IA_DEKU_STICK
    Player_UpperAction_Wait,           // PLAYER_IA_HAMMER
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_FIRE
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_ICE
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_LIGHT
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_0C
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_0D
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_BOW_0E
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_SLINGSHOT
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_HOOKSHOT
    Player_UpperAction_HoldFpsItem,    // PLAYER_IA_LONGSHOT
    Player_UpperAction_CarryActor,     // PLAYER_IA_BOMB
    Player_UpperAction_CarryActor,     // PLAYER_IA_BOMBCHU
    Player_UpperAction_CarryBoomerang, // PLAYER_IA_BOOMERANG
    Player_UpperAction_Wait,           // PLAYER_IA_MAGIC_SPELL_15
    Player_UpperAction_Wait,           // PLAYER_IA_MAGIC_SPELL_16
    Player_UpperAction_Wait,           // PLAYER_IA_MAGIC_SPELL_17
    Player_UpperAction_Wait,           // PLAYER_IA_FARORES_WIND
    Player_UpperAction_Wait,           // PLAYER_IA_NAYRUS_LOVE
    Player_UpperAction_Wait,           // PLAYER_IA_DINS_FIRE
    Player_UpperAction_Wait,           // PLAYER_IA_DEKU_NUT
    Player_UpperAction_Wait,           // PLAYER_IA_OCARINA_FAIRY
    Player_UpperAction_Wait,           // PLAYER_IA_OCARINA_OF_TIME
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_FISH
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_FIRE
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_BUG
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_POE
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_BIG_POE
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_RUTOS_LETTER
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_POTION_RED
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_POTION_BLUE
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_POTION_GREEN
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_MILK_FULL
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_MILK_HALF
    Player_UpperAction_Wait,           // PLAYER_IA_BOTTLE_FAIRY
    Player_UpperAction_Wait,           // PLAYER_IA_ZELDAS_LETTER
    Player_UpperAction_Wait,           // PLAYER_IA_WEIRD_EGG
    Player_UpperAction_Wait,           // PLAYER_IA_CHICKEN
    Player_UpperAction_Wait,           // PLAYER_IA_MAGIC_BEAN
    Player_UpperAction_Wait,           // PLAYER_IA_POCKET_EGG
    Player_UpperAction_Wait,           // PLAYER_IA_POCKET_CUCCO
    Player_UpperAction_Wait,           // PLAYER_IA_COJIRO
    Player_UpperAction_Wait,           // PLAYER_IA_ODD_MUSHROOM
    Player_UpperAction_Wait,           // PLAYER_IA_ODD_POTION
    Player_UpperAction_Wait,           // PLAYER_IA_POACHERS_SAW
    Player_UpperAction_Wait,           // PLAYER_IA_BROKEN_GORONS_SWORD
    Player_UpperAction_Wait,           // PLAYER_IA_PRESCRIPTION
    Player_UpperAction_Wait,           // PLAYER_IA_FROG
    Player_UpperAction_Wait,           // PLAYER_IA_EYEDROPS
    Player_UpperAction_Wait,           // PLAYER_IA_CLAIM_CHECK
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_KEATON
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_SKULL
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_SPOOKY
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_BUNNY_HOOD
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_GORON
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_ZORA
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_GERUDO
    Player_UpperAction_Wait,           // PLAYER_IA_MASK_TRUTH
    Player_UpperAction_Wait,           // PLAYER_IA_LENS_OF_TRUTH
    Player_UpperAction_WearBoosters,   // PLAYER_IA_BOOSTERS
};

static void (*sItemActionInitFuncs[])(PlayState* play, Player* this) = {
    Player_InitDefaultIA,        // PLAYER_IA_NONE
    Player_InitDefaultIA,        // PLAYER_IA_SWORD_CS
    Player_InitDefaultIA,        // PLAYER_IA_FISHING_POLE
    Player_InitDefaultIA,        // PLAYER_IA_SWORD_MASTER
    Player_InitDefaultIA,        // PLAYER_IA_SWORD_KOKIRI
    Player_InitDefaultIA,        // PLAYER_IA_SWORD_BIGGORON
    Player_InitDekuStickIA,      // PLAYER_IA_DEKU_STICK
    Player_InitHammerIA,         // PLAYER_IA_HAMMER
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_FIRE
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_ICE
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_LIGHT
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_0C
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_0D
    Player_InitBowOrSlingshotIA, // PLAYER_IA_BOW_0E
    Player_InitBowOrSlingshotIA, // PLAYER_IA_SLINGSHOT
    Player_InitHookshotIA,       // PLAYER_IA_HOOKSHOT
    Player_InitHookshotIA,       // PLAYER_IA_LONGSHOT
    Player_InitExplosiveIA,      // PLAYER_IA_BOMB
    Player_InitExplosiveIA,      // PLAYER_IA_BOMBCHU
    Player_InitBoomerangIA,      // PLAYER_IA_BOOMERANG
    Player_InitDefaultIA,        // PLAYER_IA_MAGIC_SPELL_15
    Player_InitDefaultIA,        // PLAYER_IA_MAGIC_SPELL_16
    Player_InitDefaultIA,        // PLAYER_IA_MAGIC_SPELL_17
    Player_InitDefaultIA,        // PLAYER_IA_FARORES_WIND
    Player_InitDefaultIA,        // PLAYER_IA_NAYRUS_LOVE
    Player_InitDefaultIA,        // PLAYER_IA_DINS_FIRE
    Player_InitDefaultIA,        // PLAYER_IA_DEKU_NUT
    Player_InitDefaultIA,        // PLAYER_IA_OCARINA_FAIRY
    Player_InitDefaultIA,        // PLAYER_IA_OCARINA_OF_TIME
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_FISH
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_FIRE
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_BUG
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_POE
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_BIG_POE
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_RUTOS_LETTER
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_POTION_RED
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_POTION_BLUE
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_POTION_GREEN
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_MILK_FULL
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_MILK_HALF
    Player_InitDefaultIA,        // PLAYER_IA_BOTTLE_FAIRY
    Player_InitDefaultIA,        // PLAYER_IA_ZELDAS_LETTER
    Player_InitDefaultIA,        // PLAYER_IA_WEIRD_EGG
    Player_InitDefaultIA,        // PLAYER_IA_CHICKEN
    Player_InitDefaultIA,        // PLAYER_IA_MAGIC_BEAN
    Player_InitDefaultIA,        // PLAYER_IA_POCKET_EGG
    Player_InitDefaultIA,        // PLAYER_IA_POCKET_CUCCO
    Player_InitDefaultIA,        // PLAYER_IA_COJIRO
    Player_InitDefaultIA,        // PLAYER_IA_ODD_MUSHROOM
    Player_InitDefaultIA,        // PLAYER_IA_ODD_POTION
    Player_InitDefaultIA,        // PLAYER_IA_POACHERS_SAW
    Player_InitDefaultIA,        // PLAYER_IA_BROKEN_GORONS_SWORD
    Player_InitDefaultIA,        // PLAYER_IA_PRESCRIPTION
    Player_InitDefaultIA,        // PLAYER_IA_FROG
    Player_InitDefaultIA,        // PLAYER_IA_EYEDROPS
    Player_InitDefaultIA,        // PLAYER_IA_CLAIM_CHECK
    Player_InitDefaultIA,        // PLAYER_IA_MASK_KEATON
    Player_InitDefaultIA,        // PLAYER_IA_MASK_SKULL
    Player_InitDefaultIA,        // PLAYER_IA_MASK_SPOOKY
    Player_InitDefaultIA,        // PLAYER_IA_MASK_BUNNY_HOOD
    Player_InitDefaultIA,        // PLAYER_IA_MASK_GORON
    Player_InitDefaultIA,        // PLAYER_IA_MASK_ZORA
    Player_InitDefaultIA,        // PLAYER_IA_MASK_GERUDO
    Player_InitDefaultIA,        // PLAYER_IA_MASK_TRUTH
    Player_InitDefaultIA,        // PLAYER_IA_LENS_OF_TRUTH
    Player_InitBoostersIA,       // PLAYER_IA_BOOSTERS
};

typedef enum {
    /*  0 */ PLAYER_ITEM_CHG_DEFAULT,
    /*  1 */ PLAYER_ITEM_CHG_SHIELD_TO_1HAND,
    /*  2 */ PLAYER_ITEM_CHG_SHIELD_TO_2HAND,
    /*  3 */ PLAYER_ITEM_CHG_SHIELD,
    /*  4 */ PLAYER_ITEM_CHG_2HAND_TO_1HAND,
    /*  5 */ PLAYER_ITEM_CHG_1HAND,
    /*  6 */ PLAYER_ITEM_CHG_2HAND,
    /*  7 */ PLAYER_ITEM_CHG_2HAND_TO_2HAND,
    /*  8 */ PLAYER_ITEM_CHG_DEFAULT_2,
    /*  9 */ PLAYER_ITEM_CHG_1HAND_TO_BOMB,
    /* 10 */ PLAYER_ITEM_CHG_2HAND_TO_BOMB,
    /* 11 */ PLAYER_ITEM_CHG_BOMB,
    /* 12 */ PLAYER_ITEM_CHG_12,
    /* 13 */ PLAYER_ITEM_CHG_LEFT_HAND,
    /* 14 */ PLAYER_ITEM_CHG_MAX
} ItemChangeType;

static ItemChangeInfo sItemChangeInfo[PLAYER_ITEM_CHG_MAX] = {
    /* PLAYER_ITEM_CHG_DEFAULT */ { &gPlayerAnim_link_normal_free2free, 12 },
    /* PLAYER_ITEM_CHG_SHIELD_TO_1HAND */ { &gPlayerAnim_link_normal_normal2fighter, 6 },
    /* PLAYER_ITEM_CHG_SHIELD_TO_2HAND */ { &gPlayerAnim_link_hammer_normal2long, 8 },
    /* PLAYER_ITEM_CHG_SHIELD */ { &gPlayerAnim_link_normal_normal2free, 8 },
    /* PLAYER_ITEM_CHG_2HAND_TO_1HAND */ { &gPlayerAnim_link_fighter_fighter2long, 8 },
    /* PLAYER_ITEM_CHG_1HAND */ { &gPlayerAnim_link_normal_fighter2free, 10 },
    /* PLAYER_ITEM_CHG_2HAND */ { &gPlayerAnim_link_hammer_long2free, 7 },
    /* PLAYER_ITEM_CHG_2HAND_TO_2HAND */ { &gPlayerAnim_link_hammer_long2long, 11 },
    /* PLAYER_ITEM_CHG_DEFAULT_2 */ { &gPlayerAnim_link_normal_free2free, 12 },
    /* PLAYER_ITEM_CHG_1HAND_TO_BOMB */ { &gPlayerAnim_link_normal_normal2bom, 4 },
    /* PLAYER_ITEM_CHG_2HAND_TO_BOMB */ { &gPlayerAnim_link_normal_long2bom, 4 },
    /* PLAYER_ITEM_CHG_BOMB */ { &gPlayerAnim_link_normal_free2bom, 4 },
    /* PLAYER_ITEM_CHG_12 */ { &gPlayerAnim_link_anchor_anchor2fighter, 5 },
    /* PLAYER_ITEM_CHG_LEFT_HAND */ { &gPlayerAnim_link_normal_free2freeB, 13 },
};

// Maps the appropriate ItemChangeType based on current and next animtype.
// A negative type value means the corresponding animation should be played in reverse.
static s8 sItemChangeTypes[PLAYER_ANIMTYPE_MAX][PLAYER_ANIMTYPE_MAX] = {
    { PLAYER_ITEM_CHG_DEFAULT_2, -PLAYER_ITEM_CHG_1HAND, -PLAYER_ITEM_CHG_SHIELD, -PLAYER_ITEM_CHG_2HAND,
      PLAYER_ITEM_CHG_DEFAULT_2, PLAYER_ITEM_CHG_BOMB },
    { PLAYER_ITEM_CHG_1HAND, PLAYER_ITEM_CHG_DEFAULT, -PLAYER_ITEM_CHG_SHIELD_TO_1HAND, PLAYER_ITEM_CHG_2HAND_TO_1HAND,
      PLAYER_ITEM_CHG_1HAND, PLAYER_ITEM_CHG_1HAND_TO_BOMB },
    { PLAYER_ITEM_CHG_SHIELD, PLAYER_ITEM_CHG_SHIELD_TO_1HAND, PLAYER_ITEM_CHG_DEFAULT, PLAYER_ITEM_CHG_SHIELD_TO_2HAND,
      PLAYER_ITEM_CHG_SHIELD, PLAYER_ITEM_CHG_1HAND_TO_BOMB },
    { PLAYER_ITEM_CHG_2HAND, -PLAYER_ITEM_CHG_2HAND_TO_1HAND, -PLAYER_ITEM_CHG_SHIELD_TO_2HAND,
      PLAYER_ITEM_CHG_2HAND_TO_2HAND, PLAYER_ITEM_CHG_2HAND, PLAYER_ITEM_CHG_2HAND_TO_BOMB },
    { PLAYER_ITEM_CHG_DEFAULT_2, -PLAYER_ITEM_CHG_1HAND, -PLAYER_ITEM_CHG_SHIELD, -PLAYER_ITEM_CHG_2HAND,
      PLAYER_ITEM_CHG_DEFAULT_2, PLAYER_ITEM_CHG_BOMB },
    { PLAYER_ITEM_CHG_DEFAULT_2, -PLAYER_ITEM_CHG_1HAND, -PLAYER_ITEM_CHG_SHIELD, -PLAYER_ITEM_CHG_2HAND,
      PLAYER_ITEM_CHG_DEFAULT_2, PLAYER_ITEM_CHG_BOMB },
};

static ExplosiveInfo sExplosiveInfos[] = {
    { ITEM_BOMB, ACTOR_EN_BOM },
    { ITEM_BOMBCHU, ACTOR_EN_BOM_CHU },
};

static MeleeAttackAnimInfo sMeleeAttackAnims[PLAYER_MELEEATKTYPE_MAX] = {
    /* PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H */
    { &gPlayerAnim_link_fighter_normal_kiru, &gPlayerAnim_link_fighter_normal_kiru_end,
      &gPlayerAnim_link_fighter_normal_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_FORWARD_SLASH_2H */
    { &gPlayerAnim_link_fighter_Lnormal_kiru, &gPlayerAnim_link_fighter_Lnormal_kiru_end,
      &gPlayerAnim_link_anchor_Lnormal_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_FORWARD_COMBO_1H */
    { &gPlayerAnim_link_fighter_normal_kiru_finsh, &gPlayerAnim_link_fighter_normal_kiru_finsh_end,
      &gPlayerAnim_link_anchor_normal_kiru_finsh_endR, 0, 5 },
    /* PLAYER_MELEEATKTYPE_FORWARD_COMBO_2H */
    { &gPlayerAnim_link_fighter_Lnormal_kiru_finsh, &gPlayerAnim_link_fighter_Lnormal_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lnormal_kiru_finsh_endR, 1, 7 },
    /* PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H */
    { &gPlayerAnim_link_fighter_Lside_kiru, &gPlayerAnim_link_fighter_Lside_kiru_end,
      &gPlayerAnim_link_anchor_Lside_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_RIGHT_SLASH_2H */
    { &gPlayerAnim_link_fighter_LLside_kiru, &gPlayerAnim_link_fighter_LLside_kiru_end,
      &gPlayerAnim_link_anchor_LLside_kiru_endL, 0, 5 },
    /* PLAYER_MELEEATKTYPE_RIGHT_COMBO_1H */
    { &gPlayerAnim_link_fighter_Lside_kiru_finsh, &gPlayerAnim_link_fighter_Lside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lside_kiru_finsh_endR, 2, 8 },
    /* PLAYER_MELEEATKTYPE_RIGHT_COMBO_2H */
    { &gPlayerAnim_link_fighter_LLside_kiru_finsh, &gPlayerAnim_link_fighter_LLside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_LLside_kiru_finsh_endR, 3, 8 },
    /* PLAYER_MELEEATKTYPE_LEFT_SLASH_1H */
    { &gPlayerAnim_link_fighter_Rside_kiru, &gPlayerAnim_link_fighter_Rside_kiru_end,
      &gPlayerAnim_link_anchor_Rside_kiru_endR, 0, 4 },
    /* PLAYER_MELEEATKTYPE_LEFT_SLASH_2H */
    { &gPlayerAnim_link_fighter_LRside_kiru, &gPlayerAnim_link_fighter_LRside_kiru_end,
      &gPlayerAnim_link_anchor_LRside_kiru_endR, 0, 5 },
    /* PLAYER_MELEEATKTYPE_LEFT_COMBO_1H */
    { &gPlayerAnim_link_fighter_Rside_kiru_finsh, &gPlayerAnim_link_fighter_Rside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Rside_kiru_finsh_endR, 0, 6 },
    /* PLAYER_MELEEATKTYPE_LEFT_COMBO_2H */
    { &gPlayerAnim_link_fighter_LRside_kiru_finsh, &gPlayerAnim_link_fighter_LRside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_LRside_kiru_finsh_endL, 1, 5 },
    /* PLAYER_MELEEATKTYPE_STAB_1H */
    { &gPlayerAnim_link_fighter_pierce_kiru, &gPlayerAnim_link_fighter_pierce_kiru_end,
      &gPlayerAnim_link_anchor_pierce_kiru_endR, 0, 3 },
    /* PLAYER_MELEEATKTYPE_STAB_2H */
    { &gPlayerAnim_link_fighter_Lpierce_kiru, &gPlayerAnim_link_fighter_Lpierce_kiru_end,
      &gPlayerAnim_link_anchor_Lpierce_kiru_endL, 0, 3 },
    /* PLAYER_MELEEATKTYPE_STAB_COMBO_1H */
    { &gPlayerAnim_link_fighter_pierce_kiru_finsh, &gPlayerAnim_link_fighter_pierce_kiru_finsh_end,
      &gPlayerAnim_link_anchor_pierce_kiru_finsh_endR, 1, 9 },
    /* PLAYER_MELEEATKTYPE_STAB_COMBO_2H */
    { &gPlayerAnim_link_fighter_Lpierce_kiru_finsh, &gPlayerAnim_link_fighter_Lpierce_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lpierce_kiru_finsh_endR, 1, 8 },
    /* PLAYER_MELEEATKTYPE_FLIPSLASH_START */
    { &gPlayerAnim_link_fighter_jump_rollkiru, &gPlayerAnim_link_fighter_jump_kiru_finsh,
      &gPlayerAnim_link_fighter_jump_kiru_finsh, 1, 10 },
    /* PLAYER_MELEEATKTYPE_JUMPSLASH_START */
    { &gPlayerAnim_link_fighter_Lpower_jump_kiru, &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit,
      &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit, 1, 11 },
    /* PLAYER_MELEEATKTYPE_FLIPSLASH_FINISH */
    { &gPlayerAnim_link_fighter_jump_kiru_finsh, &gPlayerAnim_link_fighter_jump_kiru_finsh_end,
      &gPlayerAnim_link_fighter_jump_kiru_finsh_end, 1, 2 },
    /* PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH */
    { &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit, &gPlayerAnim_link_fighter_Lpower_jump_kiru_end,
      &gPlayerAnim_link_fighter_Lpower_jump_kiru_end, 1, 2 },
    /* PLAYER_MELEEATKTYPE_BACKSLASH_RIGHT */
    { &gPlayerAnim_link_fighter_turn_kiruR, &gPlayerAnim_link_fighter_turn_kiruR_end,
      &gPlayerAnim_link_fighter_turn_kiruR_end, 1, 5 },
    /* PLAYER_MELEEATKTYPE_BACKSLASH_LEFT */
    { &gPlayerAnim_link_fighter_turn_kiruL, &gPlayerAnim_link_fighter_turn_kiruL_end,
      &gPlayerAnim_link_fighter_turn_kiruL_end, 1, 4 },
    /* PLAYER_MELEEATKTYPE_HAMMER_FORWARD */
    { &gPlayerAnim_link_hammer_hit, &gPlayerAnim_link_hammer_hit_end, &gPlayerAnim_link_hammer_hit_endR, 3, 10 },
    /* PLAYER_MELEEATKTYPE_HAMMER_SIDE */
    { &gPlayerAnim_link_hammer_side_hit, &gPlayerAnim_link_hammer_side_hit_end, &gPlayerAnim_link_hammer_side_hit_endR,
      2, 11 },
    /* PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H */
    { &gPlayerAnim_link_fighter_rolling_kiru, &gPlayerAnim_link_fighter_rolling_kiru_end,
      &gPlayerAnim_link_anchor_rolling_kiru_endR, 0, 12 },
    /* PLAYER_MELEEATKTYPE_SPIN_ATTACK_2H */
    { &gPlayerAnim_link_fighter_Lrolling_kiru, &gPlayerAnim_link_fighter_Lrolling_kiru_end,
      &gPlayerAnim_link_anchor_Lrolling_kiru_endR, 0, 15 },
    /* PLAYER_MELEEATKTYPE_BIG_SPIN_1H */
    { &gPlayerAnim_link_fighter_Wrolling_kiru, &gPlayerAnim_link_fighter_Wrolling_kiru_end,
      &gPlayerAnim_link_anchor_rolling_kiru_endR, 0, 16 },
    /* PLAYER_MELEEATKTYPE_BIG_SPIN_2H */
    { &gPlayerAnim_link_fighter_Wrolling_kiru, &gPlayerAnim_link_fighter_Wrolling_kiru_end,
      &gPlayerAnim_link_anchor_Lrolling_kiru_endR, 0, 16 },
};

static LinkAnimationHeader* sSpinAttackAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_start,
    &gPlayerAnim_link_fighter_Lpower_kiru_start,
};

static LinkAnimationHeader* sSpinAttackAnimsL[] = {
    &gPlayerAnim_link_fighter_power_kiru_startL,
    &gPlayerAnim_link_fighter_Lpower_kiru_start,
};

static LinkAnimationHeader* sSpinAttackChargeAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait,
    &gPlayerAnim_link_fighter_Lpower_kiru_wait,
};

static LinkAnimationHeader* sCancelSpinAttackChargeAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait_end,
    &gPlayerAnim_link_fighter_Lpower_kiru_wait_end,
};

static LinkAnimationHeader* sSpinAttackChargeWalkAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_walk,
    &gPlayerAnim_link_fighter_Lpower_kiru_walk,
};

static LinkAnimationHeader* sSpinAttackChargeSidewalkAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_side_walk,
    &gPlayerAnim_link_fighter_Lpower_kiru_side_walk,
};

static u8 sSmallSpinAttackMeleeAtkTypes[2] = { PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H, PLAYER_MELEEATKTYPE_SPIN_ATTACK_2H };
static u8 sBigSpinAttackMeleeAtkTypes[2] = { PLAYER_MELEEATKTYPE_BIG_SPIN_1H, PLAYER_MELEEATKTYPE_BIG_SPIN_2H };

static u16 sItemButtons[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };

static u8 sMagicSpellCosts[] = { 12, 24, 24, 12, 24, 12 };

static u16 sFpsItemReadySfx[] = { NA_SE_IT_BOW_DRAW, NA_SE_IT_SLING_DRAW, NA_SE_IT_HOOKSHOT_READY };

static u8 sMagicArrowCosts[] = { 4, 4, 8 };

static LinkAnimationHeader* sRightDefendStandingAnims[] = {
    &gPlayerAnim_link_anchor_waitR2defense,
    &gPlayerAnim_link_anchor_waitR2defense_long,
};

static LinkAnimationHeader* sLeftDefendStandingAnims[] = {
    &gPlayerAnim_link_anchor_waitL2defense,
    &gPlayerAnim_link_anchor_waitL2defense_long,
};

static LinkAnimationHeader* sLeftStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitL,
};

static LinkAnimationHeader* sRightStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitR,
};

static LinkAnimationHeader* sDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_normal_defense_hit,
    &gPlayerAnim_link_fighter_defense_long_hit,
};

static LinkAnimationHeader* sReadyFpsItemWhileWalkingAnims[] = {
    &gPlayerAnim_link_bow_walk2ready,
    &gPlayerAnim_link_hook_walk2ready,
};

static LinkAnimationHeader* sReadyFpsItemAnims[] = {
    &gPlayerAnim_link_bow_bow_wait,
    &gPlayerAnim_link_hook_wait,
};

// return type can't be void due to regalloc in Player_CheckNoDebugModeCombo
BAD_RETURN(s32) Player_ZeroSpeedXZ(Player* this) {
    this->actor.speed = 0.0f;
    this->speedXZ = 0.0f;
}

// return type can't be void due to regalloc in Player_SetupGrabPushPullWallTryMiniCs
BAD_RETURN(s32) Player_ClearAttentionModeAndStopMoving(Player* this) {
    Player_ZeroSpeedXZ(this);
    this->attentionMode = 0;
}

s32 Player_CheckActorTalkOffered(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_TALK);
}

void Player_AnimPlayOnce(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnce(play, &this->skelAnime, anim);
}

void Player_AnimPlayLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoop(play, &this->skelAnime, anim);
}

void Player_AnimPlayLoopAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, PLAYER_ANIM_ADJUSTED_SPEED);
}

void Player_AnimPlayOnceAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, PLAYER_ANIM_ADJUSTED_SPEED);
}

void Player_AddRootYawToShapeYaw(Player* this) {
    this->actor.shape.rot.y += this->skelAnime.jointTable[1].y;
    this->skelAnime.jointTable[1].y = 0;
}

void Player_InactivateMeleeWeapon(Player* this) {
    this->stateFlags2 &= ~PLAYER_STATE2_RELEASING_SPIN_ATTACK;
    this->meleeWeaponState = 0;
    this->meleeWeaponInfo[0].active = this->meleeWeaponInfo[1].active = this->meleeWeaponInfo[2].active = 0;
}

void Player_ResetSubCam(PlayState* play, Player* this) {
    Camera* subCam;

    if (this->subCamId != CAM_ID_NONE) {
        subCam = play->cameraPtrs[this->subCamId];
        if ((subCam != NULL) && (subCam->csId == 1100)) {
            OnePointCutscene_EndCutscene(play, this->subCamId);
            this->subCamId = CAM_ID_NONE;
        }
    }

    this->stateFlags2 &= ~(PLAYER_STATE2_DIVING | PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER);
}

void Player_DetachHeldActor(PlayState* play, Player* this) {
    Actor* heldActor = this->heldActor;

    if ((heldActor != NULL) && !Player_HoldsHookshot(this)) {
        this->actor.child = NULL;
        this->heldActor = NULL;
        this->interactRangeActor = NULL;
        heldActor->parent = NULL;
        this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
    }

    if (Player_GetExplosiveHeld(this) >= 0) {
        Player_InitItemAction(play, this, PLAYER_IA_NONE);
        this->heldItemId = ITEM_NONE_FE;
    }
}

void Player_ResetAttributes(PlayState* play, Player* this) {
    if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor == NULL)) {
        if (this->interactRangeActor != NULL) {
            if (this->getItemId == GI_NONE) {
                this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
                this->interactRangeActor = NULL;
            }
        } else {
            this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
        }
    }

    Player_InactivateMeleeWeapon(this);
    this->attentionMode = 0;

    Player_ResetSubCam(play, this);
    Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                           PLAYER_STATE1_IN_FIRST_PERSON_MODE | PLAYER_STATE1_CLIMBING);
    this->stateFlags2 &=
        ~(PLAYER_STATE2_MOVING_PUSH_PULL_WALL | PLAYER_STATE2_RESTRAINED_BY_ENEMY | PLAYER_STATE2_CRAWLING);

    this->actor.shape.rot.x = 0;
    this->actor.shape.yOffset = 0.0f;

    this->slashCounter = this->comboTimer = 0;
}

s32 Player_TryUnequipItem(PlayState* play, Player* this) {
    if (this->heldItemAction >= PLAYER_IA_FISHING_POLE) {
        Player_UseItem(play, this, ITEM_NONE);
        return 1;
    } else {
        return 0;
    }
}

void Player_StopCarryingActor(PlayState* play, Player* this) {
    Player_ResetAttributes(play, this);
    Player_DetachHeldActor(play, this);
}

s32 Player_MashTimerThresholdExceeded(Player* this, s32 timerStep, s32 timerThreshold) {
    s16 controlStickAngleDiff = this->prevControlStickAngle - sControlStickAngle;

    this->av2.mashTimer +=
        timerStep + (s16)(ABS(controlStickAngleDiff) * fabsf(sControlStickMagnitude) * 2.5415802156203426e-06f);

    if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B)) {
        this->av2.mashTimer += 5;
    }

    return this->av2.mashTimer > timerThreshold;
}

void Player_SetFreezeFlashTimer(PlayState* play) {
    if (play->actorCtx.freezeFlashTimer == 0) {
        play->actorCtx.freezeFlashTimer = 1;
    }
}

void Player_RequestRumble(Player* this, s32 sourceStrength, s32 duration, s32 decreaseRate, s32 distSq) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        Rumble_Request(distSq, sourceStrength, duration, decreaseRate);
    }
}

void Player_PlayVoiceSfxForAge(Player* this, u16 sfxId) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        Player_PlaySfx(this, sfxId + this->ageProperties->unk_92);
    } else {
        func_800F4190(&this->actor.projectedPos, sfxId);
    }
}

void Player_StopInterruptableSfx(Player* this) {
    u16* entry = &sInterruptableSfx[0];
    s32 i;

    for (i = 0; i < 4; i++) {
        Audio_StopSfxById((u16)(*entry + this->ageProperties->unk_92));
        entry++;
    }
}

u16 Player_GetMoveSfx(Player* this, u16 sfxId) {
    return sfxId + this->floorSfxOffset;
}

void Player_PlayMoveSfx(Player* this, u16 sfxId) {
    Player_PlaySfx(this, Player_GetMoveSfx(this, sfxId));
}

u16 Player_GetMoveSfxForAge(Player* this, u16 sfxId) {
    return sfxId + this->floorSfxOffset + this->ageProperties->unk_94;
}

void Player_PlayMoveSfxForAge(Player* this, u16 sfxId) {
    Player_PlaySfx(this, Player_GetMoveSfxForAge(this, sfxId));
}

void Player_PlayWalkSfx(Player* this, f32 arg1) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_WALK_GROUND);
    }

    func_800F4010(&this->actor.projectedPos, sfxId, arg1);
}

void Player_PlayJumpSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_JUMP + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_JUMP);
    }

    Player_PlaySfx(this, sfxId);
}

void Player_PlayLandingSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_LAND + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_LAND);
    }

    Player_PlaySfx(this, sfxId);
}

void Player_PlayReactableSfx(Player* this, u16 sfxId) {
    Player_PlaySfx(this, sfxId);
    this->stateFlags2 |= PLAYER_STATE2_MAKING_REACTABLE_NOISE;
}

/**
 * Process a list of `AnimSfx` entries.
 * An `AnimSfx` entry contains a sound effect to play, a frame number that indicates
 * when during an animation it should play, and a type value that indicates how it should be played back.
 *
 * The list will stop being processed after an entry that has a negative value for the `data` field.
 *
 * Some types do not make use of `sfxId`, the SFX function called will pick a sound on its own.
 * The `sfxId` field is not used in this case and can be any value, but 0 is typically used.
 *
 * @param entry  A pointer to the first entry of an `AnimSfx` list.
 */
void Player_ProcessAnimSfxList(Player* this, AnimSfxEntry* entry) {
    s32 cont;
    s32 pad;

    do {
        s32 absData = ABS(entry->data);
        s32 type = ANIMSFX_GET_TYPE(absData);

        if (LinkAnimation_OnFrame(&this->skelAnime, fabsf(ANIMSFX_GET_FRAME(absData)))) {
            if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_1)) {
                Player_PlaySfx(this, entry->sfxId);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_2)) {
                Player_PlayMoveSfx(this, entry->sfxId);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_3)) {
                Player_PlayMoveSfxForAge(this, entry->sfxId);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_4)) {
                Player_PlayVoiceSfxForAge(this, entry->sfxId);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_5)) {
                Player_PlayLandingSfx(this);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_6)) {
                Player_PlayWalkSfx(this, 6.0f);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_7)) {
                Player_PlayJumpSfx(this);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_8)) {
                Player_PlayWalkSfx(this, 0.0f);
            } else if (type == ANIMSFX_SHIFT_TYPE(ANIMSFX_TYPE_9)) {
                func_800F4010(&this->actor.projectedPos,
                              NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD + this->ageProperties->unk_94, 0.0f);
            }
        }

        cont = (entry->data >= 0); // stop processing if `data` is negative
        entry++;
    } while (cont);
}

void Player_AnimChangeOnceMorph(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
}

void Player_AnimChangeOnceMorphAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, PLAYER_ANIM_ADJUSTED_SPEED, 0.0f, Animation_GetLastFrame(anim),
                         ANIMMODE_ONCE, -6.0f);
}

void Player_AnimChangeLoopMorph(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_AnimChangeFreeze(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, 0.0f);
}

void Player_AnimChangeLoopSlowMorph(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -16.0f);
}

s32 Player_LoopAnimContinuously(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, anim);
        return 1;
    } else {
        return 0;
    }
}

void Player_SkelAnimeResetPrevTranslRot(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    this->skelAnime.prevRot = this->actor.shape.rot.y;
}

void Player_SkelAnimeResetPrevTranslRotAgeScale(Player* this) {
    Player_SkelAnimeResetPrevTranslRot(this);
    this->skelAnime.prevTransl.x *= this->ageProperties->unk_08;
    this->skelAnime.prevTransl.y *= this->ageProperties->unk_08;
    this->skelAnime.prevTransl.z *= this->ageProperties->unk_08;
}

void Player_ZeroRootLimbYaw(Player* this) {
    this->skelAnime.jointTable[1].y = 0;
}

void Player_FinishAnimMovement(Player* this) {
    if (this->skelAnime.moveFlags != 0) {
        Player_AddRootYawToShapeYaw(this);
        this->skelAnime.jointTable[0].x = this->skelAnime.baseTransl.x;
        this->skelAnime.jointTable[0].z = this->skelAnime.baseTransl.z;
        if (this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_SETMOVE) {
            if (this->skelAnime.moveFlags & ANIM_FLAG_UPDATE_Y) {
                this->skelAnime.jointTable[0].y = this->skelAnime.prevTransl.y;
            }
        } else {
            this->skelAnime.jointTable[0].y = this->skelAnime.baseTransl.y;
        }
        Player_SkelAnimeResetPrevTranslRot(this);
        this->skelAnime.moveFlags = 0;
    }
}

void Player_UpdateAnimMovement(Player* this, s32 flags) {
    Vec3f pos;

    this->skelAnime.moveFlags = flags;
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    SkelAnime_UpdateTranslation(&this->skelAnime, &pos, this->actor.shape.rot.y);

    if (flags & 1) {
        if (!LINK_IS_ADULT) {
            pos.x *= 0.64f;
            pos.z *= 0.64f;
        }
        this->actor.world.pos.x += pos.x * this->actor.scale.x;
        this->actor.world.pos.z += pos.z * this->actor.scale.z;
    }

    if (flags & 2) {
        if (!(flags & 4)) {
            pos.y *= this->ageProperties->unk_08;
        }
        this->actor.world.pos.y += pos.y * this->actor.scale.y;
    }

    Player_AddRootYawToShapeYaw(this);
}

#define ANIM_REPLACE_APPLY_FLAG_8 (1 << 8)
#define ANIM_REPLACE_APPLY_FLAG_9 (1 << 9)

void Player_AnimReplaceApplyFlags(PlayState* play, Player* this, s32 flags) {
    if (flags & ANIM_REPLACE_APPLY_FLAG_9) {
        Player_SkelAnimeResetPrevTranslRotAgeScale(this);
    } else if ((flags & ANIM_REPLACE_APPLY_FLAG_8) || (this->skelAnime.moveFlags != 0)) {
        Player_SkelAnimeResetPrevTranslRot(this);
    } else {
        this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
        this->skelAnime.prevRot = this->actor.shape.rot.y;
    }

    this->skelAnime.moveFlags = flags & 0xFF;
    Player_ZeroSpeedXZ(this);
    AnimationContext_DisableQueue(play);
}

void Player_AnimReplacePlayOnceSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags,
                                        f32 playbackSpeed) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_AnimReplaceApplyFlags(play, this, flags);
}

void Player_AnimReplacePlayOnce(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_AnimReplacePlayOnceSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_AnimReplacePlayOnceAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_AnimReplacePlayOnceSetSpeed(play, this, anim, flags, PLAYER_ANIM_ADJUSTED_SPEED);
}

void Player_AnimReplaceNormalPlayOnceAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_AnimReplacePlayOnceAdjusted(play, this, anim,
                                       ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE);
}

void Player_AnimReplacePlayLoopSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags,
                                        f32 playbackSpeed) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_AnimReplaceApplyFlags(play, this, flags);
}

void Player_AnimReplacePlayLoop(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_AnimReplacePlayLoopSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_AnimReplacePlayLoopAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_AnimReplacePlayLoopSetSpeed(play, this, anim, flags, PLAYER_ANIM_ADJUSTED_SPEED);
}

void Player_AnimReplaceNormalPlayLoopAdjusted(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_AnimReplacePlayLoopAdjusted(play, this, anim,
                                       ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE);
}

void Player_ProcessControlStick(PlayState* play, Player* this) {
    s8 phi_v1;
    s8 phi_v0;

    this->prevControlStickMagnitude = sControlStickMagnitude;
    this->prevControlStickAngle = sControlStickAngle;

    Lib_GetControlStickData(&sControlStickMagnitude, &sControlStickAngle, sControlInput);

    sCameraOffsetControlStickAngle = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + sControlStickAngle;

    this->inputFrameCounter = (this->inputFrameCounter + 1) % 4;

    if (sControlStickMagnitude < 55.0f) {
        phi_v0 = -1;
        phi_v1 = -1;
    } else {
        phi_v1 = (u16)(sControlStickAngle + 0x2000) >> 9;
        phi_v0 = (u16)((s16)(sCameraOffsetControlStickAngle - this->actor.shape.rot.y) + 0x2000) >> 14;
    }

    this->analogStickInputs[this->inputFrameCounter] = phi_v1;
    this->relativeAnalogStickInputs[this->inputFrameCounter] = phi_v0;
}

void Player_PlayAnimOnceWithWaterInfluence(PlayState* play, Player* this, LinkAnimationHeader* linkAnim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, linkAnim, sWaterSpeedScale);
}

int Player_IsSwimming(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_SWIMMING) && !(this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag);
}

s32 Player_IsAimingBoomerang(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_AIMING_BOOMERANG);
}

void Player_SetGetItemDrawIdPlusOne(Player* this, PlayState* play) {
    GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];

    this->giDrawIdPlusOne = ABS(giEntry->gi);
}

LinkAnimationHeader* Player_GetStandStillAnim(Player* this) {
    return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType);
}

s32 Player_IsPlayingIdleAnim(Player* this) {
    LinkAnimationHeader** entry;
    s32 i;

    if (Player_GetStandStillAnim(this) != this->skelAnime.animation) {
        for (i = 0, entry = &sIdleAnims[0][0]; i < 28; i++, entry++) {
            if (this->skelAnime.animation == *entry) {
                return i + 1;
            }
        }
        return 0;
    }

    return -1;
}

void Player_PlayIdleAnimSfx(Player* this, s32 arg1) {
    if (sIdleAnimOffsetToAnimSfx[arg1] != 0) {
        Player_ProcessAnimSfxList(this, sIdleAnimSfx[sIdleAnimOffsetToAnimSfx[arg1] - 1]);
    }
}

LinkAnimationHeader* Player_GetRunningAnim(Player* this) {
    if (this->runDamageTimer != 0) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_damage_run, this->modelAnimType);
    } else if (!(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
               (this->currentBoots == PLAYER_BOOTS_IRON)) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_heavy_run, this->modelAnimType);
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_run, this->modelAnimType);
    }
}

int Player_IsAimingReadyBoomerang(Player* this) {
    return Player_IsAimingBoomerang(this) && (this->fpsItemTimer != 0);
}

LinkAnimationHeader* Player_GetFightingRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitR, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetFightingLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitL, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetFinishSidewalkAnim(Player* this) {
    if (Actor_PlayerIsAimingReadyFpsItem(this)) {
        return &gPlayerAnim_link_bow_side_walk;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walk, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkL, this->modelAnimType);
    }
}

void Player_SetUpperActionFunc(Player* this, UpperActionFunc upperActionFunc) {
    this->upperActionFunc = upperActionFunc;
    this->fpsItemShootState = 0;
    this->upperAnimBlendWeight = 0.0f;
    Player_StopInterruptableSfx(this);
}

void Player_InitItemActionWithAnim(PlayState* play, Player* this, s8 itemAction) {
    LinkAnimationHeader* current = this->skelAnime.animation;
    LinkAnimationHeader** iter = sPlayerAnimations + this->modelAnimType;
    u32 animGroup;

    this->stateFlags1 &= ~(PLAYER_STATE1_AIMING_FPS_ITEM | PLAYER_STATE1_AIMING_BOOMERANG);

    for (animGroup = 0; animGroup < PLAYER_ANIMGROUP_MAX; animGroup++) {
        if (current == *iter) {
            break;
        }
        iter += PLAYER_ANIMTYPE_MAX;
    }

    Player_InitItemAction(play, this, itemAction);

    if (animGroup < PLAYER_ANIMGROUP_MAX) {
        this->skelAnime.animation = GET_PLAYER_ANIM(animGroup, this->modelAnimType);
    }
}

s8 Player_ItemToItemAction(s32 item) {
    if (item >= ITEM_NONE_FE) {
        return PLAYER_IA_NONE;
    } else if (item == ITEM_SWORD_CS) {
        return PLAYER_IA_SWORD_CS;
    } else if (item == ITEM_FISHING_POLE) {
        return PLAYER_IA_FISHING_POLE;
    } else {
        return sItemActions[item];
    }
}

void Player_InitDefaultIA(PlayState* play, Player* this) {
}

void Player_InitDekuStickIA(PlayState* play, Player* this) {
    this->unk_85C = 1.0f;
}

void Player_InitHammerIA(PlayState* play, Player* this) {
}

void Player_InitBowOrSlingshotIA(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_FPS_ITEM;

    if (this->heldItemAction != PLAYER_IA_SLINGSHOT) {
        this->unk_860 = -1;
    } else {
        this->unk_860 = -2;
    }
}

void Player_InitExplosiveIA(PlayState* play, Player* this) {
    s32 explosiveType;
    ExplosiveInfo* explosiveInfo;
    Actor* spawnedActor;

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        Player_TryUnequipItem(play, this);
        return;
    }

    explosiveType = Player_GetExplosiveHeld(this);
    explosiveInfo = &sExplosiveInfos[explosiveType];

    spawnedActor =
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, explosiveInfo->actorId, this->actor.world.pos.x,
                           this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, 0);
    if (spawnedActor != NULL) {
        if ((explosiveType != 0) && (play->bombchuBowlingStatus != 0)) {
            play->bombchuBowlingStatus--;
            if (play->bombchuBowlingStatus == 0) {
                play->bombchuBowlingStatus = -1;
            }
        } else {
            Inventory_ChangeAmmo(explosiveInfo->itemId, -1);
        }

        this->interactRangeActor = spawnedActor;
        this->heldActor = spawnedActor;
        this->getItemId = GI_NONE;
        this->leftHandRot.y = spawnedActor->shape.rot.y - this->actor.shape.rot.y;
        this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
    }
}

void Player_InitBoostersIA(PlayState* play, Player* this) {
    this->boostCooldownTimer = 0;
}

void Player_InitHookshotIA(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_FPS_ITEM;
    this->unk_860 = -3;

    this->heldActor =
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_ARMS_HOOK, this->actor.world.pos.x,
                           this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, 0);
}

void Player_InitBoomerangIA(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_BOOMERANG;
}

void Player_InitItemAction(PlayState* play, Player* this, s8 itemAction) {
    this->unk_860 = 0;
    this->unk_85C = 0.0f;
    this->spinAttackTimer = 0.0f;

    this->heldItemAction = this->itemAction = itemAction;
    this->modelGroup = this->nextModelGroup;

    this->stateFlags1 &= ~(PLAYER_STATE1_AIMING_FPS_ITEM | PLAYER_STATE1_AIMING_BOOMERANG);

    sItemActionInitFuncs[itemAction](play, this);

    Player_SetModelGroup(this, this->modelGroup);
}

void Player_MeleeAttack(Player* this, s32 newMeleeWeaponState) {
    u16 itemSfx;
    u16 voiceSfx;

    if (this->meleeWeaponState == 0) {
        if ((this->heldItemAction == PLAYER_IA_SWORD_BIGGORON) &&
            (gSaveContext.save.info.playerData.swordHealth > 0.0f)) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else {
            itemSfx = NA_SE_IT_SWORD_SWING;
        }

        voiceSfx = NA_SE_VO_LI_SWORD_N;
        if (this->heldItemAction == PLAYER_IA_HAMMER) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else if (this->meleeAttackType >= PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
            itemSfx = 0;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        } else if (this->slashCounter >= 3) {
            itemSfx = NA_SE_IT_SWORD_SWING_HARD;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        }

        if (itemSfx != 0) {
            Player_PlayReactableSfx(this, itemSfx);
        }

        if (!((this->meleeAttackType >= PLAYER_MELEEATKTYPE_FLIPSLASH_START) &&
              (this->meleeAttackType <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH))) {
            Player_PlayVoiceSfxForAge(this, voiceSfx);
        }
    }

    this->meleeWeaponState = newMeleeWeaponState;
}

s32 Player_CheckCalmTargeting(Player* this) {
    if (this->stateFlags1 & (PLAYER_STATE1_Z_LOCK_ON | PLAYER_STATE1_Z_PARALLEL | PLAYER_STATE1_TARGET_ACTOR_LOST)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_TryBattleTargeting(Player* this) {
    if ((this->targetActor != NULL) && CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
        this->stateFlags1 |= PLAYER_STATE1_Z_TARGETING_BATTLE;
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_BATTLE) {
        this->stateFlags1 &= ~PLAYER_STATE1_Z_TARGETING_BATTLE;
        if (this->speedXZ == 0.0f) {
            this->yaw = this->actor.shape.rot.y;
        }
    }

    return 0;
}

int Player_CheckTargeting(Player* this) {
    return Player_CheckBattleTargeting(this) || Player_CheckCalmTargeting(this);
}

int Player_CheckCalmAndTryBattleTargeting(Player* this) {
    return Player_TryBattleTargeting(this) || Player_CheckCalmTargeting(this);
}

void Player_ResetLRBlendWeight(Player* this) {
    this->leftRightBlendWeight = this->leftRightBlendWeightTarget = 0.0f;
}

s32 Player_ItemIsInUse(Player* this, s32 item) {
    if ((item < ITEM_NONE_FE) && (Player_ItemToItemAction(item) == this->itemAction)) {
        return true;
    } else {
        return false;
    }
}

s32 Player_ItemIsItemAction(s32 item1, s32 itemAction) {
    if ((item1 < ITEM_NONE_FE) && (Player_ItemToItemAction(item1) == itemAction)) {
        return true;
    } else {
        return false;
    }
}

s32 Player_GetItemOnButton(PlayState* play, s32 index) {
    if (index >= 4) {
        return ITEM_NONE;
    } else if (play->bombchuBowlingStatus != 0) {
        return (play->bombchuBowlingStatus > 0) ? ITEM_BOMBCHU : ITEM_NONE;
    } else if (index == 0) {
        return B_BTN_ITEM;
    } else if (index == 1) {
        return C_BTN_ITEM(0);
    } else if (index == 2) {
        return C_BTN_ITEM(1);
    } else {
        return C_BTN_ITEM(2);
    }
}

/**
 * Handles the high level item usage and changing process based on the B and C buttons.
 *
 * Tasks include:
 *    - Put away a mask if it is not present on any C button
 *    - Put away an item if it is not present on the B button or any C button
 *    - Use an item on the B button or any C button if the corresponding button is pressed
 *    - Keep track of the current item button being held down
 */
void Player_ProcessItemButtons(Player* this, PlayState* play) {
    s32 maskItemAction;
    s32 item;
    s32 i;

    if (this->currentMask != PLAYER_MASK_NONE) {
        maskItemAction = this->currentMask - 1 + PLAYER_IA_MASK_KEATON;

        if (!Player_ItemIsItemAction(C_BTN_ITEM(0), maskItemAction) &&
            !Player_ItemIsItemAction(C_BTN_ITEM(1), maskItemAction) &&
            !Player_ItemIsItemAction(C_BTN_ITEM(2), maskItemAction)) {
            this->currentMask = PLAYER_MASK_NONE;
        }
    }

    if (!(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE)) &&
        !Player_IsShootingHookshot(this)) {
        if (this->itemAction >= PLAYER_IA_FISHING_POLE) {
            if (!Player_ItemIsInUse(this, B_BTN_ITEM) && !Player_ItemIsInUse(this, C_BTN_ITEM(0)) &&
                !Player_ItemIsInUse(this, C_BTN_ITEM(1)) && !Player_ItemIsInUse(this, C_BTN_ITEM(2))) {
                Player_UseItem(play, this, ITEM_NONE);
                return;
            }
        }

        for (i = 0; i < ARRAY_COUNT(sItemButtons); i++) {
            if (CHECK_BTN_ALL(sControlInput->press.button, sItemButtons[i])) {
                break;
            }
        }

        item = Player_GetItemOnButton(play, i);

        if (item >= ITEM_NONE_FE) {
            for (i = 0; i < ARRAY_COUNT(sItemButtons); i++) {
                if (CHECK_BTN_ALL(sControlInput->cur.button, sItemButtons[i])) {
                    break;
                }
            }

            item = Player_GetItemOnButton(play, i);

            if ((item < ITEM_NONE_FE) && (Player_ItemToItemAction(item) == this->heldItemAction)) {
                sHeldItemButtonIsHeldDown = true;
            }
        } else {
            this->heldItemButton = i;
            Player_UseItem(play, this, item);
        }
    }
}

void Player_SetupChangeHeldItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 endFrameTemp;
    f32 startFrame;
    f32 endFrame;
    f32 playSpeed;
    s32 itemChangeType;
    s8 heldItemAction;
    s32 nextAnimType;

    heldItemAction = Player_ItemToItemAction(this->heldItemId);

    Player_SetUpperActionFunc(this, Player_UpperAction_ChangeHeldItem);

    nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    itemChangeType = sItemChangeTypes[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType];

    if ((heldItemAction == PLAYER_IA_BOTTLE) || (heldItemAction == PLAYER_IA_BOOMERANG) ||
        ((heldItemAction == PLAYER_IA_NONE) &&
         ((this->heldItemAction == PLAYER_IA_BOTTLE) || (this->heldItemAction == PLAYER_IA_BOOMERANG)))) {
        itemChangeType = (heldItemAction == PLAYER_IA_NONE) ? -PLAYER_ITEM_CHG_LEFT_HAND : PLAYER_ITEM_CHG_LEFT_HAND;
    }

    this->itemChangeType = ABS(itemChangeType);
    anim = sItemChangeInfo[this->itemChangeType].anim;

    if ((anim == &gPlayerAnim_link_normal_fighter2free) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        anim = &gPlayerAnim_link_normal_free2fighter_free;
    }

    endFrameTemp = Animation_GetLastFrame(anim);
    endFrame = endFrameTemp;

    if (itemChangeType >= 0) {
        playSpeed = 1.2f;
        startFrame = 0.0f;
    } else {
        endFrame = 0.0f;
        playSpeed = -1.2f;
        startFrame = endFrameTemp;
    }

    if (heldItemAction != PLAYER_IA_NONE) {
        playSpeed *= 2.0f;
    }

    LinkAnimation_Change(play, &this->upperSkelAnime, anim, playSpeed, startFrame, endFrame, ANIMMODE_ONCE, 0.0f);

    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGING_HELD_ITEM;
}

void Player_UpdateItems(Player* this, PlayState* play) {
    if ((this->actor.category == ACTORCAT_PLAYER) && !(this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) &&
        ((this->heldItemAction == this->itemAction) || (this->stateFlags1 & PLAYER_STATE1_DEFENDING)) &&
        (gSaveContext.save.info.playerData.health != 0) && (play->csCtx.state == CS_STATE_IDLE) &&
        (this->csAction == PLAYER_CSACTION_NONE) && (play->shootingGalleryStatus == 0) &&
        (play->activeCamId == CAM_ID_MAIN) && (play->transitionTrigger != TRANS_TRIGGER_START) &&
        (gSaveContext.timerState != TIMER_STATE_STOP)) {
        Player_ProcessItemButtons(this, play);
    }

    if (this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) {
        Player_SetupChangeHeldItem(this, play);
    }
}

s32 Player_GetFpsItemAmmo(PlayState* play, Player* this, s32* itemPtr, s32* typePtr) {
    if (LINK_IS_ADULT) {
        *itemPtr = ITEM_BOW;
        if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
            *typePtr = ARROW_NORMAL_HORSE;
        } else {
            *typePtr = ARROW_NORMAL + (this->heldItemAction - PLAYER_IA_BOW);
        }
    } else {
        *itemPtr = ITEM_SLINGSHOT;
        *typePtr = ARROW_SEED;
    }

    if (gSaveContext.minigameState == 1) {
        return play->interfaceCtx.hbaAmmo;
    } else if (play->shootingGalleryStatus != 0) {
        return play->shootingGalleryStatus;
    } else {
        return AMMO(*itemPtr);
    }
}

s32 Player_TryReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 item;
    s32 arrowType;
    s32 magicArrowType;

    if ((this->heldItemAction >= PLAYER_IA_BOW_FIRE) && (this->heldItemAction <= PLAYER_IA_BOW_0E) &&
        (gSaveContext.magicState != MAGIC_STATE_IDLE)) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
    } else {
        Player_SetUpperActionFunc(this, Player_UpperAction_ReadyFpsItemToShoot);

        this->stateFlags1 |= PLAYER_STATE1_READY_TO_SHOOT;
        this->fpsItemTimer = 14;

        if (this->unk_860 >= 0) {
            Player_PlaySfx(this, sFpsItemReadySfx[ABS(this->unk_860) - 1]);

            if (!Player_HoldsHookshot(this) && (Player_GetFpsItemAmmo(play, this, &item, &arrowType) > 0)) {
                magicArrowType = arrowType - ARROW_FIRE;

                if (this->unk_860 >= 0) {
                    if ((magicArrowType >= 0) && (magicArrowType <= 2) &&
                        !Magic_RequestChange(play, sMagicArrowCosts[magicArrowType], MAGIC_CONSUME_NOW)) {
                        arrowType = ARROW_NORMAL;
                    }

                    this->heldActor = Actor_SpawnAsChild(
                        &play->actorCtx, &this->actor, play, ACTOR_EN_ARROW, this->actor.world.pos.x,
                        this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, arrowType);
                }
            }
        }

        return 1;
    }

    return 0;
}

void Player_FinalizeItemChange(PlayState* play, Player* this) {
    if (this->heldItemAction != PLAYER_IA_NONE) {
        if (Player_ActionToSword(this, this->heldItemAction) >= 0) {
            Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PUTAWAY);
        } else {
            Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
        }
    }

    Player_UseItem(play, this, this->heldItemId);

    if (Player_ActionToSword(this, this->heldItemAction) >= 0) {
        Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PICKOUT);
    } else if (this->heldItemAction != PLAYER_IA_NONE) {
        Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
    }
}

void Player_CompleteItemChange(PlayState* play, Player* this) {
    if (Player_UpperAction_ChangeHeldItem == this->upperActionFunc) {
        Player_FinalizeItemChange(play, this);
    }

    Player_SetUpperActionFunc(this, sItemActionUpdateFuncs[this->heldItemAction]);
    this->fpsItemTimer = 0;
    this->idleCounter = 0;
    Player_DetachHeldActor(play, this);
    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGING_HELD_ITEM;
}

LinkAnimationHeader* Player_GetStandingDefendAnim(PlayState* play, Player* this) {
    Player_SetUpperActionFunc(this, Player_UpperAction_StandingDefense);
    Player_DetachHeldActor(play, this);

    if (this->leftRightBlendWeight < 0.5f) {
        return sRightDefendStandingAnims[Player_HoldsTwoHandedWeapon(this)];
    } else {
        return sLeftDefendStandingAnims[Player_HoldsTwoHandedWeapon(this)];
    }
}

s32 Player_TryStartTargetingDefense(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (!(this->stateFlags1 & (PLAYER_STATE1_DEFENDING | PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_IN_CUTSCENE)) &&
        (play->shootingGalleryStatus == 0) && (this->heldItemAction == this->itemAction) &&
        (this->currentShield != PLAYER_SHIELD_NONE) && !Player_IsChildWithHylianShield(this) &&
        Player_CheckTargeting(this) && CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {

        anim = Player_GetStandingDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->upperSkelAnime, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
        Player_PlaySfx(this, NA_SE_IT_SHIELD_POSTURE);

        return 1;
    } else {
        return 0;
    }
}

s32 Player_UpperAction_Wait(Player* this, PlayState* play) {
    if (Player_TryStartTargetingDefense(play, this)) {
        return 1;
    } else {
        return 0;
    }
}

void Player_SetupFinishDefense(Player* this) {
    Player_SetUpperActionFunc(this, Player_UpperAction_FinishDefense);

    if (this->itemAction < 0) {
        Player_SetItemActionFromHeldItem(this);
    }

    Animation_Reverse(&this->upperSkelAnime);
    Player_PlaySfx(this, NA_SE_IT_SHIELD_REMOVE);
}

void Player_WaitToFinalizeItemChange(PlayState* play, Player* this) {
    ItemChangeInfo* itemChangeEntry = &sItemChangeInfo[this->itemChangeType];
    f32 changeFrame;

    changeFrame = itemChangeEntry->changeFrame;
    changeFrame = (this->upperSkelAnime.playSpeed < 0.0f) ? changeFrame - 1.0f : changeFrame;

    if (LinkAnimation_OnFrame(&this->upperSkelAnime, changeFrame)) {
        Player_FinalizeItemChange(play, this);
    }

    Player_TryBattleTargeting(this);
}

s32 Player_TryStartChangingHeldItem(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) {
        Player_SetupChangeHeldItem(this, play);
    } else {
        return 0;
    }

    return 1;
}

/**
 * The actual sword weapon is not handled here. See `Player_ActionChange_TryStartMeleeWeaponAttack` for melee weapon
 * usage. This upper body action allows for shielding or changing held items while a sword is in hand.
 */
s32 Player_UpperAction_Sword(Player* this, PlayState* play) {
    if (Player_TryStartTargetingDefense(play, this) || Player_TryStartChangingHeldItem(this, play)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_UpperAction_ChangeHeldItem(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->upperSkelAnime) ||
        ((Player_ItemToItemAction(this->heldItemId) == this->heldItemAction) &&
         (sUseHeldItem =
              (sUseHeldItem || ((this->modelAnimType != PLAYER_ANIMTYPE_3) && (play->shootingGalleryStatus == 0)))))) {
        Player_SetUpperActionFunc(this, sItemActionUpdateFuncs[this->heldItemAction]);
        this->fpsItemTimer = 0;
        this->idleCounter = 0;
        sHeldItemButtonIsHeldDown = sUseHeldItem;
        return this->upperActionFunc(this, play);
    }

    if (Player_IsPlayingIdleAnim(this) != 0) {
        Player_WaitToFinalizeItemChange(play, this);
        Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
        this->idleCounter = 0;
    } else {
        Player_WaitToFinalizeItemChange(play, this);
    }

    return 1;
}

s32 Player_UpperAction_StandingDefense(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->upperSkelAnime);

    if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
        Player_SetupFinishDefense(this);
        return 1;
    } else {
        this->stateFlags1 |= PLAYER_STATE1_DEFENDING;
        Player_SetModelsForHoldingShield(this);
        return 1;
    }
}

s32 Player_UpperAction_DeflectAttackStanding(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
        anim = Player_GetStandingDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->upperSkelAnime, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
    }

    this->stateFlags1 |= PLAYER_STATE1_DEFENDING;
    Player_SetModelsForHoldingShield(this);

    return 1;
}

s32 Player_UpperAction_FinishDefense(Player* this, PlayState* play) {
    sUseHeldItem = sHeldItemButtonIsHeldDown;

    if (sUseHeldItem || LinkAnimation_Update(play, &this->upperSkelAnime)) {
        Player_SetUpperActionFunc(this, sItemActionUpdateFuncs[this->heldItemAction]);
        LinkAnimation_PlayLoop(play, &this->upperSkelAnime,
                               GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType));
        this->idleCounter = 0;
        this->upperActionFunc(this, play);
        return 0;
    }

    return 1;
}

s32 Player_TryUseFpsItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    if (this->heldItemAction != PLAYER_IA_BOOMERANG) {
        if (!Player_TryReadyFpsItemToShoot(this, play)) {
            return 0;
        }

        if (!Player_HoldsHookshot(this)) {
            anim = &gPlayerAnim_link_bow_bow_ready;
        } else {
            anim = &gPlayerAnim_link_hook_shot_ready;
        }
        LinkAnimation_PlayOnce(play, &this->upperSkelAnime, anim);
    } else {
        Player_SetUpperActionFunc(this, Player_UpperAction_StartAimBoomerang);
        this->fpsItemTimer = 10;
        LinkAnimation_PlayOnce(play, &this->upperSkelAnime, &gPlayerAnim_link_boom_throw_wait2waitR);
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_uma_anim_walk);
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !Player_TryBattleTargeting(this)) {
        Player_AnimPlayLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType));
    }

    return 1;
}

int Player_CheckShootingGalleryShootInput(PlayState* play) {
    return (play->shootingGalleryStatus > 0) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B);
}

int Player_CheckShootingGalleryOtherInputs(PlayState* play) {
    return (play->shootingGalleryStatus != 0) &&
           ((play->shootingGalleryStatus < 0) ||
            CHECK_BTN_ANY(sControlInput->cur.button, BTN_A | BTN_B | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN));
}

s32 Player_SetAimAttention(Player* this, PlayState* play) {
    if ((this->attentionMode == 0) || (this->attentionMode == 2)) {
        if (Player_CheckTargeting(this) ||
            (Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_AIM_ADULT) == 0)) {
            return 1;
        }
        this->attentionMode = 2;
    }

    return 0;
}

s32 Player_CanUseFpsItem(Player* this, PlayState* play) {
    if ((this->doorType == PLAYER_DOORTYPE_NONE) && !(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG)) {
        if (sUseHeldItem || Player_CheckShootingGalleryShootInput(play)) {
            if (Player_TryUseFpsItem(this, play)) {
                return Player_SetAimAttention(this, play);
            }
        }
    }

    return 0;
}

s32 Player_FinishHookshotMove(Player* this) {
    if (this->actor.child != NULL) {
        if (this->heldActor == NULL) {
            this->heldActor = this->actor.child;
            Player_RequestRumble(this, 255, 10, 250, 0);
            Player_PlaySfx(this, NA_SE_IT_HOOKSHOT_RECEIVE);
        }

        return 1;
    }

    return 0;
}

s32 Player_UpperAction_HoldFpsItem(Player* this, PlayState* play) {
    if (this->unk_860 >= 0) {
        this->unk_860 = -this->unk_860;
    }

    if ((!Player_HoldsHookshot(this) || Player_FinishHookshotMove(this)) &&
        !Player_TryStartTargetingDefense(play, this) && !Player_CanUseFpsItem(this, play)) {
        return 0;
    }

    return 1;
}

s32 Player_TryShootFpsItem(PlayState* play, Player* this) {
    s32 item;
    s32 arrowType;

    if (this->heldActor != NULL) {
        if (!Player_HoldsHookshot(this)) {
            Player_GetFpsItemAmmo(play, this, &item, &arrowType);

            if (gSaveContext.minigameState == 1) {
                play->interfaceCtx.hbaAmmo--;
            } else if (play->shootingGalleryStatus != 0) {
                play->shootingGalleryStatus--;
            } else {
                Inventory_ChangeAmmo(item, -1);
            }

            if (play->shootingGalleryStatus == 1) {
                play->shootingGalleryStatus = -10;
            }

            Player_RequestRumble(this, 150, 10, 150, 0);
        } else {
            Player_RequestRumble(this, 255, 20, 150, 0);
        }

        this->fpsItemShotTimer = 4;
        this->heldActor->parent = NULL;
        this->actor.child = NULL;
        this->heldActor = NULL;

        return 1;
    }

    return 0;
}

static u16 sFpsItemNoAmmoSfx[] = { NA_SE_IT_BOW_FLICK, NA_SE_IT_SLING_FLICK };

s32 Player_UpperAction_ReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 isHoldingHookshot;

    if (!Player_HoldsHookshot(this)) {
        isHoldingHookshot = 0;
    } else {
        isHoldingHookshot = 1;
    }

    Math_ScaledStepToS(&this->upperBodyRot.z, 1200, 400);
    this->lookFlags |= 0x100;

    if ((this->fpsItemShootState == 0) && (Player_IsPlayingIdleAnim(this) == 0) &&
        (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk)) {
        LinkAnimation_PlayOnce(play, &this->upperSkelAnime, sReadyFpsItemWhileWalkingAnims[isHoldingHookshot]);
        this->fpsItemShootState = -1;
    } else if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
        LinkAnimation_PlayLoop(play, &this->upperSkelAnime, sReadyFpsItemAnims[isHoldingHookshot]);
        this->fpsItemShootState = 1;
    } else if (this->fpsItemShootState == 1) {
        this->fpsItemShootState = 2;
    }

    if (this->fpsItemTimer > 10) {
        this->fpsItemTimer--;
    }

    Player_SetAimAttention(this, play);

    if ((this->fpsItemShootState > 0) &&
        ((this->unk_860 < 0) || (!sHeldItemButtonIsHeldDown && !Player_CheckShootingGalleryOtherInputs(play)))) {
        Player_SetUpperActionFunc(this, Player_UpperAction_AimFpsItem);
        if (this->unk_860 >= 0) {
            if (isHoldingHookshot == 0) {
                if (!Player_TryShootFpsItem(play, this)) {
                    Player_PlaySfx(this, sFpsItemNoAmmoSfx[ABS(this->unk_860) - 1]);
                }
            } else if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_TryShootFpsItem(play, this);
            }
        }
        this->fpsItemTimer = 10;
        Player_ZeroSpeedXZ(this);
    } else {
        this->stateFlags1 |= PLAYER_STATE1_READY_TO_SHOOT;
    }

    return 1;
}

s32 Player_UpperAction_AimFpsItem(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->upperSkelAnime);

    if (Player_HoldsHookshot(this) && !Player_FinishHookshotMove(this)) {
        return 1;
    }

    if (!Player_TryStartTargetingDefense(play, this) &&
        (sUseHeldItem || ((this->unk_860 < 0) && sHeldItemButtonIsHeldDown) ||
         Player_CheckShootingGalleryShootInput(play))) {
        this->unk_860 = ABS(this->unk_860);

        if (Player_TryReadyFpsItemToShoot(this, play)) {
            if (Player_HoldsHookshot(this)) {
                this->fpsItemShootState = 1;
            } else {
                LinkAnimation_PlayOnce(play, &this->upperSkelAnime, &gPlayerAnim_link_bow_bow_shoot_next);
            }
        }
    } else {
        if (this->fpsItemTimer != 0) {
            this->fpsItemTimer--;
        }

        if (Player_CheckTargeting(this) || (this->attentionMode != 0) ||
            (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
            if (this->fpsItemTimer == 0) {
                this->fpsItemTimer++;
            }
            return 1;
        }

        if (Player_HoldsHookshot(this)) {
            Player_SetUpperActionFunc(this, Player_UpperAction_HoldFpsItem);
        } else {
            Player_SetUpperActionFunc(this, Player_UpperAction_FinishAimFpsItem);
            LinkAnimation_PlayOnce(play, &this->upperSkelAnime, &gPlayerAnim_link_bow_bow_shoot_end);
        }

        this->fpsItemTimer = 0;
    }

    return 1;
}

s32 Player_UpperAction_FinishAimFpsItem(Player* this, PlayState* play) {
    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || LinkAnimation_Update(play, &this->upperSkelAnime)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_HoldFpsItem);
    }

    return 1;
}

void Player_SetupZParallel(Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_Z_PARALLEL;

    if (!(this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_7) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sShapeYawToTouchedWall < 0x2000)) {
        this->yaw = this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
    }

    this->zTargetYaw = this->actor.shape.rot.y;
}

s32 Player_TryStopCarryingActor(PlayState* play, Player* this, Actor* heldActor) {
    if (heldActor == NULL) {
        Player_StopCarryingActor(play, this);
        Player_SetupContextualStandStill(this, play);
        return 1;
    }

    return 0;
}

void Player_SetupCarryingActor(Player* this, PlayState* play) {
    if (!Player_TryStopCarryingActor(play, this, this->heldActor)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_CarryActor);
        LinkAnimation_PlayLoop(play, &this->upperSkelAnime, &gPlayerAnim_link_normal_carryB_wait);
    }
}

s32 Player_UpperAction_CarryActor(Player* this, PlayState* play) {
    Actor* heldActor = this->heldActor;

    if (heldActor == NULL) {
        Player_CompleteItemChange(play, this);
    }

    if (Player_TryStartTargetingDefense(play, this)) {
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
            LinkAnimation_PlayLoop(play, &this->upperSkelAnime, &gPlayerAnim_link_normal_carryB_wait);
        }

        if ((heldActor->id == ACTOR_EN_NIW) && (this->actor.velocity.y <= 0.0f)) {
            this->actor.minVelocityY = -2.0f;
            this->actor.gravity = -0.5f;
            this->fallStartHeight = this->actor.world.pos.y;
        }

        return 1;
    }

    return Player_UpperAction_Wait(this, play);
}

void Player_SetLeftHandDlists(Player* this, Gfx** dLists) {
    this->leftHandDLists = dLists + gSaveContext.save.linkAge;
}

s32 Player_UpperAction_CarryBoomerang(Player* this, PlayState* play) {
    if (Player_TryStartTargetingDefense(play, this)) {
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
        Player_SetUpperActionFunc(this, Player_UpperAction_WaitForBoomerang);
    } else if (Player_CanUseFpsItem(this, play)) {
        return 1;
    }

    return 0;
}

s32 Player_UpperAction_StartAimBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_AimBoomerang);
        LinkAnimation_PlayLoop(play, &this->upperSkelAnime, &gPlayerAnim_link_boom_throw_waitR);
    }

    Player_SetAimAttention(this, play);

    return 1;
}

s32 Player_UpperAction_AimBoomerang(Player* this, PlayState* play) {
    LinkAnimationHeader* animSeg = this->skelAnime.animation;

    if ((Player_GetFightingRightAnim(this) == animSeg) || (Player_GetFightingLeftAnim(this) == animSeg) ||
        (Player_GetSidewalkRightAnim(this) == animSeg) || (Player_GetSidewalkLeftAnim(this) == animSeg)) {
        AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->upperSkelAnime.jointTable,
                                    this->skelAnime.jointTable);
    } else {
        LinkAnimation_Update(play, &this->upperSkelAnime);
    }

    Player_SetAimAttention(this, play);

    if (!sHeldItemButtonIsHeldDown) {
        Player_SetUpperActionFunc(this, Player_UpperAction_ThrowBoomerang);
        LinkAnimation_PlayOnce(play, &this->upperSkelAnime,
                               (this->leftRightBlendWeight < 0.5f) ? &gPlayerAnim_link_boom_throwR
                                                                   : &gPlayerAnim_link_boom_throwL);
    }

    return 1;
}

s32 Player_UpperAction_ThrowBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_WaitForBoomerang);
        this->fpsItemTimer = 0;
    } else if (LinkAnimation_OnFrame(&this->upperSkelAnime, 6.0f)) {
        f32 posX = (Math_SinS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.x;
        f32 posZ = (Math_CosS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.z;
        s32 yaw = (this->targetActor != NULL) ? this->actor.shape.rot.y + 14000 : this->actor.shape.rot.y;
        EnBoom* boomerang =
            (EnBoom*)Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOOM, posX, this->actor.world.pos.y + 30.0f, posZ,
                                 this->actor.focus.rot.x, yaw, 0, 0);

        this->boomerangActor = &boomerang->actor;
        if (boomerang != NULL) {
            boomerang->moveTo = this->targetActor;
            boomerang->returnTimer = 20;
            this->stateFlags1 |= PLAYER_STATE1_AWAITING_THROWN_BOOMERANG;
            if (!Player_CheckBattleTargeting(this)) {
                Player_SetupZParallel(this);
            }
            this->fpsItemShotTimer = 4;
            Player_PlaySfx(this, NA_SE_IT_BOOMERANG_THROW);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        }
    }

    return 1;
}

s32 Player_UpperAction_WaitForBoomerang(Player* this, PlayState* play) {
    if (Player_TryStartTargetingDefense(play, this)) {
        return 1;
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_CatchBoomerang);
        LinkAnimation_PlayOnce(play, &this->upperSkelAnime, &gPlayerAnim_link_boom_catch);
        Player_SetLeftHandDlists(this, gPlayerLeftHandBoomerangDLs);
        Player_PlaySfx(this, NA_SE_PL_CATCH_BOOMERANG);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        return 1;
    }

    return 0;
}

s32 Player_UpperAction_CatchBoomerang(Player* this, PlayState* play) {
    if (!Player_UpperAction_CarryBoomerang(this, play) && LinkAnimation_Update(play, &this->upperSkelAnime)) {
        Player_SetUpperActionFunc(this, Player_UpperAction_CarryBoomerang);
    }

    return 1;
}

// Modified Player_SetupAction
s32 Player_SetupAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 flags) {
    if (actionFunc == this->actionFunc) {
        return 0;
    }

    if (Player_Action_PlayOcarina == this->actionFunc) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        this->stateFlags2 &= ~(PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
    } else if (Player_Action_MagicSpell == this->actionFunc) {
        Player_ResetSubCam(play, this);
    }

    this->actionFunc = actionFunc;

    if ((this->itemAction != this->heldItemAction) &&
        (!(flags & 1) || !(this->stateFlags1 & PLAYER_STATE1_DEFENDING))) {
        Player_SetItemActionFromHeldItem(this);
    }

    if (!(flags & 1) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_CompleteItemChange(play, this);
        this->stateFlags1 &= ~PLAYER_STATE1_DEFENDING;
    }

    Player_FinishAnimMovement(this);

    this->stateFlags1 &= ~(PLAYER_STATE1_END_HOOKSHOT_MOVE | PLAYER_STATE1_TALKING | PLAYER_STATE1_TAKING_DAMAGE |
                           PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE |
                           PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID);
    this->stateFlags2 &=
        ~(PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING | PLAYER_STATE2_PLAYING_OCARINA_GENERAL | PLAYER_STATE2_IDLING);
    this->stateFlags3 &= ~(PLAYER_STATE3_MIDAIR | PLAYER_STATE3_ENDING_MELEE_ATTACK |
                           PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH | PLAYER_STATE3_ZORA_SWIMMING | PLAYER_STATE3_USING_BOOSTERS);

    this->av1.actionVar1 = 0;
    this->av2.actionVar2 = 0;

    this->idleCounter = 0;

    this->endZoraSwim = false;
    this->zoraSwimRoll = 0;
    this->shapeRollOffset = 0;
    this->zoraSwimTurnInput = 0;
    this->zoraSwimTurnInputSmoothed = 0;

    Player_StopInterruptableSfx(this);

    return 1;
}

#if 0
s32 Player_SetupAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 flags) {
    if (actionFunc == this->actionFunc) {
        return 0;
    }

    if (Player_Action_PlayOcarina == this->actionFunc) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        this->stateFlags2 &= ~(PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
    } else if (Player_Action_MagicSpell == this->actionFunc) {
        Player_ResetSubCam(play, this);
    }

    this->actionFunc = actionFunc;

    if ((this->itemAction != this->heldItemAction) &&
        (!(flags & 1) || !(this->stateFlags1 & PLAYER_STATE1_DEFENDING))) {
        Player_SetItemActionFromHeldItem(this);
    }

    if (!(flags & 1) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_CompleteItemChange(play, this);
        this->stateFlags1 &= ~PLAYER_STATE1_DEFENDING;
    }

    Player_FinishAnimMovement(this);

    this->stateFlags1 &= ~(PLAYER_STATE1_END_HOOKSHOT_MOVE | PLAYER_STATE1_TALKING | PLAYER_STATE1_TAKING_DAMAGE |
                           PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE |
                           PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID);
    this->stateFlags2 &=
        ~(PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING | PLAYER_STATE2_PLAYING_OCARINA_GENERAL | PLAYER_STATE2_IDLING);
    this->stateFlags3 &=
        ~(PLAYER_STATE3_MIDAIR | PLAYER_STATE3_ENDING_MELEE_ATTACK | PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH);

    this->av1.actionVar1 = 0;
    this->av2.actionVar2 = 0;

    this->idleCounter = 0;

    this->endZoraSwim = 0;
    this->zoraSwimRoll = 0;
    this->shapeRollOffset = 0;
    this->zoraSwimTurnInput = 0;
    this->zoraSwimTurnInputSmoothed = 0;

    Player_StopInterruptableSfx(this);

    return 1;
}
#endif

void Player_SetupActionKeepMoveFlags(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 flags) {
    s32 prevMoveFlags;

    prevMoveFlags = this->skelAnime.moveFlags;
    this->skelAnime.moveFlags = 0;
    Player_SetupAction(play, this, actionFunc, flags);
    this->skelAnime.moveFlags = prevMoveFlags;
}

void Player_SetupActionKeepItemAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 flags) {
    s32 prevItemAction;

    if (this->itemAction >= 0) {
        prevItemAction = this->itemAction;
        this->itemAction = this->heldItemAction;
        Player_SetupAction(play, this, actionFunc, flags);
        this->itemAction = prevItemAction;
        Player_SetModels(this, Player_ActionToModelGroup(this, this->itemAction));
    }
}

void Player_RequestCameraSetting(PlayState* play, s16 camSetting) {
    if (!Play_CamIsNotFixed(play)) {
        if (camSetting == CAM_SET_SCENE_TRANSITION) {
            Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING_ALT);
        }
    } else {
        Camera_RequestSetting(Play_GetCamera(play, CAM_ID_MAIN), camSetting);
    }
}

void Player_SetUseItemCam(PlayState* play, s32 arg1) {
    Player_RequestCameraSetting(play, CAM_SET_TURN_AROUND);
    Camera_SetCameraData(Play_GetCamera(play, CAM_ID_MAIN), 4, NULL, NULL, arg1, 0, 0);
}

void Player_DestroyHookshot(Player* this) {
    if (Player_HoldsHookshot(this)) {
        Actor* heldActor = this->heldActor;

        if (heldActor != NULL) {
            Actor_Kill(heldActor);
            this->actor.child = NULL;
            this->heldActor = NULL;
        }
    }
}

void Player_UseItem(PlayState* play, Player* this, s32 item) {
    s8 itemAction;
    s32 temp;
    s32 nextAnimType;

    itemAction = Player_ItemToItemAction(item);

    if (((this->heldItemAction == this->itemAction) &&
         (!(this->stateFlags1 & PLAYER_STATE1_DEFENDING) || (Player_ActionToMeleeWeapon(itemAction) != 0) ||
          (itemAction == PLAYER_IA_NONE))) ||
        ((this->itemAction < 0) && ((Player_ActionToMeleeWeapon(itemAction) != 0) || (itemAction == PLAYER_IA_NONE)))) {

        if ((itemAction == PLAYER_IA_NONE) || !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
            ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
             ((itemAction == PLAYER_IA_HOOKSHOT) || (itemAction == PLAYER_IA_LONGSHOT)))) {

            if ((play->bombchuBowlingStatus == 0) &&
                (((itemAction == PLAYER_IA_DEKU_STICK) && (AMMO(ITEM_DEKU_STICK) == 0)) ||
                 ((itemAction == PLAYER_IA_MAGIC_BEAN) && (AMMO(ITEM_MAGIC_BEAN) == 0)) ||
                 (temp = Player_ActionToExplosive(this, itemAction),
                  ((temp >= 0) && ((AMMO(sExplosiveInfos[temp].itemId) == 0) ||
                                   (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3)))))) {
                // Prevent some items from being used if player is out of ammo.
                // Also prevent explosives from being used if there are 3 or more active (outside of bombchu bowling)
                Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
            } else if (itemAction == PLAYER_IA_LENS_OF_TRUTH) {
                // Handle Lens of Truth
                if (Magic_RequestChange(play, 0, MAGIC_CONSUME_LENS)) {
                    if (play->actorCtx.lensActive) {
                        Actor_DisableLens(play);
                    } else {
                        play->actorCtx.lensActive = true;
                    }

                    Sfx_PlaySfxCentered((play->actorCtx.lensActive) ? NA_SE_SY_GLASSMODE_ON : NA_SE_SY_GLASSMODE_OFF);
                } else {
                    Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                }
            } else if (itemAction == PLAYER_IA_DEKU_NUT) {
                // Handle Deku Nuts
                if (AMMO(ITEM_DEKU_NUT) != 0) {
                    Player_TryThrowDekuNut(play, this);
                } else {
                    Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                }
            } else if ((temp = Player_ActionToMagicSpell(this, itemAction)) >= 0) {
                // Handle magic spells
                if (((itemAction == PLAYER_IA_FARORES_WIND) && (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0)) ||
                    ((gSaveContext.magicCapacity != 0) && (gSaveContext.magicState == MAGIC_STATE_IDLE) &&
                     (gSaveContext.save.info.playerData.magic >= sMagicSpellCosts[temp]))) {
                    this->itemAction = itemAction;
                    this->attentionMode = 4;
                } else {
                    Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                }
            } else if (itemAction >= PLAYER_IA_MASK_KEATON && itemAction < PLAYER_IA_BOOSTERS) {
                // Handle wearable masks
                if (this->currentMask != PLAYER_MASK_NONE) {
                    this->currentMask = PLAYER_MASK_NONE;
                } else {
                    this->currentMask = itemAction - PLAYER_IA_MASK_KEATON + 1;
                }

                Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
            } else if ((((itemAction >= PLAYER_IA_OCARINA_FAIRY) && (itemAction <= PLAYER_IA_OCARINA_OF_TIME)) ||
                        (itemAction >= PLAYER_IA_BOTTLE_FISH)) &&
                       (itemAction < PLAYER_IA_BOOSTERS)) {
                // Handle "cutscene items"
                if (!Player_CheckBattleTargeting(this) ||
                    ((itemAction >= PLAYER_IA_BOTTLE_POTION_RED) && (itemAction <= PLAYER_IA_BOTTLE_FAIRY))) {
                    TitleCard_Clear(play, &play->actorCtx.titleCtx);
                    this->attentionMode = 4;
                    this->itemAction = itemAction;
                }
            } else if ((itemAction != this->heldItemAction) ||
                       ((this->heldActor == NULL) && (Player_ActionToExplosive(this, itemAction) >= 0))) {
                // Handle using a new held item
                this->nextModelGroup = Player_ActionToModelGroup(this, itemAction);
                nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];

                if ((this->heldItemAction >= 0) && (Player_ActionToMagicSpell(this, itemAction) < 0) &&
                    (item != this->heldItemId) &&
                    (sItemChangeTypes[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType] !=
                     PLAYER_ITEM_CHG_DEFAULT)) {
                    // Start the held item change process
                    this->heldItemId = item;
                    this->stateFlags1 |= PLAYER_STATE1_START_CHANGING_HELD_ITEM;
                } else {
                    // Init new held item for use
                    Player_DestroyHookshot(this);
                    Player_DetachHeldActor(play, this);
                    Player_InitItemActionWithAnim(play, this, itemAction);
                }
            } else {
                // Handle using the held item already in hand
                sUseHeldItem = sHeldItemButtonIsHeldDown = true;
            }
        }
    }
}

void Player_SetupDie(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    s32 isSwimming = Player_IsSwimming(this);

    Player_StopCarryingActor(play, this);

    Player_SetupAction(play, this, isSwimming ? Player_Action_Drown : Player_Action_Die, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_DEATH_CUTSCENE;

    Player_AnimPlayOnce(play, this, anim);
    if (anim == &gPlayerAnim_link_derth_rebirth) {
        this->skelAnime.endFrame = 84.0f;
    }

    Player_ClearAttentionModeAndStopMoving(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DOWN);

    if (this->actor.category == ACTORCAT_PLAYER) {
        Audio_SetBgmVolumeOffDuringFanfare();

        if (Inventory_ConsumeFairy(play)) {
            play->gameOverCtx.state = GAMEOVER_REVIVE_START;
            this->av1.actionVar1 = 1;
        } else {
            play->gameOverCtx.state = GAMEOVER_DEATH_START;
            Audio_StopBgmAndFanfare(0);
            Audio_PlayFanfare(NA_BGM_GAME_OVER);
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
        }

        OnePointCutscene_Init(play, 9806, isSwimming ? 120 : 60, &this->actor, CAM_ID_MAIN);
        Letterbox_SetSizeTarget(32);
    }
}

int Player_CanUpdateItems(Player* this) {
    return (!(Player_Action_MiniCs == this->actionFunc) ||
            ((this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) &&
             ((this->heldItemId == ITEM_SWORD_CS) || (this->heldItemId == ITEM_NONE)))) &&
           (!(Player_UpperAction_ChangeHeldItem == this->upperActionFunc) ||
            (Player_ItemToItemAction(this->heldItemId) == this->heldItemAction));
}

s32 Player_UpdateUpperBody(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->actor.parent != NULL) &&
        Player_HoldsHookshot(this)) {
        Player_SetupAction(play, this, Player_Action_PulledByHookshot, 1);
        this->stateFlags3 |= PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH;
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_hook_fly_start);
        Player_AnimReplaceApplyFlags(play, this,
                                     ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                         ANIM_FLAG_PLAYER_7);
        Player_ClearAttentionModeAndStopMoving(this);
        this->yaw = this->actor.shape.rot.y;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        this->hoverBootsTimer = 0;
        this->lookFlags |= 0x43;
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
        return 1;
    }

    if (Player_CanUpdateItems(this)) {
        Player_UpdateItems(this, play);
        if (Player_Action_ThrowDekuNut == this->actionFunc) {
            return 1;
        }
    }

    if (!this->upperActionFunc(this, play)) {
        return 0;
    }

    if (this->upperAnimBlendWeight != 0.0f) {
        if ((Player_IsPlayingIdleAnim(this) == 0) || (this->speedXZ != 0.0f)) {
            AnimationContext_SetCopyFalse(play, this->skelAnime.limbCount, this->upperSkelAnime.jointTable,
                                          this->skelAnime.jointTable, sUpperBodyLimbCopyMap);
        }
        Math_StepToF(&this->upperAnimBlendWeight, 0.0f, 0.25f);
        AnimationContext_SetInterp(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                   this->upperSkelAnime.jointTable, 1.0f - this->upperAnimBlendWeight);
    } else if ((Player_IsPlayingIdleAnim(this) == 0) || (this->speedXZ != 0.0f)) {
        AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                     this->upperSkelAnime.jointTable, sUpperBodyLimbCopyMap);
    } else {
        AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                    this->upperSkelAnime.jointTable);
    }

    return 1;
}

s32 Player_SetupMiniCs(PlayState* play, Player* this, PlayerMiniCsFunc func) {
    this->miniCsFunc = func;
    Player_SetupAction(play, this, Player_Action_MiniCs, 0);
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    return Player_TryUnequipItem(play, this);
}

void Player_UpdateYaw(Player* this, PlayState* play) {
    s16 previousYaw = this->actor.shape.rot.y;

    if (!(this->stateFlags2 &
          (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
        if ((this->targetActor != NULL) &&
            ((play->actorCtx.targetCtx.unk_4B != 0) || (this->actor.category != ACTORCAT_PLAYER))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y,
                               Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos), 4000);
        } else if ((this->stateFlags1 & PLAYER_STATE1_Z_PARALLEL) &&
                   !(this->stateFlags2 & (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING |
                                          PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->zTargetYaw, 4000);
        }
    } else if (!(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
        Math_ScaledStepToS(&this->actor.shape.rot.y, this->yaw, 2000);
    }

    this->unk_87C = this->actor.shape.rot.y - previousYaw;
}

s32 Player_StepAngleWithOffset(s16* angle, s16 target, s16 step, s16 angleMinMax, s16 referenceAngle,
                               s16 angleDiffMinMax) {
    s16 angleDiff;
    s16 clampedAngleDiff;
    s16 originalAngle;

    angleDiff = clampedAngleDiff = referenceAngle - *angle;
    clampedAngleDiff = CLAMP(clampedAngleDiff, -angleDiffMinMax, angleDiffMinMax);
    *angle += (s16)(angleDiff - clampedAngleDiff);

    Math_ScaledStepToS(angle, target, step);

    originalAngle = *angle;
    if (*angle < -angleMinMax) {
        *angle = -angleMinMax;
    } else if (*angle > angleMinMax) {
        *angle = angleMinMax;
    }
    return originalAngle - *angle;
}

s32 Player_UpdateLookAngles(Player* this, s32 syncUpperRotToFocusRot) {
    s16 yawDiff;
    s16 lookYaw;

    lookYaw = this->actor.shape.rot.y;
    if (syncUpperRotToFocusRot != 0) {
        lookYaw = this->actor.focus.rot.y;
        this->upperBodyRot.x = this->actor.focus.rot.x;
        this->lookFlags |= 0x41;
    } else {
        Player_StepAngleWithOffset(&this->upperBodyRot.x,
                                   Player_StepAngleWithOffset(&this->headRot.x, this->actor.focus.rot.x, 600, 10000,
                                                              this->actor.focus.rot.x, 0),
                                   200, 4000, this->headRot.x, 10000);
        yawDiff = this->actor.focus.rot.y - lookYaw;
        Player_StepAngleWithOffset(&yawDiff, 0, 200, 24000, this->upperBodyRot.y, 8000);
        lookYaw = this->actor.focus.rot.y - yawDiff;
        Player_StepAngleWithOffset(&this->headRot.y, yawDiff - this->upperBodyRot.y, 200, 8000, yawDiff, 8000);
        Player_StepAngleWithOffset(&this->upperBodyRot.y, yawDiff, 200, 8000, this->headRot.y, 8000);
        this->lookFlags |= 0xD9;
    }

    return lookYaw;
}

void Player_ProcessTargeting(Player* this, PlayState* play) {
    s32 isRangeCheckDisabled = false;
    s32 zTrigPressed = CHECK_BTN_ALL(sControlInput->cur.button, BTN_Z);
    Actor* actorToTarget;
    s32 pad;
    s32 holdTarget;
    s32 actorTalkOffered;

    if (!zTrigPressed) {
        this->stateFlags1 &= ~PLAYER_STATE1_TARGET_ACTOR_LOST;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csAction != PLAYER_CSACTION_NONE) ||
        (this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE)) ||
        (this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        this->targetSwitchTimer = 0;
    } else if (zTrigPressed || (this->stateFlags2 & PLAYER_STATE2_USING_SWITCH_Z_TARGETING) ||
               (this->forcedTargetActor != NULL)) {
        if (this->targetSwitchTimer <= 5) {
            this->targetSwitchTimer = 5;
        } else {
            this->targetSwitchTimer--;
        }
    } else if (this->stateFlags1 & PLAYER_STATE1_Z_PARALLEL) {
        this->targetSwitchTimer = 0;
    } else if (this->targetSwitchTimer != 0) {
        this->targetSwitchTimer--;
    }

    if (this->targetSwitchTimer >= 6) {
        isRangeCheckDisabled = 1;
    }

    actorTalkOffered = Player_CheckActorTalkOffered(play);
    if (actorTalkOffered || (this->targetSwitchTimer != 0) ||
        (this->stateFlags1 & (PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_AWAITING_THROWN_BOOMERANG))) {
        if (!actorTalkOffered) {
            if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) &&
                ((this->heldItemAction != PLAYER_IA_FISHING_POLE) || (this->unk_860 == 0)) &&
                CHECK_BTN_ALL(sControlInput->press.button, BTN_Z)) {

                if (this->actor.category == ACTORCAT_PLAYER) {
                    actorToTarget = play->actorCtx.targetCtx.arrowPointedActor;
                } else {
                    actorToTarget = &GET_PLAYER(play)->actor;
                }

                holdTarget = (gSaveContext.zTargetSetting != 0) || (this->actor.category != ACTORCAT_PLAYER);
                this->stateFlags1 |= PLAYER_STATE1_UNUSED_Z_TARGETING_FLAG;

                if ((actorToTarget != NULL) && !(actorToTarget->flags & ACTOR_FLAG_27)) {
                    if ((actorToTarget == this->targetActor) && (this->actor.category == ACTORCAT_PLAYER)) {
                        actorToTarget = play->actorCtx.targetCtx.unk_94;
                    }

                    if (actorToTarget != this->targetActor) {
                        if (!holdTarget) {
                            this->stateFlags2 |= PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
                        }
                        this->targetActor = actorToTarget;
                        this->targetSwitchTimer = 15;
                        this->stateFlags2 &= ~(PLAYER_STATE2_CAN_SPEAK_OR_CHECK | PLAYER_STATE2_NAVI_REQUESTING_TALK);
                    } else {
                        if (!holdTarget) {
                            Player_ClearTargetActor(this);
                        }
                    }

                    this->stateFlags1 &= ~PLAYER_STATE1_TARGET_ACTOR_LOST;
                } else {
                    if (!(this->stateFlags1 & (PLAYER_STATE1_Z_PARALLEL | PLAYER_STATE1_TARGET_ACTOR_LOST))) {
                        Player_SetupZParallel(this);
                    }
                }
            }

            if (this->targetActor != NULL) {
                if ((this->actor.category == ACTORCAT_PLAYER) && (this->targetActor != this->forcedTargetActor) &&
                    Actor_OutsideTargetRange(this->targetActor, this, isRangeCheckDisabled)) {
                    Player_ClearTargetActor(this);
                    this->stateFlags1 |= PLAYER_STATE1_TARGET_ACTOR_LOST;
                } else if (this->targetActor != NULL) {
                    this->targetActor->targetPriority = 40;
                }
            } else if (this->forcedTargetActor != NULL) {
                this->targetActor = this->forcedTargetActor;
            }
        }

        if (this->targetActor != NULL) {
            this->stateFlags1 &= ~(PLAYER_STATE1_Z_LOCK_ON | PLAYER_STATE1_Z_PARALLEL);
            if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                !CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
                this->stateFlags1 |= PLAYER_STATE1_Z_LOCK_ON;
            }
        } else {
            if (this->stateFlags1 & PLAYER_STATE1_Z_PARALLEL) {
                this->stateFlags2 &= ~PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
            } else {
                Player_ClearTargetAndSetMidairStatus(this);
            }
        }
    } else {
        Player_ClearTargetAndSetMidairStatus(this);
    }
}

/**
 * These defines exist to simplify the variable used to toggle the different speed modes.
 * While the `speedMode` variable is a float and can contain a non-boolean value,
 * `Player_CalcSpeedAndYawFromControlStick` never actually uses the value for anything.
 * It simply checks if the value is non-zero to toggle the "curved" mode.
 * In practice, 0.0f or 0.018f are the only values passed to this function.
 *
 * It's clear that this value was intended to mean something in the curved mode calculation at
 * some point in development, but was either never implemented or removed.
 *
 * To see the difference between linear and curved mode, with interactive toggles for
 * speed cap and floor pitch, see the following desmos graph: https://www.desmos.com/calculator/hri7dcws4c
 */

// Linear mode is a straight line, increasing target speed at a steady rate relative to the control stick magnitude
#define SPEED_MODE_LINEAR 0.0f

// Curved mode drops any input below 20 units of magnitude, resulting in zero for target speed.
// Beyond 20 units, a gradual curve slowly moves up until around the 40 unit mark
// when target speed ramps up very quickly.
#define SPEED_MODE_CURVED 0.018f

/**
 * Calculates target speed and yaw based on input from the control stick.
 * See `Player_GetMovementSpeedAndYaw` for detailed argument descriptions.
 *
 * @return true if the control stick has any magnitude, false otherwise.
 */
s32 Player_CalcSpeedAndYawFromControlStick(PlayState* play, Player* this, f32* outSpeedTarget, s16* outYawTarget,
                                           f32 speedMode) {
    f32 temp;
    f32 sinFloorPitch;
    f32 floorPitchInfluence;
    f32 speedCap;

    if ((this->attentionMode != 0) || (play->transitionTrigger == TRANS_TRIGGER_START) ||
        (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
        *outSpeedTarget = 0.0f;
        *outYawTarget = this->actor.shape.rot.y;
    } else {
        *outSpeedTarget = sControlStickMagnitude;
        *outYawTarget = sControlStickAngle;

        // The value of `speedMode` is never actually used. It only toggles this condition.
        // See the definition of `SPEED_MODE_LINEAR` and `SPEED_MODE_CURVED` for more information.
        if (speedMode != SPEED_MODE_LINEAR) {
            *outSpeedTarget -= 20.0f;

            if (*outSpeedTarget < 0.0f) {
                // If control stick magnitude is below 20, return zero speed.
                *outSpeedTarget = 0.0f;
            } else {
                // Cosine of the control stick magnitude isn't exactly meaningful, but
                // it happens to give a desirable curve for grounded movement speed relative
                // to control stick magnitude.
                temp = 1.0f - Math_CosS(*outSpeedTarget * 450.0f);
                *outSpeedTarget = (SQ(temp) * 30.0f) + 7.0f;
            }
        } else {
            // Speed increases linearly relative to control stick magnitude
            *outSpeedTarget *= 0.8f;
        }

        if (sControlStickMagnitude != 0.0f) {
            sinFloorPitch = Math_SinS(this->floorPitch);
            speedCap = this->speedLimit;
            floorPitchInfluence = CLAMP(sinFloorPitch, 0.0f, 0.6f);

            if (this->shapeOffsetY != 0.0f) {
                speedCap -= this->shapeOffsetY * 0.008f;
                speedCap = CLAMP_MIN(speedCap, 2.0f);
            }

            *outSpeedTarget = (*outSpeedTarget * 0.14f) - (8.0f * floorPitchInfluence * floorPitchInfluence);
            *outSpeedTarget = CLAMP(*outSpeedTarget, 0.0f, speedCap);

            return true;
        }
    }

    return false;
}

s32 Player_StepSpeedXZToZero(Player* this) {
    return Math_StepToF(&this->speedXZ, 0.0f, REG(43) / 100.0f);
}

/**
 * Gets target speed and yaw values for movement based on control stick input.
 * Control stick magnitude and angle are processed in `Player_CalcSpeedAndYawFromControlStick` to get target values.
 * Additionally, this function does extra processing on the target yaw value if the control stick is neutral.
 *
 * @param outSpeedTarget  a pointer to the variable that will hold the resulting target speed value
 * @param outYawTarget    a pointer to the variable that will hold the resulting target yaw value
 * @param speedMode       toggles between a linear and curved mode for the speed value
 *
 * @see Player_CalcSpeedAndYawFromControlStick for more information on the linear vs curved speed mode.
 *
 * @return true if the control stick has any magnitude, false otherwise.
 */
s32 Player_GetMovementSpeedAndYaw(Player* this, f32* outSpeedTarget, s16* outYawTarget, f32 speedMode,
                                  PlayState* play) {
    if (!Player_CalcSpeedAndYawFromControlStick(play, this, outSpeedTarget, outYawTarget, speedMode)) {
        *outYawTarget = this->actor.shape.rot.y;

        if (this->targetActor != NULL) {
            if ((play->actorCtx.targetCtx.unk_4B != 0) &&
                !(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
                *outYawTarget = Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos);
                return false;
            }
        } else if (Player_CheckCalmTargeting(this)) {
            *outYawTarget = this->zTargetYaw;
        }

        return false;
    } else {
        *outYawTarget += Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        return true;
    }
}

typedef enum {
    /*  0 */ PLAYER_ACTION_CHG_0,
    /*  1 */ PLAYER_ACTION_CHG_1,
    /*  2 */ PLAYER_ACTION_CHG_2,
    /*  3 */ PLAYER_ACTION_CHG_3,
    /*  4 */ PLAYER_ACTION_CHG_4,
    /*  5 */ PLAYER_ACTION_CHG_5,
    /*  6 */ PLAYER_ACTION_CHG_6,
    /*  7 */ PLAYER_ACTION_CHG_7,
    /*  8 */ PLAYER_ACTION_CHG_8,
    /*  9 */ PLAYER_ACTION_CHG_9,
    /* 10 */ PLAYER_ACTION_CHG_10,
    /* 11 */ PLAYER_ACTION_CHG_11,
    /* 12 */ PLAYER_ACTION_CHG_12,
    /* 13 */ PLAYER_ACTION_CHG_13
} ActionChangeIndex;

static s8 sActionChangeList1[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_2,  PLAYER_ACTION_CHG_4, PLAYER_ACTION_CHG_9,
    PLAYER_ACTION_CHG_10, PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_8, -PLAYER_ACTION_CHG_7,
};

static s8 sActionChangeList2[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_1, PLAYER_ACTION_CHG_2, PLAYER_ACTION_CHG_5,
    PLAYER_ACTION_CHG_3,  PLAYER_ACTION_CHG_4, PLAYER_ACTION_CHG_9, PLAYER_ACTION_CHG_10,
    PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_7, PLAYER_ACTION_CHG_8, -PLAYER_ACTION_CHG_6,
};

static s8 sActionChangeList3[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_1, PLAYER_ACTION_CHG_2,  PLAYER_ACTION_CHG_3,
    PLAYER_ACTION_CHG_4,  PLAYER_ACTION_CHG_9, PLAYER_ACTION_CHG_10, PLAYER_ACTION_CHG_11,
    PLAYER_ACTION_CHG_8,  PLAYER_ACTION_CHG_7, -PLAYER_ACTION_CHG_6,
};

static s8 sActionChangeList4[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_2,  PLAYER_ACTION_CHG_4, PLAYER_ACTION_CHG_9,
    PLAYER_ACTION_CHG_10, PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_8, -PLAYER_ACTION_CHG_7,
};

static s8 sActionChangeList5[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_2,  PLAYER_ACTION_CHG_4, PLAYER_ACTION_CHG_9,  PLAYER_ACTION_CHG_10,
    PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_12, PLAYER_ACTION_CHG_8, -PLAYER_ACTION_CHG_7,
};

static s8 sActionChangeList6[] = {
    -PLAYER_ACTION_CHG_7,
};

static s8 sActionChangeList7[] = {
    PLAYER_ACTION_CHG_0, PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_1,  PLAYER_ACTION_CHG_2,
    PLAYER_ACTION_CHG_3, PLAYER_ACTION_CHG_5,  PLAYER_ACTION_CHG_4,  PLAYER_ACTION_CHG_9,
    PLAYER_ACTION_CHG_8, PLAYER_ACTION_CHG_7,  -PLAYER_ACTION_CHG_6,
};

static s8 sActionChangeList8[] = {
    PLAYER_ACTION_CHG_0, PLAYER_ACTION_CHG_11, PLAYER_ACTION_CHG_1, PLAYER_ACTION_CHG_2,
    PLAYER_ACTION_CHG_3, PLAYER_ACTION_CHG_12, PLAYER_ACTION_CHG_5, PLAYER_ACTION_CHG_4,
    PLAYER_ACTION_CHG_9, PLAYER_ACTION_CHG_8,  PLAYER_ACTION_CHG_7, -PLAYER_ACTION_CHG_6,
};

static s8 sActionChangeList9[] = {
    PLAYER_ACTION_CHG_13, PLAYER_ACTION_CHG_1, PLAYER_ACTION_CHG_2,  PLAYER_ACTION_CHG_3,  PLAYER_ACTION_CHG_12,
    PLAYER_ACTION_CHG_5,  PLAYER_ACTION_CHG_4, PLAYER_ACTION_CHG_9,  PLAYER_ACTION_CHG_10, PLAYER_ACTION_CHG_11,
    PLAYER_ACTION_CHG_8,  PLAYER_ACTION_CHG_7, -PLAYER_ACTION_CHG_6,
};

static s8 sActionChangeList10[] = {
    PLAYER_ACTION_CHG_10,
    PLAYER_ACTION_CHG_8,
    -PLAYER_ACTION_CHG_7,
};

static s8 sActionChangeList11[] = {
    PLAYER_ACTION_CHG_0,
    PLAYER_ACTION_CHG_12,
    PLAYER_ACTION_CHG_5,
    -PLAYER_ACTION_CHG_4,
};

s32 Player_ActionChange_SetupCUpBehavior(Player* this, PlayState* play);
s32 Player_ActionChange_TryOpenDoor(Player* this, PlayState* play);
s32 Player_ActionChange_TryGetItemOrCarry(Player* this, PlayState* play);
s32 Player_ActionChange_TryMountHorse(Player* this, PlayState* play);
s32 Player_ActionChange_TrySpeakOrCheck(Player* this, PlayState* play);
s32 Player_TrySpecialWallInteract(Player* this, PlayState* play);
s32 Player_ActionChange_TryRollOrPutAway(Player* this, PlayState* play);
s32 Player_ActionChange_TryStartMeleeWeaponAttack(Player* this, PlayState* play);
s32 Player_ActionChange_TryChargeSpinAttack(Player* this, PlayState* play);
s32 Player_ActionChange_9(Player* this, PlayState* play);
s32 Player_ActionChange_TryJumpSlashOrRoll(Player* this, PlayState* play);
s32 Player_ActionChange_TryDefend(Player* this, PlayState* play);
s32 Player_ActionChange_TryWallJump(Player* this, PlayState* play);
s32 Player_ActionChange_TryItemCsOrFirstPerson(Player* this, PlayState* play);

static s32 (*sActionChangeFuncs[])(Player* this, PlayState* play) = {
    /* PLAYER_ACTION_CHG_0  */ Player_ActionChange_SetupCUpBehavior,
    /* PLAYER_ACTION_CHG_1  */ Player_ActionChange_TryOpenDoor,
    /* PLAYER_ACTION_CHG_2  */ Player_ActionChange_TryGetItemOrCarry,
    /* PLAYER_ACTION_CHG_3  */ Player_ActionChange_TryMountHorse,
    /* PLAYER_ACTION_CHG_4  */ Player_ActionChange_TrySpeakOrCheck,
    /* PLAYER_ACTION_CHG_5  */ Player_TrySpecialWallInteract,
    /* PLAYER_ACTION_CHG_6  */ Player_ActionChange_TryRollOrPutAway,
    /* PLAYER_ACTION_CHG_7  */ Player_ActionChange_TryStartMeleeWeaponAttack,
    /* PLAYER_ACTION_CHG_8  */ Player_ActionChange_TryChargeSpinAttack,
    /* PLAYER_ACTION_CHG_9  */ Player_ActionChange_9,
    /* PLAYER_ACTION_CHG_10 */ Player_ActionChange_TryJumpSlashOrRoll,
    /* PLAYER_ACTION_CHG_11 */ Player_ActionChange_TryDefend,
    /* PLAYER_ACTION_CHG_12 */ Player_ActionChange_TryWallJump,
    /* PLAYER_ACTION_CHG_13 */ Player_ActionChange_TryItemCsOrFirstPerson,
};

/**
 * This function processes "Action Change Lists", which run various functions that
 * check if it is appropriate to change to a new action.
 *
 * Action Change Lists are a list of indices for the `sActionChangeFuncs` array.
 * The functions are ran in order until one of them returns true, or the end of the list is reached.
 * An Action Change index having a negative value indicates that it is the last member in the list.
 *
 * Because these lists are processed sequentially, the order of the indices in the list determines its priority.
 *
 * If the `updateUpperBody` argument is true, Player's upper body will update before the Action Change List
 * is processed. This allows for Item Action functions to run.
 *
 * @return true if a new action has been chosen
 *
 */
s32 Player_TryActionChangeList(PlayState* play, Player* this, s8* actionChangeList, s32 updateUpperBody) {
    s32 i;

    if (!(this->stateFlags1 &
          (PLAYER_STATE1_EXITING_SCENE | PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE))) {
        if (updateUpperBody) {
            sUpperBodyBusy = Player_UpdateUpperBody(this, play);

            if (Player_Action_ThrowDekuNut == this->actionFunc) {
                return true;
            }
        }

        if (Player_IsShootingHookshot(this)) {
            this->lookFlags |= 0x41;
            return true;
        }

        if (!(this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM) &&
            (Player_UpperAction_ChangeHeldItem != this->upperActionFunc)) {
            // Process all entries in the Action Change List with a positive index
            while (*actionChangeList >= 0) {
                if (sActionChangeFuncs[*actionChangeList](this, play)) {
                    return true;
                }
                if (this->stateFlags3 & PLAYER_STATE3_USING_BOOSTERS) {
                    return true;
                }
                actionChangeList++;
            }

            // Try the last entry in the list. Negate the index to make it positive again.
            if (sActionChangeFuncs[-(*actionChangeList)](this, play)) {
                return true;
            }
        }
    }

    return false;
}

// Checks if action is interrupted within a certain number of frames from the end of the current animation
// Returns -1 is action is not interrupted at all, 0 if interrupted by a sub-action, 1 if interrupted by the player
// moving
s32 Player_CheckActionInterruptStatus(PlayState* play, Player* this, SkelAnime* skelAnime, f32 frame) {
    f32 speedTarget;
    s16 yawTarget;

    if ((skelAnime->endFrame - frame) <= skelAnime->curFrame) {
        if (Player_TryActionChangeList(play, this, sActionChangeList7, true)) {
            return 0;
        }

        if (Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play)) {
            return 1;
        }
    }

    return -1;
}

void Player_SetupSpinAttackActor(PlayState* play, Player* this, s32 spinAttackParams) {
    if (spinAttackParams != 0) {
        this->spinAttackTimer = 0.0f;
    } else {
        this->spinAttackTimer = 0.5f;
    }

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (this->actor.category == ACTORCAT_PLAYER) {
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_M_THUNDER, this->bodyPartsPos[PLAYER_BODYPART_WAIST].x,
                    this->bodyPartsPos[PLAYER_BODYPART_WAIST].y, this->bodyPartsPos[PLAYER_BODYPART_WAIST].z, 0, 0, 0,
                    Player_GetMeleeWeaponHeld(this) | spinAttackParams);
    }
}

s32 Player_CanQuickspin(Player* this) {
    s8 stickInputsArray[4];
    s8* controlStickInput;
    s8* stickInput;
    s8 inputDiff1;
    s8 inputDiff2;
    s32 i;

    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) || Player_HoldsBrokenKnife(this)) {
        return 0;
    }

    controlStickInput = &this->analogStickInputs[0];
    stickInput = &stickInputsArray[0];
    for (i = 0; i < 4; i++, controlStickInput++, stickInput++) {
        if ((*stickInput = *controlStickInput) < 0) {
            return 0;
        }
        *stickInput *= 2;
    }

    inputDiff1 = stickInputsArray[0] - stickInputsArray[1];
    if (ABS(inputDiff1) < 10) {
        return 0;
    }

    stickInput = &stickInputsArray[1];
    for (i = 1; i < 3; i++, stickInput++) {
        inputDiff2 = *stickInput - *(stickInput + 1);
        if ((ABS(inputDiff2) < 10) || (inputDiff2 * inputDiff1 < 0)) {
            return 0;
        }
    }

    return 1;
}

void Player_SetSpinAttackAnims(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;

    if ((this->meleeAttackType >= PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H) &&
        (this->meleeAttackType <= PLAYER_MELEEATKTYPE_RIGHT_COMBO_2H)) {
        anim = sSpinAttackAnimsL[Player_HoldsTwoHandedWeapon(this)];
    } else {
        anim = sSpinAttackAnims[Player_HoldsTwoHandedWeapon(this)];
    }

    Player_InactivateMeleeWeapon(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 8.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -9.0f);
    Player_SetupSpinAttackActor(play, this, 0x200);
}

void Player_SetupChargeSpinAttack(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_ChargeSpinAttack, 1);
    Player_SetSpinAttackAnims(play, this);
}

static s8 sMeleeWeaponAttackDirections[] = {
    PLAYER_MELEEATKTYPE_STAB_1H,
    PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H,
    PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H,
    PLAYER_MELEEATKTYPE_LEFT_SLASH_1H,
};
static s8 sHammerAttackDirections[] = {
    PLAYER_MELEEATKTYPE_HAMMER_FORWARD,
    PLAYER_MELEEATKTYPE_HAMMER_SIDE,
    PLAYER_MELEEATKTYPE_HAMMER_FORWARD,
    PLAYER_MELEEATKTYPE_HAMMER_SIDE,
};

s32 Player_GetMeleeAttackAnim(Player* this) {
    s32 relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];
    s32 attackAnim;

    if (this->heldItemAction == PLAYER_IA_HAMMER) {
        if (relativeStickInput < 0) {
            relativeStickInput = 0;
        }
        attackAnim = sHammerAttackDirections[relativeStickInput];
        this->slashCounter = 0;
    } else {
        if (Player_CanQuickspin(this)) {
            attackAnim = PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H;
        } else {
            if (relativeStickInput < 0) {
                if (Player_CheckTargeting(this)) {
                    attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
                } else {
                    attackAnim = PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H;
                }
            } else {
                attackAnim = sMeleeWeaponAttackDirections[relativeStickInput];
                if (attackAnim == PLAYER_MELEEATKTYPE_STAB_1H) {
                    this->stateFlags2 |= PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
                    if (!Player_CheckTargeting(this)) {
                        attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
                    }
                }
            }
            if (this->heldItemAction == PLAYER_IA_DEKU_STICK) {
                attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
            }
        }
        if (Player_HoldsTwoHandedWeapon(this)) {
            attackAnim++;
        }
    }

    return attackAnim;
}

void Player_SetMeleeWeaponToucherFlags(Player* this, s32 quadIndex, u32 dmgFlags) {
    this->meleeWeaponQuads[quadIndex].info.toucher.dmgFlags = dmgFlags;

    if (dmgFlags == DMG_DEKU_STICK) {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST | TOUCH_SFX_WOOD;
    } else {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST;
    }
}

static u32 sMeleeWeaponDmgFlags[][2] = {
    { DMG_SLASH_MASTER, DMG_JUMP_MASTER }, { DMG_SLASH_KOKIRI, DMG_JUMP_KOKIRI }, { DMG_SLASH_GIANT, DMG_JUMP_GIANT },
    { DMG_DEKU_STICK, DMG_JUMP_MASTER },   { DMG_HAMMER_SWING, DMG_HAMMER_JUMP },
};

void Player_SetupMeleeWeaponAttack(PlayState* play, Player* this, s32 arg2) {
    s32 pad;
    u32 dmgFlags;
    s32 meleeWeapon;

    Player_SetupAction(play, this, Player_Action_MeleeWeaponAttack, 0);
    this->comboTimer = 8;
    if (!((arg2 >= PLAYER_MELEEATKTYPE_FLIPSLASH_FINISH) && (arg2 <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH))) {
        Player_InactivateMeleeWeapon(this);
    }

    if ((arg2 != this->meleeAttackType) || !(this->slashCounter < 3)) {
        this->slashCounter = 0;
    }

    this->slashCounter++;
    if (this->slashCounter >= 3) {
        arg2 += 2;
    }

    this->meleeAttackType = arg2;

    Player_AnimPlayOnceAdjusted(play, this, sMeleeAttackAnims[arg2].startAnim);
    if ((arg2 != PLAYER_MELEEATKTYPE_FLIPSLASH_START) && (arg2 != PLAYER_MELEEATKTYPE_JUMPSLASH_START)) {
        Player_AnimReplaceApplyFlags(play, this, ANIM_REPLACE_APPLY_FLAG_9 | ANIM_FLAG_0 | ANIM_FLAG_PLAYER_SETMOVE);
    }

    this->yaw = this->actor.shape.rot.y;

    if (Player_HoldsBrokenKnife(this)) {
        meleeWeapon = 1;
    } else {
        meleeWeapon = Player_GetMeleeWeaponHeld(this) - 1;
    }

    if ((arg2 >= PLAYER_MELEEATKTYPE_FLIPSLASH_START) && (arg2 <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH)) {
        dmgFlags = sMeleeWeaponDmgFlags[meleeWeapon][1];
    } else {
        dmgFlags = sMeleeWeaponDmgFlags[meleeWeapon][0];
    }

    Player_SetMeleeWeaponToucherFlags(this, 0, dmgFlags);
    Player_SetMeleeWeaponToucherFlags(this, 1, dmgFlags);
}

void Player_SetInvincibilityTimer(Player* this, s32 timer) {
    if (this->invincibilityTimer >= 0) {
        this->invincibilityTimer = timer;
        this->damageFlashTimer = 0;
    }
}

void Player_SetInvincibilityTimerNoDamageFlash(Player* this, s32 timer) {
    if (this->invincibilityTimer > timer) {
        this->invincibilityTimer = timer;
    }
    this->damageFlashTimer = 0;
}

s32 Player_InflictDamage(PlayState* play, Player* this, s32 damage) {
    if ((this->invincibilityTimer != 0) || (this->actor.category != ACTORCAT_PLAYER)) {
        return 1;
    }

    return Health_ChangeBy(play, damage);
}

void Player_SetLedgeGrabPosition(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
    Player_UpdateAnimMovement(this, ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y);
}

void Player_SetupFallFromLedge(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_Midair, 0);
    Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
    this->av2.actionVar2 = 1;
    if (this->attentionMode != 3) {
        this->attentionMode = 0;
    }
}

static LinkAnimationHeader* sLinkDamageAnims[] = {
    &gPlayerAnim_link_normal_front_shit, &gPlayerAnim_link_normal_front_shitR, &gPlayerAnim_link_normal_back_shit,
    &gPlayerAnim_link_normal_back_shitR, &gPlayerAnim_link_normal_front_hit,   &gPlayerAnim_link_anchor_front_hitR,
    &gPlayerAnim_link_normal_back_hit,   &gPlayerAnim_link_anchor_back_hitR,
};

void Player_TakeColliderDamage(PlayState* play, Player* this, s32 damageReaction, f32 knockbackVelXZ, f32 knockbackVelY,
                               s16 damageYaw, s32 invincibilityTimer) {
    LinkAnimationHeader* anim = NULL;
    LinkAnimationHeader** damageAnims;

    if (this->stateFlags1 & PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP) {
        Player_SetLedgeGrabPosition(this);
    }

    this->runDamageTimer = 0;

    Player_PlaySfx(this, NA_SE_PL_DAMAGE);

    if (!Player_InflictDamage(play, this, 0 - this->actor.colChkInfo.damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
            Player_SetupFallFromLedge(this, play);
        }
        return;
    }

    Player_SetInvincibilityTimer(this, invincibilityTimer);

    if (damageReaction == 3) {
        Player_SetupAction(play, this, Player_Action_FrozenInIce, 0);

        anim = &gPlayerAnim_link_normal_ice_down;

        Player_ClearAttentionModeAndStopMoving(this);
        Player_RequestRumble(this, 255, 10, 40, 0);

        Player_PlaySfx(this, NA_SE_PL_FREEZE_S);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
    } else if (damageReaction == 4) {
        Player_SetupAction(play, this, Player_Action_StartElectricShock, 0);

        Player_RequestRumble(this, 255, 80, 150, 0);

        Player_AnimPlayLoopAdjusted(play, this, &gPlayerAnim_link_normal_electric_shock);
        Player_ClearAttentionModeAndStopMoving(this);

        this->av2.actionVar2 = 20;
    } else {
        damageYaw -= this->actor.shape.rot.y;
        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            Player_SetupAction(play, this, Player_Action_DamagedSwim, 0);
            Player_RequestRumble(this, 180, 20, 50, 0);

            this->speedXZ = 4.0f;
            this->actor.velocity.y = 0.0f;

            anim = &gPlayerAnim_link_swimer_swim_hit;

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        } else if ((damageReaction == 1) || (damageReaction == 2) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                         PLAYER_STATE1_CLIMBING))) {
            Player_SetupAction(play, this, Player_Action_Knockback, 0);

            this->stateFlags3 |= PLAYER_STATE3_MIDAIR;

            Player_RequestRumble(this, 255, 20, 150, 0);
            Player_ClearAttentionModeAndStopMoving(this);

            if (damageReaction == 2) {
                this->av2.actionVar2 = 4;

                this->actor.speed = 3.0f;
                this->speedXZ = 3.0f;
                this->actor.velocity.y = 6.0f;

                Player_AnimChangeFreeze(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_damage_run, this->modelAnimType));
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
            } else {
                this->actor.speed = knockbackVelXZ;
                this->speedXZ = knockbackVelXZ;
                this->actor.velocity.y = knockbackVelY;

                if (ABS(damageYaw) > 0x4000) {
                    anim = &gPlayerAnim_link_normal_front_downA;
                } else {
                    anim = &gPlayerAnim_link_normal_back_downA;
                }

                if ((this->actor.category != ACTORCAT_PLAYER) && (this->actor.colChkInfo.health == 0)) {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_BL_DOWN);
                } else {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
                }
            }

            this->hoverBootsTimer = 0;
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        } else {
            if ((this->speedXZ > 4.0f) && !Player_CheckBattleTargeting(this)) {
                this->runDamageTimer = 20;
                Player_RequestRumble(this, 120, 20, 10, 0);
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
                return;
            }

            damageAnims = sLinkDamageAnims;

            Player_SetupAction(play, this, Player_Action_RecoverFromMinorDamage, 0);
            Player_ResetLRBlendWeight(this);

            if (this->actor.colChkInfo.damage < 5) {
                Player_RequestRumble(this, 120, 20, 10, 0);
            } else {
                Player_RequestRumble(this, 180, 20, 100, 0);
                this->speedXZ = 23.0f;
                damageAnims += 4;
            }

            if (ABS(damageYaw) <= 0x4000) {
                damageAnims += 2;
            }

            if (Player_CheckBattleTargeting(this)) {
                damageAnims += 1;
            }

            anim = *damageAnims;

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }

        this->actor.shape.rot.y += damageYaw;
        this->yaw = this->actor.shape.rot.y;
        this->actor.world.rot.y = this->actor.shape.rot.y;
        if (ABS(damageYaw) > 0x4000) {
            this->actor.shape.rot.y += 0x8000;
        }
    }

    Player_StopCarryingActor(play, this);

    this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;

    if (anim != NULL) {
        Player_AnimPlayOnceAdjusted(play, this, anim);
    }
}

s32 Player_GetHurtFloorType(s32 floorType) {
    s32 hurtFloorType = floorType - FLOOR_TYPE_HURT_FLOOR;

    if ((hurtFloorType >= FLOOR_TYPE_NONE) && (hurtFloorType <= (FLOOR_TYPE_FIRE_HURT_FLOOR - FLOOR_TYPE_HURT_FLOOR))) {
        return hurtFloorType;
    } else {
        return -1;
    }
}

int Player_IsFloorSinkingSand(s32 floorType) {
    return (floorType == FLOOR_TYPE_SHALLOW_SAND) || (floorType == FLOOR_TYPE_QUICKSAND_NO_HORSE) ||
           (floorType == FLOOR_TYPE_QUICKSAND_HORSE_CAN_CROSS);
}

void Player_BurnDekuShield(Player* this, PlayState* play) {
    if (this->currentShield == PLAYER_SHIELD_DEKU) {
        Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_SHIELD, this->actor.world.pos.x, this->actor.world.pos.y,
                    this->actor.world.pos.z, 0, 0, 0, 1);
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SHIELD);
        Message_StartTextbox(play, 0x305F, NULL);
    }
}

void Player_StartBurning(Player* this) {
    s32 i;

    // clang-format off
    for (i = 0; i < PLAYER_BODYPART_MAX; i++) { this->flameTimers[i] = Rand_S16Offset(0, 200); }
    // clang-format on

    this->isBurning = true;
}

void Player_PlayFallSfxAndCheckBurning(Player* this) {
    if (this->actor.colChkInfo.acHitEffect == 1) {
        Player_StartBurning(this);
    }
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_RoundUpInvincibilityTimer(Player* this) {
    if ((this->invincibilityTimer >= 0) && (this->invincibilityTimer < 20)) {
        this->invincibilityTimer = 20;
    }
}

s32 Player_CheckDamage(Player* this, PlayState* play) {
    s32 pad;
    s32 sinkingGroundVoidOut = false;
    s32 attackHitShield;

    if (this->voidRespawnCounter != 0) {
        if (!Player_InBlockingCsMode(play, this)) {
            Player_InflictDamageAndCheckForDeath(play, -16);
            this->voidRespawnCounter = 0;
        }
    } else {
        sinkingGroundVoidOut = ((Player_GetHeight(this) - 8.0f) < (this->shapeOffsetY * this->actor.scale.y));

        if (sinkingGroundVoidOut || (this->actor.bgCheckFlags & BGCHECKFLAG_CRUSHED) ||
            (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH) || (this->stateFlags2 & PLAYER_STATE2_FORCE_VOID_OUT)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);

            if (sinkingGroundVoidOut) {
                Play_TriggerRespawn(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                // Special case for getting crushed in Forest Temple's Checkboard Ceiling Hall or Shadow Temple's
                // Falling Spike Trap Room, to respawn the player in a specific place
                if (((play->sceneId == SCENE_FOREST_TEMPLE) && (play->roomCtx.curRoom.num == 15)) ||
                    ((play->sceneId == SCENE_SHADOW_TEMPLE) && (play->roomCtx.curRoom.num == 10))) {
                    static SpecialRespawnInfo checkboardCeilingRespawn = { { 1992.0f, 403.0f, -3432.0f }, 0 };
                    static SpecialRespawnInfo fallingSpikeTrapRespawn = { { 1200.0f, -1343.0f, 3850.0f }, 0 };
                    SpecialRespawnInfo* respawnInfo;

                    if (play->sceneId == SCENE_FOREST_TEMPLE) {
                        respawnInfo = &checkboardCeilingRespawn;
                    } else {
                        respawnInfo = &fallingSpikeTrapRespawn;
                    }

                    Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = respawnInfo->pos;
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = respawnInfo->yaw;
                }

                Play_TriggerVoidOut(play);
            }

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_TAKEN_AWAY);
            play->haltAllActors = true;
            Sfx_PlaySfxCentered(NA_SE_OC_ABYSS);
        } else if ((this->damageEffect != 0) && ((this->damageEffect >= 2) || (this->invincibilityTimer == 0))) {
            u8 damageReactions[] = { 2, 1, 1 };

            Player_PlayFallSfxAndCheckBurning(this);

            if (this->damageEffect == 3) {
                this->shockTimer = 40;
            }

            this->actor.colChkInfo.damage += this->damageAmount;
            Player_TakeColliderDamage(play, this, damageReactions[this->damageEffect - 1], this->knockbackVelXZ,
                                      this->knockbackVelY, this->damageYaw, 20);
        } else {
            attackHitShield = (this->shieldQuad.base.acFlags & AC_BOUNCED) != 0;

            //! @bug The second set of conditions here seems intended as a way for Link to "block" hits by rolling.
            // However, `Collider.atFlags` is a byte so the flag check at the end is incorrect and cannot work.
            // Additionally, `Collider.atHit` can never be set while already colliding as AC, so it's also bugged.
            // This behavior was later fixed in MM, most likely by removing both the `atHit` and `atFlags` checks.
            if (attackHitShield ||
                ((this->invincibilityTimer < 0) && (this->cylinder.base.acFlags & AC_HIT) &&
                 (this->cylinder.info.atHit != NULL) && (this->cylinder.info.atHit->atFlags & 0x20000000))) {

                Player_RequestRumble(this, 180, 20, 100, 0);

                if (!Player_IsChildWithHylianShield(this)) {
                    if (this->invincibilityTimer >= 0) {
                        LinkAnimationHeader* anim;
                        s32 sp54 = Player_Action_AimShieldCrouched == this->actionFunc;

                        if (!Player_IsSwimming(this)) {
                            Player_SetupAction(play, this, Player_Action_DeflectAttackWithShield, 0);
                        }

                        if (!(this->av1.actionVar1 = sp54)) {
                            Player_SetUpperActionFunc(this, Player_UpperAction_DeflectAttackStanding);

                            if (this->leftRightBlendWeight < 0.5f) {
                                anim = sRightStandingDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)];
                            } else {
                                anim = sLeftStandingDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)];
                            }
                            LinkAnimation_PlayOnce(play, &this->upperSkelAnime, anim);
                        } else {
                            Player_AnimPlayOnce(play, this, sDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)]);
                        }
                    }

                    if (!(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP |
                                               PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING))) {
                        this->speedXZ = -18.0f;
                        this->yaw = this->actor.shape.rot.y;
                    }
                }

                if (attackHitShield && (this->shieldQuad.info.acHitInfo->toucher.effect == 1)) {
                    Player_BurnDekuShield(this, play);
                }

                return 0;
            }

            if ((this->deathTimer != 0) || (this->invincibilityTimer > 0) ||
                (this->stateFlags1 & PLAYER_STATE1_TAKING_DAMAGE) || (this->csAction != PLAYER_CSACTION_NONE) ||
                (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) ||
                (this->meleeWeaponQuads[1].base.atFlags & AT_HIT)) {
                return 0;
            }

            if (this->cylinder.base.acFlags & AC_HIT) {
                Actor* ac = this->cylinder.base.ac;
                s32 damageReaction;

                if (ac->flags & ACTOR_FLAG_24) {
                    Player_PlaySfx(this, NA_SE_PL_BODY_HIT);
                }

                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    damageReaction = 0;
                } else if (this->actor.colChkInfo.acHitEffect == 2) {
                    damageReaction = 3;
                } else if (this->actor.colChkInfo.acHitEffect == 3) {
                    damageReaction = 4;
                } else if (this->actor.colChkInfo.acHitEffect == 4) {
                    damageReaction = 1;
                } else {
                    Player_PlayFallSfxAndCheckBurning(this);
                    damageReaction = 0;
                }

                Player_TakeColliderDamage(play, this, damageReaction, 4.0f, 5.0f,
                                          Actor_WorldYawTowardActor(ac, &this->actor), 20);
            } else if (this->invincibilityTimer != 0) {
                return 0;
            } else {
                static u8 dmgStartFrame[] = { 120, 60 };
                s32 hurtFloorType = Player_GetHurtFloorType(sFloorType);

                if (((this->actor.wallPoly != NULL) &&
                     func_80042108(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) ||
                    ((hurtFloorType >= 0) &&
                     func_80042108(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) &&
                     (this->floorTypeTimer >= dmgStartFrame[hurtFloorType])) ||
                    ((hurtFloorType >= 0) && (this->currentTunic != PLAYER_TUNIC_GORON ||
                                              (this->floorTypeTimer >= dmgStartFrame[hurtFloorType])))) {
                    this->floorTypeTimer = 0;
                    this->actor.colChkInfo.damage = 4;
                    Player_TakeColliderDamage(play, this, 0, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
                } else {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void Player_SetupJumpWithSfx(Player* this, LinkAnimationHeader* anim, f32 yVel, PlayState* play, u16 sfxId) {
    Player_SetupAction(play, this, Player_Action_Midair, 1);

    if (anim != NULL) {
        Player_AnimPlayOnceAdjusted(play, this, anim);
    }

    this->actor.velocity.y = yVel * sWaterSpeedScale;
    this->hoverBootsTimer = 0;
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;

    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, sfxId);

    this->stateFlags1 |= PLAYER_STATE1_JUMPING;
}

void Player_SetupJump(Player* this, LinkAnimationHeader* anim, f32 yVel, PlayState* play) {
    Player_SetupJumpWithSfx(this, anim, yVel, play, NA_SE_VO_LI_SWORD_N);
}

s32 Player_ActionChange_TryWallJump(Player* this, PlayState* play) {
    s32 canJumpToLedge;
    LinkAnimationHeader* anim;
    f32 wallHeight;
    f32 yVel;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;
    f32 wallDist;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->ledgeClimbType >= PLAYER_LEDGE_CLIMB_2) &&
        (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->ageProperties->unk_14 > this->yDistToLedge))) {
        canJumpToLedge = 0;

        if (Player_IsSwimming(this)) {
            if (this->actor.yDistToWater < 50.0f) {
                if ((this->ledgeClimbType < PLAYER_LEDGE_CLIMB_2) ||
                    (this->yDistToLedge > this->ageProperties->unk_10)) {
                    return 0;
                }
            } else if (!(this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) || (this->ledgeClimbType > PLAYER_LEDGE_CLIMB_2)) {
                return 0;
            }
        } else if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   ((this->ageProperties->unk_14 <= this->yDistToLedge) &&
                    (this->stateFlags1 & PLAYER_STATE1_SWIMMING))) {
            return 0;
        }

        if ((this->actor.wallBgId != BGCHECK_SCENE) && (sTouchedWallFlags & WALL_FLAG_6)) {
            if (this->ledgeClimbDelayTimer >= 6) {
                this->stateFlags2 |= PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL;
                if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                    canJumpToLedge = 1;
                }
            }
        } else if ((this->ledgeClimbDelayTimer >= 6) || CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
            canJumpToLedge = 1;
        }

        if (canJumpToLedge != 0) {
            Player_SetupAction(play, this, Player_Action_JumpUpToLedge, 0);

            this->stateFlags1 |= PLAYER_STATE1_JUMPING;

            wallHeight = this->yDistToLedge;

            if (this->ageProperties->unk_14 <= wallHeight) {
                anim = &gPlayerAnim_link_normal_250jump_start;
                this->speedXZ = 1.0f;
            } else {
                wallPolyNormalX = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.x);
                wallPolyNormalZ = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.z);
                wallDist = this->distToInteractWall + 0.5f;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING_ONTO_LEDGE;

                if (Player_IsSwimming(this)) {
                    anim = &gPlayerAnim_link_swimer_swim_15step_up;
                    wallHeight -= (60.0f * this->ageProperties->unk_08);
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;
                } else if (this->ageProperties->unk_18 <= wallHeight) {
                    anim = &gPlayerAnim_link_normal_150step_up;
                    wallHeight -= (59.0f * this->ageProperties->unk_08);
                } else {
                    anim = &gPlayerAnim_link_normal_100step_up;
                    wallHeight -= (41.0f * this->ageProperties->unk_08);
                }

                this->actor.shape.yOffset -= wallHeight * 100.0f;

                this->actor.world.pos.x -= wallDist * wallPolyNormalX;
                this->actor.world.pos.y += this->yDistToLedge;
                this->actor.world.pos.z -= wallDist * wallPolyNormalZ;

                Player_ClearAttentionModeAndStopMoving(this);
            }

            this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;

            LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 1.3f);
            AnimationContext_DisableQueue(play);

            this->actor.shape.rot.y = this->yaw = this->actor.wallYaw + 0x8000;

            return 1;
        }
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->ledgeClimbType == PLAYER_LEDGE_CLIMB_1) &&
               (this->ledgeClimbDelayTimer >= 3)) {
        yVel = (this->yDistToLedge * 0.08f) + 5.5f;
        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, yVel, play);
        this->speedXZ = 2.5f;

        return 1;
    }

    return 0;
}

void Player_SetupMiniCsMovement(PlayState* play, Player* this, f32 xzOffset, s16 yaw) {
    Player_SetupAction(play, this, Player_Action_MiniCsMovement, 0);
    Player_ResetAttributes(play, this);

    this->av1.actionVar1 = 1;
    this->av2.actionVar2 = 1;

    this->csStartPos.x = (Math_SinS(yaw) * xzOffset) + this->actor.world.pos.x;
    this->csStartPos.z = (Math_CosS(yaw) * xzOffset) + this->actor.world.pos.z;

    Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
}

void Player_SetupSwimIdle(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_SwimIdle, 0);
    Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim_wait);
}

void Player_SetupEnterGrotto(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_EnterGrotto, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID;

    Camera_RequestSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_FREE0);
}

s32 Player_ShouldEnterGrotto(PlayState* play, Player* this) {
    if ((play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (this->stateFlags1 & PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID)) {
        Player_SetupEnterGrotto(play, this);
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_S);
        Sfx_PlaySfxCentered2(NA_SE_OC_SECRET_WARP_IN);
        return 1;
    }

    return 0;
}

/**
 * The actual entrances each "return entrance" value can map to.
 * This is used by scenes that are shared between locations, like child/adult Shooting Gallery or Great Fairy Fountains.
 *
 * This 1D array is split into groups of entrances.
 * The start of each group is indexed by `sReturnEntranceGroupIndices` values.
 * The resulting groups are then indexed by the spawn value.
 *
 * The spawn value (`PlayState.spawn`) is set to a different value depending on the entrance used to enter the
 * scene, which allows these dynamic "return entrances" to link back to the previous scene.
 *
 * Note: grottos and normal fairy fountains use `ENTR_RETURN_GROTTO`
 */
s16 sReturnEntranceGroupData[] = {
    // ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_MAGIC
    /*  0 */ ENTR_DEATH_MOUNTAIN_TRAIL_4,  // from Magic Fairy Fountain
    /*  1 */ ENTR_DEATH_MOUNTAIN_CRATER_3, // from Double Magic Fairy Fountain
    /*  2 */ ENTR_HYRULE_CASTLE_2,         // from Double Defense Fairy Fountain (as adult)

    // ENTR_RETURN_2
    /*  3 */ ENTR_KAKARIKO_VILLAGE_9, // from Potion Shop in Kakariko
    /*  4 */ ENTR_MARKET_DAY_5,       // from Potion Shop in Market

    // ENTR_RETURN_BAZAAR
    /*  5 */ ENTR_KAKARIKO_VILLAGE_3,
    /*  6 */ ENTR_MARKET_DAY_6,

    // ENTR_RETURN_4
    /*  7 */ ENTR_KAKARIKO_VILLAGE_11, // from House of Skulltulas
    /*  8 */ ENTR_BACK_ALLEY_DAY_2,    // from Bombchu Shop

    // ENTR_RETURN_SHOOTING_GALLERY
    /*  9 */ ENTR_KAKARIKO_VILLAGE_10,
    /* 10 */ ENTR_MARKET_DAY_8,

    // ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_SPELLS
    /* 11 */ ENTR_ZORAS_FOUNTAIN_5,  // from Farores Wind Fairy Fountain
    /* 12 */ ENTR_HYRULE_CASTLE_2,   // from Dins Fire Fairy Fountain (as child)
    /* 13 */ ENTR_DESERT_COLOSSUS_7, // from Nayrus Love Fairy Fountain
};

/**
 * The values are indices into `sReturnEntranceGroupData` marking the start of each group
 */
u8 sReturnEntranceGroupIndices[] = {
    11, // ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_SPELLS
    9,  // ENTR_RETURN_SHOOTING_GALLERY
    3,  // ENTR_RETURN_2
    5,  // ENTR_RETURN_BAZAAR
    7,  // ENTR_RETURN_4
    0,  // ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_MAGIC
};

s32 Player_HandleExitsAndVoids(PlayState* play, Player* this, CollisionPoly* poly, u32 bgId) {
    s32 exitIndex;
    s32 temp;
    s32 yDistToExit;
    f32 speedXZ;
    s32 yaw;

    if (this->actor.category == ACTORCAT_PLAYER) {
        exitIndex = 0;

        if (!(this->stateFlags1 & PLAYER_STATE1_IN_DEATH_CUTSCENE) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
            (this->csAction == PLAYER_CSACTION_NONE) && !(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
            (((poly != NULL) && (exitIndex = SurfaceType_GetExitIndex(&play->colCtx, poly, bgId), exitIndex != 0)) ||
             (Player_IsFloorSinkingSand(sFloorType) && (this->floorProperty == FLOOR_PROPERTY_VOID_PIT_LARGE)))) {

            yDistToExit = this->sceneExitPosY - (s32)this->actor.world.pos.y;

            if (!(this->stateFlags1 &
                  (PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (yDistToExit < 100) && (sYDistToFloor > 100.0f)) {
                return 0;
            }

            if (exitIndex == 0) {
                Play_TriggerVoidOut(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                play->nextEntranceIndex = play->exitList[exitIndex - 1];

                if (play->nextEntranceIndex == ENTR_RETURN_GROTTO) {
                    gSaveContext.respawnFlag = 2;
                    play->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
                    play->transitionType = TRANS_TYPE_FADE_WHITE;
                    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
                } else if (play->nextEntranceIndex >= ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_SPELLS) {
                    play->nextEntranceIndex =
                        sReturnEntranceGroupData[sReturnEntranceGroupIndices[play->nextEntranceIndex -
                                                                             ENTR_RETURN_GREAT_FAIRYS_FOUNTAIN_SPELLS] +
                                                 play->spawn];
                    Scene_SetTransitionForNextEntrance(play);
                } else {
                    if (SurfaceType_GetFloorEffect(&play->colCtx, poly, bgId) == FLOOR_EFFECT_2) {
                        gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = play->nextEntranceIndex;
                        Play_TriggerVoidOut(play);
                        gSaveContext.respawnFlag = -2;
                    }

                    gSaveContext.retainWeatherMode = true;
                    Scene_SetTransitionForNextEntrance(play);
                }

                play->transitionTrigger = TRANS_TRIGGER_START;
            }

            if (!(this->stateFlags1 & (PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_IN_CUTSCENE)) &&
                !(this->stateFlags2 & PLAYER_STATE2_CRAWLING) && !Player_IsSwimming(this) &&
                (temp = SurfaceType_GetFloorType(&play->colCtx, poly, bgId), (temp != FLOOR_TYPE_10)) &&
                ((yDistToExit < 100) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {

                if (temp == FLOOR_TYPE_LOOK_UP) {
                    Sfx_PlaySfxCentered2(NA_SE_OC_SECRET_HOLE_OUT);
                    func_800F6964(5);
                    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
                    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
                } else {
                    speedXZ = this->speedXZ;

                    if (speedXZ < 0.0f) {
                        this->actor.world.rot.y += 0x8000;
                        speedXZ = -speedXZ;
                    }

                    if (speedXZ > R_RUN_SPEED_LIMIT / 100.0f) {
                        gSaveContext.entranceSpeed = R_RUN_SPEED_LIMIT / 100.0f;
                    } else {
                        gSaveContext.entranceSpeed = speedXZ;
                    }

                    if (sConveyorSpeed != CONVEYOR_SPEED_DISABLED) {
                        yaw = sConveyorYaw;
                    } else {
                        yaw = this->actor.world.rot.y;
                    }
                    Player_SetupMiniCsMovement(play, this, 400.0f, yaw);
                }
            } else {
                if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_ZeroSpeedXZ(this);
                }
            }

            this->stateFlags1 |= PLAYER_STATE1_EXITING_SCENE | PLAYER_STATE1_IN_CUTSCENE;

            Player_RequestCameraSetting(play, CAM_SET_SCENE_TRANSITION);

            return 1;
        } else {
            if (play->transitionTrigger == TRANS_TRIGGER_OFF) {

                if ((this->actor.world.pos.y < -4000.0f) ||
                    (((this->floorProperty == FLOOR_PROPERTY_VOID_PIT) ||
                      (this->floorProperty == FLOOR_PROPERTY_VOID_PIT_LARGE)) &&
                     ((sYDistToFloor < 100.0f) || (this->fallDistance > 400.0f) ||
                      ((play->sceneId != SCENE_SHADOW_TEMPLE) && (this->fallDistance > 200.0f)))) ||
                    ((play->sceneId == SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR) && (this->fallDistance > 320.0f))) {

                    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                        if (this->floorProperty == FLOOR_PROPERTY_VOID_PIT) {
                            Play_TriggerRespawn(play);
                        } else {
                            Play_TriggerVoidOut(play);
                        }
                        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        Sfx_PlaySfxCentered(NA_SE_OC_ABYSS);
                    } else {
                        Player_SetupEnterGrotto(play, this);
                        this->av2.actionVar2 = 9999;
                        if (this->floorProperty == FLOOR_PROPERTY_VOID_PIT) {
                            this->av1.actionVar1 = -1;
                        } else {
                            this->av1.actionVar1 = 1;
                        }
                    }
                }

                this->sceneExitPosY = this->actor.world.pos.y;
            }
        }
    }

    return 0;
}

/**
 * Gets a position relative to player's yaw.
 * An offset is applied to the provided base position in the direction of shape y rotation.
 * The resulting position is stored in `dest`
 */
void Player_GetRelativePosition(Player* this, Vec3f* base, Vec3f* offset, Vec3f* dest) {
    f32 cos = Math_CosS(this->actor.shape.rot.y);
    f32 sin = Math_SinS(this->actor.shape.rot.y);

    dest->x = base->x + ((offset->x * cos) + (offset->z * sin));
    dest->y = base->y + offset->y;
    dest->z = base->z + ((offset->z * cos) - (offset->x * sin));
}

Actor* Player_SpawnFairy(PlayState* play, Player* this, Vec3f* arg2, Vec3f* arg3, s32 type) {
    Vec3f pos;

    Player_GetRelativePosition(this, arg2, arg3, &pos);

    return Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ELF, pos.x, pos.y, pos.z, 0, 0, 0, type);
}

// Returns floor Y, or BGCHECK_MIN_Y if no floor detected
f32 Player_RaycastFloorGetPolyInfo(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos,
                                   CollisionPoly** colPoly, s32* bgId) {
    Player_GetRelativePosition(this, &this->actor.world.pos, raycastPosOffset, raycastPos);

    return BgCheck_EntityRaycastDown3(&play->colCtx, colPoly, bgId, raycastPos);
}

// Returns floor Y, or BGCHECK_MIN_Y if no floor detected
f32 Player_RaycastFloor(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos) {
    CollisionPoly* colPoly;
    s32 polyBgId;

    return Player_RaycastFloorGetPolyInfo(play, this, raycastPosOffset, raycastPos, &colPoly, &polyBgId);
}

/**
 * Checks if a line between the player's position and the provided `offset` intersect a wall.
 *
 * Point A of the line is at player's world position offset by the height provided in `offset`.
 * Point B of the line is at player's world position offset by the entire `offset` vector.
 * Point A and B are always at the same height, meaning this is a horizontal line test.
 */
s32 Player_PosVsWallLineTest(PlayState* play, Player* this, Vec3f* offset, CollisionPoly** wallPoly, s32* bgId,
                             Vec3f* posResult) {
    Vec3f posA;
    Vec3f posB;

    posA.x = this->actor.world.pos.x;
    posA.y = this->actor.world.pos.y + offset->y;
    posA.z = this->actor.world.pos.z;

    Player_GetRelativePosition(this, &this->actor.world.pos, offset, &posB);

    return BgCheck_EntityLineTest1(&play->colCtx, &posA, &posB, posResult, wallPoly, true, false, false, true, bgId);
}

s32 Player_ActionChange_TryOpenDoor(Player* this, PlayState* play) {
    SlidingDoorActorBase* slidingDoor;
    DoorActorBase* door;
    s32 doorDirection;
    f32 cos;
    f32 sin;
    Actor* doorActor;
    f32 doorOpeningPosOffset;
    s32 pad3;
    s32 frontRoom;
    Actor* attachedActor;
    LinkAnimationHeader* anim;
    CollisionPoly* groundPoly;
    Vec3f checkPos;

    if ((this->doorType != PLAYER_DOORTYPE_NONE) &&
        (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
         ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_SEED)))) {
        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) ||
            (Player_Action_TryOpenDoorFromSpawn == this->actionFunc)) {
            doorActor = this->doorActor;

            if (this->doorType <= PLAYER_DOORTYPE_AJAR) {
                doorActor->textId = 0xD0;
                Player_StartTalkToActor(play, doorActor);
                return 0;
            }

            doorDirection = this->doorDirection;
            cos = Math_CosS(doorActor->shape.rot.y);
            sin = Math_SinS(doorActor->shape.rot.y);

            if (this->doorType == PLAYER_DOORTYPE_SLIDING) {
                slidingDoor = (SlidingDoorActorBase*)doorActor;

                this->yaw = slidingDoor->dyna.actor.home.rot.y;
                if (doorDirection > 0) {
                    this->yaw -= 0x8000;
                }
                this->actor.shape.rot.y = this->yaw;

                if (this->speedXZ <= 0.0f) {
                    this->speedXZ = 0.1f;
                }

                Player_SetupMiniCsMovement(play, this, 50.0f, this->actor.shape.rot.y);

                this->av1.actionVar1 = 0;
                this->csDoorType = this->doorType;
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

                this->csStartPos.x = this->actor.world.pos.x + ((doorDirection * 20.0f) * sin);
                this->csStartPos.z = this->actor.world.pos.z + ((doorDirection * 20.0f) * cos);
                this->csEndPos.x = this->actor.world.pos.x + ((doorDirection * -120.0f) * sin);
                this->csEndPos.z = this->actor.world.pos.z + ((doorDirection * -120.0f) * cos);

                slidingDoor->isActive = true;
                Player_ClearAttentionModeAndStopMoving(this);

                if (this->doorTimer != 0) {
                    this->av2.actionVar2 = 0;
                    Player_AnimChangeOnceMorph(play, this, Player_GetStandStillAnim(this));
                    this->skelAnime.endFrame = 0.0f;
                } else {
                    this->speedXZ = 0.1f;
                }

                if (slidingDoor->dyna.actor.category == ACTORCAT_DOOR) {
                    this->cv.slidingDoorBgCamIndex =
                        play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(&slidingDoor->dyna.actor)]
                            .sides[(doorDirection > 0) ? 0 : 1]
                            .bgCamIndex;

                    Actor_DisableLens(play);
                }
            } else {
                // The door actor can be either EnDoor or DoorKiller.
                door = (DoorActorBase*)doorActor;

                door->openAnim = (doorDirection < 0.0f)
                                     ? (LINK_IS_ADULT ? DOOR_OPEN_ANIM_ADULT_L : DOOR_OPEN_ANIM_CHILD_L)
                                     : (LINK_IS_ADULT ? DOOR_OPEN_ANIM_ADULT_R : DOOR_OPEN_ANIM_CHILD_R);

                if (door->openAnim == DOOR_OPEN_ANIM_ADULT_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorA_free, this->modelAnimType);
                } else if (door->openAnim == DOOR_OPEN_ANIM_CHILD_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorA, this->modelAnimType);
                } else if (door->openAnim == DOOR_OPEN_ANIM_ADULT_R) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorB_free, this->modelAnimType);
                } else {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorB, this->modelAnimType);
                }

                Player_SetupAction(play, this, Player_Action_OpenDoor, 0);
                Player_TryUnequipItem(play, this);

                if (doorDirection < 0) {
                    this->actor.shape.rot.y = doorActor->shape.rot.y;
                } else {
                    this->actor.shape.rot.y = doorActor->shape.rot.y - 0x8000;
                }

                this->yaw = this->actor.shape.rot.y;

                doorOpeningPosOffset = (doorDirection * 22.0f);
                this->actor.world.pos.x = doorActor->world.pos.x + doorOpeningPosOffset * sin;
                this->actor.world.pos.z = doorActor->world.pos.z + doorOpeningPosOffset * cos;

                Player_PlayAnimOnceWithWaterInfluence(play, this, anim);

                if (this->doorTimer != 0) {
                    this->skelAnime.endFrame = 0.0f;
                }

                Player_ClearAttentionModeAndStopMoving(this);
                Player_AnimReplaceApplyFlags(play, this,
                                             ANIM_REPLACE_APPLY_FLAG_9 | ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y |
                                                 ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_PLAYER_7);

                // If this door is the second half of a double door (spawned as child)
                if (doorActor->parent != NULL) {
                    doorDirection = -doorDirection;
                }

                door->playerIsOpening = true;

                // If the door actor is not DoorKiller
                if (this->doorType != PLAYER_DOORTYPE_FAKE) {
                    // The door actor is EnDoor

                    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                    Actor_DisableLens(play);

                    if (ENDOOR_GET_TYPE(doorActor) == DOOR_SCENEEXIT) {
                        checkPos.x = doorActor->world.pos.x - (doorOpeningPosOffset * sin);
                        checkPos.y = doorActor->world.pos.y + 10.0f;
                        checkPos.z = doorActor->world.pos.z - (doorOpeningPosOffset * cos);

                        BgCheck_EntityRaycastDown1(&play->colCtx, &groundPoly, &checkPos);

                        //! @bug groundPoly's bgId is not guaranteed to be BGCHECK_SCENE
                        if (Player_HandleExitsAndVoids(play, this, groundPoly, BGCHECK_SCENE)) {
                            gSaveContext.entranceSpeed = 2.0f;
                            gSaveContext.entranceSound = NA_SE_OC_DOOR_OPEN;
                        }
                    } else {
                        Camera_ChangeDoorCam(Play_GetCamera(play, CAM_ID_MAIN), doorActor,
                                             play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(doorActor)]
                                                 .sides[(doorDirection > 0) ? 0 : 1]
                                                 .bgCamIndex,
                                             0, 38.0f * sInvertedWaterSpeedScale, 26.0f * sInvertedWaterSpeedScale,
                                             10.0f * sInvertedWaterSpeedScale);
                    }
                }
            }

            if ((this->doorType != PLAYER_DOORTYPE_FAKE) && (doorActor->category == ACTORCAT_DOOR)) {
                frontRoom = play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(doorActor)]
                                .sides[(doorDirection > 0) ? 0 : 1]
                                .room;

                if ((frontRoom >= 0) && (frontRoom != play->roomCtx.curRoom.num)) {
                    func_8009728C(play, &play->roomCtx, frontRoom);
                }
            }

            doorActor->room = play->roomCtx.curRoom.num;

            if (((attachedActor = doorActor->child) != NULL) || ((attachedActor = doorActor->parent) != NULL)) {
                attachedActor->room = play->roomCtx.curRoom.num;
            }

            return 1;
        }
    }

    return 0;
}

void Player_SetupBattleTargetingStandStill(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    Player_SetupAction(play, this, Player_Action_BattleTargetStandStill, 1);

    if (this->leftRightBlendWeight < 0.5f) {
        anim = Player_GetFightingRightAnim(this);
        this->leftRightBlendWeight = 0.0f;
    } else {
        anim = Player_GetFightingLeftAnim(this);
        this->leftRightBlendWeight = 1.0f;
    }

    this->leftRightBlendWeightTarget = this->leftRightBlendWeight;
    Player_AnimPlayLoop(play, this, anim);
    this->yaw = this->actor.shape.rot.y;
}

void Player_SetupCalmTargetingStandStill(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_CalmTargetStandStill, 1);
    Player_AnimChangeOnceMorph(play, this, Player_GetStandStillAnim(this));
    this->yaw = this->actor.shape.rot.y;
}

void Player_SetupContextualStandStill(Player* this, PlayState* play) {
    if (Player_CheckBattleTargeting(this)) {
        Player_SetupBattleTargetingStandStill(this, play);
    } else if (Player_CheckCalmTargeting(this)) {
        Player_SetupCalmTargetingStandStill(this, play);
    } else {
        Player_SetupStandStillMorph(this, play);
    }
}

void Player_ReturnToStandStill(Player* this, PlayState* play) {
    PlayerActionFunc actionFunc;

    if (Player_CheckBattleTargeting(this)) {
        actionFunc = Player_Action_BattleTargetStandStill;
    } else if (Player_CheckCalmTargeting(this)) {
        actionFunc = Player_Action_CalmTargetStandStill;
    } else {
        actionFunc = Player_Action_StandStill;
    }

    Player_SetupAction(play, this, actionFunc, 1);
}

void Player_StartReturnToStandStill(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    if (Player_CheckBattleTargeting(this)) {
        this->av2.actionVar2 = 1;
    }
}

void Player_StartReturnToStandStillWithAnim(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_StartReturnToStandStill(this, play);
    Player_PlayAnimOnceWithWaterInfluence(play, this, anim);
}

int Player_CanCarryActor(Player* this) {
    return (this->interactRangeActor != NULL) && (this->heldActor == NULL);
}

void Player_ProcessCarryActor(PlayState* play, Player* this) {
    if (Player_CanCarryActor(this)) {
        Actor* interactRangeActor = this->interactRangeActor;
        s32 interactActorId = interactRangeActor->id;

        if (interactActorId == ACTOR_BG_TOKI_SWD) {
            this->interactRangeActor->parent = &this->actor;
            Player_SetupAction(play, this, Player_Action_SetDrawAndStartCutsceneAfterTimer, 0);
            this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        } else {
            LinkAnimationHeader* anim;

            if (interactActorId == ACTOR_BG_HEAVY_BLOCK) {
                Player_SetupAction(play, this, Player_Action_ThrowStonePillar, 0);
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                anim = &gPlayerAnim_link_normal_heavy_carry;
            } else if ((interactActorId == ACTOR_EN_ISHI) && ((interactRangeActor->params & 0xF) == 1)) {
                Player_SetupAction(play, this, Player_Action_LiftSilverBoulder, 0);
                anim = &gPlayerAnim_link_silver_carry;
            } else if (((interactActorId == ACTOR_EN_BOMBF) || (interactActorId == ACTOR_EN_KUSA)) &&
                       (Player_GetStrength() <= PLAYER_STR_NONE)) {
                Player_SetupAction(play, this, Player_Action_FailToLiftActor, 0);
                this->actor.world.pos.x =
                    (Math_SinS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.x;
                this->actor.world.pos.z =
                    (Math_CosS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.z;
                this->yaw = this->actor.shape.rot.y = interactRangeActor->yawTowardsPlayer + 0x8000;
                anim = &gPlayerAnim_link_normal_nocarry_free;
            } else {
                if (interactActorId == ACTOR_REACTOR_FUEL && (interactRangeActor->params & 1) == 0) {
                    Player_SetupAction(play, this, Player_Action_LiftHeavyFuel, 0);
                    anim = &gPlayerAnim_link_normal_nocarry_free;
                } else {
                    Player_SetupAction(play, this, Player_Action_LiftActor, 0);
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_carryB, this->modelAnimType);
                }
            }

            Player_AnimPlayOnce(play, this, anim);
        }
    } else {
        Player_SetupContextualStandStill(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
    }
}

void Player_SetupTalkToActor(PlayState* play, Player* this) {
    Player_SetupActionKeepMoveFlags(play, this, Player_Action_TalkToActor, 0);

    this->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE;

    if (this->actor.textId != 0) {
        Message_StartTextbox(play, this->actor.textId, this->talkActor);
        this->targetActor = this->talkActor;
    }
}

void Player_SetupRidingHorse(PlayState* play, Player* this) {
    Player_SetupActionKeepMoveFlags(play, this, Player_Action_RideHorse, 0);
}

void Player_SetupGrabPushPullWall(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_GrabPushPullWall, 0);
}

void Player_SetupClimbingWallOrDownLedge(PlayState* play, Player* this) {
    s32 prevTimer = this->av2.actionVar2;
    s32 prevActionVar1 = this->av1.actionVar1;

    Player_SetupActionKeepMoveFlags(play, this, Player_Action_ClimbingWallOrDownLedge, 0);
    this->actor.velocity.y = 0.0f;

    this->av2.actionVar2 = prevTimer;
    this->av1.actionVar1 = prevActionVar1;
}

void Player_SetupCrawling(PlayState* play, Player* this) {
    Player_SetupActionKeepMoveFlags(play, this, Player_Action_Crawling, 0);
}

void Player_SetupGetItem(PlayState* play, Player* this) {
    Player_SetupActionKeepMoveFlags(play, this, Player_Action_GetItem, 0);

    this->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE;

    if (this->getItemId == GI_HEART_CONTAINER_2) {
        this->av2.actionVar2 = 20;
    } else if (this->getItemId >= 0) {
        this->av2.actionVar2 = 1;
    } else {
        this->getItemId = -this->getItemId;
    }
}

s32 Player_StartJump(Player* this, PlayState* play) {
    s16 yawDiff;
    LinkAnimationHeader* anim;
    f32 temp;

    yawDiff = this->yaw - this->actor.shape.rot.y;

    if ((ABS(yawDiff) < 0x1000) && (this->speedXZ > 4.0f)) {
        anim = &gPlayerAnim_link_normal_run_jump;
    } else {
        anim = &gPlayerAnim_link_normal_jump;
    }

    if (this->speedXZ > (IREG(66) / 100.0f)) {
        temp = IREG(67) / 100.0f;
    } else {
        temp = (IREG(68) / 100.0f) + ((IREG(69) * this->speedXZ) / 1000.0f);
    }

    Player_SetupJumpWithSfx(this, anim, temp, play, NA_SE_VO_LI_AUTO_JUMP);
    this->av2.actionVar2 = 1;

    return 1;
}

void Player_SetupGrabLedge(PlayState* play, Player* this, CollisionPoly* colPoly, f32 distToPoly,
                           LinkAnimationHeader* anim) {
    f32 nx = COLPOLY_GET_NORMAL(colPoly->normal.x);
    f32 nz = COLPOLY_GET_NORMAL(colPoly->normal.z);

    Player_SetupAction(play, this, Player_Action_GrabLedge, 0);
    Player_StopCarryingActor(play, this);
    Player_AnimPlayOnce(play, this, anim);

    this->actor.world.pos.x -= (distToPoly + 1.0f) * nx;
    this->actor.world.pos.z -= (distToPoly + 1.0f) * nz;
    this->actor.shape.rot.y = this->yaw = Math_Atan2S(nz, nx);

    Player_ClearAttentionModeAndStopMoving(this);
    Player_SkelAnimeResetPrevTranslRot(this);
}

s32 Player_TryGrabLedgeInsteadOfFalling(Player* this, PlayState* play) {
    CollisionPoly* colPoly;
    s32 bgId;
    Vec3f pos;
    Vec3f colPolyPos;
    f32 dist;

    //! @bug `floorPitch` and `floorPitchAlt` are cleared to 0 before this function is called, because the player
    //! left the ground. The angles will always be zero and therefore will always pass these checks.
    //! The intention seems to be to prevent ledge hanging or vine grabbing when walking off of a steep enough slope.
    if ((this->actor.yDistToWater < -80.0f) && (ABS(this->floorPitch) < 0xAAA) && (ABS(this->floorPitchAlt) < 0xAAA)) {
        pos.x = this->actor.prevPos.x - this->actor.world.pos.x;
        pos.z = this->actor.prevPos.z - this->actor.world.pos.z;

        dist = sqrtf(SQ(pos.x) + SQ(pos.z));
        if (dist != 0.0f) {
            dist = 5.0f / dist;
        } else {
            dist = 0.0f;
        }

        pos.x = this->actor.prevPos.x + (pos.x * dist);
        pos.y = this->actor.world.pos.y;
        pos.z = this->actor.prevPos.z + (pos.z * dist);

        if (BgCheck_EntityLineTest1(&play->colCtx, &this->actor.world.pos, &pos, &colPolyPos, &colPoly, true, false,
                                    false, true, &bgId) &&
            (ABS(colPoly->normal.y) < 600)) {
            f32 nx = COLPOLY_GET_NORMAL(colPoly->normal.x);
            f32 ny = COLPOLY_GET_NORMAL(colPoly->normal.y);
            f32 nz = COLPOLY_GET_NORMAL(colPoly->normal.z);
            f32 distToPoly;
            s32 climbDownFlag;

            distToPoly = Math3D_UDistPlaneToPos(nx, ny, nz, colPoly->dist, &this->actor.world.pos);

            climbDownFlag = (sPrevFloorProperty == FLOOR_PROPERTY_CLIMB_DOWN_ADJACENT_WALL);
            if (!climbDownFlag && (SurfaceType_GetWallFlags(&play->colCtx, colPoly, bgId) & WALL_FLAG_3)) {
                climbDownFlag = 1;
            }

            Player_SetupGrabLedge(play, this, colPoly, distToPoly,
                                  climbDownFlag ? &gPlayerAnim_link_normal_Fclimb_startB
                                                : &gPlayerAnim_link_normal_fall);

            if (climbDownFlag) {
                Player_SetupMiniCs(play, this, Player_SetupClimbingWallOrDownLedge);

                this->yaw += 0x8000;
                this->actor.shape.rot.y = this->yaw;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                Player_AnimReplaceApplyFlags(play, this,
                                             ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_2 |
                                                 ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);

                this->av2.actionVar2 = -1;
                this->av1.actionVar1 = climbDownFlag;
            } else {
                this->stateFlags1 |= PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP;
                this->stateFlags1 &= ~PLAYER_STATE1_Z_PARALLEL;
            }

            Player_PlaySfx(this, NA_SE_PL_SLIPDOWN);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
            return 1;
        }
    }

    return 0;
}

void Player_SetupClimbOntoLedge(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_ClimbOntoLedge, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 1.3f);
}

static Vec3f sWaterRaycastOffset = { 0.0f, 0.0f, 100.0f };

void Player_SetupMidairBehavior(Player* this, PlayState* play) {
    s32 yawDiff;
    CollisionPoly* floorPoly;
    s32 bgId;
    WaterBox* waterbox;
    Vec3f raycastPos;
    f32 floorPosY;
    f32 waterPosY;

    this->fallDistance = this->fallStartHeight - (s32)this->actor.world.pos.y;

    if (!(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
        !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (!Player_ShouldEnterGrotto(play, this)) {
            if (sPrevFloorProperty == FLOOR_PROPERTY_STOP_XZ_MOMENTUM) {
                this->actor.world.pos.x = this->actor.prevPos.x;
                this->actor.world.pos.z = this->actor.prevPos.z;
                return;
            }

            if ((Player_Action_ZoraSwimJump == this->actionFunc) || this->stateFlags3 & PLAYER_STATE3_USING_BOOSTERS) {
                return;
            }
            
            if (!(this->stateFlags3 & PLAYER_STATE3_MIDAIR) && !(this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_7) &&
                (Player_Action_Midair != this->actionFunc) && (Player_Action_FallingDive != this->actionFunc)) {

                if ((sPrevFloorProperty == FLOOR_PROPERTY_STOP_ALL_MOMENTUM) || (this->meleeWeaponState != 0)) {
                    Math_Vec3f_Copy(&this->actor.world.pos, &this->actor.prevPos);
                    Player_ZeroSpeedXZ(this);
                    return;
                }

                if (this->hoverBootsTimer != 0) {
                    this->actor.velocity.y = 1.0f;
                    sPrevFloorProperty = FLOOR_PROPERTY_NO_JUMPING;
                    return;
                }

                yawDiff = (s16)(this->yaw - this->actor.shape.rot.y);

                Player_SetupAction(play, this, Player_Action_Midair, 1);
                Player_ResetAttributes(play, this);

                this->floorSfxOffset = this->prevFloorSfxOffset;

                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE) &&
                    !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                    (sPrevFloorProperty != FLOOR_PROPERTY_CLIMB_DOWN_ADJACENT_WALL) &&
                    (sPrevFloorProperty != FLOOR_PROPERTY_NO_JUMPING) && (sYDistToFloor > 20.0f) &&
                    (this->meleeWeaponState == 0) && (ABS(yawDiff) < 0x2000) && (this->speedXZ > 3.0f)) {

                    if ((sPrevFloorProperty == FLOOR_PROPERTY_FALLING_DIVE) &&
                        !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {

                        floorPosY = Player_RaycastFloorGetPolyInfo(play, this, &sWaterRaycastOffset, &raycastPos,
                                                                   &floorPoly, &bgId);
                        waterPosY = this->actor.world.pos.y;

                        if (WaterBox_GetSurface1(play, &play->colCtx, raycastPos.x, raycastPos.z, &waterPosY,
                                                 &waterbox) &&
                            ((waterPosY - floorPosY) > 50.0f)) {
                            Player_SetupJump(this, &gPlayerAnim_link_normal_run_jump_water_fall, 6.0f, play);
                            Player_SetupAction(play, this, Player_Action_FallingDive, 0);
                            return;
                        }
                    }

                    Player_StartJump(this, play);
                    return;
                }
                if ((sPrevFloorProperty == FLOOR_PROPERTY_NO_JUMPING) ||
                    (sYDistToFloor <= this->ageProperties->unk_34) ||
                    !Player_TryGrabLedgeInsteadOfFalling(this, play)) {
                    Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
                    return;
                }
            }
        }
    } else {
        this->fallStartHeight = this->actor.world.pos.y;
    }
}

s32 Player_RequestFpsCamera(PlayState* play, Player* this) {
    s32 camMode;

    if (this->attentionMode == 2) {
        if (Actor_PlayerIsAimingFpsItem(this)) {
            if (LINK_IS_ADULT) {
                camMode = CAM_MODE_AIM_ADULT;
            } else {
                camMode = CAM_MODE_AIM_CHILD;
            }
        } else {
            camMode = CAM_MODE_AIM_BOOMERANG;
        }
    } else {
        camMode = CAM_MODE_FIRST_PERSON;
    }

    return Camera_RequestMode(Play_GetCamera(play, CAM_ID_MAIN), camMode);
}

/**
 * If appropriate, setup action for performing a `csAction`
 *
 * @return  true if a `csAction` is started, false if not
 */
s32 Player_SetupCsAction(PlayState* play, Player* this) {
    // attentionMode will get set to 3 in `Player_UpdateCommon` if `this->csAction` is non-zero
    // (with a special case for `PLAYER_CSACTION_END`)
    if (this->attentionMode == 3) {
        Player_SetupAction(play, this, Player_Action_CsAction, 0);

        if (this->cv.haltActorsDuringCsAction) {
            this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        }

        Player_InactivateMeleeWeapon(this);
        return true;
    } else {
        return false;
    }
}

void Player_LoadGetItemObject(Player* this, s16 objectId) {
    s32 pad;
    u32 size;

    if (objectId != OBJECT_INVALID) {
        this->giObjectLoading = true;
        osCreateMesgQueue(&this->giObjectLoadQueue, &this->giObjectLoadMsg, 1);

        size = gObjectTable[objectId].vromEnd - gObjectTable[objectId].vromStart;

        LOG_HEX("size", size, "../z_player.c", 9090);
        // ASSERT(size <= 1024 * 8, "size <= 1024 * 8", "../z_player.c", 9091);

        DmaMgr_RequestAsync(&this->giObjectDmaRequest, this->giObjectSegment, gObjectTable[objectId].vromStart, size, 0,
                            &this->giObjectLoadQueue, NULL, "../z_player.c", 9099);
    }
}

void Player_SetupMagicSpell(PlayState* play, Player* this, s32 magicSpell) {
    Player_SetupActionKeepItemAction(play, this, Player_Action_MagicSpell, 0);

    this->av1.actionVar1 = magicSpell - 3;

    //! @bug `MAGIC_CONSUME_WAIT_PREVIEW` is not guaranteed to succeed.
    //! Ideally, the return value of `Magic_RequestChange` should be checked before allowing the process of
    //! using a spell to continue. If the magic state change request fails, `gSaveContext.magicTarget` will
    //! never be set correctly.
    //! When `MAGIC_STATE_CONSUME_SETUP` is set in `Player_Action_MagicSpell`, magic will eventually be
    //! consumed to a stale target value. If that stale target value is higher than the current
    //! magic value, it will be consumed to zero.
    Magic_RequestChange(play, sMagicSpellCosts[magicSpell], MAGIC_CONSUME_WAIT_PREVIEW);

    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_magic_tame, 0.83f);

    if (magicSpell == 5) {
        this->subCamId = OnePointCutscene_Init(play, 1100, -101, NULL, CAM_ID_MAIN);
    } else {
        Player_SetUseItemCam(play, 10);
    }
}

void Player_ClearLookAngles(Player* this) {
    this->actor.focus.rot.x = this->actor.focus.rot.z = this->headRot.x = this->headRot.y = this->headRot.z =
        this->upperBodyRot.x = this->upperBodyRot.y = this->upperBodyRot.z = 0;

    this->actor.focus.rot.y = this->actor.shape.rot.y;
}

static u8 sExchangeGetItemIDs[] = {
    GI_ZELDAS_LETTER,       // EXCH_ITEM_ZELDAS_LETTER
    GI_WEIRD_EGG,           // EXCH_ITEM_WEIRD_EGG
    GI_CHICKEN,             // EXCH_ITEM_CHICKEN
    GI_MAGIC_BEAN,          // EXCH_ITEM_MAGIC_BEAN
    GI_POCKET_EGG,          // EXCH_ITEM_POCKET_EGG
    GI_POCKET_CUCCO,        // EXCH_ITEM_POCKET_CUCCO
    GI_COJIRO,              // EXCH_ITEM_COJIRO
    GI_ODD_MUSHROOM,        // EXCH_ITEM_ODD_MUSHROOM
    GI_ODD_POTION,          // EXCH_ITEM_ODD_POTION
    GI_POACHERS_SAW,        // EXCH_ITEM_POACHERS_SAW
    GI_BROKEN_GORONS_SWORD, // EXCH_ITEM_BROKEN_GORONS_SWORD
    GI_PRESCRIPTION,        // EXCH_ITEM_PRESCRIPTION
    GI_EYEBALL_FROG,        // EXCH_ITEM_EYEBALL_FROG
    GI_EYE_DROPS,           // EXCH_ITEM_EYE_DROPS
    GI_CLAIM_CHECK,         // EXCH_ITEM_CLAIM_CHECK
    GI_MASK_SKULL,          // EXCH_ITEM_MASK_SKULL
    GI_MASK_SPOOKY,         // EXCH_ITEM_MASK_SPOOKY
    GI_MASK_KEATON,         // EXCH_ITEM_MASK_KEATON
    GI_MASK_BUNNY_HOOD,     // EXCH_ITEM_MASK_BUNNY_HOOD
    GI_MASK_TRUTH,          // EXCH_ITEM_MASK_TRUTH
    GI_MASK_GORON,          // EXCH_ITEM_MASK_GORON
    GI_MASK_ZORA,           // EXCH_ITEM_MASK_ZORA
    GI_MASK_GERUDO,         // EXCH_ITEM_MASK_GERUDO
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_FISH
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BLUE_FIRE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BUG
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_POE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BIG_POE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_RUTOS_LETTER
};

static LinkAnimationHeader* sExchangeItemAnims[] = {
    &gPlayerAnim_link_normal_give_other,
    &gPlayerAnim_link_bottle_read,
    &gPlayerAnim_link_normal_take_out,
};

s32 Player_ActionChange_TryItemCsOrFirstPerson(Player* this, PlayState* play) {
    s32 item;
    s32 bottle;
    GetItemEntry* giEntry;
    Actor* talkActor;

    if ((this->attentionMode != 0) && (Player_IsSwimming(this) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                                       (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE))) {

        if (!Player_SetupCsAction(play, this)) {
            if (this->attentionMode == 4) {
                item = Player_ActionToMagicSpell(this, this->itemAction);
                if (item >= 0) {
                    if ((item != 3) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        Player_SetupMagicSpell(play, this, item);
                    } else {
                        Player_SetupAction(play, this, Player_Action_ChooseFWOption, 1);
                        this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                        Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
                        Player_SetUseItemCam(play, 4);
                    }

                    Player_ClearAttentionModeAndStopMoving(this);
                    return 1;
                }

                item = this->itemAction - PLAYER_IA_ZELDAS_LETTER;
                if ((item >= 0) ||
                    (bottle = Player_ActionToBottle(this, this->itemAction) - 1,
                     ((bottle >= 0) && (bottle < 6) &&
                      ((this->itemAction > PLAYER_IA_BOTTLE_POE) ||
                       ((this->talkActor != NULL) && (((this->itemAction == PLAYER_IA_BOTTLE_POE) &&
                                                       (this->exchangeItemId == EXCH_ITEM_BOTTLE_POE)) ||
                                                      (this->exchangeItemId == EXCH_ITEM_BOTTLE_BLUE_FIRE))))))) {

                    if ((play->actorCtx.titleCtx.delayTimer == 0) && (play->actorCtx.titleCtx.alpha == 0)) {
                        Player_SetupActionKeepItemAction(play, this, Player_Action_PresentExchangeItem, 0);

                        if (item >= 0) {
                            giEntry = &sGetItemTable[sExchangeGetItemIDs[item] - 1];
                            Player_LoadGetItemObject(this, giEntry->objectId);
                        }

                        this->stateFlags1 |=
                            PLAYER_STATE1_TALKING | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;

                        if (item >= 0) {
                            item = item + 1;
                        } else {
                            item = bottle + 0x18;
                        }

                        talkActor = this->talkActor;

                        if ((talkActor != NULL) &&
                            ((this->exchangeItemId == item) || (this->exchangeItemId == EXCH_ITEM_BOTTLE_BLUE_FIRE) ||
                             ((this->exchangeItemId == EXCH_ITEM_BOTTLE_POE) &&
                              (this->itemAction == PLAYER_IA_BOTTLE_BIG_POE)) ||
                             ((this->exchangeItemId == EXCH_ITEM_MAGIC_BEAN) &&
                              (this->itemAction == PLAYER_IA_BOTTLE_BUG))) &&
                            ((this->exchangeItemId != EXCH_ITEM_MAGIC_BEAN) ||
                             (this->itemAction == PLAYER_IA_MAGIC_BEAN))) {
                            if (this->exchangeItemId == EXCH_ITEM_MAGIC_BEAN) {
                                Inventory_ChangeAmmo(ITEM_MAGIC_BEAN, -1);
                                Player_SetupActionKeepItemAction(play, this, Player_Action_PlantMagicBean, 0);
                                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                                this->av2.actionVar2 = 0x50;
                                this->av1.actionVar1 = -1;
                            }
                            talkActor->flags |= ACTOR_FLAG_TALK;
                            this->targetActor = this->talkActor;
                        } else if (item == EXCH_ITEM_BOTTLE_RUTOS_LETTER) {
                            this->av1.actionVar1 = 1;
                            this->actor.textId = 0x4005;
                            Player_SetUseItemCam(play, 1);
                        } else {
                            this->av1.actionVar1 = 2;
                            this->actor.textId = 0xCF;
                            Player_SetUseItemCam(play, 4);
                        }

                        this->actor.flags |= ACTOR_FLAG_TALK;
                        this->exchangeItemId = item;

                        if (this->av1.actionVar1 < 0) {
                            Player_AnimChangeOnceMorph(play, this,
                                                       GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check, this->modelAnimType));
                        } else {
                            Player_AnimPlayOnce(play, this, sExchangeItemAnims[this->av1.actionVar1]);
                        }

                        Player_ClearAttentionModeAndStopMoving(this);
                    }
                    return 1;
                }

                item = Player_ActionToBottle(this, this->itemAction);
                if (item >= 0) {
                    if (item == 0xC) {
                        Player_SetupActionKeepItemAction(play, this, Player_Action_HealWithFairy, 0);
                        Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_bottle_bug_out);
                        Player_SetUseItemCam(play, 3);
                    } else if ((item > 0) && (item < 4)) {
                        Player_SetupActionKeepItemAction(play, this, Player_Action_DropItemFromBottle, 0);
                        Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_bottle_fish_out);
                        Player_SetUseItemCam(play, (item == 1) ? 1 : 5);
                    } else {
                        Player_SetupActionKeepItemAction(play, this, Player_Action_DrinkFromBottle, 0);
                        Player_AnimChangeOnceMorphAdjusted(play, this, &gPlayerAnim_link_bottle_drink_demo_start);
                        Player_SetUseItemCam(play, 2);
                    }
                } else {
                    Player_SetupActionKeepItemAction(play, this, Player_Action_PlayOcarina, 0);
                    Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_normal_okarina_start);
                    this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_GENERAL;
                    Player_SetUseItemCam(play, (this->ocarinaActor != NULL) ? 0x5B : 0x5A);
                    if (this->ocarinaActor != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR;
                        Camera_SetViewParam(Play_GetCamera(play, CAM_ID_MAIN), CAM_VIEW_TARGET, this->ocarinaActor);
                    }
                }
            } else if (Player_RequestFpsCamera(play, this) != CAM_MODE_NORMAL) {
                if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                    Player_SetupAction(play, this, Player_Action_FirstPersonAiming, 1);
                    this->av2.actionVar2 = 13;
                    Player_ClearLookAngles(this);
                }
                this->stateFlags1 |= PLAYER_STATE1_IN_FIRST_PERSON_MODE;
                Sfx_PlaySfxCentered(NA_SE_SY_CAMERA_ZOOM_UP);
                Player_ZeroSpeedXZ(this);
                return 1;
            } else {
                this->attentionMode = 0;
                Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                return 0;
            }

            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
        }

        Player_ClearAttentionModeAndStopMoving(this);
        return 1;
    }

    return 0;
}

s32 Player_ActionChange_TrySpeakOrCheck(Player* this, PlayState* play) {
    Actor* talkActor = this->talkActor;
    Actor* targetActor = this->targetActor;
    Actor* naviActor = NULL;
    s32 naviHasText = 0;
    s32 targetActorHasText;

    targetActorHasText = (targetActor != NULL) && (CHECK_FLAG_ALL(targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
                                                   (targetActor->naviEnemyId != NAVI_ENEMY_NONE));

    if (targetActorHasText || (this->naviTextId != 0)) {
        naviHasText = (this->naviTextId < 0) && ((ABS(this->naviTextId) & 0xFF00) != 0x200);
        if (naviHasText || !targetActorHasText) {
            naviActor = this->naviActor;
            if (naviHasText) {
                targetActor = NULL;
                talkActor = NULL;
            }
        } else {
            naviActor = targetActor;
        }
    }

    if ((talkActor != NULL) || (naviActor != NULL)) {
        if ((targetActor == NULL) || (targetActor == talkActor) || (targetActor == naviActor)) {
            if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                ((this->heldActor != NULL) &&
                 (naviHasText || (talkActor == this->heldActor) || (naviActor == this->heldActor) ||
                  ((talkActor != NULL) && (talkActor->flags & ACTOR_FLAG_16))))) {
                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                    (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ||
                    (Player_IsSwimming(this) && !(this->stateFlags2 & PLAYER_STATE2_DIVING))) {

                    if (talkActor != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_CAN_SPEAK_OR_CHECK;
                        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) || (talkActor->flags & ACTOR_FLAG_16)) {
                            naviActor = NULL;
                        } else if (naviActor == NULL) {
                            return 0;
                        }
                    }

                    if (naviActor != NULL) {
                        if (!naviHasText) {
                            this->stateFlags2 |= PLAYER_STATE2_NAVI_REQUESTING_TALK;
                        }

                        if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) && !naviHasText) {
                            return 0;
                        }

                        talkActor = naviActor;
                        this->talkActor = NULL;

                        if (naviHasText || !targetActorHasText) {
                            naviActor->textId = ABS(this->naviTextId);
                        } else {
                            if (naviActor->naviEnemyId != NAVI_ENEMY_NONE) {
                                naviActor->textId = naviActor->naviEnemyId + 0x600;
                            }
                        }
                    }

                    this->currentMask = sCurrentMask;
                    Player_StartTalkToActor(play, talkActor);
                    return 1;
                }
            }
        }
    }

    return 0;
}

s32 Player_ForceFirstPerson(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_RIDING_HORSE)) &&
        Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_FIRST_PERSON)) {
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
            (Player_IsSwimming(this) && (this->actor.yDistToWater < this->ageProperties->unk_2C)) ||
            (Player_IsSwimming(this) && this->currentBoots == PLAYER_BOOTS_ZORA)) {
            this->attentionMode = 1;
            return 1;
        }
    }

    return 0;
}

s32 Player_ActionChange_SetupCUpBehavior(Player* this, PlayState* play) {
    if (this->attentionMode != 0) {
        Player_ActionChange_TryItemCsOrFirstPerson(this, play);
        return 1;
    }

    if ((this->targetActor != NULL) && (CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
                                        (this->targetActor->naviEnemyId != NAVI_ENEMY_NONE))) {
        this->stateFlags2 |= PLAYER_STATE2_NAVI_REQUESTING_TALK;
    } else if ((this->naviTextId == 0) && !Player_CheckBattleTargeting(this) &&
               CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_SHOP_VIEWPOINT) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_TOGGLE_VIEWPOINT) && !Player_ForceFirstPerson(this, play)) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
    }

    return 0;
}

void Player_SetupJumpSlash(PlayState* play, Player* this, s32 arg2, f32 xzSpeed, f32 yVelocity) {
    Player_SetupMeleeWeaponAttack(play, this, arg2);
    Player_SetupAction(play, this, Player_Action_JumpSlash, 0);

    this->stateFlags3 |= PLAYER_STATE3_MIDAIR;

    this->yaw = this->actor.shape.rot.y;
    this->speedXZ = xzSpeed;
    this->actor.velocity.y = yVelocity;

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    this->hoverBootsTimer = 0;

    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
}

s32 Player_CanJumpSlash(Player* this) {
    if (!(this->stateFlags1 & PLAYER_STATE1_DEFENDING) && (Player_GetMeleeWeaponHeld(this) != 0)) {
        if (sUseHeldItem ||
            ((this->actor.category != ACTORCAT_PLAYER) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B))) {
            return 1;
        }
    }

    return 0;
}

s32 Player_TryMidairJumpSlash(Player* this, PlayState* play) {
    if (Player_CanJumpSlash(this) && (sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE)) {
        Player_SetupJumpSlash(play, this, PLAYER_MELEEATKTYPE_JUMPSLASH_START, 3.0f, 4.5f);
        return 1;
    }

    return 0;
}

void Player_SetupRolling(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_Rolling, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime,
                                   GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing_roll, this->modelAnimType),
                                   1.25f * sWaterSpeedScale);
}

s32 Player_TrySetupRolling(Player* this, PlayState* play) {
    if ((this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) &&
        (sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE)) {
        Player_SetupRolling(this, play);
        return 1;
    }

    return 0;
}

void Player_SetupBackflipOrSidehop(Player* this, PlayState* play, s32 relativeStickInput) {
    Player_SetupJumpWithSfx(this, sManualJumpAnims[relativeStickInput][0], !(relativeStickInput & 1) ? 5.8f : 3.5f,
                            play, NA_SE_VO_LI_SWORD_N);

    if (relativeStickInput) {}

    this->av2.actionVar2 = 1;
    this->av1.relativeStickInput = relativeStickInput;

    this->yaw = this->actor.shape.rot.y + (relativeStickInput << 0xE);
    this->speedXZ = !(relativeStickInput & 1) ? 6.0f : 8.5f;

    this->stateFlags2 |= PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING;

    Player_PlaySfx(this, ((relativeStickInput << 0xE) == 0x8000) ? NA_SE_PL_ROLL : NA_SE_PL_SKIP);
}

s32 Player_ActionChange_TryJumpSlashOrRoll(Player* this, PlayState* play) {
    s32 relativeStickInput;

    if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) != FLOOR_EFFECT_1)) {
        relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];

        if (relativeStickInput <= 0) {
            if (Player_CheckTargeting(this)) {
                if (this->actor.category != ACTORCAT_PLAYER) {
                    if (relativeStickInput < 0) {
                        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, REG(69) / 100.0f, play);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                } else {
                    if ((Player_GetMeleeWeaponHeld(this) != 0) && Player_CanUpdateItems(this)) {
                        Player_SetupJumpSlash(play, this, PLAYER_MELEEATKTYPE_JUMPSLASH_START, 5.0f, 5.0f);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                }
                return 1;
            }
        } else {
            Player_SetupBackflipOrSidehop(this, play, relativeStickInput);
            return 1;
        }
    }

    return 0;
}

void Player_FinishRun(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 sp30;

    sp30 = this->walkFrame - 3.0f;
    if (sp30 < 0.0f) {
        sp30 += 29.0f;
    }

    if (sp30 < 14.0f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk_endL, this->modelAnimType);
        sp30 = 11.0f - sp30;
        if (sp30 < 0.0f) {
            sp30 = 1.375f * -sp30;
        }
        sp30 /= 11.0f;
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk_endR, this->modelAnimType);
        sp30 = 26.0f - sp30;
        if (sp30 < 0.0f) {
            sp30 = 2 * -sp30;
        }
        sp30 /= 12.0f;
    }

    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE,
                         4.0f * sp30);
    this->yaw = this->actor.shape.rot.y;
}

void Player_StartFinishRun(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_FinishRun(this, play);
}

void Player_SetupStandStill(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_StandStill, 1);
    Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
    this->yaw = this->actor.shape.rot.y;
}

void Player_ClearLookAndAttention(Player* this, PlayState* play) {
    if (!(this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        Player_ClearLookAngles(this);
        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            Player_SetupSwimIdle(play, this);
        } else {
            Player_SetupContextualStandStill(this, play);
        }
        if (this->attentionMode < 4) {
            this->attentionMode = 0;
        }
    }

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                           PLAYER_STATE1_IN_FIRST_PERSON_MODE);
}

s32 Player_ActionChange_TryRollOrPutAway(Player* this, PlayState* play) {
    if (!Player_TryBattleTargeting(this) && (sUpperBodyBusy == 0) &&
        !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        if (Player_TrySetupRolling(this, play)) {
            return 1;
        }
        if ((this->putAwayTimer == 0) && (this->heldItemAction >= PLAYER_IA_SWORD_MASTER)) {
            Player_UseItem(play, this, ITEM_NONE);
        } else {
            this->stateFlags2 ^= PLAYER_STATE2_NAVI_IS_ACTIVE;
        }
    }

    return 0;
}

s32 Player_ActionChange_TryDefend(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    if ((play->shootingGalleryStatus == 0) && (this->currentShield != PLAYER_SHIELD_NONE) &&
        CHECK_BTN_ALL(sControlInput->cur.button, BTN_R) &&
        (Player_IsChildWithHylianShield(this) || (!Player_CheckCalmTargeting(this) && (this->targetActor == NULL)))) {

        Player_InactivateMeleeWeapon(this);
        Player_DetachHeldActor(play, this);

        if (Player_SetupAction(play, this, Player_Action_AimShieldCrouched, 0)) {
            this->stateFlags1 |= PLAYER_STATE1_DEFENDING;

            if (!Player_IsChildWithHylianShield(this)) {
                Player_SetModelsForHoldingShield(this);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense, this->modelAnimType);
            } else {
                anim = &gPlayerAnim_clink_normal_defense_ALL;
            }

            if (anim != this->skelAnime.animation) {
                if (Player_CheckBattleTargeting(this)) {
                    this->unk_86C = 1.0f;
                } else {
                    this->unk_86C = 0.0f;
                    Player_ResetLRBlendWeight(this);
                }
                this->upperBodyRot.x = this->upperBodyRot.y = this->upperBodyRot.z = 0;
            }

            frame = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);

            if (Player_IsChildWithHylianShield(this)) {
                Player_AnimReplaceApplyFlags(play, this, ANIM_FLAG_PLAYER_2);
            }

            Player_PlaySfx(this, NA_SE_IT_SHIELD_POSTURE);
        }

        return 1;
    }

    return 0;
}

s32 Player_TryTurnAroundWhileRunning(Player* this, f32* speedTarget, s16* yawTarget) {
    s16 yaw = this->yaw - *yawTarget;

    if (ABS(yaw) > DEG_TO_BINANG(135.0f)) {
        if (Player_StepSpeedXZToZero(this)) {
            *speedTarget = 0.0f;
            *yawTarget = this->yaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_DeactivateComboTimer(Player* this) {
    if ((this->comboTimer > 0) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        this->comboTimer = -this->comboTimer;
    }
}

s32 Player_ActionChange_TryChargeSpinAttack(Player* this, PlayState* play) {
    if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_DEFENDING) && (Player_GetMeleeWeaponHeld(this) != 0) &&
            (this->comboTimer == 1) && (this->heldItemAction != PLAYER_IA_DEKU_STICK)) {
            if ((this->heldItemAction != PLAYER_IA_SWORD_BIGGORON) ||
                (gSaveContext.save.info.playerData.swordHealth > 0.0f)) {
                Player_SetupChargeSpinAttack(play, this);
                return 1;
            }
        }
    } else {
        Player_DeactivateComboTimer(this);
    }

    return 0;
}

s32 Player_TryThrowDekuNut(PlayState* play, Player* this) {
    if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (AMMO(ITEM_DEKU_NUT) != 0)) {
        Player_SetupAction(play, this, Player_Action_ThrowDekuNut, 0);
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_light_bom);
        this->attentionMode = 0;
        return 1;
    }

    return 0;
}

static BottleSwingAnimInfo sBottleSwingAnims[] = {
    { &gPlayerAnim_link_bottle_bug_miss, &gPlayerAnim_link_bottle_bug_in, 2, 3 },
    { &gPlayerAnim_link_bottle_fish_miss, &gPlayerAnim_link_bottle_fish_in, 5, 3 },
};

s32 Player_CanSwingBottleOrCastFishingRod(PlayState* play, Player* this) {
    Vec3f sp24;

    if (sUseHeldItem) {
        if (Player_GetBottleHeld(this) >= 0) {
            Player_SetupAction(play, this, Player_Action_SwingBottle, 0);

            if (this->actor.yDistToWater > 12.0f) {
                this->av2.actionVar2 = 1;
            }

            Player_AnimPlayOnceAdjusted(play, this, sBottleSwingAnims[this->av2.actionVar2].bottleSwingAnim);

            Player_PlaySfx(this, NA_SE_IT_SWORD_SWING);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
            return 1;
        }

        if (this->heldItemAction == PLAYER_IA_FISHING_POLE) {
            sp24 = this->actor.world.pos;
            sp24.y += 50.0f;

            if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->actor.world.pos.z > 1300.0f) ||
                BgCheck_SphVsFirstPoly(&play->colCtx, &sp24, 20.0f)) {
                Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
                return 0;
            }

            Player_SetupAction(play, this, Player_Action_CastFishingRod, 0);
            this->unk_860 = 1;
            Player_ZeroSpeedXZ(this);
            Player_AnimPlayOnce(play, this, &gPlayerAnim_link_fishing_throw);
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

void Player_SetupRun(Player* this, PlayState* play) {
    PlayerActionFunc actionFunc;

    if (Player_CheckTargeting(this)) {
        actionFunc = Player_Action_TargetRun;
    } else {
        actionFunc = Player_Action_Run;
    }

    Player_SetupAction(play, this, actionFunc, 1);
    Player_AnimChangeLoopMorph(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_run, this->modelAnimType));

    this->walkFloorPitch = 0;
    this->unk_864 = this->walkFrame = 0.0f;
}

void Player_SetupZParallelRun(Player* this, PlayState* play, s16 yawTarget) {
    this->actor.shape.rot.y = this->yaw = yawTarget;
    Player_SetupRun(this, play);
}

s32 Player_TrySpawnStillOnLand(PlayState* play, Player* this, f32 arg2) {
    WaterBox* waterbox;
    f32 posY;

    posY = this->actor.world.pos.y;
    if (WaterBox_GetSurface1(play, &play->colCtx, this->actor.world.pos.x, this->actor.world.pos.z, &posY, &waterbox) !=
        0) {
        posY -= this->actor.world.pos.y;
        if (this->ageProperties->unk_24 <= posY) {
            Player_SetupAction(play, this, Player_Action_SpawnSwimming, 0);
            Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim);
            this->stateFlags1 |= PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE;
            this->av2.actionVar2 = 20;
            this->speedXZ = 2.0f;
            Player_SetBootData(play, this);
            return 0;
        }
    }

    Player_SetupMiniCsMovement(play, this, arg2, this->actor.shape.rot.y);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    return 1;
}

void Player_Spawn_StandStill(PlayState* play, Player* this) {
    if (Player_TrySpawnStillOnLand(play, this, 180.0f)) {
        this->av2.actionVar2 = -20;
    }
}

void Player_Spawn_SlowWalk(PlayState* play, Player* this) {
    this->speedXZ = 2.0f;
    gSaveContext.entranceSpeed = 2.0f;
    if (Player_TrySpawnStillOnLand(play, this, 120.0f)) {
        this->av2.actionVar2 = -15;
    }
}

void Player_Spawn_MoveWithEntranceSpeed(PlayState* play, Player* this) {
    if (gSaveContext.entranceSpeed < 0.1f) {
        gSaveContext.entranceSpeed = 0.1f;
    }

    this->speedXZ = gSaveContext.entranceSpeed;

    if (Player_TrySpawnStillOnLand(play, this, 800.0f)) {
        this->av2.actionVar2 = -80 / this->speedXZ;
        if (this->av2.actionVar2 < -20) {
            this->av2.actionVar2 = -20;
        }
    }
}

void Player_SetupZParallelBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_ZParallelBackwalk, 1);
    LinkAnimation_CopyJointToMorph(play, &this->skelAnime);
    this->unk_864 = this->walkFrame = 0.0f;
    this->yaw = yaw;
}

void Player_SetupZParallelSidewalk(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_ZParallelSidewalk, 1);
    Player_AnimChangeLoopMorph(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType));
}

void Player_SetupBattleTargetBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_BattleTargetBackwalk, 1);
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_anchor_back_walk, 2.2f, 0.0f,
                         Animation_GetLastFrame(&gPlayerAnim_link_anchor_back_walk), ANIMMODE_ONCE, -6.0f);
    this->speedXZ = 8.0f;
    this->yaw = yaw;
}

void Player_SetupBattleTargetSidewalk(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_BattleTargetSidewalk, 1);
    Player_AnimChangeLoopMorph(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType));
    this->walkFrame = 0.0f;
}

void Player_SetupFinishBattleTargetBackwalk(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_FinishBattleTargetBackwalk, 1);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_anchor_back_brake, 2.0f);
}

void Player_StartTurn(PlayState* play, Player* this, s16 yaw) {
    this->yaw = yaw;
    Player_SetupAction(play, this, Player_Action_Turn, 1);
    this->unk_87E = 1200;
    this->unk_87E *= sWaterSpeedScale;
    LinkAnimation_Change(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_45_turn, this->modelAnimType), 1.0f,
                         0.0f, 0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_FinishBattleTarget(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    Player_SetupAction(play, this, Player_Action_StandStill, 1);

    if (this->leftRightBlendWeight < 0.5f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitR2wait, this->modelAnimType);
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitL2wait, this->modelAnimType);
    }
    Player_AnimPlayOnce(play, this, anim);

    this->yaw = this->actor.shape.rot.y;
}

void Player_SetupBattleTargetStandStill(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_BattleTargetStandStill, 1);
    Player_AnimChangeOnceMorph(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait2waitR, this->modelAnimType));
    this->av2.actionVar2 = 1;
}

void Player_StartFinishBattleTarget(Player* this, PlayState* play) {
    if (this->speedXZ != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_FinishBattleTarget(this, play);
    }
}

void Player_EndMiniCsMovement(Player* this, PlayState* play) {
    if (this->speedXZ != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_SetupContextualStandStill(this, play);
    }
}

s32 Player_TrySpawnSplash(PlayState* play, Player* this, f32 arg2, s32 splashScale) {
    f32 sp3C = fabsf(arg2);
    WaterBox* sp38;
    f32 sp34;
    Vec3f splashPos;
    s32 splashType;

    if (sp3C > 2.0f) {
        splashPos.x = this->bodyPartsPos[PLAYER_BODYPART_WAIST].x;
        splashPos.z = this->bodyPartsPos[PLAYER_BODYPART_WAIST].z;
        sp34 = this->actor.world.pos.y;
        if (WaterBox_GetSurface1(play, &play->colCtx, splashPos.x, splashPos.z, &sp34, &sp38)) {
            if ((sp34 - this->actor.world.pos.y) < 100.0f) {
                splashType = (sp3C <= 10.0f) ? 0 : 1;
                splashPos.y = sp34;
                EffectSsGSplash_Spawn(play, &splashPos, NULL, NULL, splashType, splashScale);
                return 1;
            }
        }
    }

    return 0;
}

void Player_StartJumpOutOfWater(PlayState* play, Player* this, f32 arg2) {
    this->stateFlags1 |= PLAYER_STATE1_JUMPING;
    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;

    Player_ResetSubCam(play, this);
    if (Player_TrySpawnSplash(play, this, arg2, 500)) {
        Player_PlaySfx(this, NA_SE_EV_JUMP_OUT_WATER);
    }

    Player_SetBootData(play, this);
}

void Player_ZoraSwimDamaged(Player* this, PlayState* play) {
    Player_SetVerticalWaterSpeed(this);
    Math_StepToF(&this->speedXZ, 0.0f, 0.4f);
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->speedXZ < 10.0f)) {
        Player_SetupSwimIdle(play, this);
    }
}

// func_80840A30
// Check if bonked and if so, rumble, play sound, etc.
s32 Player_CheckZoraSwimBonk(PlayState* play, Player* this, f32* speedXZ, f32 speedTarget) {
    Actor* cylinderOc = NULL;

    if (speedTarget <= *speedXZ) {
        // If interacting with a wall and close to facing it
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sShapeYawToTouchedWall < 0x1C00)) {
            if (!((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF))) {
                if ((this->stateFlags3 & PLAYER_STATE3_ZORA_SWIMMING) &&
                    (Player_Action_ZoraSwimJump != this->actionFunc)) {
                    Player_SetupAction(play, this, Player_ZoraSwimDamaged, 0);
                    Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_swimer_swim_hit);
                    Player_ResetAttributes(play, this);
                    this->speedXZ *= 0.2f;
                } else {
                    Player_SetupAction(play, this, Player_Action_Rolling, 0);
                    Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_hip_down, this->modelAnimType));
                    this->av2.actionVar2 = 1;
                }

                this->speedXZ = -this->speedXZ;
                Player_RequestQuake(play, 33267, 3, 12);
                Player_RequestRumble(this, 255, 20, 150, 0);
                // Actor_SetPlayerImpact(play, PLAYER_IMPACT_BONK, 2, 100.0f, &this->actor.world.pos);
                Player_PlaySfx(this, NA_SE_PL_BODY_HIT);
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                return true;
            }
        }
    }
    return false;
}

// Zora swim jump (Player_Action_28)
void Player_Action_ZoraSwimJump(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoopAdjusted(play, this, &gLinkAdultSkelPz_fishswimAnim);
    }

    Math_SmoothStepToS(&this->zoraSwimRoll, 0, 6, 0x7D0, 0x190);
    if (!Player_CheckZoraSwimBonk(play, this, &this->speedXZ, 0.0f)) {
        if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
            this->stateFlags3 &= ~PLAYER_STATE3_ZORA_SWIMMING;
            Player_SetBootData(play, this);
            if (this->shapePitchOffset > 0x36B0) {
                this->actor.colChkInfo.damage = 0x10;
                Player_TakeColliderDamage(play, this, 1, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
            } else {
                Player_SetupRolling(this, play);
            }
        } else {
            this->actor.gravity = -1.0f;
            this->shapePitchOffset = Math_Atan2S(this->actor.speed, -this->actor.velocity.y);
            // func_8082F164(this, BTN_R);
        }
    }
}

// Modified Player_TryDive
s32 Player_TryDive(PlayState* play, Player* this, Input* input) {
    if ((!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) &&
        ((input == NULL) ||
         ((CHECK_BTN_ALL(input->press.button, BTN_A)) &&
          ((ABS_ALT(this->shapePitchOffset) < 0x2EE0) && (this->currentBoots < PLAYER_BOOTS_ZORA))))) {

        Player_SetupAction(play, this, Player_Action_Dive, 0);
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_swimer_swim_deep_start);

        this->shapePitchOffset = 0;
        this->stateFlags2 |= PLAYER_STATE2_DIVING;
        this->actor.velocity.y = 0.0f;
        if (input != NULL) {
            this->stateFlags2 |= PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER;
            Player_PlaySfx(this, NA_SE_PL_DIVE_BUBBLE);
        }

        return true;
    }

    if (((this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) ||
         ((this->stateFlags2 & PLAYER_STATE2_DIVING) &&
          (((Player_Action_ZoraSwim != this->actionFunc) && !(this->stateFlags3 & PLAYER_STATE3_ZORA_SWIMMING)) ||
           (this->shapePitchOffset < -0x1555)))) &&
        ((this->actor.yDistToWater - this->actor.velocity.y) < this->ageProperties->unk_30)) {
        s16 zoraJumpAngle;
        f32 zoraJumpSpeed;

        this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
        Player_ResetSubCam(play, this);

        if (this->stateFlags3 & PLAYER_STATE3_ZORA_SWIMMING) {
            zoraJumpAngle = this->zoraSwimRoll;
            zoraJumpSpeed = this->zoraSwimYTarget * 1.5f;
            Player_SetupAction(play, this, Player_Action_ZoraSwimJump, 1);
            this->stateFlags3 |= PLAYER_STATE3_ZORA_SWIMMING;
            this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;
            zoraJumpSpeed = CLAMP_MAX(zoraJumpSpeed, 13.5f);
            this->speedXZ = Math_CosS(this->shapePitchOffset) * zoraJumpSpeed;
            this->actor.velocity.y = -Math_SinS(this->shapePitchOffset) * zoraJumpSpeed;
            this->zoraSwimRoll = zoraJumpAngle;
            Player_PlaySfx(this, NA_SE_EV_JUMP_OUT_WATER);
            return true;
        }

        if (Player_TrySpawnSplash(play, this, this->actor.velocity.y, 500)) {
            Player_PlaySfx(this, NA_SE_PL_FACE_UP);
        }

        if (input != NULL) {
            Player_SetupAction(play, this, Player_Action_SurfaceFromDive, 1);
            if (this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) {
                this->stateFlags1 |=
                    (PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE);
            }
            this->av2.actionVar2 = 2;
        }

        Player_AnimChangeOnceMorph(play, this,
                                   (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)
                                       ? &gPlayerAnim_link_swimer_swim_get
                                       : &gPlayerAnim_link_swimer_swim_deep_end);
        return true;
    }

    return false;
}

#if 0
s32 Player_TryDive(PlayState* play, Player* this, Input* input) {
    if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
        if ((input == NULL) || (CHECK_BTN_ALL(input->press.button, BTN_A) && (ABS(this->shapePitchOffset) < 12000) &&
                                (this->currentBoots != PLAYER_BOOTS_IRON) && (this->currentBoots !=
                                PLAYER_BOOTS_ZORA))) {

            Player_SetupAction(play, this, Player_Action_Dive, 0);
            Player_AnimPlayOnce(play, this, &gPlayerAnim_link_swimer_swim_deep_start);

            this->shapePitchOffset = 0;
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
            this->actor.velocity.y = 0.0f;

            if (input != NULL) {
                this->stateFlags2 |= PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER;
                Player_PlaySfx(this, NA_SE_PL_DIVE_BUBBLE);
            }

            return 1;
        }
    }

    if ((this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
        if (this->actor.velocity.y > 0.0f) {
            if (this->actor.yDistToWater < this->ageProperties->unk_30) {

                this->stateFlags2 &= ~PLAYER_STATE2_DIVING;

                if (input != NULL) {
                    Player_SetupAction(play, this, Player_Action_SurfaceFromDive, 1);

                    if (this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) {
                        this->stateFlags1 |=
                            PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    }

                    this->av2.actionVar2 = 2;
                }

                Player_ResetSubCam(play, this);
                Player_AnimChangeOnceMorph(play, this,
                                           (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)
                                               ? &gPlayerAnim_link_swimer_swim_get
                                               : &gPlayerAnim_link_swimer_swim_deep_end);

                if (Player_TrySpawnSplash(play, this, this->actor.velocity.y, 500)) {
                    Player_PlaySfx(this, NA_SE_PL_FACE_UP);
                }

                return 1;
            }
        }
    }

    return 0;
}
#endif

// Modified Player_RiseFromDive (func_8083B798)
void Player_RiseFromDive(PlayState* play, Player* this) {
    if (this->currentBoots == PLAYER_BOOTS_ZORA) {
        Player_SetupAction(play, this, Player_Action_Swim, 0);
        LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_swimer_swim, 1.0f,
                             Animation_GetLastFrame(&gPlayerAnim_link_swimer_swim), 0.0f, ANIMMODE_LOOP, 0.0f);
        this->zoraSwimYTarget = 2.0f;
    } else {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_swimer_swim);
        this->av2.actionVar2 = 1;
    }

    this->shapePitchOffset = 16000;
}

#if 0
void Player_RiseFromDive(PlayState* play, Player* this) {
    Player_AnimPlayLoop(play, this, &gPlayerAnim_link_swimer_swim);
    this->shapePitchOffset = 16000;
    this->av2.actionVar2 = 1;
}
#endif

// func_8083B8D0
void Player_TryEnterWaterEffects(PlayState* play, Player* this) {
    if (Player_TrySpawnSplash(play, this, this->actor.velocity.y, 500)) {
        Player_PlaySfx(this, NA_SE_EV_DIVE_INTO_WATER);
        if (this->fallDistance > 800) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }
    }
}

// Modified Player_EnterWater (func_8083B930)
void Player_EnterWater(PlayState* play, Player* this) {
    if (!(this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_StopCarryingActor(play, this);

        if (Player_Action_ZoraSwimJump == this->actionFunc) {
            Player_SetupZoraSwim(play, this);
            this->stateFlags3 |= PLAYER_STATE3_ZORA_SWIMMING;
        } else if ((this->currentBoots == PLAYER_BOOTS_ZORA) && (Player_Action_FallingDive == this->actionFunc)) {
            Player_SetupZoraSwim(play, this);
            this->stateFlags3 |= PLAYER_STATE3_ZORA_SWIMMING;
            Player_AnimPlayLoopAdjusted(play, this, &gLinkAdultSkelPz_fishswimAnim);
        } else if (!(this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) && (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
            Player_TryDive(play, this, NULL);
            this->av1.actionVar1 = 1;
        } else if (Player_Action_FallingDive == this->actionFunc) {
            Player_SetupAction(play, this, Player_Action_Dive, 0);
            Player_RiseFromDive(play, this);
        } else {
            Player_SetupAction(play, this, Player_Action_SwimIdle, 1);
            Player_AnimChangeOnceMorph(play, this,
                                       (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
                                           ? &gPlayerAnim_link_swimer_wait2swim_wait
                                           : &gPlayerAnim_link_swimer_land2swim_wait);
        }
    }
    if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->actor.yDistToWater < this->ageProperties->unk_2C)) {
        Player_TryEnterWaterEffects(play, this);
    }

    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);
    this->rippleTimer = 0.0f;

    Player_SetBootData(play, this);
}

#if 0
void Player_EnterWater(PlayState* play, Player* this) {
    if ((this->currentBoots != PLAYER_BOOTS_IRON) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_StopCarryingActor(play, this);

        if ((this->currentBoots != PLAYER_BOOTS_IRON) && (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
            Player_TryDive(play, this, NULL);
            this->av1.actionVar1 = 1;
        } else if (Player_Action_FallingDive == this->actionFunc) {
            Player_SetupAction(play, this, Player_Action_Dive, 0);
            Player_RiseFromDive(play, this);
        } else {
            Player_SetupAction(play, this, Player_Action_SwimIdle, 1);
            Player_AnimChangeOnceMorph(play, this,
                                       (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
                                           ? &gPlayerAnim_link_swimer_wait2swim_wait
                                           : &gPlayerAnim_link_swimer_land2swim_wait);
        }
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->actor.yDistToWater < this->ageProperties->unk_2C)) {
        if (Player_TrySpawnSplash(play, this, this->actor.velocity.y, 500)) {
            Player_PlaySfx(this, NA_SE_EV_DIVE_INTO_WATER);

            if (this->fallDistance > 800.0f) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
            }
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);
    this->rippleTimer = 0.0f;

    Player_SetBootData(play, this);
}
#endif

// Modified Player_CheckForWater (func_8083BB4C)
void Player_CheckForWater(PlayState* play, Player* this) {
    f32 waterSurface = this->actor.yDistToWater - this->ageProperties->unk_2C;

    if (waterSurface < 0.0f) {
        this->underwaterTimer = 0;
        Audio_SetBaseFilter(0);
    } else {
        Audio_SetBaseFilter(0x20);
        if ((this->currentBoots == PLAYER_BOOTS_ZORA) || (waterSurface < 10.0f)) {
            this->underwaterTimer = 0;
        } else if (this->underwaterTimer < 300) {
            this->underwaterTimer++;
        }
    }

    if ((this->actor.parent == NULL) && (Player_Action_JumpUpToLedge != this->actionFunc) &&
        (Player_Action_ClimbOntoLedge != this->actionFunc) &&
        ((Player_Action_ZoraSwimJump != this->actionFunc) || (this->actor.velocity.y < -2.0f))) {
        if (this->ageProperties->unk_2C < this->actor.yDistToWater) {
            if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
                (((this->currentBoots == PLAYER_BOOTS_ZORA) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) &&
                 (Player_Action_FirstPersonAiming != this->actionFunc) &&
                 (Player_Action_DamagedSwim != this->actionFunc) && (Player_Action_Drown != this->actionFunc) &&
                 (Player_Action_SwimIdle != this->actionFunc) && (Player_Action_Swim != this->actionFunc) &&
                 (Player_Action_ZTargetSwim != this->actionFunc) && (Player_Action_Dive != this->actionFunc) &&
                 (Player_Action_SurfaceFromDive != this->actionFunc) &&
                 (Player_Action_SpawnSwimming != this->actionFunc) && (Player_Action_ZoraSwim != this->actionFunc))) {
                Player_EnterWater(play, this);
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                   (this->actor.yDistToWater < this->ageProperties->unk_24) &&
                   (((Player_Action_ZoraSwim != this->actionFunc) &&
                     !(this->stateFlags3 & PLAYER_STATE3_ZORA_SWIMMING)) ||
                    (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
            if (this->skelAnime.moveFlags == 0) {
                Player_StartTurn(play, this, this->actor.shape.rot.y);
            }
            Player_StartJumpOutOfWater(play, this, this->actor.velocity.y);
        }
    }
}

#if 0
void Player_CheckForWater(PlayState* play, Player* this) {
    if (this->actor.yDistToWater < this->ageProperties->unk_2C) {
        Audio_SetBaseFilter(0);
        this->underwaterTimer = 0;
    } else {
        Audio_SetBaseFilter(0x20);
        if (this->underwaterTimer < 300) {
            this->underwaterTimer++;
        }
    }

    if ((Player_Action_JumpUpToLedge != this->actionFunc) && (Player_Action_ClimbOntoLedge != this->actionFunc)) {
        if (this->ageProperties->unk_2C < this->actor.yDistToWater) {
            if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
                (!((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) &&
                 (Player_Action_DamagedSwim != this->actionFunc) && (Player_Action_Drown != this->actionFunc) &&
                 (Player_Action_SwimIdle != this->actionFunc) && (Player_Action_Swim != this->actionFunc) &&
                 (Player_Action_ZTargetSwim != this->actionFunc) && (Player_Action_Dive != this->actionFunc) &&
                 (Player_Action_SurfaceFromDive != this->actionFunc) &&
                 (Player_Action_SpawnSwimming != this->actionFunc))) {
                Player_EnterWater(play, this);
                return;
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                   (this->actor.yDistToWater < this->ageProperties->unk_24)) {
            if ((this->skelAnime.moveFlags == 0) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
                Player_StartTurn(play, this, this->actor.shape.rot.y);
            }
            Player_StartJumpOutOfWater(play, this, this->actor.velocity.y);
        }
    }
}
#endif

void Player_ProcessEnvironmentPhysics(PlayState* play, Player* this) {
    Vec3f ripplePos;
    f32 floatUpSpeed;
    f32 maxSinkSpeed;
    f32 sinkSpeed;
    f32 moveMagnitude;

    this->actor.minVelocityY = -20.0f;
    this->actor.gravity = REG(68) / 100.0f;

    if (Player_IsFloorSinkingSand(sFloorType)) {
        floatUpSpeed = fabsf(this->speedXZ) * 20.0f;
        sinkSpeed = 0.0f;

        if (sFloorType == FLOOR_TYPE_SHALLOW_SAND) {
            if (this->shapeOffsetY > 1300.0f) {
                maxSinkSpeed = this->shapeOffsetY;
            } else {
                maxSinkSpeed = 1300.0f;
            }
            if (this->currentBoots == PLAYER_BOOTS_HOVER) {
                floatUpSpeed += floatUpSpeed;
            } else if (this->currentBoots == PLAYER_BOOTS_IRON) {
                floatUpSpeed *= 0.3f;
            }
        } else {
            maxSinkSpeed = 20000.0f;
            if (this->currentBoots != PLAYER_BOOTS_HOVER) {
                floatUpSpeed += floatUpSpeed;
            } else if ((sFloorType == FLOOR_TYPE_QUICKSAND_NO_HORSE) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                floatUpSpeed = 0;
            }
        }

        if (this->currentBoots != PLAYER_BOOTS_HOVER) {
            sinkSpeed = (maxSinkSpeed - this->shapeOffsetY) * 0.02f;
            sinkSpeed = CLAMP(sinkSpeed, 0.0f, 300.0f);
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                sinkSpeed += sinkSpeed;
            }
        }

        this->shapeOffsetY += sinkSpeed - floatUpSpeed;
        this->shapeOffsetY = CLAMP(this->shapeOffsetY, 0.0f, maxSinkSpeed);

        this->actor.gravity -= this->shapeOffsetY * 0.004f;
    } else {
        this->shapeOffsetY = 0.0f;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
        if (this->actor.yDistToWater < 50.0f) {
            moveMagnitude = fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].x - this->prevWaistPos.x) +
                            fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].y - this->prevWaistPos.y) +
                            fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].z - this->prevWaistPos.z);
            if (moveMagnitude > 4.0f) {
                moveMagnitude = 4.0f;
            }
            this->rippleTimer += moveMagnitude;

            if (this->rippleTimer > 15.0f) {
                this->rippleTimer = 0.0f;

                ripplePos.x = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.x;
                ripplePos.y = this->actor.world.pos.y + this->actor.yDistToWater;
                ripplePos.z = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.z;
                EffectSsGRipple_Spawn(play, &ripplePos, 100, 500, 0);

                if ((this->speedXZ > 4.0f) && !Player_IsSwimming(this) &&
                    ((this->actor.world.pos.y + this->actor.yDistToWater) <
                     this->bodyPartsPos[PLAYER_BODYPART_WAIST].y)) {
                    Player_TrySpawnSplash(play, this, 20.0f,
                                          (fabsf(this->speedXZ) * 50.0f) + (this->actor.yDistToWater * 5.0f));
                }
            }
        }

        if (this->actor.yDistToWater > 40.0f) {
            s32 numBubbles = 0;
            s32 i;

            if ((this->actor.velocity.y > -1.0f) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                if (Rand_ZeroOne() < 0.2f) {
                    numBubbles = 1;
                }
            } else {
                numBubbles = this->actor.velocity.y * -2.0f;
            }

            for (i = 0; i < numBubbles; i++) {
                EffectSsBubble_Spawn(play, &this->actor.world.pos, 20.0f, 10.0f, 20.0f, 0.13f);
            }
        }
    }
}

s32 Player_LookAtTargetActor(Player* this, s32 syncUpperRotToFocusRot) {
    Actor* targetActor = this->targetActor;
    Vec3f eyePos;
    s16 lookPitch;
    s16 lookYaw;

    eyePos.x = this->actor.world.pos.x;
    eyePos.y = this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 3.0f;
    eyePos.z = this->actor.world.pos.z;
    lookPitch = Math_Vec3f_Pitch(&eyePos, &targetActor->focus.pos);
    lookYaw = Math_Vec3f_Yaw(&eyePos, &targetActor->focus.pos);
    Math_SmoothStepToS(&this->actor.focus.rot.y, lookYaw, 4, 10000, 0);
    Math_SmoothStepToS(&this->actor.focus.rot.x, lookPitch, 4, 10000, 0);
    this->lookFlags |= 2;

    return Player_UpdateLookAngles(this, syncUpperRotToFocusRot);
}

static Vec3f lookFloorRaycastOffset = { 0.0f, 100.0f, 40.0f };

void Player_SetLookAngle(Player* this, PlayState* play) {
    s16 focusTargetPitch;
    s16 floorPitchAngle;
    f32 floorPosY;
    Vec3f raycastPos;

    if (this->targetActor != NULL) {
        if (Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Player_LookAtTargetActor(this, 0);
        }
        return;
    }

    if (sFloorType == FLOOR_TYPE_LOOK_UP) {
        Math_SmoothStepToS(&this->actor.focus.rot.x, -20000, 10, 4000, 800);
    } else {
        focusTargetPitch = 0;
        floorPosY = Player_RaycastFloor(play, this, &lookFloorRaycastOffset, &raycastPos);
        if (floorPosY > BGCHECK_Y_MIN) {
            floorPitchAngle = Math_Atan2S(40.0f, this->actor.world.pos.y - floorPosY);
            focusTargetPitch = CLAMP(floorPitchAngle, -4000, 4000);
        }
        this->actor.focus.rot.y = this->actor.shape.rot.y;
        Math_SmoothStepToS(&this->actor.focus.rot.x, focusTargetPitch, 14, 4000, 30);
    }

    Player_UpdateLookAngles(this, Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this));
}

void Player_SetRunLookAngles(Player* this, PlayState* play) {
    s16 targetPitch;
    s16 targetRoll;

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) && (this->speedXZ > 5.0f)) {
        targetPitch = this->speedXZ * 200.0f;
        targetRoll = (s16)(this->yaw - this->actor.shape.rot.y) * this->speedXZ * 0.1f;
        targetPitch = CLAMP(targetPitch, -4000, 4000);
        targetRoll = CLAMP(-targetRoll, -4000, 4000);
        Math_ScaledStepToS(&this->upperBodyRot.x, targetPitch, 900);
        this->headRot.x = -(f32)this->upperBodyRot.x * 0.5f;
        Math_ScaledStepToS(&this->headRot.z, targetRoll, 300);
        Math_ScaledStepToS(&this->upperBodyRot.z, targetRoll, 200);
        this->lookFlags |= 0x168;
    } else {
        Player_SetLookAngle(this, play);
    }
}

void Player_SetRunSpeedAndYaw(Player* this, f32 speedTarget, s16 yawTarget) {
    Math_AsymStepToF(&this->speedXZ, speedTarget, REG(19) / 100.0f, 1.5f);
    Math_ScaledStepToS(&this->yaw, yawTarget, REG(27));
}

void Player_AdjustMidairSpeedAndYaw(Player* this, f32* speedTarget, s16* yawTarget) {
    s16 yawDiff = this->yaw - *yawTarget;

    if (this->meleeWeaponState == 0) {
        this->speedXZ = CLAMP(this->speedXZ, -(R_RUN_SPEED_LIMIT / 100.0f), (R_RUN_SPEED_LIMIT / 100.0f));
    }

    if (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) {
        if (Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
            this->yaw = *yawTarget;
        }
    } else {
        Math_AsymStepToF(&this->speedXZ, *speedTarget, 0.05f, 0.1f);
        Math_ScaledStepToS(&this->yaw, *yawTarget, 200);
    }
}

static HorseMountAnimInfo sMountHorseAnims[] = {
    { &gPlayerAnim_link_uma_left_up, 35.17f, 6.6099997f },
    { &gPlayerAnim_link_uma_right_up, -34.16f, 7.91f },
};

s32 Player_ActionChange_TryMountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 riderOffsetX;
    f32 riderOffsetZ;
    f32 cosYaw;
    f32 sinYaw;
    s32 mountedLeftOfHorse;

    if ((rideActor != NULL) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        cosYaw = Math_CosS(rideActor->actor.shape.rot.y);
        sinYaw = Math_SinS(rideActor->actor.shape.rot.y);

        Player_SetupMiniCs(play, this, Player_SetupRidingHorse);

        this->stateFlags1 |= PLAYER_STATE1_RIDING_HORSE;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_WATER;

        if (this->mountSide < 0) {
            mountedLeftOfHorse = 0;
        } else {
            mountedLeftOfHorse = 1;
        }

        riderOffsetX = sMountHorseAnims[mountedLeftOfHorse].riderOffsetX;
        riderOffsetZ = sMountHorseAnims[mountedLeftOfHorse].riderOffsetZ;
        this->actor.world.pos.x =
            rideActor->actor.world.pos.x + rideActor->riderPos.x + ((riderOffsetX * cosYaw) + (riderOffsetZ * sinYaw));
        this->actor.world.pos.z =
            rideActor->actor.world.pos.z + rideActor->riderPos.z + ((riderOffsetZ * cosYaw) - (riderOffsetX * sinYaw));

        this->rideOffsetY = rideActor->actor.world.pos.y - this->actor.world.pos.y;
        this->yaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

        Actor_MountHorse(play, this, &rideActor->actor);
        Player_AnimPlayOnce(play, this, sMountHorseAnims[mountedLeftOfHorse].anim);
        Player_AnimReplaceApplyFlags(play, this,
                                     ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                         ANIM_FLAG_PLAYER_7);
        this->actor.parent = this->rideActor;
        Player_ClearAttentionModeAndStopMoving(this);
        Actor_DisableLens(play);
        return 1;
    }

    return 0;
}

void Player_GetSlopeDirection(CollisionPoly* floorPoly, Vec3f* slopeNormal, s16* downwardSlopeYaw) {
    slopeNormal->x = COLPOLY_GET_NORMAL(floorPoly->normal.x);
    slopeNormal->y = COLPOLY_GET_NORMAL(floorPoly->normal.y);
    slopeNormal->z = COLPOLY_GET_NORMAL(floorPoly->normal.z);

    *downwardSlopeYaw = Math_Atan2S(slopeNormal->z, slopeNormal->x);
}

s32 Player_HandleSlopes(PlayState* play, Player* this, CollisionPoly* floorPoly) {
    static LinkAnimationHeader* sSlopeSlipAnims[] = {
        &gPlayerAnim_link_normal_down_slope_slip,
        &gPlayerAnim_link_normal_up_slope_slip,
    };
    s32 pad;
    s16 playerVelYaw;
    Vec3f slopeNormal;
    s16 downwardSlopeYaw;
    f32 slopeSlowdownSpeed;
    f32 slopeSlowdownSpeedStep;
    s16 velYawToDownwardSlope;

    if (!Player_InBlockingCsMode(play, this) && (Player_Action_SlipOnSlope != this->actionFunc) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) == FLOOR_EFFECT_1)) {
        // Get direction of movement relative to the downward direction of the slope
        playerVelYaw = Math_Atan2S(this->actor.velocity.z, this->actor.velocity.x);
        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);
        velYawToDownwardSlope = downwardSlopeYaw - playerVelYaw;

        if (ABS(velYawToDownwardSlope) > 0x3E80) { // 87.9 degrees
            // moving parallel or upwards on the slope, player does not slip but does slow down
            slopeSlowdownSpeed = (1.0f - slopeNormal.y) * 40.0f;
            slopeSlowdownSpeedStep = SQ(slopeSlowdownSpeed) * 0.015f;

            if (slopeSlowdownSpeedStep < 1.2f) {
                slopeSlowdownSpeedStep = 1.2f;
            }

            // slows down speed as player is climbing a slope
            this->pushedYaw = downwardSlopeYaw;
            Math_StepToF(&this->pushedSpeed, slopeSlowdownSpeed, slopeSlowdownSpeedStep);
        } else {
            // moving downward on the slope, causing player to slip
            Player_SetupAction(play, this, Player_Action_SlipOnSlope, 0);
            Player_StopCarryingActor(play, this);

            if (sFloorShapePitch >= 0) {
                this->av1.actionVar1 = 1;
            }
            Player_AnimChangeLoopMorph(play, this, sSlopeSlipAnims[this->av1.actionVar1]);
            this->speedXZ = sqrtf(SQ(this->actor.velocity.x) + SQ(this->actor.velocity.z));
            this->yaw = playerVelYaw;
            return true;
        }
    }

    return false;
}

// unknown data (unused)
static s32 D_80854598[] = {
    0xFFDB0871, 0xF8310000, 0x00940470, 0xF3980000, 0xFFB504A9, 0x0C9F0000, 0x08010402,
};

void Player_PickupItemDrop(PlayState* play, Player* this, GetItemEntry* giEntry) {
    s32 dropType = giEntry->field & 0x1F;

    if (!(giEntry->field & 0x80)) {
        Item_DropCollectible(play, &this->actor.world.pos, dropType | 0x8000);
        if ((dropType != ITEM00_BOMBS_A) && (dropType != ITEM00_ARROWS_SMALL) && (dropType != ITEM00_ARROWS_MEDIUM) &&
            (dropType != ITEM00_ARROWS_LARGE) && (dropType != ITEM00_RUPEE_GREEN) && (dropType != ITEM00_RUPEE_BLUE) &&
            (dropType != ITEM00_RUPEE_RED) && (dropType != ITEM00_RUPEE_PURPLE) && (dropType != ITEM00_RUPEE_ORANGE)) {
            Item_Give(play, giEntry->itemId);
        }
    } else {
        Item_Give(play, giEntry->itemId);
    }

    Sfx_PlaySfxCentered((this->getItemId < 0) ? NA_SE_SY_GET_BOXITEM : NA_SE_SY_GET_ITEM);
}

s32 Player_ActionChange_TryGetItemOrCarry(Player* this, PlayState* play) {
    Actor* interactedActor;

    if (iREG(67) ||
        (((interactedActor = this->interactRangeActor) != NULL) && TitleCard_Clear(play, &play->actorCtx.titleCtx))) {
        if (iREG(67) || (this->getItemId > GI_NONE)) {
            if (iREG(67)) {
                this->getItemId = iREG(68);
            }

            if (this->getItemId < GI_MAX) {
                GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];

                if ((interactedActor != &this->actor) && !iREG(67)) {
                    interactedActor->parent = &this->actor;
                }

                iREG(67) = false;

                if ((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) ||
                    (play->sceneId == SCENE_BOMBCHU_BOWLING_ALLEY)) {
                    Player_DetachHeldActor(play, this);
                    Player_LoadGetItemObject(this, giEntry->objectId);

                    if (!(this->stateFlags2 & PLAYER_STATE2_DIVING) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                        Player_SetupMiniCs(play, this, Player_SetupGetItem);
                        Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_demo_get_itemB);
                        Player_SetUseItemCam(play, 9);
                    }

                    this->stateFlags1 |=
                        PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    Player_ClearAttentionModeAndStopMoving(this);
                    return 1;
                }

                Player_PickupItemDrop(play, this, giEntry);
                this->getItemId = GI_NONE;
            }
        } else if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
                   !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            if (this->getItemId != GI_NONE) {
                GetItemEntry* giEntry = &sGetItemTable[-this->getItemId - 1];
                EnBox* chest = (EnBox*)interactedActor;

                if (giEntry->itemId != ITEM_NONE) {
                    if (((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) && (giEntry->field & 0x40)) ||
                        ((Item_CheckObtainability(giEntry->itemId) != ITEM_NONE) && (giEntry->field & 0x20))) {
                        this->getItemId = -GI_RUPEE_BLUE;
                        giEntry = &sGetItemTable[GI_RUPEE_BLUE - 1];
                    }
                }

                Player_SetupMiniCs(play, this, Player_SetupGetItem);
                this->stateFlags1 |=
                    PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                Player_LoadGetItemObject(this, giEntry->objectId);
                this->actor.world.pos.x =
                    chest->dyna.actor.world.pos.x - (Math_SinS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->actor.world.pos.z =
                    chest->dyna.actor.world.pos.z - (Math_CosS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->yaw = this->actor.shape.rot.y = chest->dyna.actor.shape.rot.y;
                Player_ClearAttentionModeAndStopMoving(this);

                if ((giEntry->itemId != ITEM_NONE) && (giEntry->gi >= 0) &&
                    (Item_CheckObtainability(giEntry->itemId) == ITEM_NONE)) {
                    Player_AnimPlayOnceAdjusted(play, this, this->ageProperties->unk_98);
                    Player_AnimReplaceApplyFlags(play, this,
                                                 ANIM_REPLACE_APPLY_FLAG_9 | ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y |
                                                     ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE |
                                                     ANIM_FLAG_PLAYER_7);
                    chest->unk_1F4 = 1;
                    Camera_RequestSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_SLOW_CHEST_CS);
                } else {
                    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_box_kick);
                    chest->unk_1F4 = -1;
                }

                return 1;
            }

            if ((this->heldActor == NULL) || Player_HoldsHookshot(this)) {
                if ((interactedActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                    s32 itemAction = this->itemAction;

                    this->itemAction = PLAYER_IA_NONE;
                    this->modelAnimType = PLAYER_ANIMTYPE_0;
                    this->heldItemAction = this->itemAction;
                    Player_SetupMiniCs(play, this, Player_ProcessCarryActor);

                    if (itemAction == PLAYER_IA_SWORD_MASTER) {
                        this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_IA_SWORD_CS);
                        Player_InitItemAction(play, this, PLAYER_IA_SWORD_CS);
                    } else {
                        Player_UseItem(play, this, ITEM_SWORD_CS);
                    }
                } else {
                    s32 strength = Player_GetStrength();

                    if ((interactedActor->id == ACTOR_EN_ISHI) && ((interactedActor->params & 0xF) == 1) &&
                        (strength < PLAYER_STR_SILVER_G)) {
                        return 0;
                    }

                    Player_SetupMiniCs(play, this, Player_ProcessCarryActor);
                }

                Player_ClearAttentionModeAndStopMoving(this);
                this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
                return 1;
            }
        }
    }

    return 0;
}

void Player_SetupStartThrowActor(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_StartThrowActor, 1);
    Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_throw, this->modelAnimType));
}

s32 Player_CanThrowActor(Player* this, Actor* actor) {
    if ((actor != NULL) && !(actor->flags & ACTOR_FLAG_23) &&
        ((this->speedXZ < 1.1f) || (actor->id == ACTOR_EN_BOM_CHU))) {
        return 0;
    }

    return 1;
}

s32 Player_ActionChange_9(Player* this, PlayState* play) {
    if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor != NULL) &&
        CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
        if (!Player_TryStopCarryingActor(play, this, this->heldActor)) {
            if (!Player_CanThrowActor(this, this->heldActor)) {
                Player_SetupAction(play, this, Player_Action_PutDownActor, 1);
                Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_put, this->modelAnimType));
            } else {
                Player_SetupStartThrowActor(this, play);
            }
        }
        return 1;
    }

    return 0;
}

s32 Player_TryClimbWallOrLadder(Player* this, PlayState* play, u32 wallFlags) {
    if (this->yDistToLedge >= 79.0f) {
        if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) ||
            (this->actor.yDistToWater < this->ageProperties->unk_2C)) {
            s32 isClimbableWall = (wallFlags & WALL_FLAG_3) ? 2 : 0;

            if ((isClimbableWall != 0) || (wallFlags & WALL_FLAG_1) ||
                SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) {
                f32 yOffset;
                CollisionPoly* wallPoly = this->actor.wallPoly;
                f32 xOffset;
                f32 zOffset;
                f32 xOffset2;
                f32 zOffset2;

                yOffset = xOffset2 = 0.0f;

                if (isClimbableWall != 0) {
                    xOffset = this->actor.world.pos.x;
                    zOffset = this->actor.world.pos.z;
                } else {
                    Vec3f wallVertices[3];
                    s32 i;
                    f32 yOffsetDiff;
                    Vec3f* testVtx = &wallVertices[0];
                    s32 pad;

                    CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, wallVertices);

                    xOffset = xOffset2 = testVtx->x;
                    zOffset = zOffset2 = testVtx->z;
                    yOffset = testVtx->y;
                    for (i = 1; i < 3; i++) {
                        testVtx++;
                        if (xOffset > testVtx->x) {
                            xOffset = testVtx->x;
                        } else if (xOffset2 < testVtx->x) {
                            xOffset2 = testVtx->x;
                        }

                        if (zOffset > testVtx->z) {
                            zOffset = testVtx->z;
                        } else if (zOffset2 < testVtx->z) {
                            zOffset2 = testVtx->z;
                        }

                        if (yOffset > testVtx->y) {
                            yOffset = testVtx->y;
                        }
                    }

                    xOffset = (xOffset + xOffset2) * 0.5f;
                    zOffset = (zOffset + zOffset2) * 0.5f;

                    xOffset2 = ((this->actor.world.pos.x - xOffset) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                               ((this->actor.world.pos.z - zOffset) * COLPOLY_GET_NORMAL(wallPoly->normal.x));
                    yOffsetDiff = this->actor.world.pos.y - yOffset;

                    yOffset = ((f32)(s32)((yOffsetDiff / 15.000000223517418) + 0.5) * 15.000000223517418) - yOffsetDiff;
                    xOffset2 = fabsf(xOffset2);
                }

                if (xOffset2 < 8.0f) {
                    f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                    f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                    f32 dist = this->distToInteractWall;
                    LinkAnimationHeader* anim;

                    Player_SetupMiniCs(play, this, Player_SetupClimbingWallOrDownLedge);
                    this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;

                    if ((isClimbableWall != 0) || (wallFlags & WALL_FLAG_1)) {
                        if ((this->av1.actionVar1 = isClimbableWall) != 0) {
                            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                                anim = &gPlayerAnim_link_normal_Fclimb_startA;
                            } else {
                                anim = &gPlayerAnim_link_normal_Fclimb_hold2upL;
                            }
                            dist = (this->ageProperties->wallCheckRadius - 1.0f) - dist;
                        } else {
                            anim = this->ageProperties->unk_A4;
                            dist = dist - 1.0f;
                        }
                        this->av2.actionVar2 = -2;
                        this->actor.world.pos.y += yOffset;
                        this->actor.shape.rot.y = this->yaw = this->actor.wallYaw + 0x8000;
                    } else {
                        anim = this->ageProperties->unk_A8;
                        this->av2.actionVar2 = -4;
                        this->actor.shape.rot.y = this->yaw = this->actor.wallYaw;
                    }

                    this->actor.world.pos.x = (dist * wallPolyNormalX) + xOffset;
                    this->actor.world.pos.z = (dist * wallPolyNormalZ) + zOffset;
                    Player_ClearAttentionModeAndStopMoving(this);
                    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
                    Player_AnimPlayOnce(play, this, anim);
                    Player_AnimReplaceApplyFlags(play, this,
                                                 ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_2 |
                                                     ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);

                    return true;
                }
            }
        }
    }

    return false;
}

void Player_SetupEndClimb(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetupActionKeepMoveFlags(play, this, Player_Action_EndClimb, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, (4.0f / 3.0f));
}

/**
 * @return true if Player chooses to enter crawlspace
 */
s32 Player_TryEnteringCrawlspace(Player* this, PlayState* play, u32 interactWallFlags) {
    CollisionPoly* wallPoly;
    Vec3f wallVertices[3];
    f32 xVertex1;
    f32 xVertex2;
    f32 zVertex1;
    f32 zVertex2;
    s32 i;

    if (!LINK_IS_ADULT && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) && (interactWallFlags & WALL_FLAG_CRAWLSPACE)) {
        wallPoly = this->actor.wallPoly;
        CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, wallVertices);

        // Determines min and max vertices for x & z (edges of the crawlspace hole)
        xVertex1 = xVertex2 = wallVertices[0].x;
        zVertex1 = zVertex2 = wallVertices[0].z;
        for (i = 1; i < 3; i++) {
            if (xVertex1 > wallVertices[i].x) {
                // Update x min
                xVertex1 = wallVertices[i].x;
            } else if (xVertex2 < wallVertices[i].x) {
                // Update x max
                xVertex2 = wallVertices[i].x;
            }
            if (zVertex1 > wallVertices[i].z) {
                // Update z min
                zVertex1 = wallVertices[i].z;
            } else if (zVertex2 < wallVertices[i].z) {
                // Update z max
                zVertex2 = wallVertices[i].z;
            }
        }

        // XZ Center of the crawlspace hole
        xVertex1 = (xVertex1 + xVertex2) * 0.5f;
        zVertex1 = (zVertex1 + zVertex2) * 0.5f;

        // Perpendicular (sideways) XZ-Distance from player pos to crawlspace line
        // Uses y-component of crossproduct formula for the distance from a point to a line
        xVertex2 = ((this->actor.world.pos.x - xVertex1) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                   ((this->actor.world.pos.z - zVertex1) * COLPOLY_GET_NORMAL(wallPoly->normal.x));

        if (fabsf(xVertex2) < 8.0f) {
            // Give do-action prompt to "Enter on A" for the crawlspace
            this->stateFlags2 |= PLAYER_STATE2_DO_ACTION_ENTER;

            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                // Enter Crawlspace
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 distToInteractWall = this->distToInteractWall;

                Player_SetupMiniCs(play, this, Player_SetupCrawling);
                this->stateFlags2 |= PLAYER_STATE2_CRAWLING;
                this->actor.shape.rot.y = this->yaw = this->actor.wallYaw + 0x8000;
                this->actor.world.pos.x = xVertex1 + (distToInteractWall * wallPolyNormalX);
                this->actor.world.pos.z = zVertex1 + (distToInteractWall * wallPolyNormalZ);
                Player_ClearAttentionModeAndStopMoving(this);
                this->actor.prevPos = this->actor.world.pos;
                Player_AnimPlayOnce(play, this, &gPlayerAnim_link_child_tunnel_start);
                Player_AnimReplaceApplyFlags(play, this,
                                             ANIM_FLAG_0 | ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE |
                                                 ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);

                return true;
            }
        }
    }

    return false;
}

s32 Player_SetPositionAndYawOnClimbWall(PlayState* play, Player* this, f32 yOffset, f32 xzDistToWall, f32 xzCheckBScale,
                                        f32 xzCheckAScale) {
    CollisionPoly* wallPoly;
    s32 wallBgId;
    Vec3f checkPosA;
    Vec3f checkPosB;
    Vec3f posResult;
    f32 yawCos;
    f32 yawSin;
    s32 targetYaw;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;

    yawCos = Math_CosS(this->actor.shape.rot.y);
    yawSin = Math_SinS(this->actor.shape.rot.y);

    checkPosA.x = this->actor.world.pos.x + (xzCheckAScale * yawSin);
    checkPosA.z = this->actor.world.pos.z + (xzCheckAScale * yawCos);
    checkPosB.x = this->actor.world.pos.x + (xzCheckBScale * yawSin);
    checkPosB.z = this->actor.world.pos.z + (xzCheckBScale * yawCos);
    checkPosB.y = checkPosA.y = this->actor.world.pos.y + yOffset;

    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPosA, &checkPosB, &posResult, &this->actor.wallPoly, true, false,
                                false, true, &wallBgId)) {
        wallPoly = this->actor.wallPoly;

        this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
        this->actor.wallBgId = wallBgId;

        sTouchedWallFlags = SurfaceType_GetWallFlags(&play->colCtx, wallPoly, wallBgId);

        wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
        wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
        targetYaw = Math_Atan2S(-wallPolyNormalZ, -wallPolyNormalX);
        Math_ScaledStepToS(&this->actor.shape.rot.y, targetYaw, 800);

        this->yaw = this->actor.shape.rot.y;
        this->actor.world.pos.x = posResult.x - (Math_SinS(this->actor.shape.rot.y) * xzDistToWall);
        this->actor.world.pos.z = posResult.z - (Math_CosS(this->actor.shape.rot.y) * xzDistToWall);

        return 1;
    }

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;

    return 0;
}

s32 Player_PushPullSetPositionAndYaw(PlayState* play, Player* this) {
    return Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->wallCheckRadius + 5.0f, 30.0f,
                                               0.0f);
}

/**
 * Two exit walls are placed at each end of the crawlspace, separate to the two entrance walls used to enter the
 * crawlspace. These front and back exit walls are futher into the crawlspace than the front and
 * back entrance walls. When player interacts with either of these two interior exit walls, start the leaving-crawlspace
 * cutscene and return true. Else, return false
 */
s32 Player_TryLeavingCrawlspace(Player* this, PlayState* play) {
    s16 yawToWall;

    if ((this->speedXZ != 0.0f) && (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) &&
        (sTouchedWallFlags & WALL_FLAG_CRAWLSPACE)) {

        // The exit wallYaws will always point inward on the crawlline
        // Interacting with the exit wall in front will have a yaw diff of 0x8000
        // Interacting with the exit wall behind will have a yaw diff of 0
        yawToWall = this->actor.shape.rot.y - this->actor.wallYaw;
        if (this->speedXZ < 0.0f) {
            yawToWall += 0x8000;
        }

        if (ABS(yawToWall) > 0x4000) {
            Player_SetupAction(play, this, Player_Action_ExitCrawlspace, 0);

            if (this->speedXZ > 0.0f) {
                // Leaving a crawlspace forwards
                this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
                Player_AnimPlayOnce(play, this, &gPlayerAnim_link_child_tunnel_end);
                Player_AnimReplaceApplyFlags(play, this,
                                             ANIM_FLAG_0 | ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE |
                                                 ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
                OnePointCutscene_Init(play, 9601, 999, NULL, CAM_ID_MAIN);
            } else {
                // Leaving a crawlspace backwards
                this->actor.shape.rot.y = this->actor.wallYaw;
                LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_child_tunnel_start, -1.0f,
                                     Animation_GetLastFrame(&gPlayerAnim_link_child_tunnel_start), 0.0f, ANIMMODE_ONCE,
                                     0.0f);
                Player_AnimReplaceApplyFlags(play, this,
                                             ANIM_FLAG_0 | ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE |
                                                 ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
                OnePointCutscene_Init(play, 9602, 999, NULL, CAM_ID_MAIN);
            }

            this->yaw = this->actor.shape.rot.y;
            Player_ZeroSpeedXZ(this);

            return true;
        }
    }

    return false;
}

void Player_SetupGrabPushPullWallTryMiniCs(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    if (!Player_SetupMiniCs(play, this, Player_SetupGrabPushPullWall)) {
        Player_SetupAction(play, this, Player_Action_GrabPushPullWall, 0);
    }

    Player_AnimPlayOnce(play, this, anim);
    Player_ClearAttentionModeAndStopMoving(this);

    // Is this actually needed? It just seems to break things
    // this->actor.shape.rot.y = this->yaw = this->actor.wallYaw + 0x8000;
}

s32 Player_TrySpecialWallInteract(Player* this, PlayState* play) {
    DynaPolyActor* wallPolyActor;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sShapeYawToTouchedWall < 0x3000)) {

        if (((this->speedXZ > 0.0f) && Player_TryClimbWallOrLadder(this, play, sTouchedWallFlags)) ||
            Player_TryEnteringCrawlspace(this, play, sTouchedWallFlags)) {
            return 1;
        }

        if (!Player_IsSwimming(this) &&
            ((this->speedXZ == 0.0f) || !(this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL)) &&
            (sTouchedWallFlags & WALL_FLAG_6) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
            (this->yDistToLedge >= 39.0f)) {

            this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL;

            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {

                if ((this->actor.wallBgId != BGCHECK_SCENE) &&
                    ((wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId)) != NULL)) {

                    if (wallPolyActor->actor.id == ACTOR_BG_HEAVY_BLOCK) {
                        if (Player_GetStrength() < PLAYER_STR_GOLD_G) {
                            return 0;
                        }

                        Player_SetupMiniCs(play, this, Player_ProcessCarryActor);
                        this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
                        this->interactRangeActor = &wallPolyActor->actor;
                        this->getItemId = GI_NONE;
                        this->yaw = this->actor.wallYaw + 0x8000;
                        Player_ClearAttentionModeAndStopMoving(this);

                        return 1;
                    }

                    this->pushPullActor = &wallPolyActor->actor;
                } else {
                    this->pushPullActor = NULL;
                }

                Player_SetupGrabPushPullWallTryMiniCs(this, &gPlayerAnim_link_normal_push_wait, play);

                return 1;
            }
        }
    }

    return 0;
}

s32 Player_TryContinuePushPullWallInteract(PlayState* play, Player* this) {
    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) ||
         CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))) {
        DynaPolyActor* wallPolyActor = NULL;

        if (this->actor.wallBgId != BGCHECK_SCENE) {
            wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
        }

        if (&wallPolyActor->actor == this->pushPullActor) {
            if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    Player_ReturnToStandStill(this, play);
    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_push_wait_end);
    this->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    return 1;
}

void Player_SetupPushWall(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_PushWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_push_start);
}

void Player_SetupPullWall(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_PullWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pull_start, this->modelAnimType));
}

void Player_ClimbingLetGo(Player* this, PlayState* play) {
    this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING | PLAYER_STATE1_SWIMMING);
    Player_SetupFallFromLedge(this, play);
    this->speedXZ = -0.4f;
}

s32 Player_TryClimbingLetGo(Player* this, PlayState* play) {
    if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((sTouchedWallFlags & WALL_FLAG_3) || (sTouchedWallFlags & WALL_FLAG_1) ||
         SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId))) {
        return false;
    }

    Player_ClimbingLetGo(this, play);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
    return true;
}

// Bug? This will never return a non-zero value in final due to battle target movement speeds never going above 6.0f
s32 Player_GetBattleTargetMoveDirection(Player* this, f32 speedTarget, s16 yawTarget) {
    f32 yawTargetDiff = (s16)(yawTarget - this->actor.shape.rot.y);
    f32 normalizedYaw;

    if (this->targetActor != NULL) {
        Player_LookAtTargetActor(this, Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this));
    }

    normalizedYaw = fabsf(yawTargetDiff) / 32768.0f;

    if (speedTarget > (((normalizedYaw * normalizedYaw) * 50.0f) + 6.0f)) {
        return 1;
    } else if (speedTarget > (((1.0f - normalizedYaw) * 10.0f) + 6.8f)) {
        return -1;
    }

    return 0;
}

s32 Player_GetZParallelMoveDirection(Player* this, f32* speedTarget, s16* yawTarget, PlayState* play) {
    s16 yawTargetDiff = *yawTarget - this->zTargetYaw;
    u16 absYawTargetDiff = ABS(yawTargetDiff);

    if ((Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) &&
        (this->targetActor == NULL)) {
        *speedTarget *= Math_SinS(absYawTargetDiff);

        if (*speedTarget != 0.0f) {
            *yawTarget = (((yawTargetDiff >= 0) ? 1 : -1) << 0xE) + this->actor.shape.rot.y;
        } else {
            *yawTarget = this->actor.shape.rot.y;
        }

        if (this->targetActor != NULL) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Math_SmoothStepToS(&this->actor.focus.rot.x, sControlInput->rel.stick_y * 240.0f, 14, 4000, 30);
            Player_UpdateLookAngles(this, 1);
        }
    } else {
        if (this->targetActor != NULL) {
            return Player_GetBattleTargetMoveDirection(this, *speedTarget, *yawTarget);
        } else {
            Player_SetLookAngle(this, play);
            if ((*speedTarget != 0.0f) && (absYawTargetDiff < 6000)) {
                return 1;
            } else if (*speedTarget > Math_SinS((0x4000 - (absYawTargetDiff >> 1))) * 200.0f) {
                return -1;
            }
        }
    }

    return 0;
}

s32 Player_GetPushPullDirection(Player* this, f32* speedTarget, s16* yawTarget) {
    s16 yawTargetDiff = *yawTarget - this->actor.shape.rot.y;
    u16 absYawTargetDiff = ABS(yawTargetDiff);
    f32 cosYawDiff = Math_CosS(absYawTargetDiff);

    *speedTarget *= cosYawDiff;

    if (*speedTarget != 0.0f) {
        if (cosYawDiff > 0) {
            return 1;
        } else {
            return -1;
        }
    }

    return 0;
}

s32 Player_GetSpinAttackMoveDirection(Player* this, f32* speedTarget, s16* yawTarget, PlayState* play) {
    Player_SetLookAngle(this, play);

    if ((*speedTarget != 0.0f) || (ABS(this->unk_87C) > 400)) {
        s16 yawTargetDiff = *yawTarget - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        u16 clampedAbsYawDiff = (ABS(yawTargetDiff) - 0x2000) & 0xFFFF;

        if ((clampedAbsYawDiff < 0x4000) || (this->unk_87C != 0)) {
            return -1;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_SetLRBlendWeight(Player* this, f32 speedTarget, s16 yawTarget) {
    s16 yawDiff = yawTarget - this->actor.shape.rot.y;

    if (speedTarget > 0.0f) {
        if (yawDiff < 0) {
            this->leftRightBlendWeightTarget = 0.0f;
        } else {
            this->leftRightBlendWeightTarget = 1.0f;
        }
    }

    Math_StepToF(&this->leftRightBlendWeight, this->leftRightBlendWeightTarget, 0.3f);
}

void Player_PlayBlendedFightAnims(PlayState* play, Player* this) {
    LinkAnimation_BlendToJoint(play, &this->skelAnime, Player_GetFightingRightAnim(this), this->walkFrame,
                               Player_GetFightingLeftAnim(this), this->walkFrame, this->leftRightBlendWeight,
                               this->blendTable);
}

s32 Player_CheckWalkFrame(f32 curFrame, f32 walkSpeed, f32 animEndFrame, f32 targetFrame) {
    f32 frameDiff;

    if ((targetFrame == 0.0f) && (walkSpeed > 0.0f)) {
        targetFrame = animEndFrame;
    }

    frameDiff = (curFrame + walkSpeed) - targetFrame;

    if (((frameDiff * walkSpeed) >= 0.0f) && (((frameDiff - walkSpeed) * walkSpeed) < 0.0f)) {
        return 1;
    }

    return 0;
}

void Player_SetWalkFrameAndSfx(Player* this, f32 walkSpeed) {
    f32 updateScale = R_UPDATE_RATE * 0.5f;

    walkSpeed *= updateScale;
    if (walkSpeed < -7.25) {
        walkSpeed = -7.25;
    } else if (walkSpeed > 7.25f) {
        walkSpeed = 7.25f;
    }

    if (1) {}

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        (this->hoverBootsTimer != 0)) {
        func_8002F8F0(&this->actor, NA_SE_PL_HOBBERBOOTS_LV - SFX_FLAG);
    } else if (Player_CheckWalkFrame(this->walkFrame, walkSpeed, 29.0f, 10.0f) ||
               Player_CheckWalkFrame(this->walkFrame, walkSpeed, 29.0f, 24.0f)) {
        Player_PlayWalkSfx(this, this->speedXZ);
        if (this->speedXZ > 4.0f) {
            this->stateFlags2 |= PLAYER_STATE2_MAKING_REACTABLE_NOISE;
        }
    }

    this->walkFrame += walkSpeed;

    if (this->walkFrame < 0.0f) {
        this->walkFrame += 29.0f;
    } else if (this->walkFrame >= 29.0f) {
        this->walkFrame -= 29.0f;
    }
}

void Player_Action_BattleTargetStandStill(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    u32 walkFrame;
    s16 yawTargetDiff;
    s32 absYawTargetDiff;

    if (this->stateFlags3 & PLAYER_STATE3_ENDING_MELEE_ATTACK) {
        if (Player_GetMeleeWeaponHeld(this) != 0) {
            this->stateFlags2 |=
                PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
        } else {
            this->stateFlags3 &= ~PLAYER_STATE3_ENDING_MELEE_ATTACK;
        }
    }

    if (this->av2.actionVar2 != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_FinishAnimMovement(this);
            Player_AnimPlayLoop(play, this, Player_GetFightingRightAnim(this));
            this->av2.actionVar2 = 0;
            this->stateFlags3 &= ~PLAYER_STATE3_ENDING_MELEE_ATTACK;
        }
        Player_ResetLRBlendWeight(this);
    } else {
        Player_PlayBlendedFightAnims(play, this);
    }

    Player_StepSpeedXZToZero(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList1, true)) {
        if (!Player_TryBattleTargeting(this) &&
            (!Player_CheckCalmTargeting(this) || (Player_UpperAction_StandingDefense != this->upperActionFunc))) {
            Player_StartFinishBattleTarget(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        moveDir = Player_GetBattleTargetMoveDirection(this, speedTarget, yawTarget);

        if (moveDir > 0) {
            Player_SetupZParallelRun(this, play, yawTarget);
            return;
        }

        if (moveDir < 0) {
            Player_SetupBattleTargetBackwalk(this, yawTarget, play);
            return;
        }

        if (speedTarget > 4.0f) {
            Player_SetupBattleTargetSidewalk(this, play);
            return;
        }

        Player_SetWalkFrameAndSfx(this, (this->speedXZ * 0.3f) + 1.0f);
        Player_SetLRBlendWeight(this, speedTarget, yawTarget);

        walkFrame = this->walkFrame;
        if ((walkFrame < 6) || ((walkFrame - 0xE) < 6)) {
            Math_StepToF(&this->speedXZ, 0.0f, 1.5f);
            return;
        }

        yawTargetDiff = yawTarget - this->yaw;
        absYawTargetDiff = ABS(yawTargetDiff);

        if (absYawTargetDiff > 0x4000) {
            if (Math_StepToF(&this->speedXZ, 0.0f, 1.5f)) {
                this->yaw = yawTarget;
            }
            return;
        }

        Math_AsymStepToF(&this->speedXZ, speedTarget * 0.3f, 2.0f, 1.5f);

        if (!(this->stateFlags3 & PLAYER_STATE3_ENDING_MELEE_ATTACK)) {
            Math_ScaledStepToS(&this->yaw, yawTarget, absYawTargetDiff * 0.1f);
        }
    }
}

void Player_Action_CalmTargetStandStill(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 temp1;
    s16 temp2;
    s32 temp3;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_FinishAnimMovement(this);
        Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
    }

    Player_StepSpeedXZToZero(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList2, true)) {
        if (Player_TryBattleTargeting(this)) {
            Player_SetupBattleTargetStandStill(this, play);
            return;
        }

        if (!Player_CheckCalmTargeting(this)) {
            Player_SetupActionKeepMoveFlags(play, this, Player_Action_StandStill, 1);
            this->yaw = this->actor.shape.rot.y;
            return;
        }

        if (Player_UpperAction_StandingDefense == this->upperActionFunc) {
            Player_SetupBattleTargetStandStill(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        temp1 = Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play);

        if (temp1 > 0) {
            Player_SetupZParallelRun(this, play, yawTarget);
            return;
        }

        if (temp1 < 0) {
            Player_SetupZParallelBackwalk(this, yawTarget, play);
            return;
        }

        if (speedTarget > 4.9f) {
            Player_SetupBattleTargetSidewalk(this, play);
            Player_ResetLRBlendWeight(this);
            return;
        }
        if (speedTarget != 0.0f) {
            Player_SetupZParallelSidewalk(this, play);
            return;
        }

        temp2 = yawTarget - this->actor.shape.rot.y;
        temp3 = ABS(temp2);

        if (temp3 > 800) {
            Player_StartTurn(play, this, yawTarget);
        }
    }
}

void Player_SetIdleAnim(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    LinkAnimationHeader** animPtr;
    s32 healthIsCritical;
    s32 idleAnimType;
    s32 rand;

    if ((this->targetActor != NULL) ||
        (!(healthIsCritical = Health_IsCritical()) && ((this->idleCounter = (this->idleCounter + 1) & 1) != 0))) {
        this->stateFlags2 &= ~PLAYER_STATE2_IDLING;
        anim = Player_GetStandStillAnim(this);
    } else {
        this->stateFlags2 |= PLAYER_STATE2_IDLING;
        if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
            anim = Player_GetStandStillAnim(this);
        } else {
            idleAnimType = play->roomCtx.curRoom.behaviorType2;
            if (healthIsCritical) {
                if (this->idleCounter >= 0) {
                    idleAnimType = 7;
                    this->idleCounter = -1;
                } else {
                    idleAnimType = 8;
                }
            } else {
                rand = Rand_ZeroOne() * 5.0f;
                if (rand < 4) {
                    if (((rand != 0) && (rand != 3)) || ((this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) &&
                                                         ((rand == 3) || (Player_GetMeleeWeaponHeld(this) != 0)))) {
                        if ((rand == 0) && Player_HoldsTwoHandedWeapon(this)) {
                            rand = 4;
                        }
                        idleAnimType = rand + 9;
                    }
                }
            }
            animPtr = &sIdleAnims[idleAnimType][0];
            if (this->modelAnimType != PLAYER_ANIMTYPE_1) {
                animPtr = &sIdleAnims[idleAnimType][1];
            }
            anim = *animPtr;
        }
    }

    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f) * sWaterSpeedScale, 0.0f,
                         Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
}

void Player_Action_StandStill(Player* this, PlayState* play) {
    s32 idleAnimOffset;
    s32 animDone;
    f32 speedTarget;
    s16 yawTarget;
    s16 yawTargetDiff;

    idleAnimOffset = Player_IsPlayingIdleAnim(this);
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (idleAnimOffset > 0) {
        Player_PlayIdleAnimSfx(this, idleAnimOffset - 1);
    }

    if (animDone != 0) {
        if (this->av2.actionVar2 != 0) {
            if (DECR(this->av2.actionVar2) == 0) {
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
            this->skelAnime.jointTable[0].y =
                (this->skelAnime.jointTable[0].y + ((this->av2.actionVar2 & 1) * 0x50)) - 0x28;
        } else {
            Player_FinishAnimMovement(this);
            Player_SetIdleAnim(play, this);
        }
    }

    Player_StepSpeedXZToZero(this);

    if (this->av2.actionVar2 == 0) {
        if (!Player_TryActionChangeList(play, this, sActionChangeList7, true)) {
            if (Player_TryBattleTargeting(this)) {
                Player_SetupBattleTargetStandStill(this, play);
                return;
            }

            if (Player_CheckCalmTargeting(this)) {
                Player_SetupCalmTargetingStandStill(this, play);
                return;
            }

            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play);

            if (speedTarget != 0.0f) {
                Player_SetupZParallelRun(this, play, yawTarget);
                return;
            }

            yawTargetDiff = yawTarget - this->actor.shape.rot.y;
            if (ABS(yawTargetDiff) > 800) {
                Player_StartTurn(play, this, yawTarget);
                return;
            }

            Math_ScaledStepToS(&this->actor.shape.rot.y, yawTarget, 1200);
            this->yaw = this->actor.shape.rot.y;
            if (Player_GetStandStillAnim(this) == this->skelAnime.animation) {
                Player_SetLookAngle(this, play);
            }
        }
    }
}

void Player_Action_ZParallelSidewalk(Player* this, PlayState* play) {
    f32 frames;
    f32 coeff;
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    s16 yawTargetDiff;
    s32 absYawTargetDiff;
    s32 direction;

    this->skelAnime.mode = 0;
    LinkAnimation_SetUpdateFunction(&this->skelAnime);

    this->skelAnime.animation = Player_GetFinishSidewalkAnim(this);

    if (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk) {
        frames = 24.0f;
        coeff = -(MREG(95) / 100.0f);
    } else {
        frames = 29.0f;
        coeff = MREG(95) / 100.0f;
    }

    this->skelAnime.animLength = frames;
    this->skelAnime.endFrame = frames - 1.0f;

    if ((s16)(this->yaw - this->actor.shape.rot.y) >= 0) {
        direction = 1;
    } else {
        direction = -1;
    }

    this->skelAnime.playSpeed = direction * (this->speedXZ * coeff);

    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 0.0f) || LinkAnimation_OnFrame(&this->skelAnime, frames * 0.5f)) {
        Player_PlayWalkSfx(this, this->speedXZ);
    }

    if (!Player_TryActionChangeList(play, this, sActionChangeList3, true)) {
        if (Player_TryBattleTargeting(this)) {
            Player_SetupBattleTargetStandStill(this, play);
            return;
        }

        if (!Player_CheckCalmTargeting(this)) {
            Player_SetupStandStillMorph(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        moveDir = Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play);

        if (moveDir > 0) {
            Player_SetupZParallelRun(this, play, yawTarget);
            return;
        }

        if (moveDir < 0) {
            Player_SetupZParallelBackwalk(this, yawTarget, play);
            return;
        }

        if (speedTarget > 4.9f) {
            Player_SetupBattleTargetSidewalk(this, play);
            Player_ResetLRBlendWeight(this);
            return;
        }

        if ((speedTarget == 0.0f) && (this->speedXZ == 0.0f)) {
            Player_SetupCalmTargetingStandStill(this, play);
            return;
        }

        yawTargetDiff = yawTarget - this->yaw;
        absYawTargetDiff = ABS(yawTargetDiff);

        if (absYawTargetDiff > 0x4000) {
            if (Math_StepToF(&this->speedXZ, 0.0f, 1.5f)) {
                this->yaw = yawTarget;
            }
            return;
        }

        Math_AsymStepToF(&this->speedXZ, speedTarget * 0.4f, 1.5f, 1.5f);
        Math_ScaledStepToS(&this->yaw, yawTarget, absYawTargetDiff * 0.1f);
    }
}

void Player_HandleZParallelBackwalkAnim(Player* this, PlayState* play) {
    f32 morphWeight;
    f32 velocity;

    if (this->unk_864 < 1.0f) {
        morphWeight = R_UPDATE_RATE * 0.5f;
        Player_SetWalkFrameAndSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                  GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType), this->walkFrame);
        this->unk_864 += 1 * morphWeight;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }
        morphWeight = this->unk_864;
    } else {
        velocity = this->speedXZ - (REG(48) / 100.0f);
        if (velocity < 0.0f) {
            morphWeight = 1.0f;
            Player_SetWalkFrameAndSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->speedXZ));
            LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType),
                                      this->walkFrame);
        } else {
            morphWeight = (REG(37) / 1000.0f) * velocity;
            if (morphWeight < 1.0f) {
                Player_SetWalkFrameAndSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->speedXZ));
            } else {
                morphWeight = 1.0f;
                Player_SetWalkFrameAndSfx(this, 1.2f + ((REG(38) / 1000.0f) * velocity));
            }
            LinkAnimation_LoadToMorph(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType),
                                      this->walkFrame);
            LinkAnimation_LoadToJoint(play, &this->skelAnime, &gPlayerAnim_link_normal_back_run,
                                      this->walkFrame * (16.0f / 29.0f));
        }
    }

    if (morphWeight < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - morphWeight);
    }
}

void Player_SetupBrakeZParallelBackwalk(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_BreakZParallelBackwalk, 1);
    Player_AnimChangeOnceMorph(play, this, &gPlayerAnim_link_normal_back_brake);
}

s32 Player_TryFinishZParallelBackwalk(Player* this, f32* speedTarget, s16* yawTarget, PlayState* play) {
    if (this->speedXZ > 6.0f) {
        Player_SetupBrakeZParallelBackwalk(this, play);
        return 1;
    }

    if (*speedTarget != 0.0f) {
        if (Player_StepSpeedXZToZero(this)) {
            *speedTarget = 0.0f;
            *yawTarget = this->yaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_Action_ZParallelBackwalk(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    s16 yawTargetDiff;

    Player_HandleZParallelBackwalkAnim(this, play);

    if (!Player_TryActionChangeList(play, this, sActionChangeList4, true)) {
        if (!Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupZParallelRun(this, play, this->yaw);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        moveDir = Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play);

        if (moveDir >= 0) {
            if (!Player_TryFinishZParallelBackwalk(this, &speedTarget, &yawTarget, play)) {
                if (moveDir != 0) {
                    Player_SetupRun(this, play);
                } else if (speedTarget > 4.9f) {
                    Player_SetupBattleTargetSidewalk(this, play);
                } else {
                    Player_SetupZParallelSidewalk(this, play);
                }
            }
        } else {
            yawTargetDiff = yawTarget - this->yaw;

            Math_AsymStepToF(&this->speedXZ, speedTarget * 1.5f, 1.5f, 2.0f);
            Math_ScaledStepToS(&this->yaw, yawTarget, yawTargetDiff * 0.1f);

            if ((speedTarget == 0.0f) && (this->speedXZ == 0.0f)) {
                Player_SetupCalmTargetingStandStill(this, play);
            }
        }
    }
}

void Player_SetupFinishBrakeZParallelBackwalk(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_FinishBrakeZParallelBackwalk, 1);
    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_back_brake_end);
}

void Player_Action_BreakZParallelBackwalk(Player* this, PlayState* play) {
    s32 animdone;
    f32 speedTarget;
    s16 yawTarget;

    animdone = LinkAnimation_Update(play, &this->skelAnime);
    Player_StepSpeedXZToZero(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList4, true)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if (this->speedXZ == 0.0f) {
            this->yaw = this->actor.shape.rot.y;

            if (Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play) > 0) {
                Player_SetupRun(this, play);
            } else if ((speedTarget != 0.0f) || (animdone != 0)) {
                Player_SetupFinishBrakeZParallelBackwalk(this, play);
            }
        }
    }
}

void Player_Action_FinishBrakeZParallelBackwalk(Player* this, PlayState* play) {
    s32 animDone;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_TryActionChangeList(play, this, sActionChangeList4, true)) {
        if (animDone != 0) {
            Player_SetupCalmTargetingStandStill(this, play);
        }
    }
}

void Player_PlayBlendedSidewalkAnims(PlayState* play, Player* this) {
    f32 frame;
    LinkAnimationHeader* sp38 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkL, this->modelAnimType);
    LinkAnimationHeader* sp34 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType);

    this->skelAnime.animation = sp38;

    Player_SetWalkFrameAndSfx(this, (REG(30) / 1000.0f) + ((REG(32) / 1000.0f) * this->speedXZ));

    frame = this->walkFrame * (16.0f / 29.0f);
    LinkAnimation_BlendToJoint(play, &this->skelAnime, sp34, frame, sp38, frame, this->leftRightBlendWeight,
                               this->blendTable);
}

void Player_Action_BattleTargetSidewalk(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    s16 yawTargetDiff;
    s32 absYawTargetDiff;

    Player_PlayBlendedSidewalkAnims(play, this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList5, true)) {
        if (!Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if (Player_CheckCalmTargeting(this)) {
            moveDir = Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play);
        } else {
            moveDir = Player_GetBattleTargetMoveDirection(this, speedTarget, yawTarget);
        }

        if (moveDir > 0) {
            Player_SetupRun(this, play);
            return;
        }

        if (moveDir < 0) {
            if (Player_CheckCalmTargeting(this)) {
                Player_SetupZParallelBackwalk(this, yawTarget, play);
            } else {
                Player_SetupBattleTargetBackwalk(this, yawTarget, play);
            }
            return;
        }

        if ((this->speedXZ < 3.6f) && (speedTarget < 4.0f)) {
            if (!Player_CheckBattleTargeting(this) && Player_CheckCalmTargeting(this)) {
                Player_SetupZParallelSidewalk(this, play);
            } else {
                Player_SetupContextualStandStill(this, play);
            }
            return;
        }

        Player_SetLRBlendWeight(this, speedTarget, yawTarget);

        yawTargetDiff = yawTarget - this->yaw;
        absYawTargetDiff = ABS(yawTargetDiff);

        if (absYawTargetDiff > 0x4000) {
            if (Math_StepToF(&this->speedXZ, 0.0f, 3.0f) != 0) {
                this->yaw = yawTarget;
            }
            return;
        }

        speedTarget *= 0.9f;
        Math_AsymStepToF(&this->speedXZ, speedTarget, 2.0f, 3.0f);
        Math_ScaledStepToS(&this->yaw, yawTarget, absYawTargetDiff * 0.1f);
    }
}

void Player_Action_Turn(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    LinkAnimation_Update(play, &this->skelAnime);

    if (Player_HoldsTwoHandedWeapon(this)) {
        AnimationContext_SetLoadFrame(play, Player_GetStandStillAnim(this), 0, this->skelAnime.limbCount,
                                      this->skelAnime.morphTable);
        AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                     this->skelAnime.morphTable, sUpperBodyLimbCopyMap);
    }

    Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play);

    if (!Player_TryActionChangeList(play, this, sActionChangeList6, true)) {
        if (speedTarget != 0.0f) {
            this->actor.shape.rot.y = yawTarget;
            Player_SetupRun(this, play);
        } else if (Math_ScaledStepToS(&this->actor.shape.rot.y, yawTarget, this->unk_87E)) {
            Player_SetupStandStill(this, play);
        }

        this->yaw = this->actor.shape.rot.y;
    }
}

void Player_PlayBlendedWalkAnims(Player* this, s32 blendToMorph, PlayState* play) {
    LinkAnimationHeader* anim;
    s16 target;
    f32 rate;

    if (ABS(sFloorShapePitch) < DEG_TO_BINANG(20.0f)) {
        target = 0;
    } else {
        target = CLAMP(sFloorShapePitch, -DEG_TO_BINANG(60.0f), DEG_TO_BINANG(60.0f));
    }

    Math_ScaledStepToS(&this->walkFloorPitch, target, DEG_TO_BINANG(2.1973f));

    if ((this->modelAnimType == PLAYER_ANIMTYPE_3) || ((this->walkFloorPitch == 0) && (this->shapeOffsetY <= 0.0f))) {
        if (blendToMorph == 0) {
            LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType), this->walkFrame);
        } else {
            LinkAnimation_LoadToMorph(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType), this->walkFrame);
        }
        return;
    }

    if (this->walkFloorPitch != 0) {
        rate = this->walkFloorPitch / 10922.0f;
    } else {
        rate = this->shapeOffsetY * 0.0006f;
    }

    rate *= fabsf(this->speedXZ) * 0.5f;

    if (rate > 1.0f) {
        rate = 1.0f;
    }

    if (rate < 0.0f) {
        anim = &gPlayerAnim_link_normal_climb_down;
        rate = -rate;
    } else {
        anim = &gPlayerAnim_link_normal_climb_up;
    }

    if (blendToMorph == 0) {
        LinkAnimation_BlendToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, rate, this->blendTable);
    } else {
        LinkAnimation_BlendToMorph(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, rate, this->blendTable);
    }
}

void Player_SetupWalkAnims(Player* this, PlayState* play) {
    f32 temp1;
    f32 temp2;

    if (this->unk_864 < 1.0f) {
        temp1 = R_UPDATE_RATE * 0.5f;

        Player_SetWalkFrameAndSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                  this->walkFrame);

        this->unk_864 += 1 * temp1;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }

        temp1 = this->unk_864;
    } else {
        temp2 = this->speedXZ - (REG(48) / 100.0f);

        if (temp2 < 0.0f) {
            temp1 = 1.0f;
            Player_SetWalkFrameAndSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->speedXZ));

            Player_PlayBlendedWalkAnims(this, 0, play);
        } else {
            temp1 = (REG(37) / 1000.0f) * temp2;
            if (temp1 < 1.0f) {
                Player_SetWalkFrameAndSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->speedXZ));
            } else {
                temp1 = 1.0f;
                Player_SetWalkFrameAndSfx(this, 1.2f + ((REG(38) / 1000.0f) * temp2));
            }

            Player_PlayBlendedWalkAnims(this, 1, play);

            LinkAnimation_LoadToJoint(play, &this->skelAnime, Player_GetRunningAnim(this),
                                      this->walkFrame * (20.0f / 29.0f));
        }
    }

    if (temp1 < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - temp1);
    }
}

void Player_Action_Run(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);

    if (!Player_TryActionChangeList(play, this, sActionChangeList8, true)) {
        if (Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play);

        if (!Player_TryTurnAroundWhileRunning(this, &speedTarget, &yawTarget)) {
            Player_SetRunSpeedAndYaw(this, speedTarget, yawTarget);
            Player_SetRunLookAngles(this, play);

            if ((this->speedXZ == 0.0f) && (speedTarget == 0.0f)) {
                Player_StartFinishRun(this, play);
            }
        }
    }
}

void Player_Action_TargetRun(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);

    if (!Player_TryActionChangeList(play, this, sActionChangeList9, true)) {
        if (!Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if (!Player_TryTurnAroundWhileRunning(this, &speedTarget, &yawTarget)) {
            if ((Player_CheckCalmTargeting(this) && (speedTarget != 0.0f) &&
                 (Player_GetZParallelMoveDirection(this, &speedTarget, &yawTarget, play) <= 0)) ||
                (!Player_CheckCalmTargeting(this) &&
                 (Player_GetBattleTargetMoveDirection(this, speedTarget, yawTarget) <= 0))) {
                Player_SetupContextualStandStill(this, play);
                return;
            }

            Player_SetRunSpeedAndYaw(this, speedTarget, yawTarget);
            Player_SetRunLookAngles(this, play);

            if ((this->speedXZ == 0) && (speedTarget == 0)) {
                Player_SetupContextualStandStill(this, play);
            }
        }
    }
}

void Player_Action_BattleTargetBackwalk(Player* this, PlayState* play) {
    s32 animDone;
    f32 speedTarget;
    s16 yawTarget;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_TryActionChangeList(play, this, sActionChangeList5, true)) {
        if (!Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if ((this->skelAnime.morphWeight == 0.0f) && (this->skelAnime.curFrame > 5.0f)) {
            Player_StepSpeedXZToZero(this);

            if ((this->skelAnime.curFrame > 10.0f) &&
                (Player_GetBattleTargetMoveDirection(this, speedTarget, yawTarget) < 0)) {
                Player_SetupBattleTargetBackwalk(this, yawTarget, play);
                return;
            }

            if (animDone != 0) {
                Player_SetupFinishBattleTargetBackwalk(this, play);
            }
        }
    }
}

void Player_Action_FinishBattleTargetBackwalk(Player* this, PlayState* play) {
    s32 animDone;
    f32 speedTarget;
    s16 yawTarget;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    Player_StepSpeedXZToZero(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList10, true)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if (this->speedXZ == 0.0f) {
            this->yaw = this->actor.shape.rot.y;

            if (Player_GetBattleTargetMoveDirection(this, speedTarget, yawTarget) > 0) {
                Player_SetupRun(this, play);
                return;
            }

            if ((speedTarget != 0.0f) || (animDone != 0)) {
                Player_SetupContextualStandStill(this, play);
            }
        }
    }
}

void Player_GetDustPos(Vec3f* src, Vec3f* dest, f32 floorOffset, f32 scaleXZ, f32 scaleY) {
    dest->x = (Rand_ZeroOne() * scaleXZ) + src->x;
    dest->y = (Rand_ZeroOne() * scaleY) + (src->y + floorOffset);
    dest->z = (Rand_ZeroOne() * scaleXZ) + src->z;
}

static Vec3f sDustVel = { 0.0f, 0.0f, 0.0f };
static Vec3f sDustAccel = { 0.0f, 0.0f, 0.0f };

s32 Player_TrySpawnDustAtFeet(PlayState* play, Player* this) {
    Vec3f dustPos;

    if ((this->floorSfxOffset == SURFACE_SFX_OFFSET_DIRT) || (this->floorSfxOffset == SURFACE_SFX_OFFSET_SAND)) {
        Player_GetDustPos(&this->actor.shape.feetPos[FOOT_LEFT], &dustPos,
                          this->actor.floorHeight - this->actor.shape.feetPos[FOOT_LEFT].y, 7.0f, 5.0f);
        func_800286CC(play, &dustPos, &sDustVel, &sDustAccel, 50, 30);
        Player_GetDustPos(&this->actor.shape.feetPos[FOOT_RIGHT], &dustPos,
                          this->actor.floorHeight - this->actor.shape.feetPos[FOOT_RIGHT].y, 7.0f, 5.0f);
        func_800286CC(play, &this->actor.shape.feetPos[FOOT_RIGHT], &sDustVel, &sDustAccel, 50, 30);
        return 1;
    }

    return 0;
}

void Player_Action_PlantMagicBean(Player* this, PlayState* play) {
    Player_LoopAnimContinuously(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check_wait, this->modelAnimType));

    if (DECR(this->av2.actionVar2) == 0) {
        if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
            Player_StartReturnToStandStillWithAnim(
                this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check_end, this->modelAnimType), play);
        }

        this->actor.flags &= ~ACTOR_FLAG_TALK;
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

s32 Player_TryMeleeAttack(Player* this, f32 arg1, f32 arg2, f32 arg3) {
    if ((arg1 <= this->skelAnime.curFrame) && (this->skelAnime.curFrame <= arg3)) {
        Player_MeleeAttack(this, (arg2 <= this->skelAnime.curFrame) ? 1 : -1);
        return 1;
    }

    Player_InactivateMeleeWeapon(this);
    return 0;
}

s32 Player_TryAttackWhileDefending(Player* this, PlayState* play) {
    if (!Player_IsChildWithHylianShield(this) && (Player_GetMeleeWeaponHeld(this) != 0) && sUseHeldItem) {
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_defense_kiru);
        this->av1.actionVar1 = 1;
        this->meleeAttackType = PLAYER_MELEEATKTYPE_STAB_1H;
        this->yaw = this->actor.shape.rot.y + this->upperBodyRot.y;
        return 1;
    }

    return 0;
}

int Player_IsBusyInteracting(Player* this, PlayState* play) {
    return Player_ActionChange_TryItemCsOrFirstPerson(this, play) || Player_ActionChange_TrySpeakOrCheck(this, play) ||
           Player_ActionChange_TryGetItemOrCarry(this, play);
}

void Player_RequestQuake(PlayState* play, s32 speed, s32 y, s32 duration) {
    s32 quakeIndex = Quake_Request(Play_GetCamera(play, CAM_ID_MAIN), QUAKE_TYPE_3);

    Quake_SetSpeed(quakeIndex, speed);
    Quake_SetPerturbations(quakeIndex, y, 0, 0, 0);
    Quake_SetDuration(quakeIndex, duration);
}

void Player_HammerHit(PlayState* play, Player* this) {
    Player_RequestQuake(play, 27767, 7, 20);
    play->actorCtx.unk_02 = 4;
    Player_RequestRumble(this, 255, 20, 150, 0);
    Player_PlaySfx(this, NA_SE_IT_HAMMER_HIT);
}

void Player_BreakDekuStick(PlayState* play, Player* this) {
    Inventory_ChangeAmmo(ITEM_DEKU_STICK, -1);
    Player_UseItem(play, this, ITEM_NONE);
}

s32 Player_TryBreakDekuStick(PlayState* play, Player* this) {
    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) && (this->unk_85C > 0.5f)) {
        if (AMMO(ITEM_DEKU_STICK) != 0) {
            EffectSsStick_Spawn(play, &this->bodyPartsPos[PLAYER_BODYPART_R_HAND], this->actor.shape.rot.y + 0x8000);
            this->unk_85C = 0.5f;
            Player_BreakDekuStick(play, this);
            Player_PlaySfx(this, NA_SE_IT_WOODSTICK_BROKEN);
        }

        return 1;
    }

    return 0;
}

s32 Player_TryBreakBiggoronSword(PlayState* play, Player* this) {
    if (this->heldItemAction == PLAYER_IA_SWORD_BIGGORON) {
        if (!gSaveContext.save.info.playerData.bgsFlag && (gSaveContext.save.info.playerData.swordHealth > 0.0f)) {
            if ((gSaveContext.save.info.playerData.swordHealth -= 1.0f) <= 0.0f) {
                EffectSsStick_Spawn(play, &this->bodyPartsPos[PLAYER_BODYPART_R_HAND],
                                    this->actor.shape.rot.y + 0x8000);
                func_800849EC(play);
                Player_PlaySfx(this, NA_SE_IT_MAJIN_SWORD_BROKEN);
            }
        }

        return 1;
    }

    return 0;
}

void Player_TryBreakWeapon(PlayState* play, Player* this) {
    Player_TryBreakDekuStick(play, this);
    Player_TryBreakBiggoronSword(play, this);
}

static LinkAnimationHeader* sWeaponReboundAnims[] = {
    &gPlayerAnim_link_fighter_rebound,
    &gPlayerAnim_link_fighter_rebound_long,
    &gPlayerAnim_link_fighter_reboundR,
    &gPlayerAnim_link_fighter_rebound_longR,
};

void Player_SetupMeleeWeaponRebound(PlayState* play, Player* this) {
    s32 pad;
    s32 battleOffset;

    if (Player_Action_AimShieldCrouched != this->actionFunc) {
        Player_ResetAttributes(play, this);
        Player_SetupAction(play, this, Player_Action_MeleeWeaponRebound, 0);

        if (Player_CheckBattleTargeting(this)) {
            battleOffset = 2;
        } else {
            battleOffset = 0;
        }

        Player_AnimPlayOnceAdjusted(play, this, sWeaponReboundAnims[Player_HoldsTwoHandedWeapon(this) + battleOffset]);
    }

    Player_RequestRumble(this, 180, 20, 100, 0);
    this->speedXZ = -18.0f;
    Player_TryBreakWeapon(play, this);
}

s32 Player_TryWeaponHit(PlayState* play, Player* this) {
    f32 dist;
    CollisionPoly* groundPoly;
    s32 bgId;
    Vec3f checkPos;
    Vec3f weaponHitPos;
    Vec3f tipBaseDiff;
    s32 weaponHitAT;
    s32 surfaceMaterial;

    if (this->meleeWeaponState > 0) {
        if (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
            if (!(this->meleeWeaponQuads[0].base.atFlags & AT_BOUNCED) &&
                !(this->meleeWeaponQuads[1].base.atFlags & AT_BOUNCED)) {
                if (this->skelAnime.curFrame >= 2.0f) {

                    dist = Math_Vec3f_DistXYZAndStoreDiff(&this->meleeWeaponInfo[0].tip, &this->meleeWeaponInfo[0].base,
                                                          &tipBaseDiff);
                    if (dist != 0.0f) {
                        dist = (dist + 10.0f) / dist;
                    }

                    checkPos.x = this->meleeWeaponInfo[0].tip.x + (tipBaseDiff.x * dist);
                    checkPos.y = this->meleeWeaponInfo[0].tip.y + (tipBaseDiff.y * dist);
                    checkPos.z = this->meleeWeaponInfo[0].tip.z + (tipBaseDiff.z * dist);

                    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPos, &this->meleeWeaponInfo[0].tip, &weaponHitPos,
                                                &groundPoly, true, false, false, true, &bgId) &&
                        !SurfaceType_IsIgnoredByEntities(&play->colCtx, groundPoly, bgId) &&
                        (SurfaceType_GetFloorType(&play->colCtx, groundPoly, bgId) != FLOOR_TYPE_NO_FALL_DAMAGE) &&
                        (func_8002F9EC(play, &this->actor, groundPoly, bgId, &weaponHitPos) == 0)) {

                        if (this->heldItemAction == PLAYER_IA_HAMMER) {
                            Player_SetFreezeFlashTimer(play);
                            Player_HammerHit(play, this);
                            Player_SetupMeleeWeaponRebound(play, this);
                            return 1;
                        }

                        if (this->speedXZ >= 0.0f) {
                            surfaceMaterial = SurfaceType_GetMaterial(&play->colCtx, groundPoly, bgId);

                            if (surfaceMaterial == SURFACE_MATERIAL_WOOD) {
                                CollisionCheck_SpawnShieldParticlesWood(play, &weaponHitPos, &this->actor.projectedPos);
                            } else {
                                CollisionCheck_SpawnShieldParticles(play, &weaponHitPos);
                                if (surfaceMaterial == SURFACE_MATERIAL_DIRT_SOFT) {
                                    Player_PlaySfx(this, NA_SE_IT_WALL_HIT_SOFT);
                                } else {
                                    Player_PlaySfx(this, NA_SE_IT_WALL_HIT_HARD);
                                }
                            }

                            Player_TryBreakWeapon(play, this);
                            this->speedXZ = -14.0f;
                            Player_RequestRumble(this, 180, 20, 100, 0);
                        }
                    }
                }
            } else {
                Player_SetupMeleeWeaponRebound(play, this);
                Player_SetFreezeFlashTimer(play);
                return 1;
            }
        }

        weaponHitAT =
            (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) || (this->meleeWeaponQuads[1].base.atFlags & AT_HIT);

        if (weaponHitAT) {
            if (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
                Actor* at = this->meleeWeaponQuads[weaponHitAT ? 1 : 0].base.at;

                if ((at != NULL) && (at->id != ACTOR_EN_KANBAN)) {
                    Player_SetFreezeFlashTimer(play);
                }
            }

            if ((Player_TryBreakDekuStick(play, this) == 0) && (this->heldItemAction != PLAYER_IA_HAMMER)) {
                Player_TryBreakBiggoronSword(play, this);

                if (this->actor.colChkInfo.atHitEffect == 1) {
                    this->actor.colChkInfo.damage = 8;
                    Player_TakeColliderDamage(play, this, 4, 0.0f, 0.0f, this->actor.shape.rot.y, 20);
                    return 1;
                }
            }
        }
    }

    return 0;
}

void Player_Action_AimShieldCrouched(Player* this, PlayState* play) {
    f32 stickInputY;
    f32 stickInputX;
    s16 yaw;
    s16 shieldPitchTarget;
    s16 shieldYawTarget;
    s16 shieldPitchStep;
    s16 shieldYawStep;
    f32 cosYaw;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_IsChildWithHylianShield(this)) {
            Player_AnimPlayLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense_wait, this->modelAnimType));
        }
        this->av2.actionVar2 = 1;
        this->av1.actionVar1 = 0;
    }

    if (!Player_IsChildWithHylianShield(this)) {
        this->stateFlags1 |= PLAYER_STATE1_DEFENDING;
        Player_UpdateUpperBody(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_DEFENDING;
    }

    Player_StepSpeedXZToZero(this);

    if (this->av2.actionVar2 != 0) {
        stickInputY = sControlInput->rel.stick_y * 100;
        stickInputX = sControlInput->rel.stick_x * -120;
        yaw = this->actor.shape.rot.y - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

        cosYaw = Math_CosS(yaw);
        shieldPitchTarget = (Math_SinS(yaw) * stickInputX) + (stickInputY * cosYaw);
        cosYaw = Math_CosS(yaw);
        shieldYawTarget = (stickInputX * cosYaw) - (Math_SinS(yaw) * stickInputY);

        if (shieldPitchTarget > 3500) {
            shieldPitchTarget = 3500;
        }

        shieldPitchStep = ABS(shieldPitchTarget - this->actor.focus.rot.x) * 0.25f;
        if (shieldPitchStep < 100) {
            shieldPitchStep = 100;
        }

        shieldYawStep = ABS(shieldYawTarget - this->upperBodyRot.y) * 0.25f;
        if (shieldYawStep < 50) {
            shieldYawStep = 50;
        }

        Math_ScaledStepToS(&this->actor.focus.rot.x, shieldPitchTarget, shieldPitchStep);
        this->upperBodyRot.x = this->actor.focus.rot.x;
        Math_ScaledStepToS(&this->upperBodyRot.y, shieldYawTarget, shieldYawStep);

        if (this->av1.actionVar1 != 0) {
            if (!Player_TryWeaponHit(play, this)) {
                if (this->skelAnime.curFrame < 2.0f) {
                    Player_MeleeAttack(this, 1);
                }
            } else {
                this->av2.actionVar2 = 1;
                this->av1.actionVar1 = 0;
            }
        } else if (!Player_IsBusyInteracting(this, play)) {
            if (Player_ActionChange_TryDefend(this, play)) {
                Player_TryAttackWhileDefending(this, play);
            } else {
                this->stateFlags1 &= ~PLAYER_STATE1_DEFENDING;
                Player_InactivateMeleeWeapon(this);

                if (Player_IsChildWithHylianShield(this)) {
                    Player_StartReturnToStandStill(this, play);
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_clink_normal_defense_ALL, 1.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_clink_normal_defense_ALL), 0.0f,
                                         ANIMMODE_ONCE, 0.0f);
                    Player_AnimReplaceApplyFlags(play, this, ANIM_FLAG_PLAYER_2);
                } else {
                    if (this->itemAction < 0) {
                        Player_SetItemActionFromHeldItem(this);
                    }
                    Player_StartReturnToStandStillWithAnim(
                        this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense_end, this->modelAnimType), play);
                }

                Player_PlaySfx(this, NA_SE_IT_SHIELD_REMOVE);
                return;
            }
        } else {
            return;
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_DEFENDING;
    Player_SetModelsForHoldingShield(this);

    this->lookFlags |= 0xC1;
}

void Player_Action_DeflectAttackWithShield(Player* this, PlayState* play) {
    s32 temp;
    LinkAnimationHeader* anim;
    f32 frames;

    Player_StepSpeedXZToZero(this);

    if (this->av1.actionVar1 == 0) {
        sUpperBodyBusy = Player_UpdateUpperBody(this, play);
        if ((Player_UpperAction_StandingDefense == this->upperActionFunc) ||
            (Player_CheckActionInterruptStatus(play, this, &this->upperSkelAnime, 4.0f) > 0)) {
            Player_SetupAction(play, this, Player_Action_BattleTargetStandStill, 1);
        }
    } else {
        temp = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 4.0f);
        if ((temp != 0) && ((temp > 0) || LinkAnimation_Update(play, &this->skelAnime))) {
            Player_SetupAction(play, this, Player_Action_AimShieldCrouched, 1);
            this->stateFlags1 |= PLAYER_STATE1_DEFENDING;
            Player_SetModelsForHoldingShield(this);
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense, this->modelAnimType);
            frames = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frames, frames, ANIMMODE_ONCE, 0.0f);
        }
    }
}

void Player_Action_RecoverFromMinorDamage(Player* this, PlayState* play) {
    s32 actionInterruptStatus;

    Player_StepSpeedXZToZero(this);

    actionInterruptStatus = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 16.0f);
    if ((actionInterruptStatus != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (actionInterruptStatus > 0))) {
        Player_SetupContextualStandStill(this, play);
    }
}

void Player_Action_Knockback(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    Player_RoundUpInvincibilityTimer(this);

    if (!(this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) && (this->av2.actionVar2 == 0) && (this->damageEffect != 0)) {
        s16 temp = this->actor.shape.rot.y - this->damageYaw;

        this->yaw = this->actor.shape.rot.y = this->damageYaw;
        this->speedXZ = this->knockbackVelXZ;

        if (ABS(temp) > 0x4000) {
            this->actor.shape.rot.y = this->damageYaw + 0x8000;
        }

        if (this->actor.velocity.y < 0.0f) {
            this->actor.gravity = 0.0f;
            this->actor.velocity.y = 0.0f;
        }
    }

    if (LinkAnimation_Update(play, &this->skelAnime) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->av2.actionVar2 != 0) {
            this->av2.actionVar2--;
            if (this->av2.actionVar2 == 0) {
                Player_SetupStandStillMorph(this, play);
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) ||
                   (!(this->cylinder.base.acFlags & AC_HIT) && (this->damageEffect == 0))) {
            if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
                this->av2.actionVar2++;
            } else {
                Player_SetupAction(play, this, Player_Action_DownFromKnockback, 0);
                this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
            }

            Player_AnimPlayOnce(play, this,
                                (this->yaw != this->actor.shape.rot.y) ? &gPlayerAnim_link_normal_front_downB
                                                                       : &gPlayerAnim_link_normal_back_downB);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
        }
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
    }
}

void Player_Action_DownFromKnockback(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    Player_RoundUpInvincibilityTimer(this);

    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime) && (this->speedXZ == 0.0f)) {
        if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
            this->av2.actionVar2++;
        } else {
            Player_SetupAction(play, this, Player_Action_GetUpFromKnockback, 0);
            this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
        }

        Player_AnimPlayOnceAdjusted(play, this,
                                    (this->yaw != this->actor.shape.rot.y) ? &gPlayerAnim_link_normal_front_down_wake
                                                                           : &gPlayerAnim_link_normal_back_down_wake);
        this->yaw = this->actor.shape.rot.y;
    }
}

static AnimSfxEntry D_808545DC[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 20) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_8, 30) },
};

void Player_Action_GetUpFromKnockback(Player* this, PlayState* play) {
    s32 actionInterruptStatus;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_RoundUpInvincibilityTimer(this);

    if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
        LinkAnimation_Update(play, &this->skelAnime);
    } else {
        actionInterruptStatus = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 16.0f);
        if ((actionInterruptStatus != 0) &&
            (LinkAnimation_Update(play, &this->skelAnime) || (actionInterruptStatus > 0))) {
            Player_SetupContextualStandStill(this, play);
        }
    }

    Player_ProcessAnimSfxList(this, D_808545DC);
}

static Vec3f sDeathReviveFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void Player_FinishDie(PlayState* play, Player* this) {
    if (this->av2.actionVar2 != 0) {
        if (this->av2.actionVar2 > 0) {
            this->av2.actionVar2--;
            if (this->av2.actionVar2 == 0) {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_swimer_swim_wait, 1.0f, 0.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_link_swimer_swim_wait), ANIMMODE_ONCE,
                                         -16.0f);
                } else {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_derth_rebirth, 1.0f, 99.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_link_derth_rebirth), ANIMMODE_ONCE, 0.0f);
                }
                gSaveContext.healthAccumulator = 0x140;
                this->av2.actionVar2 = -1;
            }
        } else if (gSaveContext.healthAccumulator == 0) {
            this->stateFlags1 &= ~PLAYER_STATE1_IN_DEATH_CUTSCENE;
            if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                Player_SetupSwimIdle(play, this);
            } else {
                Player_SetupStandStillMorph(this, play);
            }
            this->deathTimer = 20;
            Player_SetInvincibilityTimerNoDamageFlash(this, -20);
            Audio_SetBgmVolumeOnDuringFanfare();
        }
    } else if (this->av1.actionVar1 != 0) {
        this->av2.actionVar2 = 60;
        Player_SpawnFairy(play, this, &this->actor.world.pos, &sDeathReviveFairyPosOffset, FAIRY_REVIVE_DEATH);
        Player_PlaySfx(this, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
        OnePointCutscene_Init(play, 9908, 125, &this->actor, CAM_ID_MAIN);
    } else if (play->gameOverCtx.state == GAMEOVER_DEATH_WAIT_GROUND) {
        play->gameOverCtx.state = GAMEOVER_DEATH_DELAY_MENU;
    }
}

static AnimSfxEntry D_808545F0[] = {
    { NA_SE_PL_BOUND, ANIMSFX_DATA(ANIMSFX_TYPE_2, 60) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 140) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 164) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_8, 170) },
};

void Player_Action_Die(Player* this, PlayState* play) {
    if (this->currentTunic != PLAYER_TUNIC_GORON) {
        if ((play->roomCtx.curRoom.behaviorType2 == ROOM_BEHAVIOR_TYPE2_3) ||
            (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH) ||
            ((Player_GetHurtFloorType(sFloorType) >= 0) &&
             !func_80042108(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId))) {
            Player_StartBurning(this);
        }
    }

    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->actor.category == ACTORCAT_PLAYER) {
            Player_FinishDie(play, this);
        }
        return;
    }

    if (this->skelAnime.animation == &gPlayerAnim_link_derth_rebirth) {
        Player_ProcessAnimSfxList(this, D_808545F0);
    } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_electric_shock_end) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 88.0f)) {
            Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
        }
    }
}

void Player_PlayFallingVoiceSfx(Player* this, u16 sfxId) {
    Player_PlayVoiceSfxForAge(this, sfxId);

    if ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)) {
        Actor_PlaySfx(this->heldActor, NA_SE_VO_RT_FALL);
    }
}

static FallImpactInfo sFallImpactInfo[] = {
    { -8, 180, 40, 100, NA_SE_VO_LI_LAND_DAMAGE_S },
    { -16, 255, 140, 150, NA_SE_VO_LI_LAND_DAMAGE_S },
};

s32 Player_LandFromFall(PlayState* play, Player* this) {
    s32 fallDistance;

    if ((sFloorType == FLOOR_TYPE_NO_FALL_DAMAGE) || (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH)) {
        fallDistance = 0;
    } else {
        fallDistance = this->fallDistance;
    }

    Math_StepToF(&this->speedXZ, 0.0f, 1.0f);

    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    if (fallDistance >= 400) {
        s32 impactIndex;
        FallImpactInfo* impactInfo;

        if (this->fallDistance < 800) {
            impactIndex = 0;
        } else {
            impactIndex = 1;
        }

        impactInfo = &sFallImpactInfo[impactIndex];

        if (Player_InflictDamageAndCheckForDeath(play, impactInfo->damage)) {
            return -1;
        }

        Player_SetInvincibilityTimer(this, 40);
        Player_RequestQuake(play, 32967, 2, 30);
        Player_RequestRumble(this, impactInfo->rumbleStrength, impactInfo->rumbleDuration,
                             impactInfo->rumbleDecreaseRate, 0);
        Player_PlaySfx(this, NA_SE_PL_BODY_HIT);
        Player_PlayVoiceSfxForAge(this, impactInfo->sfxId);

        return impactIndex + 1;
    }

    if (fallDistance > 200) {
        fallDistance *= 2;

        if (fallDistance > 255) {
            fallDistance = 255;
        }

        Player_RequestRumble(this, (u8)fallDistance, (u8)(fallDistance * 0.1f), (u8)fallDistance, 0);

        if (sFloorType == FLOOR_TYPE_NO_FALL_DAMAGE) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }
    }

    Player_PlayLandingSfx(this);

    return 0;
}

void Player_ThrowActor(PlayState* play, Player* this, f32 speedXZ, f32 velocityY) {
    Actor* heldActor = this->heldActor;

    if (!Player_TryStopCarryingActor(play, this, heldActor)) {
        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speed = speedXZ;
        heldActor->velocity.y = velocityY;
        Player_CompleteItemChange(play, this);
        Player_PlaySfx(this, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_Action_Midair(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    if (this->stateFlags3 & PLAYER_STATE3_USING_BOOSTERS) {
        return;
    }

    if (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 40) {
        this->actor.gravity = 0.0f;
    } else if (Player_CheckBattleTargeting(this)) {
        this->actor.gravity = -1.2f;
    }

    Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
            Actor* heldActor = this->heldActor;

            if (!Player_TryStopCarryingActor(play, this, heldActor) && (heldActor->id == ACTOR_EN_NIW) &&
                CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
                Player_ThrowActor(play, this, this->speedXZ + 2.0f, this->actor.velocity.y + 2.0f);
            }
        }

        LinkAnimation_Update(play, &this->skelAnime);

        if (!(this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING)) {
            Player_AdjustMidairSpeedAndYaw(this, &speedTarget, &yawTarget);
        }

        Player_UpdateUpperBody(this, play);

        if (((this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) && (this->av1.actionVar1 == 2)) ||
            !Player_TryMidairJumpSlash(this, play)) {
            if (this->actor.velocity.y < 0.0f) {
                if (this->av2.actionVar2 >= 0) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_WALL) || (this->av2.actionVar2 == 0) ||
                        (this->fallDistance > 0)) {
                        if ((sYDistToFloor > 800.0f) || (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE)) {
                            Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_S);
                            this->stateFlags1 &= ~PLAYER_STATE1_END_HOOKSHOT_MOVE;
                        }

                        LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_normal_landing, 1.0f, 0.0f, 0.0f,
                                             ANIMMODE_ONCE, 8.0f);
                        this->av2.actionVar2 = -1;
                    }
                } else {
                    if ((this->av2.actionVar2 == -1) && (this->fallDistance > 120.0f) && (sYDistToFloor > 280.0f)) {
                        this->av2.actionVar2 = -2;
                        Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_L);
                    }

                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
                        !(this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) &&
                        !(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_SWIMMING)) &&
                        (this->speedXZ > 0.0f)) {
                        if ((this->yDistToLedge >= 150.0f) &&
                            (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0)) {
                            Player_TryClimbWallOrLadder(this, play, sTouchedWallFlags);
                        } else if ((this->ledgeClimbType >= PLAYER_LEDGE_CLIMB_2) && (this->yDistToLedge < 150.0f) &&
                                   (((this->actor.world.pos.y - this->actor.floorHeight) + this->yDistToLedge) >
                                    (70.0f * this->ageProperties->unk_08))) {
                            AnimationContext_DisableQueue(play);
                            if (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE) {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HOOKSHOT_HANG);
                            } else {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
                            }
                            this->actor.world.pos.y += this->yDistToLedge;
                            Player_SetupGrabLedge(
                                play, this, this->actor.wallPoly, this->distToInteractWall,
                                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_hold, this->modelAnimType));
                            this->actor.shape.rot.y = this->yaw += 0x8000;
                            this->stateFlags1 |= PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP;
                        }
                    }
                }
            }
        }
    } else {
        LinkAnimationHeader* anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing, this->modelAnimType);
        s32 sp3C;

        if (this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) {
            if (Player_CheckBattleTargeting(this)) {
                anim = sManualJumpAnims[this->av1.actionVar1][2];
            } else {
                anim = sManualJumpAnims[this->av1.actionVar1][1];
            }
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_run_jump) {
            anim = &gPlayerAnim_link_normal_run_jump_end;
        } else if (Player_CheckBattleTargeting(this)) {
            anim = &gPlayerAnim_link_anchor_landingR;
            Player_ResetLRBlendWeight(this);
        } else if (this->fallDistance <= 80) {
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_short_landing, this->modelAnimType);
        } else if ((this->fallDistance < 800) && (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) &&
                   !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            Player_SetupRolling(this, play);
            return;
        }

        sp3C = Player_LandFromFall(play, this);

        if (sp3C > 0) {
            Player_StartReturnToStandStillWithAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing, this->modelAnimType),
                                                   play);
            this->skelAnime.endFrame = 8.0f;
            if (sp3C == 1) {
                this->av2.actionVar2 = 10;
            } else {
                this->av2.actionVar2 = 20;
            }
        } else if (sp3C == 0) {
            Player_StartReturnToStandStillWithAnim(this, anim, play);
        }
    }
}

static AnimSfxEntry D_8085460C[] = {
    { NA_SE_VO_LI_SWORD_N, ANIMSFX_DATA(ANIMSFX_TYPE_4, 1) },
    { NA_SE_PL_WALK_GROUND, ANIMSFX_DATA(ANIMSFX_TYPE_3, 6) },
    { NA_SE_PL_ROLL, ANIMSFX_DATA(ANIMSFX_TYPE_1, 6) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_5, 18) },
};

void Player_Action_Rolling(Player* this, PlayState* play) {
    Actor* cylinderOc;
    s32 temp;
    s32 sp44;
    DynaPolyActor* wallPolyActor;
    s32 pad;
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    cylinderOc = NULL;
    sp44 = LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
        Player_SetInvincibilityTimerNoDamageFlash(this, -10);
    }

    if (Player_IsBusyInteracting(this, play) == 0) {
        if (this->av2.actionVar2 != 0) {
            Math_StepToF(&this->speedXZ, 0.0f, 2.0f);

            temp = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 5.0f);
            if ((temp != 0) && ((temp > 0) || sp44)) {
                Player_StartReturnToStandStill(this, play);
            }
        } else {
            if (this->speedXZ >= 7.0f) {
                if (((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
                     (sWorldYawToTouchedWall < 0x2000)) ||
                    ((this->cylinder.base.ocFlags1 & OC1_HIT) &&
                     (cylinderOc = this->cylinder.base.oc,
                      ((cylinderOc->id == ACTOR_EN_WOOD02) &&
                       (ABS((s16)(this->actor.world.rot.y - cylinderOc->yawTowardsPlayer)) > 0x6000))))) {

                    if (cylinderOc != NULL) {
                        cylinderOc->home.rot.y = 1;
                    } else if (this->actor.wallBgId != BGCHECK_SCENE) {
                        wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
                        if ((wallPolyActor != NULL) && (wallPolyActor->actor.id == ACTOR_OBJ_KIBAKO2)) {
                            wallPolyActor->actor.home.rot.z = 1;
                        }
                    }

                    Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_hip_down, this->modelAnimType));
                    this->speedXZ = -this->speedXZ;
                    Player_RequestQuake(play, 33267, 3, 12);
                    Player_RequestRumble(this, 255, 20, 150, 0);
                    Player_PlaySfx(this, NA_SE_PL_BODY_HIT);
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                    this->av2.actionVar2 = 1;
                    return;
                }
            }

            if ((this->skelAnime.curFrame < 15.0f) || !Player_ActionChange_TryStartMeleeWeaponAttack(this, play)) {
                if (this->skelAnime.curFrame >= 20.0f) {
                    Player_StartReturnToStandStill(this, play);
                    return;
                }

                Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play);

                speedTarget *= 1.5f;
                if ((speedTarget < 3.0f) || (this->relativeAnalogStickInputs[this->inputFrameCounter] != 0)) {
                    speedTarget = 3.0f;
                }

                Player_SetRunSpeedAndYaw(this, speedTarget, this->actor.shape.rot.y);

                if (Player_TrySpawnDustAtFeet(play, this)) {
                    func_8002F8F0(&this->actor, NA_SE_PL_ROLL_DUST - SFX_FLAG);
                }

                Player_ProcessAnimSfxList(this, D_8085460C);
            }
        }
    }
}

void Player_Action_FallingDive(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_run_jump_water_fall_wait);
    }

    Math_StepToF(&this->speedXZ, 0.0f, 0.05f);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        if (this->fallDistance >= 400) {
            this->actor.colChkInfo.damage = 0x10;
            Player_TakeColliderDamage(play, this, 1, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
        } else {
            Player_SetupRolling(this, play);
        }
    }
}

void Player_Action_JumpSlash(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    this->actor.gravity = -1.2f;
    LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_TryWeaponHit(play, this)) {
        Player_TryMeleeAttack(this, 6.0f, 7.0f, 99.0f);

        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
            Player_AdjustMidairSpeedAndYaw(this, &speedTarget, &this->yaw);
            return;
        }

        if (Player_LandFromFall(play, this) >= 0) {
            this->meleeAttackType += 2;
            Player_SetupMeleeWeaponAttack(play, this, this->meleeAttackType);
            this->slashCounter = 3;
            Player_PlayLandingSfx(this);
        }
    }
}

s32 Player_TryReleaseSpinAttack(Player* this, PlayState* play) {
    s32 meleeAttackType;

    if (Player_SetupCsAction(play, this)) {
        this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
    } else {
        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
            if ((this->spinAttackTimer >= 0.85f) || Player_CanQuickspin(this)) {
                meleeAttackType = sBigSpinAttackMeleeAtkTypes[Player_HoldsTwoHandedWeapon(this)];
            } else {
                meleeAttackType = sSmallSpinAttackMeleeAtkTypes[Player_HoldsTwoHandedWeapon(this)];
            }

            Player_SetupMeleeWeaponAttack(play, this, meleeAttackType);
            Player_SetInvincibilityTimerNoDamageFlash(this, -8);

            this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
            if (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) {
                this->stateFlags2 |= PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
            }
        } else {
            return 0;
        }
    }

    return 1;
}

void Player_SetupWalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_WalkChargingSpinAttack, 1);
}

void Player_SetupSidewalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_SidewalkChargingSpinAttack, 1);
}

void Player_CancelSpinAttackCharge(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_InactivateMeleeWeapon(this);
    Player_AnimChangeOnceMorph(play, this, sCancelSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
    this->yaw = this->actor.shape.rot.y;
}

void Player_FinishWalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_ChargeSpinAttack, 1);
    this->walkFrame = 0.0f;
    Player_AnimPlayLoop(play, this, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
    this->av2.actionVar2 = 1;
}

void Player_UpdateSpinAttackTimer(Player* this) {
    Math_StepToF(&this->spinAttackTimer, 1.0f, 0.02f);
}

void Player_Action_ChargeSpinAttack(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_FinishAnimMovement(this);
        Player_SetupZParallel(this);
        this->stateFlags1 &= ~PLAYER_STATE1_Z_PARALLEL;
        Player_AnimPlayLoop(play, this, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
        this->av2.actionVar2 = -1;
    }

    Player_StepSpeedXZToZero(this);

    if (!Player_IsBusyInteracting(this, play) && (this->av2.actionVar2 != 0)) {
        Player_UpdateSpinAttackTimer(this);

        if (this->av2.actionVar2 < 0) {
            if (this->spinAttackTimer >= 0.1f) {
                this->slashCounter = 0;
                this->av2.actionVar2 = 1;
            } else if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                Player_CancelSpinAttackCharge(this, play);
            }
        } else if (!Player_TryReleaseSpinAttack(this, play)) {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

            moveDir = Player_GetSpinAttackMoveDirection(this, &speedTarget, &yawTarget, play);
            if (moveDir > 0) {
                Player_SetupWalkChargingSpinAttack(this, play);
            } else if (moveDir < 0) {
                Player_SetupSidewalkChargingSpinAttack(this, play);
            }
        }
    }
}

void Player_Action_WalkChargingSpinAttack(Player* this, PlayState* play) {
    s16 yawDiff;
    s32 absYawDiff;
    f32 speedMagnitude;
    f32 blendWeight;
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    s16 yawTargetDiff;
    s32 absYawTargetDiff;

    yawDiff = this->yaw - this->actor.shape.rot.y;
    absYawDiff = ABS(yawDiff);

    speedMagnitude = fabsf(this->speedXZ);
    blendWeight = speedMagnitude * 1.5f;

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (blendWeight < 1.5f) {
        blendWeight = 1.5f;
    }

    blendWeight = ((absYawDiff < 0x4000) ? -1.0f : 1.0f) * blendWeight;

    Player_SetWalkFrameAndSfx(this, blendWeight);

    blendWeight = CLAMP(speedMagnitude * 0.5f, 0.5f, 1.0f);

    LinkAnimation_BlendToJoint(play, &this->skelAnime, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)], 0.0f,
                               sSpinAttackChargeWalkAnims[Player_HoldsTwoHandedWeapon(this)],
                               this->walkFrame * (21.0f / 29.0f), blendWeight, this->blendTable);

    if (!Player_IsBusyInteracting(this, play) && !Player_TryReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        moveDir = Player_GetSpinAttackMoveDirection(this, &speedTarget, &yawTarget, play);

        if (moveDir < 0) {
            Player_SetupSidewalkChargingSpinAttack(this, play);
            return;
        }

        if (moveDir == 0) {
            speedTarget = 0.0f;
            yawTarget = this->yaw;
        }

        yawTargetDiff = yawTarget - this->yaw;
        absYawTargetDiff = ABS(yawTargetDiff);

        if (absYawTargetDiff > 0x4000) {
            if (Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
                this->yaw = yawTarget;
            }
            return;
        }

        Math_AsymStepToF(&this->speedXZ, speedTarget * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->yaw, yawTarget, absYawTargetDiff * 0.1f);

        if ((speedTarget == 0.0f) && (this->speedXZ == 0.0f)) {
            Player_FinishWalkChargingSpinAttack(this, play);
        }
    }
}

void Player_Action_SidewalkChargingSpinAttack(Player* this, PlayState* play) {
    f32 speedMagnitude;
    f32 blendWeight;
    f32 speedTarget;
    s16 yawTarget;
    s32 moveDir;
    s16 yawTargetDiff;
    s32 absYawTargetDiff;

    speedMagnitude = fabsf(this->speedXZ);

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (speedMagnitude == 0.0f) {
        speedMagnitude = ABS(this->unk_87C) * 0.0015f;
        if (speedMagnitude < 400.0f) {
            speedMagnitude = 0.0f;
        }
        Player_SetWalkFrameAndSfx(this, ((this->unk_87C >= 0) ? 1 : -1) * speedMagnitude);
    } else {
        blendWeight = speedMagnitude * 1.5f;
        if (blendWeight < 1.5f) {
            blendWeight = 1.5f;
        }
        Player_SetWalkFrameAndSfx(this, blendWeight);
    }

    blendWeight = CLAMP(speedMagnitude * 0.5f, 0.5f, 1.0f);

    LinkAnimation_BlendToJoint(play, &this->skelAnime, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)], 0.0f,
                               sSpinAttackChargeSidewalkAnims[Player_HoldsTwoHandedWeapon(this)],
                               this->walkFrame * (21.0f / 29.0f), blendWeight, this->blendTable);

    if (!Player_IsBusyInteracting(this, play) && !Player_TryReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        moveDir = Player_GetSpinAttackMoveDirection(this, &speedTarget, &yawTarget, play);

        if (moveDir > 0) {
            Player_SetupWalkChargingSpinAttack(this, play);
            return;
        }

        if (moveDir == 0) {
            speedTarget = 0.0f;
            yawTarget = this->yaw;
        }

        yawTargetDiff = yawTarget - this->yaw;
        absYawTargetDiff = ABS(yawTargetDiff);

        if (absYawTargetDiff > 0x4000) {
            if (Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
                this->yaw = yawTarget;
            }
            return;
        }

        Math_AsymStepToF(&this->speedXZ, speedTarget * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->yaw, yawTarget, absYawTargetDiff * 0.1f);

        if ((speedTarget == 0.0f) && (this->speedXZ == 0.0f) && (speedMagnitude == 0.0f)) {
            Player_FinishWalkChargingSpinAttack(this, play);
        }
    }
}

void Player_Action_JumpUpToLedge(Player* this, PlayState* play) {
    s32 animDone;
    f32 jumpVelocityY;
    s32 actionInterruptState;
    f32 landingSfxFrame;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (this->skelAnime.animation == &gPlayerAnim_link_normal_250jump_start) {
        this->speedXZ = 1.0f;

        if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
            jumpVelocityY = this->yDistToLedge;

            if (jumpVelocityY > this->ageProperties->unk_0C) {
                jumpVelocityY = this->ageProperties->unk_0C;
            }

            if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                jumpVelocityY *= 0.085f;
            } else {
                jumpVelocityY *= 0.072f;
            }

            if (!LINK_IS_ADULT) {
                jumpVelocityY += 1.0f;
            }

            Player_SetupJumpWithSfx(this, NULL, jumpVelocityY, play, NA_SE_VO_LI_AUTO_JUMP);
            this->av2.actionVar2 = -1;
            return;
        }
    } else {
        actionInterruptState = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 4.0f);

        if (actionInterruptState == 0) {
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        if ((animDone != 0) || (actionInterruptState > 0)) {
            Player_SetupStandStill(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        landingSfxFrame = 0.0f;

        if (this->skelAnime.animation == &gPlayerAnim_link_swimer_swim_15step_up) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                Player_StartJumpOutOfWater(play, this, 10.0f);
            }
            landingSfxFrame = 50.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_150step_up) {
            landingSfxFrame = 30.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) {
            landingSfxFrame = 16.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, landingSfxFrame)) {
            Player_PlayLandingSfx(this);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }

        if ((this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) || (this->skelAnime.curFrame > 5.0f)) {
            if (this->av2.actionVar2 == 0) {
                Player_PlayJumpSfx(this);
                this->av2.actionVar2 = 1;
            }
            Math_StepToF(&this->actor.shape.yOffset, 0.0f, 150.0f);
        }
    }
}

void Player_Action_MiniCs(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);

    if (((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor != NULL) &&
         (this->getItemId == GI_NONE)) ||
        !Player_UpdateUpperBody(this, play)) {
        this->miniCsFunc(play, this);
    }
}

s32 Player_CsMovement(PlayState* play, Player* this, CsCmdActorCue* cue, f32 speedTarget, s16 yawTarget, s32 moveMode) {
    if ((moveMode != 0) && (this->speedXZ == 0.0f)) {
        return LinkAnimation_Update(play, &this->skelAnime);
    }

    if (moveMode != 2) {
        f32 rate = R_UPDATE_RATE * 0.5f;
        f32 distToEndX = cue->endPos.x - this->actor.world.pos.x;
        f32 distToEndZ = cue->endPos.z - this->actor.world.pos.z;
        f32 distToEndXZ = sqrtf(SQ(distToEndX) + SQ(distToEndZ)) / rate;
        s32 framesUntilEnd = (cue->endFrame - play->csCtx.curFrame) + 1;

        yawTarget = Math_Atan2S(distToEndZ, distToEndX);

        if (moveMode == 1) {
            f32 distX = cue->endPos.x - cue->startPos.x;
            f32 distZ = cue->endPos.z - cue->startPos.z;
            s32 temp = (((sqrtf(SQ(distX) + SQ(distZ)) / rate) / (cue->endFrame - cue->startFrame)) / 1.5f) * 4.0f;

            if (temp >= framesUntilEnd) {
                yawTarget = this->actor.shape.rot.y;
                speedTarget = 0.0f;
            } else {
                speedTarget = distToEndXZ / ((framesUntilEnd - temp) + 1);
            }
        } else {
            speedTarget = distToEndXZ / framesUntilEnd;
        }
    }

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);
    Player_SetRunSpeedAndYaw(this, speedTarget, yawTarget);

    if ((speedTarget == 0.0f) && (this->speedXZ == 0.0f)) {
        Player_FinishRun(this, play);
    }

    return 0;
}

// Returns remaining distance until within range
s32 Player_ApproachCsMovePos(PlayState* play, Player* this, f32* speedTarget, s32 endRange) {
    f32 dx = this->csStartPos.x - this->actor.world.pos.x;
    f32 dz = this->csStartPos.z - this->actor.world.pos.z;
    s32 dist = sqrtf(SQ(dx) + SQ(dz));
    s16 yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (dist < endRange) {
        *speedTarget = 0.0f;
        yaw = this->actor.shape.rot.y;
    }

    if (Player_CsMovement(play, this, NULL, *speedTarget, yaw, 2)) {
        return 0;
    }

    return dist;
}

s32 func_80845C68(PlayState* play, s32 arg1) {
    if (arg1 == 0) {
        Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
    }
    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 0;
    return arg1;
}

void Player_Action_MiniCsMovement(Player* this, PlayState* play) {
    f32 speedTarget;
    s32 distToRange;
    f32 speedTarget2;
    s32 endRange;
    s32 pad;

    if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
        if (this->av2.actionVar2 == 0) {
            LinkAnimation_Update(play, &this->skelAnime);

            if (DECR(this->doorTimer) == 0) {
                this->speedXZ = 0.1f;
                this->av2.actionVar2 = 1;
            }
        } else if (this->av1.actionVar1 == 0) {
            speedTarget = 5.0f * sWaterSpeedScale;

            if (Player_ApproachCsMovePos(play, this, &speedTarget, -1) < 30) {
                this->av1.actionVar1 = 1;
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

                this->csStartPos.x = this->csEndPos.x;
                this->csStartPos.z = this->csEndPos.z;
            }
        } else {
            speedTarget2 = 5.0f;
            endRange = 20;

            if (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) {
                speedTarget2 = gSaveContext.entranceSpeed;

                if (sConveyorSpeed != CONVEYOR_SPEED_DISABLED) {
                    this->csStartPos.x = (Math_SinS(sConveyorYaw) * 400.0f) + this->actor.world.pos.x;
                    this->csStartPos.z = (Math_CosS(sConveyorYaw) * 400.0f) + this->actor.world.pos.z;
                }
            } else if (this->av2.actionVar2 < 0) {
                this->av2.actionVar2++;

                speedTarget2 = gSaveContext.entranceSpeed;
                endRange = -1;
            }

            distToRange = Player_ApproachCsMovePos(play, this, &speedTarget2, endRange);

            if ((this->av2.actionVar2 == 0) ||
                ((distToRange == 0) && (this->speedXZ == 0.0f) &&
                 (Play_GetCamera(play, CAM_ID_MAIN)->stateFlags & CAM_STATE_CAM_FUNC_FINISH))) {

                Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
                func_80845C68(play, gSaveContext.respawn[RESPAWN_MODE_DOWN].data);

                if (!Player_ActionChange_TrySpeakOrCheck(this, play)) {
                    Player_EndMiniCsMovement(this, play);
                }
            }
        }
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        Player_UpdateUpperBody(this, play);
    }
}

void Player_Action_OpenDoor(Player* this, PlayState* play) {
    s32 animDone;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    Player_UpdateUpperBody(this, play);

    if (animDone) {
        if (this->av2.actionVar2 == 0) {
            if (DECR(this->doorTimer) == 0) {
                this->av2.actionVar2 = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandStill(this, play);
            if (play->roomCtx.prevRoom.num >= 0) {
                func_80097534(play, &play->roomCtx);
            }
            Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
            Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
        }
        return;
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) && LinkAnimation_OnFrame(&this->skelAnime, 15.0f)) {
        play->stopPlayerMovement(this, play);
    }
}

void Player_Action_LiftActor(Player* this, PlayState* play) {
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupContextualStandStill(this, play);
        Player_SetupCarryingActor(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* interactRangeActor = this->interactRangeActor;

        if (!Player_TryStopCarryingActor(play, this, interactRangeActor)) {
            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            interactRangeActor->bgCheckFlags &=
                ~(BGCHECKFLAG_GROUND | BGCHECKFLAG_GROUND_TOUCH | BGCHECKFLAG_GROUND_LEAVE | BGCHECKFLAG_WALL |
                  BGCHECKFLAG_CEILING | BGCHECKFLAG_WATER | BGCHECKFLAG_WATER_TOUCH | BGCHECKFLAG_GROUND_STRICT);
            this->leftHandRot.y = interactRangeActor->shape.rot.y - this->actor.shape.rot.y;
        }
        return;
    }

    Math_ScaledStepToS(&this->leftHandRot.y, 0, 4000);
}

static AnimSfxEntry sThrowStonePillarAnimSfx[] = {
    { NA_SE_VO_LI_SWORD_L, ANIMSFX_DATA(ANIMSFX_TYPE_4, 49) },
    { NA_SE_VO_LI_SWORD_N, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 230) },
};

void Player_Action_ThrowStonePillar(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->av2.actionVar2++ > 20)) {
        if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
            Player_StartReturnToStandStillWithAnim(this, &gPlayerAnim_link_normal_heavy_carry_end, play);
        }
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 41.0f)) {
        BgHeavyBlock* heavyBlock = (BgHeavyBlock*)this->interactRangeActor;

        this->heldActor = &heavyBlock->dyna.actor;
        this->actor.child = &heavyBlock->dyna.actor;
        heavyBlock->dyna.actor.parent = &this->actor;
        func_8002DBD0(&heavyBlock->dyna.actor, &heavyBlock->unk_164, &this->leftHandPos);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 229.0f)) {
        Actor* heldActor = this->heldActor;

        heldActor->speed = Math_SinS(heldActor->shape.rot.x) * 40.0f;
        heldActor->velocity.y = Math_CosS(heldActor->shape.rot.x) * 40.0f;
        heldActor->gravity = -2.0f;
        heldActor->minVelocityY = -30.0f;
        Player_DetachHeldActor(play, this);
        return;
    }

    Player_ProcessAnimSfxList(this, sThrowStonePillarAnimSfx);
}

void Player_Action_LiftSilverBoulder(Player* this, PlayState* play) {
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_silver_wait);
        this->av2.actionVar2 = 1;
        return;
    }

    if (this->av2.actionVar2 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 27.0f)) {
            Actor* interactRangeActor = this->interactRangeActor;

            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            return;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, 25.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
            return;
        }

    } else if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
        Player_SetupAction(play, this, Player_Action_ThrowSilverBoulder, 1);
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_silver_throw);
    }
}

void Player_Action_LiftHeavyFuel(Player* this, PlayState* play) {
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupAction(play, this, Player_Action_LiftActor, 0);
        Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_carryB, this->modelAnimType));
    }
}

void Player_Action_ThrowSilverBoulder(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupContextualStandStill(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Actor* heldActor = this->heldActor;

        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speed = 10.0f;
        heldActor->velocity.y = 20.0f;
        Player_CompleteItemChange(play, this);
        Player_PlaySfx(this, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_Action_FailToLiftActor(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_nocarry_free_wait);
        this->av2.actionVar2 = 15;
        return;
    }

    if (this->av2.actionVar2 != 0) {
        this->av2.actionVar2--;
        if (this->av2.actionVar2 == 0) {
            Player_StartReturnToStandStillWithAnim(this, &gPlayerAnim_link_normal_nocarry_free_end, play);
            this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }
    }
}

void Player_Action_PutDownActor(Player* this, PlayState* play) {
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupContextualStandStill(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* heldActor = this->heldActor;

        if (!Player_TryStopCarryingActor(play, this, heldActor)) {
            heldActor->velocity.y = 0.0f;
            heldActor->speed = 0.0f;
            Player_CompleteItemChange(play, this);
            if (heldActor->id == ACTOR_EN_BOM_CHU) {
                Player_ForceFirstPerson(this, play);
            }
        }
    }
}

void Player_Action_StartThrowActor(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime) ||
        ((this->skelAnime.curFrame >= 8.0f) &&
         Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play))) {
        Player_SetupContextualStandStill(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Player_ThrowActor(play, this, this->speedXZ + 8.0f, 12.0f);
    }
}

static ColliderCylinderInit sBodyCylInit = {
    {
        COLTYPE_HIT5,
        AT_NONE,
        AC_ON | AC_TYPE_ENEMY,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_PLAYER,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK1,
        { 0x00000000, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 12, 60, 0, { 0, 0, 0 } },
};

static ColliderQuadInit sMeleeWpnQuadInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000100, 0x00, 0x01 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

static ColliderQuadInit sShieldQuadInit = {
    {
        COLTYPE_METAL,
        AT_ON | AT_TYPE_PLAYER,
        AC_ON | AC_HARD | AC_TYPE_ENEMY,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00100000, 0x00, 0x00 },
        { 0xDFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

void Player_DoNothingFromSpawn(Actor* thisx, PlayState* play) {
}

void Player_Spawn_NoUpdateOrDraw(PlayState* play, Player* this) {
    this->actor.update = Player_DoNothingFromSpawn;
    this->actor.draw = NULL;
}

void Player_Spawn_FromBlueWarp(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_DescendFromBlueWarp, 0);
    if ((play->sceneId == SCENE_LAKE_HYLIA) && IS_CUTSCENE_LAYER) {
        this->av1.actionVar1 = 1;
    }
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_okarina_warp_goal, 2.0f / 3.0f, 0.0f, 24.0f,
                         ANIMMODE_ONCE, 0.0f);
    this->actor.world.pos.y += 800.0f;
}

static u8 swordItemsByAge[] = { ITEM_SWORD_MASTER, ITEM_SWORD_KOKIRI };

void Player_DrawCsSword(PlayState* play, Player* this, s32 sfxFlag) {
    s32 item = swordItemsByAge[(void)0, gSaveContext.save.linkAge];
    s32 itemAction = sItemActions[item];

    Player_DestroyHookshot(this);
    Player_DetachHeldActor(play, this);

    this->heldItemId = item;
    this->nextModelGroup = Player_ActionToModelGroup(this, itemAction);

    Player_InitItemAction(play, this, itemAction);
    Player_CompleteItemChange(play, this);

    if (sfxFlag != 0) {
        Player_PlaySfx(this, NA_SE_IT_SWORD_PICKOUT);
    }
}

static Vec3f sEndTimeTravelStartPos = { -1.0f, 69.0f, 20.0f };

void Player_Spawn_FromTimeTravel(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_FinishTimeTravel, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    Math_Vec3f_Copy(&this->actor.world.pos, &sEndTimeTravelStartPos);
    this->yaw = this->actor.shape.rot.y = -0x8000;
    LinkAnimation_Change(play, &this->skelAnime, this->ageProperties->unk_A0, 2.0f / 3.0f, 0.0f, 0.0f, ANIMMODE_ONCE,
                         0.0f);
    Player_AnimReplaceApplyFlags(play, this,
                                 ANIM_REPLACE_APPLY_FLAG_9 | ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_2 |
                                     ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_PLAYER_7);
    if (LINK_IS_ADULT) {
        Player_DrawCsSword(play, this, 0);
    }
    this->av2.actionVar2 = 20;
}

void Player_Spawn_OpeningDoor(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_TryOpenDoorFromSpawn, 0);
    Player_AnimReplaceApplyFlags(play, this,
                                 ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                     ANIM_FLAG_PLAYER_7);
}

void Player_Spawn_ExitingGrotto(PlayState* play, Player* this) {
    Player_SetupJump(this, &gPlayerAnim_link_normal_jump, 12.0f, play);
    Player_SetupAction(play, this, Player_Action_JumpFromGrotto, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    this->fallStartHeight = this->actor.world.pos.y;
    OnePointCutscene_Init(play, 5110, 40, &this->actor, CAM_ID_MAIN);
}

void Player_Spawn_WithKnockback(PlayState* play, Player* this) {
    Player_TakeColliderDamage(play, this, 1, 2.0f, 2.0f, this->actor.shape.rot.y + 0x8000, 0);
}

void Player_Spawn_FromWarpSong(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_AppearFromWarpSong, 0);
    this->actor.draw = NULL;
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
}

static s16 sMagicSpellActors[] = { ACTOR_MAGIC_WIND, ACTOR_MAGIC_DARK, ACTOR_MAGIC_FIRE };

Actor* Player_SpawnMagicSpell(PlayState* play, Player* this, s32 arg2) {
    return Actor_Spawn(&play->actorCtx, play, sMagicSpellActors[arg2], this->actor.world.pos.x, this->actor.world.pos.y,
                       this->actor.world.pos.z, 0, 0, 0, 0);
}

void Player_Spawn_FromFW(PlayState* play, Player* this) {
    this->actor.draw = NULL;
    Player_SetupAction(play, this, Player_Action_AppearFromFW, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
}

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(targetArrowOffset, 500, ICHAIN_STOP),
};

static EffectBlureInit2 sBlure2InitParams = {
    0, 8, 0, { 255, 255, 255, 255 }, { 255, 255, 255, 64 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, 4,
    0, 2, 0, { 0, 0, 0, 0 },         { 0, 0, 0, 0 },
};

static Vec3s sSkeletonBaseTransl = { -57, 3377, 0 };

void Player_InitCommon(Player* this, PlayState* play, FlexSkeletonHeader* skelHeader) {
    this->ageProperties = &sAgeProperties[gSaveContext.save.linkAge];
    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;
    this->yaw = this->actor.world.rot.y;
    Player_CompleteItemChange(play, this);

    SkelAnime_InitLink(play, &this->skelAnime, skelHeader, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType),
                       9, this->jointTable, this->morphTable, PLAYER_LIMB_MAX);
    this->skelAnime.baseTransl = sSkeletonBaseTransl;
    SkelAnime_InitLink(play, &this->upperSkelAnime, skelHeader, Player_GetStandStillAnim(this), 9,
                       this->upperJointTable, this->upperMorphTable, PLAYER_LIMB_MAX);
    this->upperSkelAnime.baseTransl = sSkeletonBaseTransl;

    Effect_Add(play, &this->meleeWeaponEffectIndex, EFFECT_BLURE2, 0, 0, &sBlure2InitParams);
    Effect_Add(play, &this->zoraSwimEffectIndex[0], EFFECT_BLURE2, 0, 0, &sBlure2InitParams);
    Effect_Add(play, &this->zoraSwimEffectIndex[1], EFFECT_BLURE2, 0, 0, &sBlure2InitParams);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawFeet, this->ageProperties->unk_04);
    this->subCamId = CAM_ID_NONE;

    Collider_InitCylinder(play, &this->cylinder);
    Collider_SetCylinder(play, &this->cylinder, &this->actor, &sBodyCylInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[0]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[0], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[1]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[1], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->shieldQuad);
    Collider_SetQuad(play, &this->shieldQuad, &this->actor, &sShieldQuadInit);
}

static void (*sSpawnFuncs[])(PlayState* play, Player* this) = {
    /* 0x0 */ Player_Spawn_NoUpdateOrDraw,
    /* 0x1 */ Player_Spawn_FromTimeTravel, // From time travel
    /* 0x2 */ Player_Spawn_FromBlueWarp,
    /* 0x3 */ Player_Spawn_OpeningDoor,
    /* 0x4 */ Player_Spawn_ExitingGrotto,
    /* 0x5 */ Player_Spawn_FromWarpSong,
    /* 0x6 */ Player_Spawn_FromFW,
    /* 0x7 */ Player_Spawn_WithKnockback,
    /* 0x8 */ Player_Spawn_SlowWalk,
    /* 0x9 */ Player_Spawn_SlowWalk,
    /* 0xA */ Player_Spawn_SlowWalk,
    /* 0xB */ Player_Spawn_SlowWalk,
    /* 0xC */ Player_Spawn_SlowWalk,
    /* 0xD */ Player_Spawn_StandStill,
    /* 0xE */ Player_Spawn_SlowWalk,
    /* 0xF */ Player_Spawn_MoveWithEntranceSpeed,
};

static Vec3f sNaviPosOffset = { 0.0f, 50.0f, 0.0f };

void Player_Init(Actor* thisx, PlayState* play2) {
    Player* this = (Player*)thisx;
    PlayState* play = play2;
    SceneTableEntry* scene = play->loadedScene;
    u32 titleFileSize;
    s32 initMode;
    s32 respawnFlag;
    s32 respawnMode;

    play->shootingGalleryStatus = play->bombchuBowlingStatus = 0;

    play->playerInit = Player_InitCommon;
    play->playerUpdate = Player_UpdateCommon;
    play->isPlayerDroppingFish = Player_IsDroppingFish;
    play->startPlayerFishing = Player_StartFishing;
    play->grabPlayer = Player_TryRestrainedByEnemy;
    play->tryPlayerCsAction = Player_TryCsAction;
    play->stopPlayerMovement = Player_SetupStandStillMorph;
    play->damagePlayer = Player_InflictDamageAndCheckForDeath;
    play->talkWithPlayer = Player_StartTalkToActor;

    thisx->room = -1;
    this->ageProperties = &sAgeProperties[gSaveContext.save.linkAge];
    this->itemAction = this->heldItemAction = -1;
    this->heldItemId = ITEM_NONE;

    Player_UseItem(play, this, ITEM_NONE);
    Player_SetEquipmentData(play, this);
    this->prevBoots = this->currentBoots;
    Player_InitCommon(this, play, gPlayerSkelHeaders[((void)0, gSaveContext.save.linkAge)]);
    this->giObjectSegment = (void*)(((uintptr_t)ZeldaArena_MallocDebug(0xB008, "../z_player.c", 17175) + 8) & ~0xF);

    respawnFlag = gSaveContext.respawnFlag;

    if (respawnFlag != 0) {
        if (respawnFlag == -3) {
            thisx->params = gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams;
        } else {
            if ((respawnFlag == 1) || (respawnFlag == -1)) {
                this->voidRespawnCounter = -2;
            }

            if (respawnFlag < 0) {
                respawnMode = RESPAWN_MODE_DOWN;
            } else {
                respawnMode = respawnFlag - 1;
                Math_Vec3f_Copy(&thisx->world.pos, &gSaveContext.respawn[respawnMode].pos);
                Math_Vec3f_Copy(&thisx->home.pos, &thisx->world.pos);
                Math_Vec3f_Copy(&thisx->prevPos, &thisx->world.pos);
                this->fallStartHeight = thisx->world.pos.y;
                this->yaw = thisx->shape.rot.y = gSaveContext.respawn[respawnMode].yaw;
                thisx->params = gSaveContext.respawn[respawnMode].playerParams;
            }

            play->actorCtx.flags.tempSwch = gSaveContext.respawn[respawnMode].tempSwchFlags & 0xFFFFFF;
            play->actorCtx.flags.tempCollect = gSaveContext.respawn[respawnMode].tempCollectFlags;
        }
    }

    if ((respawnFlag == 0) || (respawnFlag < -1)) {
        titleFileSize = scene->titleFile.vromEnd - scene->titleFile.vromStart;
        if ((titleFileSize != 0) && gSaveContext.showTitleCard) {
            if (!IS_CUTSCENE_LAYER &&
                (gEntranceTable[((void)0, gSaveContext.save.entranceIndex) + ((void)0, gSaveContext.sceneLayer)].field &
                 ENTRANCE_INFO_DISPLAY_TITLE_CARD_FLAG) &&
                ((play->sceneId != SCENE_DODONGOS_CAVERN) || GET_EVENTCHKINF(EVENTCHKINF_B0)) &&
                ((play->sceneId != SCENE_BOMBCHU_SHOP) || GET_EVENTCHKINF(EVENTCHKINF_25))) {
                TitleCard_InitPlaceName(play, &play->actorCtx.titleCtx, this->giObjectSegment, 160, 120, 144, 24, 20);
            }
        }
        gSaveContext.showTitleCard = true;
    }

    if (func_80845C68(play, (respawnFlag == 2) ? 1 : 0) == 0) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = (thisx->params & 0xFF) | 0xD00;
    }

    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 1;

    if (play->sceneId <= SCENE_INSIDE_GANONS_CASTLE_COLLAPSE) {
        gSaveContext.save.info.infTable[INFTABLE_1AX_INDEX] |= gBitFlags[play->sceneId];
    }

    initMode = (thisx->params & 0xF00) >> 8;
    if ((initMode == 5) || (initMode == 6)) {
        if (gSaveContext.save.cutsceneIndex >= 0xFFF0) {
            initMode = 13;
        }
    }

    sSpawnFuncs[initMode](play, this);

    if (initMode != 0) {
        if ((gSaveContext.gameMode == GAMEMODE_NORMAL) || (gSaveContext.gameMode == GAMEMODE_END_CREDITS)) {
            // this->naviActor = Player_SpawnFairy(play, this, &thisx->world.pos, &sNaviPosOffset, FAIRY_NAVI);
            this->naviActor = NULL;
            if (gSaveContext.dogParams != 0) {
                gSaveContext.dogParams |= 0x8000;
            }
        }
    }

    if (gSaveContext.nayrusLoveTimer != 0) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpell(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }

    if (gSaveContext.entranceSound != 0) {
        Actor_PlaySfx(&this->actor, ((void)0, gSaveContext.entranceSound));
        gSaveContext.entranceSound = 0;
    }

    Map_SavePlayerInitialInfo(play);
    MREG(64) = 0;
}

void Player_StepValueToZero(s16* pValue) {
    s16 step;

    step = (ABS(*pValue) * 100.0f) / 1000.0f;
    step = CLAMP(step, 400, 4000);

    Math_ScaledStepToS(pValue, 0, step);
}

void Player_StepLookToZero(Player* this) {
    s16 focusYawDiff;

    if (!(this->lookFlags & 2)) {
        focusYawDiff = this->actor.focus.rot.y - this->actor.shape.rot.y;
        Player_StepValueToZero(&focusYawDiff);
        this->actor.focus.rot.y = this->actor.shape.rot.y + focusYawDiff;
    }

    if (!(this->lookFlags & 1)) {
        Player_StepValueToZero(&this->actor.focus.rot.x);
    }

    if (!(this->lookFlags & 8)) {
        Player_StepValueToZero(&this->headRot.x);
    }

    if (!(this->lookFlags & 0x40)) {
        Player_StepValueToZero(&this->upperBodyRot.x);
    }

    if (!(this->lookFlags & 4)) {
        Player_StepValueToZero(&this->actor.focus.rot.z);
    }

    if (!(this->lookFlags & 0x10)) {
        Player_StepValueToZero(&this->headRot.y);
    }

    if (!(this->lookFlags & 0x20)) {
        Player_StepValueToZero(&this->headRot.z);
    }

    if (!(this->lookFlags & 0x80)) {
        if (this->upperBodyYawOffset != 0) {
            Player_StepValueToZero(&this->upperBodyYawOffset);
        } else {
            Player_StepValueToZero(&this->upperBodyRot.y);
        }
    }

    if (!(this->lookFlags & 0x100)) {
        Player_StepValueToZero(&this->upperBodyRot.z);
    }

    this->lookFlags = 0;
}

static f32 sScaleDiveDists[] = { 120.0f, 240.0f, 360.0f };

static u8 sDiveDoActions[] = { DO_ACTION_1, DO_ACTION_2, DO_ACTION_3, DO_ACTION_4,
                               DO_ACTION_5, DO_ACTION_6, DO_ACTION_7, DO_ACTION_8 };

void Player_SetDoActionText(PlayState* play, Player* this) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) && (this->actor.category == ACTORCAT_PLAYER)) {
        Actor* heldActor = this->heldActor;
        Actor* interactRangeActor = this->interactRangeActor;
        s32 sp24;
        s32 sp20 = this->relativeAnalogStickInputs[this->inputFrameCounter];
        s32 sp1C = Player_IsSwimming(this);
        s32 doAction = DO_ACTION_NONE;

        if (!Player_InBlockingCsMode(play, this)) {
            if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
                doAction = DO_ACTION_RETURN;
            } else if ((this->heldItemAction == PLAYER_IA_FISHING_POLE) && (this->unk_860 != 0)) {
                if (this->unk_860 == 2) {
                    doAction = DO_ACTION_REEL;
                }
            } else if ((Player_Action_PlayOcarina != this->actionFunc) &&
                       !(this->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
                if ((this->doorType != PLAYER_DOORTYPE_NONE) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                     ((heldActor != NULL) && (heldActor->id == ACTOR_SEED)))) {
                    doAction = DO_ACTION_OPEN;
                } else if ((!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) || (heldActor == NULL)) &&
                           (interactRangeActor != NULL) &&
                           ((!sp1C && (this->getItemId == GI_NONE)) ||
                            ((this->getItemId < 0) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)))) {
                    if (this->getItemId < 0) {
                        doAction = DO_ACTION_OPEN;
                    } else if ((interactRangeActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                        doAction = DO_ACTION_DROP;
                    } else {
                        doAction = DO_ACTION_GRAB;
                    }
                } else if (!sp1C && (this->stateFlags2 & PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL)) {
                    doAction = DO_ACTION_GRAB;
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL) ||
                           (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->rideActor != NULL))) {
                    doAction = DO_ACTION_CLIMB;
                } else if ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
                           !EN_HORSE_CHECK_4((EnHorse*)this->rideActor) &&
                           (Player_Action_DismountHorse != this->actionFunc)) {
                    if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                        if (this->talkActor->category == ACTORCAT_NPC) {
                            doAction = DO_ACTION_SPEAK;
                        } else {
                            doAction = DO_ACTION_CHECK;
                        }
                    } else if (!Actor_PlayerIsAimingReadyFpsItem(this) &&
                               !(this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
                        doAction = DO_ACTION_FASTER;
                    }
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                    if (this->talkActor->category == ACTORCAT_NPC) {
                        doAction = DO_ACTION_SPEAK;
                    } else {
                        doAction = DO_ACTION_CHECK;
                    }
                } else if ((this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING)) ||
                           ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
                            (this->stateFlags2 & PLAYER_STATE2_CAN_DISMOUNT_HORSE))) {
                    doAction = DO_ACTION_DOWN;
                } else if (this->stateFlags2 & PLAYER_STATE2_DO_ACTION_ENTER) {
                    doAction = DO_ACTION_ENTER;
                } else if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->getItemId == GI_NONE) &&
                           (heldActor != NULL)) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (heldActor->id == ACTOR_EN_NIW)) {
                        if (Player_CanThrowActor(this, heldActor) == 0) {
                            doAction = DO_ACTION_DROP;
                        } else {
                            doAction = DO_ACTION_THROW;
                        }
                    }
                } else if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) && Player_CanCarryActor(this) &&
                           (this->getItemId < GI_MAX)) {
                    doAction = DO_ACTION_GRAB;
                } else if (this->stateFlags2 & PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER) {
                    sp24 = (sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)] - this->actor.yDistToWater) / 40.0f;
                    sp24 = CLAMP(sp24, 0, 7);
                    doAction = sDiveDoActions[sp24];
                } else if (sp1C && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
                    doAction = DO_ACTION_DIVE;
                } else if (!sp1C && (!(this->stateFlags1 & PLAYER_STATE1_DEFENDING) || Player_CheckTargeting(this) ||
                                     !Player_IsChildWithHylianShield(this))) {
                    if ((!(this->stateFlags1 & PLAYER_STATE1_CLIMBING_ONTO_LEDGE) && (sp20 <= 0) &&
                         (Player_CheckBattleTargeting(this) ||
                          ((sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE) &&
                           (Player_CheckCalmTargeting(this) ||
                            ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                             !(this->stateFlags1 & PLAYER_STATE1_DEFENDING) && (sp20 == 0))))))) {
                        doAction = DO_ACTION_ATTACK;
                    } else if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                               Player_CheckTargeting(this) && (sp20 > 0)) {
                        doAction = DO_ACTION_JUMP;
                    } else if ((this->heldItemAction >= PLAYER_IA_SWORD_MASTER) ||
                               ((this->stateFlags2 & PLAYER_STATE2_NAVI_IS_ACTIVE) &&
                                (play->actorCtx.targetCtx.arrowPointedActor == NULL))) {
                        doAction = DO_ACTION_PUTAWAY;
                    }
                }
            }
        }

        if (doAction != DO_ACTION_PUTAWAY) {
            this->putAwayTimer = 20;
        } else if (this->putAwayTimer != 0) {
            doAction = DO_ACTION_NONE;
            this->putAwayTimer--;
        }

        Interface_SetDoAction(play, doAction);

        if (this->stateFlags2 & PLAYER_STATE2_NAVI_REQUESTING_TALK) {
            if (this->targetActor != NULL) {
                Interface_SetNaviCall(play, 0x1E);
            } else {
                Interface_SetNaviCall(play, 0x1D);
            }
            Interface_SetNaviCall(play, 0x1E);
        } else {
            Interface_SetNaviCall(play, 0x1F);
        }
    }
}

/**
 * Updates state related to the Hover Boots.
 * Handles a special case where the Hover Boots are able to activate when standing on certain floor types even if the
 * player is standing on the ground.
 *
 * If the player is not on the ground, regardless of the usage of the Hover Boots, various floor related variables are
 * reset.
 *
 * @return true if not on the ground, false otherwise. Note this is independent of the Hover Boots state.
 */
s32 Player_UpdateHoverBoots(Player* this) {
    s32 canHoverOnGround;

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && (this->hoverBootsTimer != 0)) {
        this->hoverBootsTimer--;
    } else {
        this->hoverBootsTimer = 0;
    }

    canHoverOnGround = (this->currentBoots == PLAYER_BOOTS_HOVER) &&
                       ((this->actor.yDistToWater >= 0.0f) || (Player_GetHurtFloorType(sFloorType) >= 0) ||
                        Player_IsFloorSinkingSand(sFloorType));

    if (canHoverOnGround && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->hoverBootsTimer != 0)) {
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        if (!canHoverOnGround) {
            this->hoverBootsTimer = 19;
        }

        return false;
    } else {
        sFloorType = FLOOR_TYPE_NONE;
        this->floorPitch = this->floorPitchAlt = sFloorShapePitch = 0;

        return true;
    }
}

/**
 * Peforms various tasks related to scene collision.
 *
 * This includes:
 * - Update BgCheckInfo, parameters adjusted due to various state flags
 * - Update floor type, floor property and floor sfx offset
 * - Update conveyor, reverb and light settings according to the current floor poly
 * - Handle exits and voids
 * - Update information relating to the "interact wall"
 * - Update information for ledge climbing
 * - Update hover boots
 * - Calculate floor poly angles
 *
 */
void Player_ProcessSceneCollision(PlayState* play, Player* this) {
    static Vec3f sInteractWallCheckOffset = { 0.0f, 18.0f, 0.0f };
    u8 nextLedgeClimbType = PLAYER_LEDGE_CLIMB_NONE;
    CollisionPoly* floorPoly;
    Vec3f unusedWorldPos;
    f32 float0; // multi-purpose variable, see define names (fake match?)
    f32 float1; // multi-purpose variable, see define names (fake match?)
    f32 ceilingCheckHeight;
    u32 flags;

    sPrevFloorProperty = this->floorProperty;

#define vWallCheckRadius float0
#define vWallCheckHeight float1

    if (this->stateFlags2 & PLAYER_STATE2_CRAWLING) {
        vWallCheckRadius = 10.0f;
        vWallCheckHeight = 15.0f;
        ceilingCheckHeight = 30.0f;
    } else {
        vWallCheckRadius = this->ageProperties->wallCheckRadius;
        vWallCheckHeight = 26.0f;
        ceilingCheckHeight = this->ageProperties->ceilingCheckHeight;
    }

    if (this->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID)) {
        if (this->stateFlags1 & PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID) {
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
            flags = UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if ((this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                   ((this->sceneExitPosY - (s32)this->actor.world.pos.y) >= 100)) {
            flags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                   ((Player_Action_OpenDoor == this->actionFunc) ||
                    (Player_Action_MiniCsMovement == this->actionFunc))) {
            this->actor.bgCheckFlags &= ~(BGCHECKFLAG_WALL | BGCHECKFLAG_PLAYER_WALL_INTERACT);
            flags = UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else {
            flags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
                    UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        }
    } else {
        flags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
                UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
    }

    if (this->stateFlags3 & PLAYER_STATE3_IGNORE_CEILING_FLOOR_AND_WATER) {
        flags &= ~(UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
    }

    if (flags & UPDBGCHECKINFO_FLAG_2) {
        this->stateFlags3 |= PLAYER_STATE3_CHECKING_FLOOR_AND_WATER_COLLISION;
    }

    Math_Vec3f_Copy(&unusedWorldPos, &this->actor.world.pos);

    Actor_UpdateBgCheckInfo(play, &this->actor, vWallCheckHeight, vWallCheckRadius, ceilingCheckHeight, flags);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_CEILING) {
        this->actor.velocity.y = 0.0f;
    }

    sYDistToFloor = this->actor.world.pos.y - this->actor.floorHeight;
    sConveyorSpeed = CONVEYOR_SPEED_DISABLED;
    floorPoly = this->actor.floorPoly;

    if (floorPoly != NULL) {
        this->floorProperty = SurfaceType_GetFloorProperty(&play->colCtx, floorPoly, this->actor.floorBgId);
        this->prevFloorSfxOffset = this->floorSfxOffset;

        if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
            if (this->actor.yDistToWater < 20.0f) {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_WATER_SHALLOW;
            } else {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_WATER_DEEP;
            }
        } else {
            if (this->stateFlags2 & PLAYER_STATE2_FORCE_SAND_FLOOR_SOUND) {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_SAND;
            } else {
                this->floorSfxOffset = SurfaceType_GetSfxOffset(&play->colCtx, floorPoly, this->actor.floorBgId);
            }
        }

        if (this->actor.category == ACTORCAT_PLAYER) {
            Audio_SetCodeReverb(SurfaceType_GetEcho(&play->colCtx, floorPoly, this->actor.floorBgId));

            if (this->actor.floorBgId == BGCHECK_SCENE) {
                Environment_ChangeLightSetting(
                    play, SurfaceType_GetLightSetting(&play->colCtx, floorPoly, this->actor.floorBgId));
            } else {
                DynaPoly_SetPlayerAbove(&play->colCtx, this->actor.floorBgId);
            }
        }

        sConveyorSpeed = SurfaceType_GetConveyorSpeed(&play->colCtx, floorPoly, this->actor.floorBgId);

        if (sConveyorSpeed != CONVEYOR_SPEED_DISABLED) {
            sIsFloorConveyor = SurfaceType_IsFloorConveyor(&play->colCtx, floorPoly, this->actor.floorBgId);

            if ((!sIsFloorConveyor && (this->actor.yDistToWater > 20.0f) &&
                 (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                (sIsFloorConveyor && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
                sConveyorYaw = CONVEYOR_DIRECTION_TO_BINANG(
                    SurfaceType_GetConveyorDirection(&play->colCtx, floorPoly, this->actor.floorBgId));
            } else {
                sConveyorSpeed = CONVEYOR_SPEED_DISABLED;
            }
        }
    }

    Player_HandleExitsAndVoids(play, this, floorPoly, this->actor.floorBgId);

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;

    if (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) {
        CollisionPoly* wallPoly;
        s32 wallBgId;
        s16 yawDiff;
        s32 pad;

        sInteractWallCheckOffset.y = 18.0f;
        sInteractWallCheckOffset.z = this->ageProperties->wallCheckRadius + 10.0f;

        if (!(this->stateFlags2 & PLAYER_STATE2_CRAWLING) &&
            Player_PosVsWallLineTest(play, this, &sInteractWallCheckOffset, &wallPoly, &wallBgId,
                                     &sInteractWallCheckResult)) {
            this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;

            if (this->actor.wallPoly != wallPoly) {
                this->actor.wallPoly = wallPoly;
                this->actor.wallBgId = wallBgId;
                this->actor.wallYaw = Math_Atan2S(wallPoly->normal.z, wallPoly->normal.x);
            }
        }

        yawDiff = this->actor.shape.rot.y - (s16)(this->actor.wallYaw + 0x8000);
        sTouchedWallFlags = SurfaceType_GetWallFlags(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId);
        sShapeYawToTouchedWall = ABS(yawDiff);

        yawDiff = this->yaw - (s16)(this->actor.wallYaw + 0x8000);
        sWorldYawToTouchedWall = ABS(yawDiff);

#define vSpeedScale float0
#define vSpeedLimit float1

        vSpeedScale = sWorldYawToTouchedWall * 0.00008f;

        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || vSpeedScale >= 1.0f) {
            this->speedLimit = R_RUN_SPEED_LIMIT / 100.0f;
        } else {
            vSpeedLimit = (R_RUN_SPEED_LIMIT / 100.0f * vSpeedScale);
            this->speedLimit = vSpeedLimit;

            if (vSpeedLimit < 0.1f) {
                this->speedLimit = 0.1f;
            }
        }

        if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sShapeYawToTouchedWall < 0x3000)) {
            CollisionPoly* wallPoly = this->actor.wallPoly;

            if (ABS(wallPoly->normal.y) < 600) {
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalY = COLPOLY_GET_NORMAL(wallPoly->normal.y);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 yDistToLedge;
                CollisionPoly* ledgeFloorPoly;
                CollisionPoly* poly;
                s32 bgId;
                Vec3f ledgeCheckPos;
                f32 ledgePosY;
                f32 ceillingPosY;
                s32 wallYawDiff;

                this->distToInteractWall = Math3D_UDistPlaneToPos(wallPolyNormalX, wallPolyNormalY, wallPolyNormalZ,
                                                                  wallPoly->dist, &this->actor.world.pos);

#define vLedgeCheckOffsetXZ float0

                vLedgeCheckOffsetXZ = this->distToInteractWall + 10.0f;

                ledgeCheckPos.x = this->actor.world.pos.x - (vLedgeCheckOffsetXZ * wallPolyNormalX);
                ledgeCheckPos.z = this->actor.world.pos.z - (vLedgeCheckOffsetXZ * wallPolyNormalZ);
                ledgeCheckPos.y = this->actor.world.pos.y + this->ageProperties->unk_0C;

                ledgePosY = BgCheck_EntityRaycastDown1(&play->colCtx, &ledgeFloorPoly, &ledgeCheckPos);
                yDistToLedge = ledgePosY - this->actor.world.pos.y;
                this->yDistToLedge = yDistToLedge;

                if ((this->yDistToLedge < 18.0f) ||
                    BgCheck_EntityCheckCeiling(&play->colCtx, &ceillingPosY, &this->actor.world.pos,
                                               (ledgePosY - this->actor.world.pos.y) + 20.0f, &poly, &bgId,
                                               &this->actor)) {
                    this->yDistToLedge = LEDGE_DIST_MAX;
                } else {
                    sInteractWallCheckOffset.y = (ledgePosY + 5.0f) - this->actor.world.pos.y;

                    if (Player_PosVsWallLineTest(play, this, &sInteractWallCheckOffset, &poly, &bgId,
                                                 &sInteractWallCheckResult) &&
                        (wallYawDiff = this->actor.wallYaw - Math_Atan2S(poly->normal.z, poly->normal.x),
                         ABS(wallYawDiff) < 0x4000) &&
                        !SurfaceType_CheckWallFlag1(&play->colCtx, poly, bgId)) {
                        this->yDistToLedge = LEDGE_DIST_MAX;
                    } else if (SurfaceType_CheckWallFlag0(&play->colCtx, wallPoly, this->actor.wallBgId) == 0) {
                        if (this->ageProperties->unk_1C <= this->yDistToLedge) {
                            if (ABS(ledgeFloorPoly->normal.y) > 0x6D60) {
                                if (this->ageProperties->unk_14 <= this->yDistToLedge) {
                                    nextLedgeClimbType = PLAYER_LEDGE_CLIMB_4;
                                } else if (this->ageProperties->unk_18 <= this->yDistToLedge) {
                                    nextLedgeClimbType = PLAYER_LEDGE_CLIMB_3;
                                } else {
                                    nextLedgeClimbType = PLAYER_LEDGE_CLIMB_2;
                                }
                            }
                        } else {
                            nextLedgeClimbType = PLAYER_LEDGE_CLIMB_1;
                        }
                    }
                }
            }
        }
    } else {
        this->speedLimit = R_RUN_SPEED_LIMIT / 100.0f;
        this->ledgeClimbDelayTimer = 0;
        this->yDistToLedge = 0.0f;
    }

    if (nextLedgeClimbType == this->ledgeClimbType) {
        if ((this->speedXZ != 0.0f) && (this->ledgeClimbDelayTimer < 100)) {
            this->ledgeClimbDelayTimer++;
        }
    } else {
        this->ledgeClimbType = nextLedgeClimbType;
        this->ledgeClimbDelayTimer = 0;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        sFloorType = SurfaceType_GetFloorType(&play->colCtx, floorPoly, this->actor.floorBgId);

        if (!Player_UpdateHoverBoots(this)) {
            f32 floorPolyNormalX;
            f32 invFloorPolyNormalY;
            f32 floorPolyNormalZ;
            f32 sin;
            s32 pad2;
            f32 cos;
            s32 pad3;

            if (this->actor.floorBgId != BGCHECK_SCENE) {
                DynaPoly_SetPlayerOnTop(&play->colCtx, this->actor.floorBgId);
            }

            floorPolyNormalX = COLPOLY_GET_NORMAL(floorPoly->normal.x);
            invFloorPolyNormalY = 1.0f / COLPOLY_GET_NORMAL(floorPoly->normal.y);
            floorPolyNormalZ = COLPOLY_GET_NORMAL(floorPoly->normal.z);

            sin = Math_SinS(this->yaw);
            cos = Math_CosS(this->yaw);

            this->floorPitch =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sin) - (floorPolyNormalZ * cos)) * invFloorPolyNormalY);
            this->floorPitchAlt =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * cos) - (floorPolyNormalZ * sin)) * invFloorPolyNormalY);

            sin = Math_SinS(this->actor.shape.rot.y);
            cos = Math_CosS(this->actor.shape.rot.y);

            sFloorShapePitch =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sin) - (floorPolyNormalZ * cos)) * invFloorPolyNormalY);

            Player_HandleSlopes(play, this, floorPoly);
        }
    } else {
        Player_UpdateHoverBoots(this);
    }

    if (this->prevFloorType == sFloorType) {
        this->floorTypeTimer++;
    } else {
        this->prevFloorType = sFloorType;
        this->floorTypeTimer = 0;
    }
}

void Player_UpdateCamAndSeqModes(PlayState* play, Player* this) {
    u8 seqMode;
    s32 pad;
    Actor* targetActor;
    s32 camMode;

    if (this->actor.category == ACTORCAT_PLAYER) {
        seqMode = SEQ_MODE_DEFAULT;

        if (this->csAction != PLAYER_CSACTION_NONE) {
            Camera_RequestMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_NORMAL);
        } else if (!(this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
            if ((this->actor.parent != NULL) && (this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
                camMode = CAM_MODE_HOOKSHOT_FLY;
                Camera_SetViewParam(Play_GetCamera(play, CAM_ID_MAIN), CAM_VIEW_TARGET, this->actor.parent);
            } else if (Player_Action_Knockback == this->actionFunc) {
                camMode = CAM_MODE_STILL;
            } else if (this->stateFlags3 & PLAYER_STATE3_ZORA_SWIMMING &&
                       !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                camMode = CAM_MODE_FREE_FALL;
            } else if (this->stateFlags2 & PLAYER_STATE2_ENABLE_PUSH_PULL_CAM) {
                camMode = CAM_MODE_PUSH_PULL;
            } else if ((targetActor = this->targetActor) != NULL) {
                if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_TALK)) {
                    camMode = CAM_MODE_TALK;
                } else if (this->stateFlags1 & PLAYER_STATE1_Z_LOCK_ON) {
                    if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
                        camMode = CAM_MODE_FOLLOW_BOOMERANG;
                    } else {
                        camMode = CAM_MODE_Z_TARGET_FRIENDLY;
                    }
                } else {
                    camMode = CAM_MODE_Z_TARGET_UNFRIENDLY;
                }
                Camera_SetViewParam(Play_GetCamera(play, CAM_ID_MAIN), CAM_VIEW_TARGET, targetActor);
            } else if (this->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK) {
                camMode = CAM_MODE_CHARGE;
            } else if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
                camMode = CAM_MODE_FOLLOW_BOOMERANG;
                Camera_SetViewParam(Play_GetCamera(play, CAM_ID_MAIN), CAM_VIEW_TARGET, this->boomerangActor);
            } else if (this->stateFlags1 &
                       (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE)) {
                if (Player_CheckCalmTargeting(this)) {
                    camMode = CAM_MODE_Z_LEDGE_HANG;
                } else {
                    camMode = CAM_MODE_LEDGE_HANG;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_Z_PARALLEL | PLAYER_STATE1_TARGET_ACTOR_LOST)) {
                if (Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) {
                    camMode = CAM_MODE_Z_AIM;
                } else if (this->stateFlags1 & PLAYER_STATE1_CLIMBING) {
                    camMode = CAM_MODE_Z_WALL_CLIMB;
                } else {
                    camMode = CAM_MODE_Z_PARALLEL;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_JUMPING | PLAYER_STATE1_CLIMBING)) {
                if ((Player_Action_JumpUpToLedge == this->actionFunc) || (this->stateFlags1 & PLAYER_STATE1_CLIMBING)) {
                    camMode = CAM_MODE_WALL_CLIMB;
                } else {
                    camMode = CAM_MODE_JUMP;
                }
            } else if (this->stateFlags1 & PLAYER_STATE1_FREEFALLING) {
                camMode = CAM_MODE_FREE_FALL;
            } else if ((this->meleeWeaponState != 0) &&
                       (this->meleeAttackType >= PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H) &&
                       (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H)) {
                camMode = CAM_MODE_STILL;
            } else {
                camMode = CAM_MODE_NORMAL;
                if ((this->speedXZ == 0.0f) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) || (this->rideActor->speed == 0.0f))) {
                    // not moving
                    seqMode = SEQ_MODE_STILL;
                }
            }

            Camera_RequestMode(Play_GetCamera(play, CAM_ID_MAIN), camMode);
        } else {
            // First person mode
            seqMode = SEQ_MODE_STILL;
        }

        if (play->actorCtx.targetCtx.bgmEnemy != NULL) {
            seqMode = SEQ_MODE_ENEMY;
            Audio_SetBgmEnemyVolume(sqrtf(play->actorCtx.targetCtx.bgmEnemy->xyzDistToPlayerSq));
        }

        if (play->sceneId != SCENE_FISHING_POND) {
            Audio_SetSequenceMode(seqMode);
        }
    }
}

static Vec3f sStickFlameVelocity = { 0.0f, 0.5f, 0.0f };
static Vec3f sStickFlameAccel = { 0.0f, 0.5f, 0.0f };

static Color_RGBA8 sStickFlamePrimColor = { 255, 255, 100, 255 };
static Color_RGBA8 sStickFlameEnvColor = { 255, 50, 0, 0 };

void Player_UpdateDekuStick(PlayState* play, Player* this) {
    f32 newDekuStickLength;

    if (this->unk_85C == 0.0f) {
        Player_UseItem(play, this, 0xFF);
        return;
    }

    newDekuStickLength = 1.0f;
    if (DECR(this->unk_860) == 0) {
        Inventory_ChangeAmmo(ITEM_DEKU_STICK, -1);
        this->unk_860 = 1;
        newDekuStickLength = 0.0f;
        this->unk_85C = newDekuStickLength;
    } else if (this->unk_860 > 200) {
        newDekuStickLength = (210 - this->unk_860) / 10.0f;
    } else if (this->unk_860 < 20) {
        newDekuStickLength = this->unk_860 / 20.0f;
        this->unk_85C = newDekuStickLength;
    }

    func_8002836C(play, &this->meleeWeaponInfo[0].tip, &sStickFlameVelocity, &sStickFlameAccel, &sStickFlamePrimColor,
                  &sStickFlameEnvColor, newDekuStickLength * 200.0f, 0, 8);
}

void Player_ElectricShock(PlayState* play, Player* this) {
    Vec3f shockPos;
    Vec3f* randBodyPart;
    s32 shockScale;

    this->shockTimer--;
    this->unk_892 += this->shockTimer;

    if (this->unk_892 > 20) {
        shockScale = this->shockTimer * 2;
        this->unk_892 -= 20;

        if (shockScale > 40) {
            shockScale = 40;
        }

        randBodyPart = this->bodyPartsPos + (s32)Rand_ZeroFloat(PLAYER_BODYPART_MAX - 0.1f);
        shockPos.x = (Rand_CenteredFloat(5.0f) + randBodyPart->x) - this->actor.world.pos.x;
        shockPos.y = (Rand_CenteredFloat(5.0f) + randBodyPart->y) - this->actor.world.pos.y;
        shockPos.z = (Rand_CenteredFloat(5.0f) + randBodyPart->z) - this->actor.world.pos.z;

        EffectSsFhgFlash_SpawnShock(play, &this->actor, &shockPos, shockScale, FHGFLASH_SHOCK_PLAYER);
        func_8002F8F0(&this->actor, NA_SE_PL_SPARK - SFX_FLAG);
    }
}

void Player_Burning(PlayState* play, Player* this) {
    s32 spawnedFlame;
    u8* timerPtr;
    s32 timerStep;
    f32 flameScale;
    f32 flameIntensity;
    s32 dmgCooldown;
    s32 i;
    s32 timerExtraStep;
    s32 timerBaseStep;

    if (this->currentTunic == PLAYER_TUNIC_GORON) {
        timerBaseStep = 20;
    } else {
        timerBaseStep = (s32)(this->speedXZ * 0.4f) + 1;
    }

    spawnedFlame = false;
    timerPtr = this->flameTimers;

    if (this->stateFlags2 & PLAYER_STATE2_MAKING_REACTABLE_NOISE) {
        timerExtraStep = 100;
    } else {
        timerExtraStep = 0;
    }

    Player_BurnDekuShield(this, play);

    for (i = 0; i < PLAYER_BODYPART_MAX; i++, timerPtr++) {
        timerStep = timerExtraStep + timerBaseStep;

        if (*timerPtr <= timerStep) {
            *timerPtr = 0;
        } else {
            spawnedFlame = true;
            *timerPtr -= timerStep;

            if (*timerPtr > 20.0f) {
                flameIntensity = (*timerPtr - 20.0f) * 0.01f;
                flameScale = CLAMP(flameIntensity, 0.19999999f, 0.2f);
            } else {
                flameScale = *timerPtr * 0.01f;
            }

            flameIntensity = (*timerPtr - 25.0f) * 0.02f;
            flameIntensity = CLAMP(flameIntensity, 0.0f, 1.0f);
            EffectSsFireTail_SpawnFlameOnPlayer(play, flameScale, i, flameIntensity);
        }
    }

    if (spawnedFlame) {
        Player_PlaySfx(this, NA_SE_EV_TORCH - SFX_FLAG);

        if (play->sceneId == SCENE_SPIRIT_TEMPLE_BOSS) {
            dmgCooldown = 0;
        } else {
            dmgCooldown = 7;
        }

        if ((dmgCooldown & play->gameplayFrames) == 0) {
            Player_InflictDamageAndCheckForDeath(play, -1);
        }
    } else {
        this->isBurning = false;
    }
}

void Player_RumbleNearSecret(Player* this) {
    if (CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) {
        f32 temp = 200000.0f - (this->closestSecretDistSq * 5.0f);

        if (temp < 0.0f) {
            temp = 0.0f;
        }

        this->secretRumbleTimer += temp;
        if (this->secretRumbleTimer > 4000000.0f) {
            this->secretRumbleTimer = 0.0f;
            Player_RequestRumble(this, 120, 20, 10, 0);
        }
    }
}

// Cues to CS actions
static s8 sCueToCsActionMap[PLAYER_CUEID_MAX] = {
    PLAYER_CSACTION_NONE,                                      // PLAYER_CUEID_NONE
    PLAYER_CSACTION_3,                                         // PLAYER_CUEID_1
    PLAYER_CSACTION_3,                                         // PLAYER_CUEID_2
    PLAYER_CSACTION_SURPRISED,                                 // PLAYER_CUEID_3
    PLAYER_CSACTION_4,                                         // PLAYER_CUEID_4
    PLAYER_CSACTION_WAIT,                                      // PLAYER_CUEID_5
    PLAYER_CSACTION_TURN_AROUND_SURPRISED_LONG,                // PLAYER_CUEID_6
    PLAYER_CSACTION_START_GET_SPIRITUAL_STONE,                 // PLAYER_CUEID_7
    PLAYER_CSACTION_GET_SPIRITUAL_STONE,                       // PLAYER_CUEID_8
    PLAYER_CSACTION_END_GET_SPIRITUAL_STONE,                   // PLAYER_CUEID_9
    PLAYER_CSACTION_GET_UP_FROM_DEKU_TREE_STORY,               // PLAYER_CUEID_10
    PLAYER_CSACTION_SIT_LISTENING_TO_DEKU_TREE_STORY,          // PLAYER_CUEID_11
    PLAYER_CSACTION_SWORD_INTO_PEDESTAL,                       // PLAYER_CUEID_12
    -PLAYER_CSACTION_WARP_TO_SAGES,                            // PLAYER_CUEID_13
    PLAYER_CSACTION_LOOK_AT_SELF,                              // PLAYER_CUEID_14
    PLAYER_CSACTION_KNOCKED_TO_GROUND,                         // PLAYER_CUEID_15
    PLAYER_CSACTION_GET_UP_FROM_GROUND,                        // PLAYER_CUEID_16
    PLAYER_CSACTION_START_PLAY_OCARINA,                        // PLAYER_CUEID_17
    PLAYER_CSACTION_END_PLAY_OCARINA,                          // PLAYER_CUEID_18
    PLAYER_CSACTION_GET_ITEM,                                  // PLAYER_CUEID_19
    PLAYER_CSACTION_IDLE_2,                                    // PLAYER_CUEID_20
    PLAYER_CSACTION_CLOSE_EYES,                                // PLAYER_CUEID_21
    PLAYER_CSACTION_OPEN_EYES,                                 // PLAYER_CUEID_22
    PLAYER_CSACTION_SURPRIED_STUMBLE_BACK_AND_FALL,            // PLAYER_CUEID_23
    PLAYER_CSACTION_SURFACE_FROM_DIVE,                         // PLAYER_CUEID_24
    -PLAYER_CSACTION_GET_ITEM_IN_WATER,                        // PLAYER_CUEID_25
    PLAYER_CSACTION_DRAW_AND_BRANDISH_SWORD,                   // PLAYER_CUEID_26
    PLAYER_CSACTION_GENTLE_KNOCKBACK_INTO_SIT,                 // PLAYER_CUEID_27
    PLAYER_CSACTION_SLEEPING_RESTLESS,                         // PLAYER_CUEID_28
    -PLAYER_CSACTION_SLEEPING,                                 // PLAYER_CUEID_29
    -PLAYER_CSACTION_AWAKEN,                                   // PLAYER_CUEID_30
    -PLAYER_CSACTION_GET_OFF_BED,                              // PLAYER_CUEID_31
    PLAYER_CSACTION_BLOWN_BACKWARD,                            // PLAYER_CUEID_32
    PLAYER_CSACTION_STAND_UP_AND_WATCH,                        // PLAYER_CUEID_33
    PLAYER_CSACTION_STOP,                                      // PLAYER_CUEID_34
    PLAYER_CSACTION_STOP_2,                                    // PLAYER_CUEID_35
    PLAYER_CSACTION_NONE,                                      // PLAYER_CUEID_36
    PLAYER_CSACTION_NONE,                                      // PLAYER_CUEID_37
    PLAYER_CSACTION_NONE,                                      // PLAYER_CUEID_38
    PLAYER_CSACTION_START_LOOK_AROUND_AFTER_SWORD_WARP,        // PLAYER_CUEID_39
    PLAYER_CSACTION_STEP_BACK_CAUTIOUSLY,                      // PLAYER_CUEID_40
    PLAYER_CSACTION_LOOK_THROUGH_PEEPHOLE,                     // PLAYER_CUEID_41
    -PLAYER_CSACTION_DRAW_SWORD_CHILD,                         // PLAYER_CUEID_42
    PLAYER_CSACTION_JUMP_TO_ZELDAS_CRYSTAL,                    // PLAYER_CUEID_43
    -PLAYER_CSACTION_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL,      // PLAYER_CUEID_44
    -PLAYER_CSACTION_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING,      // PLAYER_CUEID_45
    PLAYER_CSACTION_TURN_AROUND_SLOWLY,                        // PLAYER_CUEID_46
    PLAYER_CSACTION_END_SHIELD_EYES_WITH_HAND,                 // PLAYER_CUEID_47
    PLAYER_CSACTION_SHIELD_EYES_WITH_HAND,                     // PLAYER_CUEID_48
    PLAYER_CSACTION_LOOK_AROUND_SURPRISED,                     // PLAYER_CUEID_49
    PLAYER_CSACTION_INSPECT_GROUND_CAREFULLY,                  // PLAYER_CUEID_50
    PLAYER_CSACTION_STARTLED_BY_GORONS_FALLING,                // PLAYER_CUEID_51
    PLAYER_CSACTION_FALL_TO_KNEE,                              // PLAYER_CUEID_52
    PLAYER_CSACTION_FLAT_ON_BACK,                              // PLAYER_CUEID_53
    PLAYER_CSACTION_RAISE_FROM_FLAT_ON_BACK,                   // PLAYER_CUEID_54
    PLAYER_CSACTION_START_SPIN_ATTACK,                         // PLAYER_CUEID_55
    PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_IDLE,                // PLAYER_CUEID_56
    -PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA, // PLAYER_CUEID_57
    -PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA,   // PLAYER_CUEID_58
    PLAYER_CSACTION_END_LOOK_AROUND_AFTER_SWORD_WARP,          // PLAYER_CUEID_59
    PLAYER_CSACTION_RAISED_BY_WARP,                            // PLAYER_CUEID_60
    PLAYER_CSACTION_LOOK_AROUND_AND_AT_SELF_QUICKLY,           // PLAYER_CUEID_61
    PLAYER_CSACTION_START_LEARN_OCARINA_SONG_ADULT,            // PLAYER_CUEID_62
    PLAYER_CSACTION_END_LEARN_OCARINA_SONG_ADULT,              // PLAYER_CUEID_63
    PLAYER_CSACTION_WAIT,                                      // PLAYER_CUEID_64
    PLAYER_CSACTION_WAIT,                                      // PLAYER_CUEID_65
    PLAYER_CSACTION_START_LEARN_OCARINA_SONG_CHILD,            // PLAYER_CUEID_66
    PLAYER_CSACTION_END_LEARN_OCARINA_SONG_CHILD,              // PLAYER_CUEID_67
    PLAYER_CSACTION_LOOK_TO_CHARACTER_AT_SIDE_SMILING,         // PLAYER_CUEID_68
    PLAYER_CSACTION_LOOK_TO_CHARACTER_ABOVE_SMILING,           // PLAYER_CUEID_69
    PLAYER_CSACTION_SURPRISED_DEFENSE,                         // PLAYER_CUEID_70
    PLAYER_CSACTION_SPIN_ATTACK_IDLE,                          // PLAYER_CUEID_71
    PLAYER_CSACTION_INSPECT_WEAPON,                            // PLAYER_CUEID_72
    PLAYER_CSACTION_91,                                        // PLAYER_CUEID_73
    PLAYER_CSACTION_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT,      // PLAYER_CUEID_74
    PLAYER_CSACTION_LOOK_UP_STARTLED,                          // PLAYER_CUEID_75
    PLAYER_CSACTION_REACT_TO_QUAKE,                            // PLAYER_CUEID_76
    PLAYER_CSACTION_GET_SWORD_BACK,                            // PLAYER_CUEID_77
};

static Vec3f sHorseRaycastOffset = { 0.0f, 0.0f, 200.0f };

static f32 sWaterConveyorSpeeds[CONVEYOR_SPEED_MAX - 1] = {
    2.0f, // CONVEYOR_SPEED_SLOW
    4.0f, // CONVEYOR_SPEED_MEDIUM
    7.0f, // CONVEYOR_SPEED_FAST
};
static f32 sFloorConveyorSpeeds[CONVEYOR_SPEED_MAX - 1] = {
    0.5f, // CONVEYOR_SPEED_SLOW
    1.0f, // CONVEYOR_SPEED_MEDIUM
    3.0f, // CONVEYOR_SPEED_FAST
};

void Player_UpdateCommon(Player* this, PlayState* play, Input* input) {
    s32 pad;

    sControlInput = input;

    if (this->voidRespawnCounter < 0) {
        this->voidRespawnCounter++;
        if (this->voidRespawnCounter == 0) {
            this->voidRespawnCounter = 1;
            Sfx_PlaySfxCentered(NA_SE_OC_REVENGE);
        }
    }

    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.home.pos);

    if (this->fpsItemShotTimer != 0) {
        this->fpsItemShotTimer--;
    }

    if (this->endTalkTimer != 0) {
        this->endTalkTimer--;
    }

    if (this->deathTimer != 0) {
        this->deathTimer--;
    }

    if (this->invincibilityTimer < 0) {
        this->invincibilityTimer++;
    } else if (this->invincibilityTimer > 0) {
        this->invincibilityTimer--;
    }

    if (this->runDamageTimer != 0) {
        this->runDamageTimer--;
    }

    Player_SetDoActionText(play, this);
    Player_ProcessTargeting(this, play);

    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) && (this->unk_860 != 0)) {
        Player_UpdateDekuStick(play, this);
    } else if ((this->heldItemAction == PLAYER_IA_FISHING_POLE) && (this->unk_860 < 0)) {
        this->unk_860++;
    }

    if (this->shockTimer != 0) {
        Player_ElectricShock(play, this);
    }

    if (this->isBurning) {
        Player_Burning(play, this);
    }

    if (this->stateFlags3 & PLAYER_STATE3_USING_BOOSTERS) {
        Player_SpawnBoosterEffects(play, this);
    }

    if ((this->stateFlags3 & PLAYER_STATE3_RESTORE_NAYRUS_LOVE) && (gSaveContext.nayrusLoveTimer != 0) &&
        (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpell(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }

    if (this->stateFlags2 & PLAYER_STATE2_PAUSE_MOST_UPDATING) {
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_ZeroSpeedXZ(this);
            Actor_MoveXZGravity(&this->actor);
        }

        Player_ProcessSceneCollision(play, this);
    } else {
        f32 temp_f0;
        f32 phi_f12;

        if (this->currentBoots == PLAYER_BOOTS_ZORA) {
            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_B) && (((this->actor.yDistToWater - this->ageProperties->unk_2C) > 0.0f) || Player_IsSwimming(this))) {
                this->zoraDescendFlag ^= 1;
            }
        }

        if (this->currentBoots != this->prevBoots) {
            if (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    Player_ResetSubCam(play, this);
                    if (this->ageProperties->unk_2C < this->actor.yDistToWater) {
                        this->stateFlags2 |= PLAYER_STATE2_DIVING;
                    }
                }
            } else {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    if ((this->prevBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                        Player_EnterWater(play, this);
                        this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
                    }
                }
            }

            this->prevBoots = this->currentBoots;
        }

        if ((this->actor.parent == NULL) && (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
            this->actor.parent = this->rideActor;
            Player_SetupRidingHorse(play, this);
            this->stateFlags1 |= PLAYER_STATE1_RIDING_HORSE;
            Player_AnimPlayOnce(play, this, &gPlayerAnim_link_uma_wait_1);
            Player_AnimReplaceApplyFlags(play, this,
                                         ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_SETMOVE |
                                             ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
            this->av2.actionVar2 = 99;
        }

        if (this->comboTimer == 0) {
            this->slashCounter = 0;
        } else if (this->comboTimer < 0) {
            this->comboTimer++;
        } else {
            this->comboTimer--;
        }

        Math_ScaledStepToS(&this->shapePitchOffset, 0, 400);
        func_80032CB4(this->unk_3A8, 20, 80, 6);

        this->actor.shape.face = this->unk_3A8[0] + ((play->gameplayFrames & 32) ? 0 : 3);

        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Player_UpdateBunnyEars(this);
        }

        if (Actor_PlayerIsAimingFpsItem(this) != 0) {
            Player_BowStringPhysics(this);
        }

        if (!(this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_7)) {
            if (((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (sFloorType == FLOOR_TYPE_SLIPPERY_ICE) &&
                 (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                ((this->currentBoots == PLAYER_BOOTS_HOVER) &&
                 !(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)))) {
                f32 speedXZTarget = this->speedXZ;
                s16 yawTarget = this->yaw;
                s16 yawDiff = this->actor.world.rot.y - yawTarget;
                s32 pad;

                if ((ABS(yawDiff) > 0x6000) && (this->actor.speed != 0.0f)) {
                    speedXZTarget = 0.0f;
                    yawTarget += 0x8000;
                }

                if (Math_StepToF(&this->actor.speed, speedXZTarget, 0.35f) && (speedXZTarget == 0.0f)) {
                    this->actor.world.rot.y = this->yaw;
                }

                if (this->speedXZ != 0.0f) {
                    s32 step;

                    step = (fabsf(this->speedXZ) * 700.0f) - (fabsf(this->actor.speed) * 100.0f);
                    step = CLAMP(step, 0, 1350);

                    Math_ScaledStepToS(&this->actor.world.rot.y, yawTarget, step);
                }

                if ((this->speedXZ == 0.0f) && (this->actor.speed != 0.0f)) {
                    func_800F4138(&this->actor.projectedPos, NA_SE_PL_SLIP_LEVEL - SFX_FLAG, this->actor.speed);
                }
            } else {
                this->actor.speed = this->speedXZ;
                this->actor.world.rot.y = this->yaw;
            }

            Actor_UpdateVelocityXZGravity(&this->actor);

            if ((this->pushedSpeed != 0.0f) && !Player_InCsMode(play) &&
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                       PLAYER_STATE1_CLIMBING)) &&
                (Player_Action_JumpUpToLedge != this->actionFunc) && (Player_Action_MagicSpell != this->actionFunc)) {
                this->actor.velocity.x += this->pushedSpeed * Math_SinS(this->pushedYaw);
                this->actor.velocity.z += this->pushedSpeed * Math_CosS(this->pushedYaw);
            }

            Actor_UpdatePos(&this->actor);
            Player_ProcessSceneCollision(play, this);
        } else {
            sFloorType = FLOOR_TYPE_NONE;
            this->floorProperty = FLOOR_PROPERTY_0;

            if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                EnHorse* rideActor = (EnHorse*)this->rideActor;
                CollisionPoly* floorPoly;
                s32 bgId;
                Vec3f raycastPos;

                if (!(rideActor->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_RaycastFloorGetPolyInfo(play, this, &sHorseRaycastOffset, &raycastPos, &floorPoly, &bgId);
                } else {
                    floorPoly = rideActor->actor.floorPoly;
                    bgId = rideActor->actor.floorBgId;
                }

                if ((floorPoly != NULL) && Player_HandleExitsAndVoids(play, this, floorPoly, bgId)) {
                    if (DREG(25) != 0) {
                        DREG(25) = 0;
                    } else {
                        AREG(6) = 1;
                    }
                }
            }

            sConveyorSpeed = CONVEYOR_SPEED_DISABLED;
            this->pushedSpeed = 0.0f;
        }

        // This block applies the bg conveyor to pushedSpeed
        if ((sConveyorSpeed != CONVEYOR_SPEED_DISABLED) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
            f32 conveyorSpeed;

            // converts 1-index to 0-index
            sConveyorSpeed--;

            if (!sIsFloorConveyor) {
                conveyorSpeed = sWaterConveyorSpeeds[sConveyorSpeed];

                if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                    conveyorSpeed *= 0.25f;
                }
            } else {
                conveyorSpeed = sFloorConveyorSpeeds[sConveyorSpeed];
            }

            Math_StepToF(&this->pushedSpeed, conveyorSpeed, conveyorSpeed * 0.1f);

            Math_ScaledStepToS(&this->pushedYaw, sConveyorYaw,
                               ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) ? 400.0f : 800.0f) * conveyorSpeed);
        } else if (this->pushedSpeed != 0.0f) {
            Math_StepToF(&this->pushedSpeed, 0.0f, (this->stateFlags1 & PLAYER_STATE1_SWIMMING) ? 0.5f : 1.0f);
        }

        if (!Player_InBlockingCsMode(play, this) && !(this->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
            Player_CheckForWater(play, this);

            if ((this->actor.category == ACTORCAT_PLAYER) && (gSaveContext.save.info.playerData.health == 0)) {
                if (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                         PLAYER_STATE1_CLIMBING)) {
                    Player_ResetAttributes(play, this);
                    Player_SetupFallFromLedge(this, play);
                } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                           (this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                    Player_SetupDie(play, this,
                                    Player_IsSwimming(this)   ? &gPlayerAnim_link_swimer_swim_down
                                    : (this->shockTimer != 0) ? &gPlayerAnim_link_normal_electric_shock_end
                                                              : &gPlayerAnim_link_derth_rebirth);
                }
            } else {
                if ((this->actor.parent == NULL) && ((play->transitionTrigger == TRANS_TRIGGER_START) ||
                                                     (this->deathTimer != 0) || !Player_CheckDamage(this, play))) {
                    Player_SetupMidairBehavior(this, play);
                } else {
                    this->fallStartHeight = this->actor.world.pos.y;
                }
                Player_RumbleNearSecret(this);
            }
        }

        if ((play->csCtx.state != CS_STATE_IDLE) && (this->csAction != PLAYER_CSACTION_6) &&
            !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
            !(this->stateFlags2 & PLAYER_STATE2_RESTRAINED_BY_ENEMY) && (this->actor.category == ACTORCAT_PLAYER)) {
            CsCmdActorCue* cue = play->csCtx.playerCue;

            if ((cue != NULL) && (sCueToCsActionMap[cue->id] != PLAYER_CSACTION_NONE)) {
                Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_6);
                Player_ZeroSpeedXZ(this);
            } else if ((this->csAction == PLAYER_CSACTION_NONE) && !(this->stateFlags2 & PLAYER_STATE2_DIVING) &&
                       (play->csCtx.state != CS_STATE_STOP)) {
                Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_IDLE_4);
                Player_ZeroSpeedXZ(this);
            }
        }

        if (this->csAction != PLAYER_CSACTION_NONE) {
            if ((this->csAction != PLAYER_CSACTION_END) ||
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                       PLAYER_STATE1_CLIMBING | PLAYER_STATE1_TAKING_DAMAGE))) {
                this->attentionMode = 3;
            } else if (Player_Action_CsAction != this->actionFunc) {
                Player_Cutscene_Finish(play, this, NULL);
            }
        } else {
            this->prevCsAction = PLAYER_CSACTION_NONE;
        }

        Player_ProcessEnvironmentPhysics(play, this);

        if ((this->targetActor == NULL) && (this->naviTextId == 0)) {
            this->stateFlags2 &= ~(PLAYER_STATE2_CAN_SPEAK_OR_CHECK | PLAYER_STATE2_NAVI_REQUESTING_TALK);
        }

        this->stateFlags1 &= ~(PLAYER_STATE1_SWINGING_BOTTLE | PLAYER_STATE1_READY_TO_SHOOT |
                               PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_DEFENDING);
        this->stateFlags2 &=
            ~(PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL |
              PLAYER_STATE2_MAKING_REACTABLE_NOISE | PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING |
              PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION | PLAYER_STATE2_ENABLE_PUSH_PULL_CAM |
              PLAYER_STATE2_FORCE_SAND_FLOOR_SOUND | PLAYER_STATE2_IDLE_WHILE_CLIMBING | PLAYER_STATE2_FROZEN_IN_ICE |
              PLAYER_STATE2_DO_ACTION_ENTER | PLAYER_STATE2_CAN_DISMOUNT_HORSE | PLAYER_STATE2_ENABLE_REFLECTION);
        this->stateFlags3 &= ~PLAYER_STATE3_CHECKING_FLOOR_AND_WATER_COLLISION;

        Player_StepLookToZero(this);
        Player_ProcessControlStick(play, this);

        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            sWaterSpeedScale = 0.5f;
        } else {
            sWaterSpeedScale = 1.0f;
        }

        sInvertedWaterSpeedScale = 1.0f / sWaterSpeedScale;
        sUseHeldItem = sHeldItemButtonIsHeldDown = 0;
        sCurrentMask = this->currentMask;

        if (!(this->stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC)) {
            this->actionFunc(this, play);
        }

        Player_UpdateCamAndSeqModes(play, this);

        if (this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_SETMOVE) {
            AnimationContext_SetMoveActor(
                play, &this->actor, &this->skelAnime,
                (this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_2) ? 1.0f : this->ageProperties->unk_08);
        }

        Player_UpdateYaw(this, play);

        if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_TALK)) {
            this->talkActorDistance = 0.0f;
        } else {
            this->talkActor = NULL;
            this->talkActorDistance = MAXFLOAT;
            this->exchangeItemId = EXCH_ITEM_NONE;
        }

        if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            this->interactRangeActor = NULL;
            this->getItemDirection = 0x6000;
        }

        if (this->actor.parent == NULL) {
            this->rideActor = NULL;
        }

        this->naviTextId = 0;

        if (!(this->stateFlags2 & PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR)) {
            this->ocarinaActor = NULL;
        }

        this->stateFlags2 &= ~PLAYER_STATE2_NEAR_OCARINA_ACTOR;
        this->closestSecretDistSq = MAXFLOAT;

        temp_f0 = this->actor.world.pos.y - this->actor.prevPos.y;

        this->doorType = PLAYER_DOORTYPE_NONE;
        this->damageEffect = 0;
        this->forcedTargetActor = NULL;

        phi_f12 =
            ((this->bodyPartsPos[PLAYER_BODYPART_L_FOOT].y + this->bodyPartsPos[PLAYER_BODYPART_R_FOOT].y) * 0.5f) +
            temp_f0;
        temp_f0 += this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 10.0f;

        this->cylinder.dim.height = temp_f0 - phi_f12;

        if (this->cylinder.dim.height < 0) {
            phi_f12 = temp_f0;
            this->cylinder.dim.height = -this->cylinder.dim.height;
        }

        this->cylinder.dim.yShift = phi_f12 - this->actor.world.pos.y;

        if (this->stateFlags1 & PLAYER_STATE1_DEFENDING) {
            this->cylinder.dim.height *= 0.8f;
        }

        Collider_UpdateCylinder(&this->actor, &this->cylinder);

        if (!(this->stateFlags2 & PLAYER_STATE2_FROZEN_IN_ICE)) {
            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP |
                                       PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_RIDING_HORSE))) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->cylinder.base);
            }

            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_TAKING_DAMAGE)) &&
                (this->invincibilityTimer <= 0)) {
                CollisionCheck_SetAC(play, &play->colChkCtx, &this->cylinder.base);

                if (this->invincibilityTimer < 0) {
                    CollisionCheck_SetAT(play, &play->colChkCtx, &this->cylinder.base);
                }
            }
        }

        AnimationContext_SetNextQueue(play);
    }

    Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);
    Math_Vec3f_Copy(&this->prevWaistPos, &this->bodyPartsPos[PLAYER_BODYPART_WAIST]);

    if (this->stateFlags1 &
        (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE)) {
        this->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        this->actor.colChkInfo.mass = 50;
    }

    this->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;

    Collider_ResetCylinderAC(play, &this->cylinder.base);

    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[0].base);
    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[1].base);

    Collider_ResetQuadAC(play, &this->shieldQuad.base);
    Collider_ResetQuadAT(play, &this->shieldQuad.base);
}

static Vec3f sDogSpawnOffset = { 0.0f, 0.0f, -30.0f };

void Player_Update(Actor* thisx, PlayState* play) {
    static Vec3f sDogSpawnPos;
    Player* this = (Player*)thisx;
    s32 dogParams;
    s32 pad;
    Input input;
    Actor* dog;

    if (Player_CheckNoDebugModeCombo(this, play)) {
        if (gSaveContext.dogParams < 0) {
            if (Object_GetSlot(&play->objectCtx, OBJECT_DOG) < 0) {
                gSaveContext.dogParams = 0;
            } else {
                gSaveContext.dogParams &= 0x7FFF;
                Player_GetRelativePosition(this, &this->actor.world.pos, &sDogSpawnOffset, &sDogSpawnPos);
                dogParams = gSaveContext.dogParams;

                dog = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_DOG, sDogSpawnPos.x, sDogSpawnPos.y, sDogSpawnPos.z,
                                  0, this->actor.shape.rot.y, 0, dogParams | 0x8000);
                if (dog != NULL) {
                    dog->room = 0;
                }
            }
        }

        if ((this->interactRangeActor != NULL) && (this->interactRangeActor->update == NULL)) {
            this->interactRangeActor = NULL;
        }

        if ((this->heldActor != NULL) && (this->heldActor->update == NULL)) {
            Player_DetachHeldActor(play, this);
        }

        if (this->stateFlags1 & (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE)) {
            bzero(&input, sizeof(input));
        } else {
            input = play->state.input[0];
            if (this->endTalkTimer != 0) {
                input.cur.button &= ~(BTN_A | BTN_B | BTN_CUP);
                input.press.button &= ~(BTN_A | BTN_B | BTN_CUP);
            }
        }

        Player_UpdateCommon(this, play, &input);
    }

    MREG(52) = this->actor.world.pos.x;
    MREG(53) = this->actor.world.pos.y;
    MREG(54) = this->actor.world.pos.z;
    MREG(55) = this->actor.world.rot.y;
}

typedef struct {
    /* 0x0 */ Vec3s rot;
    /* 0x6 */ Vec3s angVel;
} BunnyEarKinematics; // size = 0xC

static BunnyEarKinematics sBunnyEarKinematics;

static Vec3s sFishingBlendTable[25];

static Gfx* sMaskDlists[PLAYER_MASK_MAX - 1] = {
    gLinkChildKeatonMaskDL, gLinkChildSkullMaskDL, gLinkChildSpookyMaskDL, gLinkChildBunnyHoodDL,
    gLinkChildGoronMaskDL,  gLinkChildZoraMaskDL,  gLinkChildGerudoMaskDL, gLinkChildMaskOfTruthDL,
};

static Vec3s sHoverEffectRot = { 0, 0, 0 };

void Player_DrawGameplay(PlayState* play, Player* this, s32 lod, Gfx* cullDList, OverrideLimbDrawOpa overrideLimbDraw) {
    static s32 sHoverEffectAlpha = 255;

    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19228);

    gSPSegment(POLY_OPA_DISP++, 0x0C, cullDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, cullDList);

    Player_DrawImpl(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, lod,
                    this->currentTunic, this->currentBoots, this->actor.shape.face, overrideLimbDraw,
                    Player_PostLimbDrawGameplay, this);

    if ((overrideLimbDraw == Player_OverrideLimbDrawGameplayDefault) && (this->currentMask != PLAYER_MASK_NONE)) {
        Mtx* bunnyEarMtx = Graph_Alloc(play->state.gfxCtx, 2 * sizeof(Mtx));

        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Vec3s earRot;

            gSPSegment(POLY_OPA_DISP++, 0x0B, bunnyEarMtx);

            // Right ear
            earRot.x = sBunnyEarKinematics.rot.y + 0x3E2;
            earRot.y = sBunnyEarKinematics.rot.z + 0xDBE;
            earRot.z = sBunnyEarKinematics.rot.x - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, -240.0f, &earRot);
            Matrix_ToMtx(bunnyEarMtx++, "../z_player.c", 19273);

            // Left ear
            earRot.x = sBunnyEarKinematics.rot.y - 0x3E2;
            earRot.y = -sBunnyEarKinematics.rot.z - 0xDBE;
            earRot.z = sBunnyEarKinematics.rot.x - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, 240.0f, &earRot);
            Matrix_ToMtx(bunnyEarMtx, "../z_player.c", 19279);
        }

        gSPDisplayList(POLY_OPA_DISP++, sMaskDlists[this->currentMask - 1]);
    }

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->hoverBootsTimer != 0)) {
        s32 sp5C;
        s32 hoverBootsTimer = this->hoverBootsTimer;

        if (this->hoverBootsTimer < 19) {
            if (hoverBootsTimer >= 15) {
                sHoverEffectAlpha = (19 - hoverBootsTimer) * 51.0f;
            } else if (hoverBootsTimer < 19) {
                sp5C = hoverBootsTimer;

                if (sp5C > 9) {
                    sp5C = 9;
                }

                sHoverEffectAlpha = (-sp5C * 4) + 36;
                sHoverEffectAlpha = SQ(sHoverEffectAlpha);
                sHoverEffectAlpha = (s32)((Math_CosS(sHoverEffectAlpha) * 100.0f) + 100.0f) + 55.0f;
                sHoverEffectAlpha *= sp5C * (1.0f / 9.0f);
            }

            Matrix_SetTranslateRotateYXZ(this->actor.world.pos.x, this->actor.world.pos.y + 2.0f,
                                         this->actor.world.pos.z, &sHoverEffectRot);
            Matrix_Scale(4.0f, 4.0f, 4.0f, MTXMODE_APPLY);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19317),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, 0,
                                        (play->gameplayFrames * -15) % 128, 16, 32));
            gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, sHoverEffectAlpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 120, 90, 30, 128);
            gSPDisplayList(POLY_XLU_DISP++, gHoverBootsCircleDL);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19328);
}

void Player_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Player* this = (Player*)thisx;

    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19346);

    if (!(this->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW)) {
        OverrideLimbDrawOpa overrideLimbDraw = Player_OverrideLimbDrawGameplayDefault;
        s32 lod;
        s32 pad;

        if ((this->csAction != PLAYER_CSACTION_NONE) || (Player_CheckBattleTargeting(this) && 0) ||
            (this->actor.projectedPos.z < 160.0f)) {
            lod = 0;
        } else {
            lod = 1;
        }

        func_80093C80(play);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);

        if (this->invincibilityTimer > 0) {
            this->damageFlashTimer += CLAMP(50 - this->invincibilityTimer, 8, 40);
            POLY_OPA_DISP = Gfx_SetFog2(POLY_OPA_DISP, 255, 0, 0, 0, 0,
                                        4000 - (s32)(Math_CosS(this->damageFlashTimer * 256) * 2000.0f));
        }

        func_8002EBCC(&this->actor, play, 0);
        func_8002ED80(&this->actor, play, 0);

        if (this->attentionMode != 0) {
            Vec3f projectedHeadPos;

            SkinMatrix_Vec3fMtxFMultXYZ(&play->viewProjectionMtxF, &this->actor.focus.pos, &projectedHeadPos);
            if (projectedHeadPos.z < -4.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplayFirstPerson;
            }
        } else if (this->stateFlags2 & PLAYER_STATE2_CRAWLING) {
            if (this->actor.projectedPos.z < 0.0f) {
                // Player is behind the camera
                overrideLimbDraw = Player_OverrideLimbDrawGameplayCrawling;
            }
        }

        if (this->stateFlags2 & PLAYER_STATE2_ENABLE_REFLECTION) {
            f32 sp78 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 600));
            f32 sp74 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 1000));

            Matrix_Push();
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_SetTranslateRotateYXZ(
                this->actor.world.pos.x,
                (this->actor.floorHeight + (this->actor.floorHeight - this->actor.world.pos.y)) +
                    (this->actor.shape.yOffset * this->actor.scale.y),
                this->actor.world.pos.z, &this->actor.shape.rot);
            Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
            Matrix_RotateX(sp78, MTXMODE_APPLY);
            Matrix_RotateY(sp74, MTXMODE_APPLY);
            Matrix_Scale(1.1f, 0.95f, 1.05f, MTXMODE_APPLY);
            Matrix_RotateY(-sp74, MTXMODE_APPLY);
            Matrix_RotateX(-sp78, MTXMODE_APPLY);
            Player_DrawGameplay(play, this, lod, gCullFrontDList, overrideLimbDraw);
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_Pop();
        }

        gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BOTH);

        Player_DrawGameplay(play, this, lod, gCullBackDList, overrideLimbDraw);

        if (this->invincibilityTimer > 0) {
            POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);
        }

        if (this->stateFlags2 & PLAYER_STATE2_FROZEN_IN_ICE) {
            f32 scale = (this->av1.actionVar1 >> 1) * 22.0f;

            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, (0 - play->gameplayFrames) % 128, 32,
                                        32, 1, 0, (play->gameplayFrames * -2) % 128, 32, 32));

            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19459),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, 255);
            gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment3DL);
        }

        if (this->giDrawIdPlusOne > 0) {
            Player_DrawGetItem(play, this);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19473);
}

void Player_Destroy(Actor* thisx, PlayState* play) {
    Player* this = (Player*)thisx;

    Effect_Delete(play, this->meleeWeaponEffectIndex);
    Effect_Delete(play, this->zoraSwimEffectIndex[0]);
    Effect_Delete(play, this->zoraSwimEffectIndex[1]);

    Collider_DestroyCylinder(play, &this->cylinder);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[0]);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[1]);
    Collider_DestroyQuad(play, &this->shieldQuad);

    Magic_Reset(play);

    gSaveContext.save.linkAge = play->linkAgeOnLoad;
}

s16 Player_SetFirstPersonAimLookAngles(PlayState* play, Player* this, s32 arg2, s16 lookYawOffset) {
    s32 angleMinMax;
    s16 aimAngleY;
    s16 aimAngleX;

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) && (arg2 == 0)) {
        aimAngleY = sControlInput->rel.stick_y * 240.0f;
        Math_SmoothStepToS(&this->actor.focus.rot.x, aimAngleY, 14, 4000, 30);

        aimAngleY = sControlInput->rel.stick_x * -16.0f;
        aimAngleY = CLAMP(aimAngleY, -3000, 3000);
        this->actor.focus.rot.y += aimAngleY;
    } else {
        angleMinMax = (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ? 3500 : 14000;
        aimAngleX = ((sControlInput->rel.stick_y >= 0) ? 1 : -1) *
                    (s32)((1.0f - Math_CosS(sControlInput->rel.stick_y * 200)) * 1500.0f);
        this->actor.focus.rot.x += aimAngleX;
        this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -angleMinMax, angleMinMax);

        angleMinMax = 19114;
        aimAngleY = this->actor.focus.rot.y - this->actor.shape.rot.y;
        aimAngleX = ((sControlInput->rel.stick_x >= 0) ? 1 : -1) *
                    (s32)((1.0f - Math_CosS(sControlInput->rel.stick_x * 200)) * -1500.0f);
        aimAngleY += aimAngleX;
        this->actor.focus.rot.y = CLAMP(aimAngleY, -angleMinMax, angleMinMax) + this->actor.shape.rot.y;
    }

    this->lookFlags |= 2;
    return Player_UpdateLookAngles(this, (play->shootingGalleryStatus != 0) || Actor_PlayerIsAimingReadyFpsItem(this) ||
                                             Player_IsAimingReadyBoomerang(this)) -
           lookYawOffset;
}

void Player_UpdateSwimMovement(Player* this, f32* speed, f32 speedTarget, s16 targetYaw) {
    f32 incrStep = this->skelAnime.curFrame - 10.0f;
    f32 maxSpeed = (R_RUN_SPEED_LIMIT / 100.0f) * 0.8f;

    if (*speed > maxSpeed) {
        *speed = maxSpeed;
    }

    if ((0.0f < incrStep) && (incrStep < 16.0f)) {
        incrStep = fabsf(incrStep) * 0.5f;
    } else {
        incrStep = 0.0f;
        speedTarget = 0.0f;
    }

    Math_AsymStepToF(speed, speedTarget * 0.8f, incrStep, (fabsf(*speed) * 0.02f) + 0.05f);
    Math_ScaledStepToS(&this->yaw, targetYaw, 1600);
}

#if 1
// Modified Player_SetVerticalWaterSpeed (func_808475B4)
void Player_SetVerticalWaterSpeed(Player* this) {
    f32 accel;
    f32 waterSurface;
    f32 targetSpeed = -5.0f;
    f32 waterSurfaceOffset = this->ageProperties->unk_28;
    f32 clampedWaterSurface;

    waterSurface = this->actor.yDistToWater - waterSurfaceOffset;
    if (this->actor.velocity.y < 0.0f) {
        waterSurfaceOffset += 1.0f;
    }

    if (this->actor.yDistToWater < waterSurfaceOffset) {
        waterSurface = CLAMP(waterSurface, -0.4f, -0.2f);
        accel = waterSurface - ((this->actor.velocity.y <= 0.0f) ? 0.0f : this->actor.velocity.y * 0.5f);
    } else {
        if (!(this->stateFlags1 & PLAYER_STATE1_IN_DEATH_CUTSCENE) && (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) &&
            (this->actor.velocity.y >= -5.0f)) {
            accel = -0.3f;
        } else {
            targetSpeed = 2.0f;
            clampedWaterSurface = CLAMP(waterSurface, 0.1f, 0.4f);
            accel = ((this->actor.velocity.y >= 0.0f) ? 0.0f : this->actor.velocity.y * -0.3f) + clampedWaterSurface;
        }

        if (this->actor.yDistToWater > 100.0f) {
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
        }
    }

    this->actor.velocity.y += accel;
    if (((this->actor.velocity.y - targetSpeed) * accel) > 0.0f) {
        this->actor.velocity.y = targetSpeed;
    }
    this->actor.gravity = 0.0f;
}
#endif

#if 0
void Player_SetVerticalWaterSpeed(Player* this) {
    f32 accel;
    f32 waterSurface;
    f32 targetSpeed;
    f32 yDistToWater;

    targetSpeed = -5.0f;

    waterSurface = this->ageProperties->unk_28;
    if (this->actor.velocity.y < 0.0f) {
        waterSurface += 1.0f;
    }

    if (this->actor.yDistToWater < waterSurface) {
        if (this->actor.velocity.y <= 0.0f) {
            waterSurface = 0.0f;
        } else {
            waterSurface = this->actor.velocity.y * 0.5f;
        }
        accel = -0.1f - waterSurface;
    } else {
        if (!(this->stateFlags1 & PLAYER_STATE1_IN_DEATH_CUTSCENE) && (this->currentBoots == PLAYER_BOOTS_IRON) &&
            (this->actor.velocity.y >= -3.0f)) {
            accel = -0.2f;
        } else {
            targetSpeed = 2.0f;
            if (this->actor.velocity.y >= 0.0f) {
                waterSurface = 0.0f;
            } else {
                waterSurface = this->actor.velocity.y * -0.3f;
            }
            accel = waterSurface + 0.1f;
        }

        yDistToWater = this->actor.yDistToWater;
        if (yDistToWater > 100.0f) {
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
        }
    }

    this->actor.velocity.y += accel;

    if (((this->actor.velocity.y - targetSpeed) * accel) > 0) {
        this->actor.velocity.y = targetSpeed;
    }

    this->actor.gravity = 0.0f;
}
#endif

// func_808477D0 in MM
void Player_PlaySwimAnim(PlayState* play, Player* this, Input* input, f32 arg3) {
    f32 temp;

    if ((input != NULL) && CHECK_BTN_ANY(input->press.button, BTN_A | BTN_B)) {
        temp = 1.0f;
    } else {
        temp = 0.5f;
    }

    temp *= arg3;

    temp = CLAMP(temp, 1.0f, 2.5f);

    this->skelAnime.playSpeed = temp;
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Action_FirstPersonAiming(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
        Player_SetVerticalWaterSpeed(this);
        Player_UpdateSwimMovement(this, &this->speedXZ, 0, this->actor.shape.rot.y);
    } else {
        Player_StepSpeedXZToZero(this);
    }

    if ((this->attentionMode == 2) && (Actor_PlayerIsAimingFpsItem(this) || Player_IsAimingBoomerang(this))) {
        Player_UpdateUpperBody(this, play);
    }

    if ((this->csAction != PLAYER_CSACTION_NONE) || (this->attentionMode == 0) || (this->attentionMode >= 4) ||
        Player_TryBattleTargeting(this) || (this->targetActor != NULL) ||
        (Player_RequestFpsCamera(play, this) == CAM_MODE_NORMAL) ||
        (((this->attentionMode == 2) &&
          (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_R) || Player_CheckCalmTargeting(this) ||
           (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this)))) ||
         ((this->attentionMode == 1) &&
          CHECK_BTN_ANY(sControlInput->press.button,
                        BTN_A | BTN_B | BTN_R | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)))) {
        Player_ClearLookAndAttention(this, play);
        Sfx_PlaySfxCentered(NA_SE_SY_CAMERA_ZOOM_UP);
    } else if ((DECR(this->av2.actionVar2) == 0) || (this->attentionMode != 2)) {
        if (Player_IsShootingHookshot(this)) {
            this->lookFlags |= 0x43;
        } else {
            this->actor.shape.rot.y = Player_SetFirstPersonAimLookAngles(play, this, 0, 0);
        }
    }

    this->yaw = this->actor.shape.rot.y;
}

s32 Player_TryShootingGalleryPlay(PlayState* play, Player* this) {
    if (play->shootingGalleryStatus != 0) {
        Player_StopCarryingActor(play, this);
        Player_SetupAction(play, this, Player_Action_ShootingGalleryPlay, 0);

        if (!Actor_PlayerIsAimingFpsItem(this) || Player_HoldsHookshot(this)) {
            Player_UseItem(play, this, 3);
        }

        this->stateFlags1 |= PLAYER_STATE1_IN_FIRST_PERSON_MODE;
        Player_AnimPlayOnce(play, this, Player_GetStandStillAnim(this));
        Player_ZeroSpeedXZ(this);
        Player_ClearLookAngles(this);
        return 1;
    }

    return 0;
}

void Player_SetOcarinaItemAction(Player* this) {
    this->itemAction =
        (INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY) ? PLAYER_IA_OCARINA_FAIRY : PLAYER_IA_OCARINA_OF_TIME;
}

s32 Player_TryPullOcarina(PlayState* play, Player* this) {
    if (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA) {
        this->stateFlags3 &= ~PLAYER_STATE3_FORCE_PULL_OCARINA;
        Player_SetOcarinaItemAction(this);
        this->attentionMode = 4;
        Player_ActionChange_TryItemCsOrFirstPerson(this, play);
        return 1;
    }

    return 0;
}

void Player_Action_TalkToActor(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_UpdateUpperBody(this, play);

    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        this->actor.flags &= ~ACTOR_FLAG_TALK;

        if (!CHECK_FLAG_ALL(this->talkActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
            this->stateFlags2 &= ~PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
        }

        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));

        if (!Player_TryPullOcarina(play, this) && !Player_TryShootingGalleryPlay(play, this) &&
            !Player_SetupCsAction(play, this)) {
            if ((this->talkActor != this->interactRangeActor) || !Player_ActionChange_TryGetItemOrCarry(this, play)) {
                if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
                    s32 sp24 = this->av2.actionVar2;
                    Player_SetupRidingHorse(play, this);
                    this->av2.actionVar2 = sp24;
                } else if (Player_IsSwimming(this)) {
                    Player_SetupSwimIdle(play, this);
                } else {
                    Player_SetupStandStillMorph(this, play);
                }
            }
        }

        this->endTalkTimer = 10;
        return;
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_Action_RideHorse(this, play);
    } else if (Player_IsSwimming(this)) {
        Player_Action_SwimIdle(this, play);
    } else if (!Player_CheckBattleTargeting(this) && LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->skelAnime.moveFlags != 0) {
            Player_FinishAnimMovement(this);
            if ((this->talkActor->category == ACTORCAT_NPC) && (this->heldItemAction != PLAYER_IA_FISHING_POLE)) {
                Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_normal_talk_free);
            } else {
                Player_AnimPlayLoop(play, this, Player_GetStandStillAnim(this));
            }
        } else {
            Player_AnimPlayLoopAdjusted(play, this, &gPlayerAnim_link_normal_talk_free_wait);
        }
    }

    if (this->targetActor != NULL) {
        this->yaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_Action_GrabPushPullWall(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 pushPullDir;

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;
    Player_PushPullSetPositionAndYaw(play, this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_TryContinuePushPullWallInteract(play, this)) {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
            pushPullDir = Player_GetPushPullDirection(this, &speedTarget, &yawTarget);
            if (pushPullDir > 0) {
                Player_SetupPushWall(this, play);
            } else if (pushPullDir < 0) {
                Player_SetupPullWall(this, play);
            }
        }
    }
}

void Player_MoveDynaPolyActor(PlayState* play, Player* this, f32 moveDist) {
    if (this->actor.wallBgId != BGCHECK_SCENE) {
        DynaPolyActor* dynaPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);

        if (dynaPolyActor != NULL) {
            Actor_MoveDynaPolyActor(dynaPolyActor, moveDist, this->actor.world.rot.y);
        }
    }
}

static AnimSfxEntry sPushWallAnimSfx[] = {
    { NA_SE_PL_SLIP, ANIMSFX_DATA(ANIMSFX_TYPE_2, 3) },
    { NA_SE_PL_SLIP, -ANIMSFX_DATA(ANIMSFX_TYPE_2, 21) },
};

void Player_Action_PushWall(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s32 temp;

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

    if (Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_normal_pushing)) {
        this->av2.actionVar2 = 1;
    } else if (this->av2.actionVar2 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
        }
    }

    Player_ProcessAnimSfxList(this, sPushWallAnimSfx);
    Player_PushPullSetPositionAndYaw(play, this);

    if (!Player_TryContinuePushPullWallInteract(play, this)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        temp = Player_GetPushPullDirection(this, &speedTarget, &yawTarget);
        if (temp < 0) {
            Player_SetupPullWall(this, play);
        } else if (temp == 0) {
            Player_SetupGrabPushPullWallTryMiniCs(this, &gPlayerAnim_link_normal_push_end, play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        }
    }

    if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
        Player_MoveDynaPolyActor(play, this, 2.0f);
        this->speedXZ = 2.0f;
    }
}

static AnimSfxEntry sPullWallAnimSfx[] = {
    { NA_SE_PL_SLIP, ANIMSFX_DATA(ANIMSFX_TYPE_2, 4) },
    { NA_SE_PL_SLIP, -ANIMSFX_DATA(ANIMSFX_TYPE_2, 24) },
};

static Vec3f sPullWallRaycastOffset = { 0.0f, 26.0f, -40.0f };

void Player_Action_PullWall(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 speedTarget;
    s16 yawTarget;
    s32 pushPullDir;
    Vec3f raycastPos;
    f32 floorPosY;
    CollisionPoly* colPoly;
    s32 polyBgId;
    Vec3f lineCheckPos;
    Vec3f posResult;

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pulling, this->modelAnimType);
    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

    if (Player_LoopAnimContinuously(play, this, anim)) {
        this->av2.actionVar2 = 1;
    } else {
        if (this->av2.actionVar2 == 0) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
            }
        } else {
            Player_ProcessAnimSfxList(this, sPullWallAnimSfx);
        }
    }

    Player_PushPullSetPositionAndYaw(play, this);

    if (!Player_TryContinuePushPullWallInteract(play, this)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        pushPullDir = Player_GetPushPullDirection(this, &speedTarget, &yawTarget);
        if (pushPullDir > 0) {
            Player_SetupPushWall(this, play);
        } else if (pushPullDir == 0) {
            Player_SetupGrabPushPullWallTryMiniCs(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pull_end, this->modelAnimType),
                                                  play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        }
    }

    if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
        floorPosY = Player_RaycastFloor(play, this, &sPullWallRaycastOffset, &raycastPos) - this->actor.world.pos.y;
        if (fabsf(floorPosY) < 20.0f) {
            lineCheckPos.x = this->actor.world.pos.x;
            lineCheckPos.z = this->actor.world.pos.z;
            lineCheckPos.y = raycastPos.y;
            if (!BgCheck_EntityLineTest1(&play->colCtx, &lineCheckPos, &raycastPos, &posResult, &colPoly, true, false,
                                         false, true, &polyBgId)) {
                Player_MoveDynaPolyActor(play, this, -2.0f);
                return;
            }
        }
        this->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    }
}

void Player_Action_GrabLedge(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    LinkAnimationHeader* anim;
    f32 temp;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        // clang-format off
        anim = (this->av1.actionVar1 > 0) ? &gPlayerAnim_link_normal_fall_wait : GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_wait, this->modelAnimType); Player_AnimPlayLoop(play, this, anim);
        // clang-format on
    } else if (this->av1.actionVar1 == 0) {
        if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
            temp = 11.0f;
        } else {
            temp = 1.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, temp)) {
            Player_PlayMoveSfx(this, NA_SE_PL_WALK_GROUND);
            if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
                this->av1.actionVar1 = 1;
            } else {
                this->av1.actionVar1 = -1;
            }
        }
    }

    Math_ScaledStepToS(&this->actor.shape.rot.y, this->yaw, 0x800);

    if (this->av1.actionVar1 != 0) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        if (this->analogStickInputs[this->inputFrameCounter] >= 0) {
            if (this->av1.actionVar1 > 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_fall_up, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_up, this->modelAnimType);
            }
            Player_SetupClimbOntoLedge(this, anim, play);
            return;
        }

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) || (this->actor.shape.feetFloorFlag != 0)) {
            Player_SetLedgeGrabPosition(this);
            if (this->av1.actionVar1 < 0) {
                this->speedXZ = -0.8f;
            } else {
                this->speedXZ = 0.8f;
            }
            Player_SetupFallFromLedge(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE);
        }
    }
}

void Player_Action_ClimbOntoLedge(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_UpdateAnimMovement(this, ANIM_FLAG_0);
        Player_SetupStandStill(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 6.0f)) {
        Player_PlayLandingSfx(this);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 34.0f)) {
        this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE);
        Player_PlaySfx(this, NA_SE_PL_CLIMB_CLIFF);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
    }
}

void Player_PlayClimbingSfx(Player* this) {
    Player_PlaySfx(this, (this->av1.actionVar1 != 0) ? NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_VINE
                                                     : NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD);
}

void Player_Action_ClimbingWallOrDownLedge(Player* this, PlayState* play) {
    static Vec3f raycastOffset = { 0.0f, 0.0f, 26.0f };
    s32 stickDistY;
    s32 stickDistX;
    f32 animSpeedInfluence;
    f32 playbackDir;
    Vec3f wallPosDiff;
    s32 sp68;
    Vec3f raycastPos;
    f32 floorPosY;
    LinkAnimationHeader* verticalClimbAnim;
    LinkAnimationHeader* horizontalClimbAnim;

    stickDistY = sControlInput->rel.stick_y;
    stickDistX = sControlInput->rel.stick_x;

    this->fallStartHeight = this->actor.world.pos.y;
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if ((this->av1.actionVar1 != 0) && (ABS(stickDistY) < ABS(stickDistX))) {
        animSpeedInfluence = ABS(stickDistX) * 0.0325f;
        stickDistY = 0;
    } else {
        animSpeedInfluence = ABS(stickDistY) * 0.05f;
        stickDistX = 0;
    }

    if (animSpeedInfluence < 1.0f) {
        animSpeedInfluence = 1.0f;
    } else if (animSpeedInfluence > 3.35f) {
        animSpeedInfluence = 3.35f;
    }

    if (this->skelAnime.playSpeed >= 0.0f) {
        playbackDir = 1.0f;
    } else {
        playbackDir = -1.0f;
    }

    this->skelAnime.playSpeed = playbackDir * animSpeedInfluence;

    if (this->av2.actionVar2 >= 0) {
        if ((this->actor.wallPoly != NULL) && (this->actor.wallBgId != BGCHECK_SCENE)) {
            DynaPolyActor* wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
            if (wallPolyActor != NULL) {
                Math_Vec3f_Diff(&wallPolyActor->actor.world.pos, &wallPolyActor->actor.prevPos, &wallPosDiff);
                Math_Vec3f_Sum(&this->actor.world.pos, &wallPosDiff, &this->actor.world.pos);
            }
        }

        Actor_UpdateBgCheckInfo(play, &this->actor, 26.0f, 6.0f, this->ageProperties->ceilingCheckHeight,
                                UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->unk_3C, 50.0f, -20.0f);
    }

    if ((this->av2.actionVar2 < 0) || !Player_TryClimbingLetGo(this, play)) {
        if (LinkAnimation_Update(play, &this->skelAnime) != 0) {
            if (this->av2.actionVar2 < 0) {
                this->av2.actionVar2 = ABS(this->av2.actionVar2) & 1;
                return;
            }

            if (stickDistY != 0) {
                sp68 = this->av1.actionVar1 + this->av2.actionVar2;

                if (stickDistY > 0) {
                    raycastOffset.y = this->ageProperties->unk_40;
                    floorPosY = Player_RaycastFloor(play, this, &raycastOffset, &raycastPos);

                    if (this->actor.world.pos.y < floorPosY) {
                        if (this->av1.actionVar1 != 0) {
                            this->actor.world.pos.y = floorPosY;
                            this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
                            Player_SetupGrabLedge(play, this, this->actor.wallPoly, this->ageProperties->unk_3C,
                                                  &gPlayerAnim_link_normal_jump_climb_up_free);
                            this->yaw += 0x8000;
                            this->actor.shape.rot.y = this->yaw;
                            Player_SetupClimbOntoLedge(this, &gPlayerAnim_link_normal_jump_climb_up_free, play);
                            this->stateFlags1 |= PLAYER_STATE1_CLIMBING_ONTO_LEDGE;
                        } else {
                            Player_SetupEndClimb(this, this->ageProperties->unk_CC[this->av2.actionVar2], play);
                        }
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_4A[sp68];
                        Player_AnimPlayOnce(play, this, this->ageProperties->unk_AC[sp68]);
                    }
                } else {
                    if ((this->actor.world.pos.y - this->actor.floorHeight) < 15.0f) {
                        if (this->av1.actionVar1 != 0) {
                            Player_ClimbingLetGo(this, play);
                        } else {
                            if (this->av2.actionVar2 != 0) {
                                this->skelAnime.prevTransl = this->ageProperties->unk_44;
                            }
                            Player_SetupEndClimb(this, this->ageProperties->unk_C4[this->av2.actionVar2], play);
                            this->av2.actionVar2 = 1;
                        }
                    } else {
                        sp68 ^= 1;
                        this->skelAnime.prevTransl = this->ageProperties->unk_62[sp68];
                        verticalClimbAnim = this->ageProperties->unk_AC[sp68];
                        LinkAnimation_Change(play, &this->skelAnime, verticalClimbAnim, -1.0f,
                                             Animation_GetLastFrame(verticalClimbAnim), 0.0f, ANIMMODE_ONCE, 0.0f);
                    }
                }
                this->av2.actionVar2 ^= 1;
            } else {
                if ((this->av1.actionVar1 != 0) && (stickDistX != 0)) {
                    horizontalClimbAnim = this->ageProperties->unk_BC[this->av2.actionVar2];

                    if (stickDistX > 0) {
                        this->skelAnime.prevTransl = this->ageProperties->unk_7A[this->av2.actionVar2];
                        Player_AnimPlayOnce(play, this, horizontalClimbAnim);
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_86[this->av2.actionVar2];
                        LinkAnimation_Change(play, &this->skelAnime, horizontalClimbAnim, -1.0f,
                                             Animation_GetLastFrame(horizontalClimbAnim), 0.0f, ANIMMODE_ONCE, 0.0f);
                    }
                } else {
                    this->stateFlags2 |= PLAYER_STATE2_IDLE_WHILE_CLIMBING;
                }
            }

            return;
        }
    }

    if (this->av2.actionVar2 < 0) {
        if (((this->av2.actionVar2 == -2) &&
             (LinkAnimation_OnFrame(&this->skelAnime, 14.0f) || LinkAnimation_OnFrame(&this->skelAnime, 29.0f))) ||
            ((this->av2.actionVar2 == -4) &&
             (LinkAnimation_OnFrame(&this->skelAnime, 22.0f) || LinkAnimation_OnFrame(&this->skelAnime, 35.0f) ||
              LinkAnimation_OnFrame(&this->skelAnime, 49.0f) || LinkAnimation_OnFrame(&this->skelAnime, 55.0f)))) {
            Player_PlayClimbingSfx(this);
        }
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, (this->skelAnime.playSpeed > 0.0f) ? 20.0f : 0.0f)) {
        Player_PlayClimbingSfx(this);
    }
}

static f32 sClimbEndFrames[] = { 10.0f, 20.0f };
static f32 sClimbLadderEndFrames[] = { 40.0f, 50.0f };

static AnimSfxEntry sClimbLadderEndAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, ANIMSFX_DATA(ANIMSFX_TYPE_1, 10) },
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, ANIMSFX_DATA(ANIMSFX_TYPE_1, 20) },
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 30) },
};

void Player_Action_EndClimb(Player* this, PlayState* play) {
    s32 actionInterruptState;
    f32* endFrames;
    CollisionPoly* groundPoly;
    s32 bgId;
    Vec3f raycastPos;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    actionInterruptState = Player_CheckActionInterruptStatus(play, this, &this->skelAnime, 4.0f);

    if (actionInterruptState == 0) {
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    if ((actionInterruptState > 0) || LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandStill(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    endFrames = sClimbEndFrames;

    if (this->av2.actionVar2 != 0) {
        Player_ProcessAnimSfxList(this, sClimbLadderEndAnimSfx);
        endFrames = sClimbLadderEndFrames;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, endFrames[0]) ||
        LinkAnimation_OnFrame(&this->skelAnime, endFrames[1])) {
        raycastPos.x = this->actor.world.pos.x;
        raycastPos.y = this->actor.world.pos.y + 20.0f;
        raycastPos.z = this->actor.world.pos.z;
        if (BgCheck_EntityRaycastDown3(&play->colCtx, &groundPoly, &bgId, &raycastPos) != 0.0f) {
            //! @bug should use `SurfaceType_GetSfxOffset` instead of `SurfaceType_GetMaterial`.
            // Most material and sfxOffsets share identical enum values,
            // so this will mostly result in the correct sfx played, but not in all cases, such as carpet and ice.
            this->floorSfxOffset = SurfaceType_GetMaterial(&play->colCtx, groundPoly, bgId);
            Player_PlayLandingSfx(this);
        }
    }
}

static AnimSfxEntry sCrawlspaceCrawlAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 40) },   { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 48) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 56) },   { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 64) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 72) },   { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 80) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 88) },   { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 96) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 104) },
};

/**
 * Update player's animation while entering the crawlspace.
 * Once inside, stop all player animations and update player's movement.
 */
void Player_Action_Crawling(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
            // While inside a crawlspace, player's skeleton does not move
            if (this->skelAnime.moveFlags != 0) {
                this->skelAnime.moveFlags = 0;
                return;
            }

            if (!Player_TryLeavingCrawlspace(this, play)) {
                // Move forward and back while inside the crawlspace
                this->speedXZ = sControlInput->rel.stick_y * 0.03f;
            }
        }
        return;
    }

    // Still entering crawlspace
    Player_ProcessAnimSfxList(this, sCrawlspaceCrawlAnimSfx);
}

static AnimSfxEntry sExitCrawlspaceAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 10) },  { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 18) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 26) },  { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 34) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 52) },  { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 60) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 68) },  { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 76) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 84) },
};

/**
 * Update player's animation while leaving the crawlspace.
 */
void Player_Action_ExitCrawlspace(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        // Player is finished exiting the crawlspace and control is returned
        Player_SetupStandStill(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_CRAWLING;
        return;
    }

    // Continue animation of leaving crawlspace
    Player_ProcessAnimSfxList(this, sExitCrawlspaceAnimSfx);
}

static Vec3f sHorseDismountRaycastOffset[] = {
    { 40.0f, 0.0f, 0.0f },
    { -40.0f, 0.0f, 0.0f },
};

static Vec3f sHorseLineTestTopOffset[] = {
    { 60.0f, 20.0f, 0.0f },
    { -60.0f, 20.0f, 0.0f },
};

static Vec3f sHorseLineTestBottomOffset[] = {
    { 60.0f, -20.0f, 0.0f },
    { -60.0f, -20.0f, 0.0f },
};

int Player_CanDismountHorse(PlayState* play, Player* this, s32 arg2, f32* arg3) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 sp50;
    f32 sp4C;
    Vec3f sp40;
    Vec3f sp34;
    CollisionPoly* sp30;
    s32 sp2C;

    sp50 = rideActor->actor.world.pos.y + 20.0f;
    sp4C = rideActor->actor.world.pos.y - 20.0f;

    *arg3 = Player_RaycastFloor(play, this, &sHorseDismountRaycastOffset[arg2], &sp40);

    return (sp4C < *arg3) && (*arg3 < sp50) &&
           !Player_PosVsWallLineTest(play, this, &sHorseLineTestTopOffset[arg2], &sp30, &sp2C, &sp34) &&
           !Player_PosVsWallLineTest(play, this, &sHorseLineTestBottomOffset[arg2], &sp30, &sp2C, &sp34);
}

s32 Player_TryDismountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    s32 sp38;
    f32 sp34;

    if (this->av2.actionVar2 < 0) {
        this->av2.actionVar2 = 99;
    } else {
        sp38 = (this->mountSide < 0) ? 0 : 1;
        if (!Player_CanDismountHorse(play, this, sp38, &sp34)) {
            sp38 ^= 1;
            if (!Player_CanDismountHorse(play, this, sp38, &sp34)) {
                return 0;
            } else {
                this->mountSide = -this->mountSide;
            }
        }

        if ((play->csCtx.state == CS_STATE_IDLE) && (play->transitionMode == TRANS_MODE_OFF) &&
            (EN_HORSE_CHECK_1(rideActor) || EN_HORSE_CHECK_4(rideActor))) {
            this->stateFlags2 |= PLAYER_STATE2_CAN_DISMOUNT_HORSE;

            if (EN_HORSE_CHECK_1(rideActor) ||
                (EN_HORSE_CHECK_4(rideActor) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A))) {
                rideActor->actor.child = NULL;
                Player_SetupActionKeepMoveFlags(play, this, Player_Action_DismountHorse, 0);
                this->rideOffsetY = sp34 - rideActor->actor.world.pos.y;
                Player_AnimPlayOnce(play, this,
                                    (this->mountSide < 0) ? &gPlayerAnim_link_uma_left_down
                                                          : &gPlayerAnim_link_uma_right_down);
                return 1;
            }
        }
    }

    return 0;
}

void Player_AdjustVerticalPosWhileRiding(Player* this, f32 moveMagnitude, f32 frame) {
    f32 dist;
    f32 dir;

    if ((this->rideOffsetY != 0.0f) && (frame <= this->skelAnime.curFrame)) {
        if (moveMagnitude < fabsf(this->rideOffsetY)) {
            if (this->rideOffsetY >= 0.0f) {
                dir = 1.0f;
            } else {
                dir = -1.0f;
            }
            dist = dir * moveMagnitude;
        } else {
            dist = this->rideOffsetY;
        }
        this->actor.world.pos.y += dist;
        this->rideOffsetY -= dist;
    }
}

static LinkAnimationHeader* sHorseMoveAnims[] = {
    &gPlayerAnim_link_uma_anim_stop,
    &gPlayerAnim_link_uma_anim_stand,
    &gPlayerAnim_link_uma_anim_walk,
    &gPlayerAnim_link_uma_anim_slowrun,
    &gPlayerAnim_link_uma_anim_fastrun,
    &gPlayerAnim_link_uma_anim_jump100,
    &gPlayerAnim_link_uma_anim_jump200,
    NULL,
    NULL,
};

static LinkAnimationHeader* sHorseWhipAnims[] = {
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_slowrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    NULL,
    NULL,
};

static LinkAnimationHeader* sHorseIdleAnims[] = {
    &gPlayerAnim_link_uma_wait_3,
    &gPlayerAnim_link_uma_wait_1,
    &gPlayerAnim_link_uma_wait_2,
};

static u8 sMountSfxFrames[2][2] = {
    { 32, 58 },
    { 25, 42 },
};

static Vec3s sRideHorsePrevTransl = { -69, 7146, -266 };

static AnimSfxEntry sHorseIdleAnimSfx[] = {
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 48) },  { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 58) },
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 68) },  { NA_SE_PL_CALM_PAT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 92) },
    { NA_SE_PL_CALM_PAT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 110) }, { NA_SE_PL_CALM_PAT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 126) },
    { NA_SE_PL_CALM_PAT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 132) }, { NA_SE_PL_CALM_PAT, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 136) },
};

void Player_Action_RideHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    u8* arr;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    Player_AdjustVerticalPosWhileRiding(this, 1.0f, 10.0f);

    if (this->av2.actionVar2 == 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->skelAnime.animation = &gPlayerAnim_link_uma_wait_1;
            this->av2.actionVar2 = 99;
            return;
        }

        arr = sMountSfxFrames[(this->mountSide < 0) ? 0 : 1];

        if (LinkAnimation_OnFrame(&this->skelAnime, arr[0])) {
            Player_PlaySfx(this, NA_SE_PL_CLIMB_CLIFF);
            return;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, arr[1])) {
            func_8002DE74(play, this);
            Player_PlaySfx(this, NA_SE_PL_SIT_ON_HORSE);
            return;
        }

        return;
    }

    func_8002DE74(play, this);
    this->skelAnime.prevTransl = sRideHorsePrevTransl;

    if ((rideActor->animationIdx != this->av2.actionVar2) &&
        ((rideActor->animationIdx >= 2) || (this->av2.actionVar2 >= 2))) {
        if ((this->av2.actionVar2 = rideActor->animationIdx) < 2) {
            f32 rand = Rand_ZeroOne();
            s32 temp = 0;

            this->av2.actionVar2 = 1;

            if (rand < 0.1f) {
                temp = 2;
            } else if (rand < 0.2f) {
                temp = 1;
            }
            Player_AnimPlayOnce(play, this, sHorseIdleAnims[temp]);
        } else {
            this->skelAnime.animation = sHorseMoveAnims[this->av2.actionVar2 - 2];
            Animation_SetMorph(play, &this->skelAnime, 8.0f);
            if (this->av2.actionVar2 < 4) {
                Player_CompleteItemChange(play, this);
                this->av1.actionVar1 = 0;
            }
        }
    }

    if (this->av2.actionVar2 == 1) {
        if ((sUpperBodyBusy != 0) || Player_CheckActorTalkOffered(play)) {
            Player_AnimPlayOnce(play, this, &gPlayerAnim_link_uma_wait_3);
        } else if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->av2.actionVar2 = 99;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_uma_wait_1) {
            Player_ProcessAnimSfxList(this, sHorseIdleAnimSfx);
        }
    } else {
        this->skelAnime.curFrame = rideActor->curFrame;
        LinkAnimation_AnimateFrame(play, &this->skelAnime);
    }

    AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.morphTable,
                                this->skelAnime.jointTable);

    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csAction != PLAYER_CSACTION_NONE)) {
        if (this->csAction == PLAYER_CSACTION_END) {
            this->csAction = PLAYER_CSACTION_NONE;
        }
        this->attentionMode = 0;
        this->av1.actionVar1 = 0;
    } else if ((this->av2.actionVar2 < 2) || (this->av2.actionVar2 >= 4)) {
        sUpperBodyBusy = Player_UpdateUpperBody(this, play);
        if (sUpperBodyBusy != 0) {
            this->av1.actionVar1 = 0;
        }
    }

    this->actor.world.pos.x = rideActor->actor.world.pos.x + rideActor->riderPos.x;
    this->actor.world.pos.y = (rideActor->actor.world.pos.y + rideActor->riderPos.y) - 27.0f;
    this->actor.world.pos.z = rideActor->actor.world.pos.z + rideActor->riderPos.z;

    this->yaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

    if ((this->csAction != PLAYER_CSACTION_NONE) ||
        (!Player_CheckActorTalkOffered(play) &&
         ((rideActor->actor.speed != 0.0f) || !Player_ActionChange_TrySpeakOrCheck(this, play)) &&
         !Player_ActionChange_TryRollOrPutAway(this, play))) {
        if (sUpperBodyBusy == 0) {
            if (this->av1.actionVar1 != 0) {
                if (LinkAnimation_Update(play, &this->upperSkelAnime)) {
                    rideActor->stateFlags &= ~ENHORSE_FLAG_8;
                    this->av1.actionVar1 = 0;
                }

                if (this->upperSkelAnime.animation == &gPlayerAnim_link_uma_stop_muti) {
                    if (LinkAnimation_OnFrame(&this->upperSkelAnime, 23.0f)) {
                        Player_PlaySfx(this, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }

                    AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                                this->upperSkelAnime.jointTable);
                } else {
                    if (LinkAnimation_OnFrame(&this->upperSkelAnime, 10.0f)) {
                        Player_PlaySfx(this, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }

                    AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                                 this->upperSkelAnime.jointTable, sUpperBodyLimbCopyMap);
                }
            } else {
                LinkAnimationHeader* anim = NULL;

                if (EN_HORSE_CHECK_3(rideActor)) {
                    anim = &gPlayerAnim_link_uma_stop_muti;
                } else if (EN_HORSE_CHECK_2(rideActor)) {
                    if ((this->av2.actionVar2 >= 2) && (this->av2.actionVar2 != 99)) {
                        anim = sHorseWhipAnims[this->av2.actionVar2 - 2];
                    }
                }

                if (anim != NULL) {
                    LinkAnimation_PlayOnce(play, &this->upperSkelAnime, anim);
                    this->av1.actionVar1 = 1;
                }
            }
        }

        if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
            if ((Player_RequestFpsCamera(play, this) == CAM_MODE_NORMAL) ||
                CHECK_BTN_ANY(sControlInput->press.button, BTN_A) || Player_CheckTargeting(this)) {
                this->attentionMode = 0;
                this->stateFlags1 &= ~PLAYER_STATE1_IN_FIRST_PERSON_MODE;
            } else {
                this->upperBodyRot.y =
                    Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                this->upperBodyRot.y += 5000;
                this->upperBodyYawOffset = -5000;
            }
            return;
        }

        if ((this->csAction != PLAYER_CSACTION_NONE) ||
            (!Player_TryDismountHorse(this, play) && !Player_ActionChange_TryItemCsOrFirstPerson(this, play))) {
            if (this->targetActor != NULL) {
                if (Actor_PlayerIsAimingReadyFpsItem(this) != 0) {
                    this->upperBodyRot.y = Player_LookAtTargetActor(this, 1) - this->actor.shape.rot.y;
                    this->upperBodyRot.y = CLAMP(this->upperBodyRot.y, -0x4AAA, 0x4AAA);
                    this->actor.focus.rot.y = this->actor.shape.rot.y + this->upperBodyRot.y;
                    this->upperBodyRot.y += 5000;
                    this->lookFlags |= 0x80;
                } else {
                    Player_LookAtTargetActor(this, 0);
                }
            } else {
                if (Actor_PlayerIsAimingReadyFpsItem(this) != 0) {
                    this->upperBodyRot.y =
                        Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                    this->upperBodyRot.y += 5000;
                    this->upperBodyYawOffset = -5000;
                }
            }
        }
    }
}

static AnimSfxEntry sHorseDismountAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_5, 0) },
    { NA_SE_PL_GET_OFF_HORSE, ANIMSFX_DATA(ANIMSFX_TYPE_1, 10) },
    { NA_SE_PL_SLIPDOWN, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 25) },
};

void Player_Action_DismountHorse(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    Player_AdjustVerticalPosWhileRiding(this, 1.0f, 10.0f);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        EnHorse* rideActor = (EnHorse*)this->rideActor;

        Player_SetupStandStill(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_RIDING_HORSE;
        this->actor.parent = NULL;
        AREG(6) = 0;

        if (Flags_GetEventChkInf(EVENTCHKINF_EPONA_OBTAINED) || (DREG(1) != 0)) {
            gSaveContext.save.info.horseData.pos.x = rideActor->actor.world.pos.x;
            gSaveContext.save.info.horseData.pos.y = rideActor->actor.world.pos.y;
            gSaveContext.save.info.horseData.pos.z = rideActor->actor.world.pos.z;
            gSaveContext.save.info.horseData.angle = rideActor->actor.shape.rot.y;
        }
    } else {
        Camera_RequestSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_NORMAL0);

        if (this->mountSide < 0) {
            sHorseDismountAnimSfx[0].data = ANIMSFX_DATA(ANIMSFX_TYPE_5, 40);
        } else {
            sHorseDismountAnimSfx[0].data = ANIMSFX_DATA(ANIMSFX_TYPE_5, 29);
        }
        Player_ProcessAnimSfxList(this, sHorseDismountAnimSfx);
    }
}

static AnimSfxEntry sSwimAnimSfx[] = {
    { NA_SE_PL_SWIM, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 0) },
};

void Player_SetSwimMovement(Player* this, f32* speedXZ, f32 swimSpeed, s16 swimYaw) {
    Player_UpdateSwimMovement(this, speedXZ, swimSpeed, swimYaw);
    Player_ProcessAnimSfxList(this, sSwimAnimSfx);
}

void Player_SetupSwim(PlayState* play, Player* this, s16 swimYaw) {
    Player_SetupAction(play, this, Player_Action_Swim, 0);
    this->actor.shape.rot.y = this->yaw = swimYaw;
    Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_SetupZTargetSwim(PlayState* play, Player* this) {
    Player_SetupAction(play, this, Player_Action_ZTargetSwim, 0);
    Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_UpdateZoraSwimSpeed(Player* this) {
    this->speedXZ = Math_CosS(this->shapePitchOffset) * this->zoraSwimYTarget;
    this->actor.velocity.y = -Math_SinS(this->shapePitchOffset) * this->zoraSwimYTarget;
}

void Player_UpdateZoraSwimYaw(Player* this, f32 arg1) {
    f32 temp_fv0;
    s16 temp_ft0;

    Math_AsymStepToF(&this->zoraSwimYTarget, arg1, 1.0f, (fabsf(this->zoraSwimYTarget) * 0.01f) + 0.4f);
    temp_fv0 = Math_CosS(sControlInput->rel.stick_x * 0x10E);

    temp_ft0 = (((sControlInput->rel.stick_x >= 0) ? 1 : -1) * (1.0f - temp_fv0) * -1100.0f);
    temp_ft0 = CLAMP(temp_ft0, -0x1F40, 0x1F40);

    this->yaw += temp_ft0;
}

#define PLAYER_ZORA_SWIM_SPEED_MULTIPLIER 2.0f

void Player_Action_ZoraSwim(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 rollInput;
    s16 yawTarget;
    s16 pitchInput;
    s16 sp3C;
    s16 sp3A;
    f32 fov = 0.0f;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    Player_SetVerticalWaterSpeed(this);

    if (Player_TryActionChangeList(play, this, sActionChangeList11, false)) {
        return;
    }

    if (Player_TryDive(play, this, sControlInput)) {
        return;
    }

    if (Player_CheckZoraSwimBonk(play, this, &this->speedXZ, 0.0f)) {
        return;
    }

    speedTarget = 0.0f;

    if (this->av2.actionVar2 != 0) {
        if ((!((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF)) &&
             !CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) ||
            (this->currentBoots != PLAYER_BOOTS_ZORA)) {
            this->endZoraSwim = true;
        }

        if (LinkAnimation_Update(play, &this->skelAnime) && (DECR(this->av2.actionVar2) == 0)) {
            if (this->endZoraSwim == true) {
                this->stateFlags3 &= ~PLAYER_STATE3_ZORA_SWIMMING;
                Player_AnimPlayOnceAdjusted(play, this, &gLinkAdultSkelPz_swimtowaitAnim);
            } else {
                Player_AnimPlayLoopAdjusted(play, this, &gLinkAdultSkelPz_fishswimAnim);
            }
        } else {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
            Math_ScaledStepToS(&this->yaw, yawTarget, 0x640);

            if (this->skelAnime.curFrame >= 13.0f) {
                if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
                    speedTarget = 12.0f * PLAYER_ZORA_SWIM_SPEED_MULTIPLIER;
                } else {
                    speedTarget = 12.0f;
                }

                if (LinkAnimation_OnFrame(&this->skelAnime, 13.0f)) {
                    this->zoraSwimYTarget = 16.0f;
                }
                this->stateFlags3 |= PLAYER_STATE3_ZORA_SWIMMING;
            } else {
                speedTarget = 0.0f;
            }
        }

        Math_SmoothStepToS(&this->zoraSwimRoll, sControlInput->rel.stick_x * 0xC8, 0xA, 0x3E8, 0x64);
        // Are these IREGs even set up?
        Math_SmoothStepToS(&this->zoraSwimTurnInputSmoothed, this->zoraSwimRoll, IREG(40) + 1, IREG(41), IREG(42));
    } else if (this->endZoraSwim == false) {
        LinkAnimation_Update(play, &this->skelAnime);

        if ((!((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF)) &&
             !CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) ||
            (this->currentBoots != PLAYER_BOOTS_ZORA)) {
            this->stateFlags3 &= ~PLAYER_STATE3_ZORA_SWIMMING;
            Player_AnimPlayOnceAdjusted(play, this, &gLinkAdultSkelPz_swimtowaitAnim);
            this->endZoraSwim = true;
        } else {
            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_R)) {
                Player_PlaySfx(this, NA_SE_PL_SWORD_CHARGE);
            }
            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
                speedTarget = 9.0f * PLAYER_ZORA_SWIM_SPEED_MULTIPLIER;
                fov = 10.0f;
            } else {
                speedTarget = 9.0f;
            }
            Player_PlaySfx(this, NA_SE_PL_SWIM - SFX_FLAG);
        }

        // Y
        pitchInput = sControlInput->rel.stick_y * 0xC8;
        if (this->shapeRollOffset != 0) {
            this->shapeRollOffset--;
            pitchInput = CLAMP_MAX(pitchInput, (s16)(this->floorPitch - 0xFA0));
        }

        if ((this->shapePitchOffset >= -0x1555) && (this->actor.yDistToWater < (this->ageProperties->unk_24 + 10.0f))) {
            pitchInput = CLAMP_MIN(pitchInput, 0x7D0);
        }
        Math_SmoothStepToS(&this->shapePitchOffset, pitchInput, 4, 0xFA0, 0x190);

        // X
        rollInput = sControlInput->rel.stick_x * 0x64;
        if (Math_ScaledStepToS(&this->zoraSwimTurnInput, rollInput, 0x384) && (rollInput == 0)) {
            Math_SmoothStepToS(&this->zoraSwimRoll, 0, 4, 0x5DC, 0x64);
            Math_SmoothStepToS(&this->zoraSwimTurnInputSmoothed, this->zoraSwimRoll, IREG(44) + 1, IREG(45), IREG(46));
        } else {
            sp3C = this->zoraSwimRoll;
            sp3A = (this->zoraSwimTurnInput < 0) ? -0x3A98 : 0x3A98;
            this->zoraSwimRoll += this->zoraSwimTurnInput;
            Math_SmoothStepToS(&this->zoraSwimTurnInputSmoothed, this->zoraSwimRoll, IREG(47) + 1, IREG(48), IREG(49));

            if ((ABS_ALT(this->zoraSwimTurnInput) > 0xFA0) &&
                ((((sp3C + this->zoraSwimTurnInput) - sp3A) * (sp3C - sp3A)) <= 0)) {
                Player_PlaySfx(this, NA_SE_PL_MOVE_BUBBLE);
            }
        }

        if (sYDistToFloor < 20.0f) {
            // Dust effect
            // func_80850D20(play, this);
        }
    } else {
        // Come to a stop after letting go of A
        Math_SmoothStepToS(&this->zoraSwimRoll, 0, 4, 0xFA0, 0x190);
        if ((this->skelAnime.curFrame <= 5.0f) || !Player_TryStartZoraSwim(play, this)) {
            if (LinkAnimation_Update(play, &this->skelAnime)) {
                Player_SetupSwimIdle(play, this);
            }
        }

        // Player_ResetCylinder(this);
    }

    // Bounce off of floor
    if ((this->shapeRollOffset < 8) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        DynaPolyActor* dynaActor;

        if ((this->actor.floorBgId == BGCHECK_SCENE) ||
            ((dynaActor = DynaPoly_GetActor(&play->colCtx, this->actor.floorBgId)) == NULL)) {
            this->shapePitchOffset += (s16)((-this->floorPitch - this->shapePitchOffset) * 2);
            this->shapeRollOffset = 0xF;
        }

        // func_80850D20(play, this);
        Player_PlaySfx(this, NA_SE_PL_BODY_BOUND);
    }

    if (speedTarget >= 9.0f) {
        // Change FOV for speed effect
        Math_SmoothStepToF(&play->mainCamera.fov, 70.0f + fov, 1.0f, 4.0f, 0.0f);
    }

    Player_UpdateZoraSwimYaw(this, speedTarget);
    Player_UpdateZoraSwimSpeed(this);
}

// Player_SetupZoraSwim
void Player_SetupZoraSwim(PlayState* play, Player* this) {
    this->currentBoots = PLAYER_BOOTS_ZORA;
    this->prevBoots = PLAYER_BOOTS_ZORA;
    Player_SetupAction(play, this, Player_Action_ZoraSwim, 0);
    this->zoraSwimYTarget = sqrtf(SQ(this->speedXZ) + SQ(this->actor.velocity.y));
    // Player_OverrideBlureColors(play, this, 1, 8);
    this->currentBoots = PLAYER_BOOTS_ZORA;
    this->prevBoots = PLAYER_BOOTS_ZORA;
}

// Player_TryStartZoraSwim (func_80850734)
s32 Player_TryStartZoraSwim(PlayState* play, Player* this) {
    if ((this->currentBoots == PLAYER_BOOTS_ZORA) && CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {
        Player_SetupZoraSwim(play, this);
        this->stateFlags2 |= PLAYER_STATE2_DIVING;
        LinkAnimation_Change(play, &this->skelAnime, &gLinkAdultSkelPz_waterrollAnim, PLAYER_ANIM_ADJUSTED_SPEED, 4.0f,
                             Animation_GetLastFrame(&gLinkAdultSkelPz_waterrollAnim), ANIMMODE_ONCE, -6.0f);
        this->av2.actionVar2 = 5;
        this->endZoraSwim = false;
        this->zoraSwimYTarget = this->speedXZ;
        this->actor.velocity.y = 0.0f;
        Player_PlaySfx(this, NA_SE_PL_ROLL);
        return true;
    }
    return false;
}

// Modified Player_Action_SwimIdle
void Player_Action_SwimIdle(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_swimer_swim_wait);
    Player_SetVerticalWaterSpeed(this);

    if (this->av2.actionVar2 != 0) {
        this->av2.actionVar2--;
    }

    if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        this->av2.actionVar2 = 0;
    }

    if (!Player_CheckActorTalkOffered(play) && !Player_TryActionChangeList(play, this, sActionChangeList11, true) &&
        !Player_TryDive(play, this, sControlInput) &&
        ((this->av2.actionVar2 != 0) || !Player_TryStartZoraSwim(play, this))) {
        speedTarget = 0.0f;
        yawTarget = this->actor.shape.rot.y;

        if (this->attentionMode != 1) {
            this->attentionMode = 0;
        }

        if (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) {
            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_StartReturnToStandStillWithAnim(
                    this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_short_landing, this->modelAnimType), play);
                Player_PlayLandingSfx(this);
            }
        } else {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

            if (speedTarget != 0.0f) {
                s16 temp = this->actor.shape.rot.y - yawTarget;

                if ((ABS(temp) > 0x6000) && !Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
                    return;
                }

                if (Player_CheckCalmAndTryBattleTargeting(this)) {
                    Player_SetupZTargetSwim(play, this);
                } else {
                    Player_SetupSwim(play, this, yawTarget);
                }
            }
        }

        Player_UpdateSwimMovement(this, &this->speedXZ, speedTarget, yawTarget);
    }
}

#if 0
void Player_Action_SwimIdle(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_swimer_swim_wait);
    Player_SetVerticalWaterSpeed(this);

    if (!Player_CheckActorTalkOffered(play) && !Player_TryActionChangeList(play, this, sActionChangeList11, true) &&
        !Player_TryDive(play, this, sControlInput)) {
        if (this->attentionMode != 1) {
            this->attentionMode = 0;
        }

        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            speedTarget = 0.0f;
            yawTarget = this->actor.shape.rot.y;

            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_StartReturnToStandStillWithAnim(
                    this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_short_landing, this->modelAnimType), play);
                Player_PlayLandingSfx(this);
            }
        } else {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

            if (speedTarget != 0.0f) {
                s16 temp = this->actor.shape.rot.y - yawTarget;

                if ((ABS(temp) > 0x6000) && !Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
                    return;
                }

                if (Player_CheckCalmAndTryBattleTargeting(this)) {
                    Player_SetupZTargetSwim(play, this);
                } else {
                    Player_SetupSwim(play, this, yawTarget);
                }
            }
        }

        Player_UpdateSwimMovement(this, &this->speedXZ, speedTarget, yawTarget);
    }
}
#endif

void Player_Action_SpawnSwimming(Player* this, PlayState* play) {
    if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
        this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

        Player_PlaySwimAnim(play, this, NULL, this->speedXZ);
        Player_SetVerticalWaterSpeed(this);

        if (DECR(this->av2.actionVar2) == 0) {
            Player_SetupSwimIdle(play, this);
        }
    }
}

// Modified Player_Action_Swim (Player_Action_57)
void Player_Action_Swim(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s16 temp;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetVerticalWaterSpeed(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList11, true) &&
        !Player_TryDive(play, this, sControlInput)) {
        Player_PlaySwimAnim(play, this, sControlInput, this->speedXZ);
        if (((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF))) {
            speedTarget = this->speedXZ;
            yawTarget = this->actor.shape.rot.y;
        } else {
            Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
        }
        temp = this->actor.shape.rot.y - yawTarget;
        if (!Player_TryStartZoraSwim(play, this)) {
            if (Player_CheckCalmAndTryBattleTargeting(this)) {
                Player_SetupZTargetSwim(play, this);
            } else {
                if ((speedTarget == 0.0f) || (ABS(temp) > 0x6000) || (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag)) {
                    Player_SetupSwimIdle(play, this);
                }
            }
            Player_SetSwimMovement(this, &this->speedXZ, speedTarget, yawTarget);
        }
    }
}

#if 0
void Player_Action_Swim(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;
    s16 temp;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    Player_PlaySwimAnim(play, this, sControlInput, this->speedXZ);
    Player_SetVerticalWaterSpeed(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList11, true) &&
        !Player_TryDive(play, this, sControlInput)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        temp = this->actor.shape.rot.y - yawTarget;
        if ((speedTarget == 0.0f) || (ABS(temp) > 0x6000) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
            Player_SetupSwimIdle(play, this);
        } else if (Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupZTargetSwim(play, this);
        }

        Player_SetSwimMovement(this, &this->speedXZ, speedTarget, yawTarget);
    }
}
#endif

s32 Player_TrySetZTargetSwimAnims(PlayState* play, Player* this, f32* arg2, s16* arg3) {
    LinkAnimationHeader* anim;
    s16 temp1;
    s32 temp2;

    temp1 = this->yaw - *arg3;

    if (ABS(temp1) > 0x6000) {
        anim = &gPlayerAnim_link_swimer_swim_wait;

        if (Math_StepToF(&this->speedXZ, 0.0f, 1.0f)) {
            this->yaw = *arg3;
        } else {
            *arg2 = 0.0f;
            *arg3 = this->yaw;
        }
    } else {
        temp2 = Player_GetZParallelMoveDirection(this, arg2, arg3, play);

        if (temp2 > 0) {
            anim = &gPlayerAnim_link_swimer_swim;
        } else if (temp2 < 0) {
            anim = &gPlayerAnim_link_swimer_back_swim;
        } else if ((temp1 = this->actor.shape.rot.y - *arg3) > 0) {
            anim = &gPlayerAnim_link_swimer_Rside_swim;
        } else {
            anim = &gPlayerAnim_link_swimer_Lside_swim;
        }
    }

    if (anim != this->skelAnime.animation) {
        Player_AnimChangeLoopSlowMorph(play, this, anim);
        return 1;
    }

    return 0;
}

void Player_Action_ZTargetSwim(Player* this, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget;

    Player_PlaySwimAnim(play, this, sControlInput, this->speedXZ);
    Player_SetVerticalWaterSpeed(this);

    if (!Player_TryActionChangeList(play, this, sActionChangeList11, true) &&
        !Player_TryDive(play, this, sControlInput)) {
        Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);

        if (speedTarget == 0.0f) {
            Player_SetupSwimIdle(play, this);
        } else if (!Player_CheckCalmAndTryBattleTargeting(this)) {
            Player_SetupSwim(play, this, yawTarget);
        } else {
            Player_TrySetZTargetSwimAnims(play, this, &speedTarget, &yawTarget);
        }

        Player_SetSwimMovement(this, &this->speedXZ, speedTarget, yawTarget);
    }
}

void Player_UpdateDiveMovement(PlayState* play, Player* this, f32 arg2) {
    f32 speedTarget;
    s16 yawTarget;

    Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, SPEED_MODE_LINEAR, play);
    Player_UpdateSwimMovement(this, &this->speedXZ, speedTarget * 0.5f, yawTarget);
    Player_UpdateSwimMovement(this, &this->actor.velocity.y, arg2, this->yaw);
}

void Player_Action_Dive(Player* this, PlayState* play) {
    f32 sp2C;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    this->actor.gravity = 0.0f;
    Player_UpdateUpperBody(this, play);

    if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
        if (this->currentBoots == PLAYER_BOOTS_ZORA && this->zoraDescendFlag) {
            Player_SetupSwimIdle(play, this);
            return;
        }

        if (this->av1.actionVar1 == 0) {
            if (this->av2.actionVar2 == 0) {
                if (LinkAnimation_Update(play, &this->skelAnime) ||
                    ((this->skelAnime.curFrame >= 22.0f) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))) {
                    Player_RiseFromDive(play, this);
                } else if (LinkAnimation_OnFrame(&this->skelAnime, 20.0f) != 0) {
                    this->actor.velocity.y = -2.0f;
                }

                Player_StepSpeedXZToZero(this);
                return;
            }

            Player_PlaySwimAnim(play, this, sControlInput, this->actor.velocity.y);
            this->shapePitchOffset = 16000;

            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) && !Player_ActionChange_TryGetItemOrCarry(this, play) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
                (this->actor.yDistToWater < sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)])) {
                Player_UpdateDiveMovement(play, this, -2.0f);
            } else {
                this->av1.actionVar1++;
                Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim_wait);
            }
        } else if (this->av1.actionVar1 == 1) {
            LinkAnimation_Update(play, &this->skelAnime);
            Player_SetVerticalWaterSpeed(this);

            if (this->shapePitchOffset < 10000) {
                this->av1.actionVar1++;
                this->av2.actionVar2 = this->actor.yDistToWater;
                Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim);
            }
        } else if (!Player_TryDive(play, this, sControlInput)) {
            sp2C = (this->av2.actionVar2 * 0.018f) + 4.0f;

            if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
                sControlInput = NULL;
            }

            Player_PlaySwimAnim(play, this, sControlInput, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->shapePitchOffset, -10000, 800);

            if (sp2C > 8.0f) {
                sp2C = 8.0f;
            }

            Player_UpdateDiveMovement(play, this, sp2C);
        }
    }
}

void Player_FinishGetItem(PlayState* play, Player* this) {
    this->giDrawIdPlusOne = 0;
    this->stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR);
    this->getItemId = GI_NONE;
    Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
}

void Play_StartFinishGetItem(PlayState* play, Player* this) {
    Player_FinishGetItem(play, this);
    Player_AddRootYawToShapeYaw(this);
    Player_SetupStandStill(this, play);
    this->yaw = this->actor.shape.rot.y;
}

s32 Player_TrySetGetItemText(PlayState* play, Player* this) {
    GetItemEntry* giEntry;
    s32 temp1;
    s32 temp2;

    if (this->getItemId == GI_NONE) {
        return 1;
    }

    if (this->av1.actionVar1 == 0) {
        giEntry = &sGetItemTable[this->getItemId - 1];
        this->av1.actionVar1 = 1;

        Message_StartTextbox(play, giEntry->textId, &this->actor);
        Item_Give(play, giEntry->itemId);

        if (((this->getItemId >= GI_RUPEE_GREEN) && (this->getItemId <= GI_RUPEE_RED)) ||
            ((this->getItemId >= GI_RUPEE_PURPLE) && (this->getItemId <= GI_RUPEE_GOLD)) ||
            ((this->getItemId >= GI_RUPEE_GREEN_LOSE) && (this->getItemId <= GI_RUPEE_PURPLE_LOSE)) ||
            (this->getItemId == GI_RECOVERY_HEART)) {
            Audio_PlaySfxGeneral(NA_SE_SY_GET_BOXITEM, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        } else {
            if ((this->getItemId == GI_HEART_CONTAINER_2) || (this->getItemId == GI_HEART_CONTAINER) ||
                ((this->getItemId == GI_HEART_PIECE) &&
                 ((gSaveContext.save.info.inventory.questItems & 0xF0000000) == (4 << QUEST_HEART_PIECE_COUNT)))) {
                temp1 = NA_BGM_HEART_GET | 0x900;
            } else {
                temp1 = temp2 = (this->getItemId == GI_HEART_PIECE) ? NA_BGM_SMALL_ITEM_GET : NA_BGM_ITEM_GET | 0x900;
            }
            Audio_PlayFanfare(temp1);
        }
    } else {
        if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
            if (this->getItemId == GI_SILVER_GAUNTLETS) {
                play->nextEntranceIndex = ENTR_DESERT_COLOSSUS_0;
                play->transitionTrigger = TRANS_TRIGGER_START;
                gSaveContext.nextCutsceneIndex = 0xFFF1;
                play->transitionType = TRANS_TYPE_SANDSTORM_END;
                this->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
                Player_TryCsAction(play, NULL, PLAYER_CSACTION_WAIT);
            }
            this->getItemId = GI_NONE;
        }
    }

    return 0;
}

void Player_Action_SurfaceFromDive(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || Player_TrySetGetItemText(play, this)) {
            Player_FinishGetItem(play, this);
            Player_SetupSwimIdle(play, this);
            Player_ResetSubCam(play, this);
        }
    } else {
        if ((this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_SetGetItemDrawIdPlusOne(this, play);
            Player_ResetSubCam(play, this);
            Player_SetUseItemCam(play, 8);
        } else if (LinkAnimation_OnFrame(&this->skelAnime, 5.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
        }
    }

    Player_SetVerticalWaterSpeed(this);
    Player_UpdateSwimMovement(this, &this->speedXZ, 0.0f, this->actor.shape.rot.y);
}

void Player_Action_DamagedSwim(Player* this, PlayState* play) {
    Player_SetVerticalWaterSpeed(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupSwimIdle(play, this);
    }

    Player_UpdateSwimMovement(this, &this->speedXZ, 0.0f, this->actor.shape.rot.y);
}

void Player_Action_Drown(Player* this, PlayState* play) {
    Player_SetVerticalWaterSpeed(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_FinishDie(play, this);
    }

    Player_UpdateSwimMovement(this, &this->speedXZ, 0.0f, this->actor.shape.rot.y);
}

static s16 sWarpSongEntrances[] = {
    ENTR_SACRED_FOREST_MEADOW_2,
    ENTR_DEATH_MOUNTAIN_CRATER_4,
    ENTR_LAKE_HYLIA_8,
    ENTR_DESERT_COLOSSUS_5,
    ENTR_GRAVEYARD_7,
    ENTR_TEMPLE_OF_TIME_7,
};

void Player_Action_PlayOcarina(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoopAdjusted(play, this, &gPlayerAnim_link_normal_okarina_swing);
        this->av2.actionVar2 = 1;
        if (this->stateFlags2 & (PLAYER_STATE2_NEAR_OCARINA_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR)) {
            this->stateFlags2 |= PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR;
        } else {
            Message_StartOcarina(play, OCARINA_ACTION_FREE_PLAY);
        }
        return;
    }

    if (this->av2.actionVar2 == 0) {
        return;
    }

    if (play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));

        if ((this->talkActor != NULL) && (this->talkActor == this->ocarinaActor)) {
            Player_StartTalkToActor(play, this->talkActor);
        } /* else if (this->naviTextId < 0) {
            this->talkActor = this->naviActor;
            this->naviActor->textId = -this->naviTextId;
            Player_StartTalkToActor(play, this->talkActor);
        } */ else if (!Player_ActionChange_TryItemCsOrFirstPerson(this, play)) {
            Player_StartReturnToStandStillWithAnim(this, &gPlayerAnim_link_normal_okarina_end, play);
        }

        this->stateFlags2 &= ~(PLAYER_STATE2_NEAR_OCARINA_ACTOR | PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR |
                               PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
        this->ocarinaActor = NULL;
    } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_02) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex = sWarpSongEntrances[play->msgCtx.lastPlayedSong];
        gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams = 0x5FF;
        gSaveContext.respawn[RESPAWN_MODE_RETURN].data = play->msgCtx.lastPlayedSong;

        this->csAction = PLAYER_CSACTION_NONE;
        this->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;

        Player_TryCsAction(play, NULL, PLAYER_CSACTION_WAIT);
        play->mainCamera.stateFlags &= ~CAM_STATE_EXTERNAL_FINISHED;

        this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
        this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_GENERAL;

        if (Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, DEMOKANKYO_WARP_OUT) ==
            NULL) {
            Environment_WarpSongLeave(play);
        }

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    }
}

void Player_Action_ThrowDekuNut(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_StartReturnToStandStillWithAnim(this, &gPlayerAnim_link_normal_light_bom_end, play);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Inventory_ChangeAmmo(ITEM_DEKU_NUT, -1);
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ARROW, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].x,
                    this->bodyPartsPos[PLAYER_BODYPART_R_HAND].y, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].z, 4000,
                    this->actor.shape.rot.y, 0, ARROW_NUT);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }

    Player_StepSpeedXZToZero(this);
}

static AnimSfxEntry sChildOpenChestAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_7, 87) },
    { NA_SE_VO_LI_CLIMB_END, ANIMSFX_DATA(ANIMSFX_TYPE_4, 87) },
    { NA_SE_VO_LI_AUTO_JUMP, ANIMSFX_DATA(ANIMSFX_TYPE_4, 69) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_5, 123) },
};

void Player_Action_GetItem(Player* this, PlayState* play) {
    s32 cond;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av2.actionVar2 != 0) {
            if (this->av2.actionVar2 >= 2) {
                this->av2.actionVar2--;
            }

            if (Player_TrySetGetItemText(play, this) && (this->av2.actionVar2 == 1)) {
                cond = ((this->talkActor != NULL) && (this->exchangeItemId < 0)) ||
                       (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA);

                if (cond || (gSaveContext.healthAccumulator == 0)) {
                    if (cond) {
                        Player_FinishGetItem(play, this);
                        this->exchangeItemId = EXCH_ITEM_NONE;

                        if (Player_TryPullOcarina(play, this) == 0) {
                            Player_StartTalkToActor(play, this->talkActor);
                        }
                    } else {
                        Play_StartFinishGetItem(play, this);
                    }
                }
            }
        } else {
            Player_FinishAnimMovement(this);

            if (this->getItemId == GI_ICE_TRAP) {
                this->stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR);

                if (this->getItemId != GI_ICE_TRAP) {
                    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, this->actor.world.pos.x,
                                this->actor.world.pos.y + 100.0f, this->actor.world.pos.z, 0, 0, 0, 0);
                    Player_SetupStandStill(this, play);
                } else {
                    this->actor.colChkInfo.damage = 0;
                    Player_TakeColliderDamage(play, this, 3, 0.0f, 0.0f, 0, 20);
                }
                return;
            }

            if (this->skelAnime.animation == &gPlayerAnim_link_normal_box_kick) {
                Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_demo_get_itemB);
            } else {
                Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_demo_get_itemA);
            }

            this->av2.actionVar2 = 2;
            Player_SetUseItemCam(play, 9);
        }
    } else {
        if (this->av2.actionVar2 == 0) {
            if (!LINK_IS_ADULT) {
                Player_ProcessAnimSfxList(this, sChildOpenChestAnimSfx);
            }
            return;
        }

        if (this->skelAnime.animation == &gPlayerAnim_link_demo_get_itemB) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000, 4000);
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, 21.0f)) {
            Player_SetGetItemDrawIdPlusOne(this, play);
        }
    }
}

static AnimSfxEntry sSwordSwingAnimSfx[] = {
    { NA_SE_IT_MASTER_SWORD_SWING, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 60) },
};

void Player_PlaySwordSwingSfx(Player* this) {
    Player_ProcessAnimSfxList(this, sSwordSwingAnimSfx);
}

static AnimSfxEntry sChildFinishTimeTravelAnimSfx[] = {
    { NA_SE_VO_LI_AUTO_JUMP, ANIMSFX_DATA(ANIMSFX_TYPE_4, 5) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_5, 15) },
};

void Player_Action_FinishTimeTravel(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av1.actionVar1 == 0) {
            if (DECR(this->av2.actionVar2) == 0) {
                this->av1.actionVar1 = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandStill(this, play);
        }
    } else {
        if (LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 158.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
            return;
        }

        if (!LINK_IS_ADULT) {
            Player_ProcessAnimSfxList(this, sChildFinishTimeTravelAnimSfx);
        } else {
            Player_PlaySwordSwingSfx(this);
        }
    }
}

static u8 sBottleDrinkEffects[] = {
    0x01, 0x03, 0x02, 0x04, 0x04,
};

void Player_Action_DrinkFromBottle(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av2.actionVar2 == 0) {
            if (this->itemAction == PLAYER_IA_BOTTLE_POE) {
                s32 rand = Rand_S16Offset(-1, 3);

                if (rand == 0) {
                    rand = 3;
                }

                if ((rand < 0) && (gSaveContext.save.info.playerData.health <= 0x10)) {
                    rand = 3;
                }

                if (rand < 0) {
                    Health_ChangeBy(play, -0x10);
                } else {
                    gSaveContext.healthAccumulator = rand * 0x10;
                }
            } else {
                s32 sp28 = sBottleDrinkEffects[this->itemAction - PLAYER_IA_BOTTLE_POTION_RED];

                if (sp28 & 1) {
                    gSaveContext.healthAccumulator = 0x140;
                }

                if (sp28 & 2) {
                    Magic_Fill(play);
                }

                if (sp28 & 4) {
                    gSaveContext.healthAccumulator = 0x50;
                }
            }

            Player_AnimPlayLoopAdjusted(play, this, &gPlayerAnim_link_bottle_drink_demo_wait);
            this->av2.actionVar2 = 1;
            return;
        }

        Player_SetupStandStill(this, play);
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
    } else if (this->av2.actionVar2 == 1) {
        if ((gSaveContext.healthAccumulator == 0) && (gSaveContext.magicState != MAGIC_STATE_FILL)) {
            Player_AnimChangeOnceMorphAdjusted(play, this, &gPlayerAnim_link_bottle_drink_demo_end);
            this->av2.actionVar2 = 2;
            Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        }
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DRINK - SFX_FLAG);
    } else if ((this->av2.actionVar2 == 2) && LinkAnimation_OnFrame(&this->skelAnime, 29.0f)) {
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
    }
}

static BottleCatchInfo sBottleCatchInfos[] = {
    { ACTOR_EN_ELF, ITEM_BOTTLE_FAIRY, PLAYER_IA_BOTTLE_FAIRY, 0x46 },
    { ACTOR_EN_FISH, ITEM_BOTTLE_FISH, PLAYER_IA_BOTTLE_FISH, 0x47 },
    { ACTOR_EN_ICE_HONO, ITEM_BOTTLE_BLUE_FIRE, PLAYER_IA_BOTTLE_FIRE, 0x5D },
    { ACTOR_EN_INSECT, ITEM_BOTTLE_BUG, PLAYER_IA_BOTTLE_BUG, 0x7A },
};

void Player_Action_SwingBottle(Player* this, PlayState* play) {
    BottleSwingAnimInfo* bottleSwingAnims;
    BottleCatchInfo* catchInfo;
    s32 temp;
    s32 i;

    bottleSwingAnims = &sBottleSwingAnims[this->av2.actionVar2];
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av1.actionVar1 != 0) {
            if (this->av2.actionVar2 == 0) {
                Message_StartTextbox(play, sBottleCatchInfos[this->av1.actionVar1 - 1].textId, &this->actor);
                Audio_PlayFanfare(NA_BGM_ITEM_GET | 0x900);
                this->av2.actionVar2 = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->av1.actionVar1 = 0;
                Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            Player_SetupStandStill(this, play);
        }
    } else {
        if (this->av1.actionVar1 == 0) {
            // Frames left until end of anim? Or until catch frame of anim?
            temp = this->skelAnime.curFrame - bottleSwingAnims->unk_08;

            if (temp >= 0) {
                if (bottleSwingAnims->unk_09 >= temp) {
                    if (this->av2.actionVar2 != 0) {
                        if (temp == 0) {
                            Player_PlaySfx(this, NA_SE_IT_SCOOP_UP_WATER);
                        }
                    }

                    if (this->actor.yDistToWater > 12.0f) {
                            this->av1.actionVar1 = 2;
                            this->av2.actionVar2 = 0;
                            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                            // this->interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_FISH, ABS(PLAYER_IA_BOTTLE_FISH));
                            Player_AnimPlayOnceAdjusted(play, this, bottleSwingAnims->bottleCatchAnim);
                            Player_SetUseItemCam(play, 4);
                    } else if (this->interactRangeActor != NULL) {
                        catchInfo = &sBottleCatchInfos[0];
                        for (i = 0; i < ARRAY_COUNT(sBottleCatchInfos); i++, catchInfo++) {
                            if (this->interactRangeActor->id == catchInfo->actorId) {
                                break;
                            }
                        }

                        if (i < ARRAY_COUNT(sBottleCatchInfos)) {
                            this->av1.actionVar1 = i + 1;
                            this->av2.actionVar2 = 0;
                            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                            this->interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, catchInfo->itemId, ABS(catchInfo->itemAction));
                            Player_AnimPlayOnceAdjusted(play, this, bottleSwingAnims->bottleCatchAnim);
                            Player_SetUseItemCam(play, 4);
                        }
                    }
                }
            }
        }
    }

    //! @bug If the animation is changed at any point above (such as by Player_SetupStandStill() or
    //! Player_AnimPlayOnceAdjusted()), it will change the curFrame to 0. This causes this flag to be set for one frame,
    //! at a time when it does not look like Player is swinging the bottle.
    if (this->skelAnime.curFrame <= 7.0f) {
        this->stateFlags1 |= PLAYER_STATE1_SWINGING_BOTTLE;
    }
}

static Vec3f sBottleFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void Player_Action_HealWithFairy(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandStill(this, play);
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 37.0f)) {
        Player_SpawnFairy(play, this, &this->leftHandPos, &sBottleFairyPosOffset, FAIRY_REVIVE_BOTTLE);
        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        Player_PlaySfx(this, NA_SE_EV_BOTTLE_CAP_OPEN);
        Player_PlaySfx(this, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 47.0f)) {
        gSaveContext.healthAccumulator = 0x140;
    }
}

static BottleDropInfo D_80854A28[] = {
    { ACTOR_BOTTLE_WATER, 0 },
    // { ACTOR_EN_FISH, FISH_DROPPED },
    { ACTOR_EN_ICE_HONO, 0 },
    { ACTOR_EN_INSECT, INSECT_TYPE_FIRST_DROPPED },
};

static AnimSfxEntry sBottleDropAnimSfx[] = {
    { NA_SE_VO_LI_AUTO_JUMP, ANIMSFX_DATA(ANIMSFX_TYPE_4, 38) },
    { NA_SE_EV_BOTTLE_CAP_OPEN, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 40) },
};

void Player_Action_DropItemFromBottle(Player* this, PlayState* play) {
    Player_StepSpeedXZToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandStill(this, play);
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 76.0f)) {
        BottleDropInfo* dropInfo = &D_80854A28[this->itemAction - PLAYER_IA_BOTTLE_FISH];

        Actor_Spawn(&play->actorCtx, play, dropInfo->actorId,
                    (Math_SinS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.x, this->leftHandPos.y,
                    (Math_CosS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.z, 0x4000, this->actor.shape.rot.y,
                    0, dropInfo->actorParams);

        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        return;
    }

    Player_ProcessAnimSfxList(this, sBottleDropAnimSfx);
}

static AnimSfxEntry sExchangeItemAnimSfx[] = {
    { NA_SE_PL_PUT_OUT_ITEM, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 30) },
};

void Player_Action_PresentExchangeItem(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av2.actionVar2 < 0) {
            Player_SetupStandStill(this, play);
        } else if (this->exchangeItemId == EXCH_ITEM_NONE) {
            Actor* talkActor = this->talkActor;

            this->giDrawIdPlusOne = 0;
            if (talkActor->textId != 0xFFFF) {
                this->actor.flags |= ACTOR_FLAG_TALK;
            }

            Player_StartTalkToActor(play, talkActor);
        } else {
            GetItemEntry* giEntry = &sGetItemTable[sExchangeGetItemIDs[this->exchangeItemId - 1] - 1];

            if (this->itemAction >= PLAYER_IA_ZELDAS_LETTER) {
                this->giDrawIdPlusOne = ABS(giEntry->gi);
            }

            if (this->av2.actionVar2 == 0) {
                Message_StartTextbox(play, this->actor.textId, &this->actor);

                if ((this->itemAction == PLAYER_IA_CHICKEN) || (this->itemAction == PLAYER_IA_POCKET_CUCCO)) {
                    Player_PlaySfx(this, NA_SE_EV_CHICKEN_CRY_M);
                }

                this->av2.actionVar2 = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->actor.flags &= ~ACTOR_FLAG_TALK;
                this->giDrawIdPlusOne = 0;

                if (this->av1.actionVar1 == 1) {
                    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_bottle_read_end);
                    this->av2.actionVar2 = -1;
                } else {
                    Player_SetupStandStill(this, play);
                }

                Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
            }
        }
    } else if (this->av2.actionVar2 >= 0) {
        Player_ProcessAnimSfxList(this, sExchangeItemAnimSfx);
    }

    if ((this->av1.actionVar1 == 0) && (this->targetActor != NULL)) {
        this->yaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_Action_RestrainedByEnemy(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_normal_re_dead_attack_wait);
    }

    if (Player_MashTimerThresholdExceeded(this, 0, 100)) {
        Player_SetupContextualStandStill(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
    }
}

void Player_Action_SlipOnSlope(Player* this, PlayState* play) {
    CollisionPoly* floorPoly;
    f32 speedTarget;
    f32 speedIncrStep;
    f32 speedDecrStep;
    s16 downwardSlopeYaw;
    s16 yawTarget;
    Vec3f slopeNormal;

    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);
    Player_TrySpawnDustAtFeet(play, this);
    func_800F4138(&this->actor.projectedPos, NA_SE_PL_SLIP_LEVEL - SFX_FLAG, this->actor.speed);

    if (Player_ActionChange_TryItemCsOrFirstPerson(this, play) == 0) {
        floorPoly = this->actor.floorPoly;

        if (floorPoly == NULL) {
            Player_SetupFallFromLedge(this, play);
            return;
        }

        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);

        yawTarget = downwardSlopeYaw;
        if (this->av1.actionVar1 != 0) {
            yawTarget = downwardSlopeYaw + 0x8000;
        }

        if (this->speedXZ < 0) {
            downwardSlopeYaw += 0x8000;
        }

        speedTarget = (1.0f - slopeNormal.y) * 40.0f;
        speedTarget = CLAMP(speedTarget, 0, 10.0f);
        speedIncrStep = (speedTarget * speedTarget) * 0.015f;
        speedDecrStep = slopeNormal.y * 0.01f;

        if (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) != FLOOR_EFFECT_1) {
            speedTarget = 0;
            speedDecrStep = slopeNormal.y * 10.0f;
        }

        if (speedIncrStep < 1.0f) {
            speedIncrStep = 1.0f;
        }

        if (Math_AsymStepToF(&this->speedXZ, speedTarget, speedIncrStep, speedDecrStep) && (speedTarget == 0)) {
            LinkAnimationHeader* anim;

            if (this->av1.actionVar1 == 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_down_slope_slip_end, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_up_slope_slip_end, this->modelAnimType);
            }
            Player_StartReturnToStandStillWithAnim(this, anim, play);
        }

        Math_SmoothStepToS(&this->yaw, downwardSlopeYaw, 10, 4000, 800);
        Math_ScaledStepToS(&this->actor.shape.rot.y, yawTarget, 2000);
    }
}

void Player_Action_SetDrawAndStartCutsceneAfterTimer(Player* this, PlayState* play) {
    if ((DECR(this->av2.actionVar2) == 0) && Player_SetupCsAction(play, this)) {
        Player_Cutscene_InitSetDraw(play, this, NULL);
        Player_SetupAction(play, this, Player_Action_CsAction, 0);
        Player_Action_CsAction(this, play);
    }
}

void Player_Action_AppearFromWarpSong(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_SetDrawAndStartCutsceneAfterTimer, 0);
    this->av2.actionVar2 = 40;
    Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, DEMOKANKYO_WARP_IN);
}

void Player_Action_DescendFromBlueWarp(Player* this, PlayState* play) {
    s32 pad;

    if ((this->av1.actionVar1 != 0) && (play->csCtx.curFrame < 305)) {
        this->actor.gravity = 0.0f;
        this->actor.velocity.y = 0.0f;
    } else if (sYDistToFloor < 150.0f) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (this->av2.actionVar2 == 0) {
                if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                    this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
                    Player_PlayLandingSfx(this);
                    this->av2.actionVar2 = 1;
                }
            } else {
                if ((play->sceneId == SCENE_KOKIRI_FOREST) && Player_SetupCsAction(play, this)) {
                    return;
                }
                Player_SetupStandStillMorph(this, play);
            }
        }
        Math_SmoothStepToF(&this->actor.velocity.y, 2.0f, 0.3f, 8.0f, 0.5f);
    }

    if ((play->sceneId == SCENE_CHAMBER_OF_THE_SAGES) && Player_SetupCsAction(play, this)) {
        return;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.playerCue != NULL)) {
        f32 sp28 = this->actor.world.pos.y;
        Player_Cutscene_SetPosAndYaw(play, this, play->csCtx.playerCue);
        this->actor.world.pos.y = sp28;
    }
}

// This is actually also voiding out
void Player_Action_EnterGrotto(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);

    if ((this->av2.actionVar2++ > 8) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {

        // Escape room custom void respawn positions
        if (play->sceneId == SCENE_ESCAPE_ROOM) {
            static SpecialRespawnInfo finalStretchRespawn = { { -5632.37f, -206.65f, -2196.52f }, DEG_TO_BINANG(90.0f) };
            SpecialRespawnInfo* respawnInfo;
            u8 noSpecialRespawn = false;

            switch (play->roomCtx.curRoom.num) {
                case 17:
                    respawnInfo = &finalStretchRespawn;
                    break;

                default:
                    noSpecialRespawn = true;
                    break;
            }

            if (!noSpecialRespawn) {
                Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
                gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = respawnInfo->pos;
                gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = respawnInfo->yaw;
            }
        }

        if (this->av1.actionVar1 != 0) {
            if (play->sceneId == SCENE_ICE_CAVERN) {
                Play_TriggerRespawn(play);
                play->nextEntranceIndex = ENTR_ICE_CAVERN_0;
            } else if (this->av1.actionVar1 < 0) {
                Play_TriggerRespawn(play);
            } else {
                Play_TriggerVoidOut(play);
            }

            play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
            Sfx_PlaySfxCentered(NA_SE_OC_ABYSS);
        } else {
            play->transitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = 0xFF;
        }

        play->transitionTrigger = TRANS_TRIGGER_START;
    }
}

void Player_Action_TryOpenDoorFromSpawn(Player* this, PlayState* play) {
    Player_ActionChange_TryOpenDoor(this, play);
}

void Player_Action_JumpFromGrotto(Player* this, PlayState* play) {
    this->actor.gravity = -1.0f;

    LinkAnimation_Update(play, &this->skelAnime);

    if (this->actor.velocity.y < 0.0f) {
        Player_SetupFallFromLedge(this, play);
    } else if (this->actor.velocity.y < 6.0f) {
        Math_StepToF(&this->speedXZ, 3.0f, 0.5f);
    }
}

void Player_Action_ShootingGalleryPlay(Player* this, PlayState* play) {
    this->attentionMode = 2;

    Player_RequestFpsCamera(play, this);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_UpdateUpperBody(this, play);

    this->upperBodyRot.y = Player_SetFirstPersonAimLookAngles(play, this, 1, 0) - this->actor.shape.rot.y;
    this->lookFlags |= 0x80;

    if (play->shootingGalleryStatus < 0) {
        play->shootingGalleryStatus++;
        if (play->shootingGalleryStatus == 0) {
            Player_ClearLookAndAttention(this, play);
        }
    }
}

void Player_Action_FrozenInIce(Player* this, PlayState* play) {
    if (this->av1.actionVar1 >= 0) {
        if (this->av1.actionVar1 < 6) {
            this->av1.actionVar1++;
        }

        if (Player_MashTimerThresholdExceeded(this, 1, 100)) {
            this->av1.actionVar1 = -1;
            EffectSsIcePiece_SpawnBurst(play, &this->actor.world.pos, this->actor.scale.x);
            Player_PlaySfx(this, NA_SE_PL_ICE_BROKEN);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_FROZEN_IN_ICE;
        }

        if ((play->gameplayFrames % 4) == 0) {
            Player_InflictDamageAndCheckForDeath(play, -1);
        }
    } else {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_SetupContextualStandStill(this, play);
            Player_SetInvincibilityTimerNoDamageFlash(this, -20);
        }
    }
}

void Player_Action_StartElectricShock(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_RoundUpInvincibilityTimer(this);

    if (((this->av2.actionVar2 % 25) != 0) || Player_InflictDamage(play, this, -1)) {
        if (DECR(this->av2.actionVar2) == 0) {
            Player_SetupContextualStandStill(this, play);
        }
    }

    this->shockTimer = 40;
    func_8002F8F0(&this->actor, NA_SE_VO_LI_TAKEN_AWAY - SFX_FLAG + this->ageProperties->unk_92);
}

s32 Player_CheckNoDebugModeCombo(Player* this, PlayState* play) {
    #ifdef ENABLE_DEBUG_FEATURES
    sControlInput = &play->state.input[0];

    if ((CHECK_BTN_ALL(sControlInput->cur.button, BTN_A | BTN_L | BTN_R) &&
         CHECK_BTN_ALL(sControlInput->press.button, BTN_B)) ||
        (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DRIGHT))) {

        sDebugModeFlag ^= 1;

        if (sDebugModeFlag) {
            Camera_RequestMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_Z_AIM);
        }
    }

    if (sDebugModeFlag) {
        f32 speed;

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
            speed = 100.0f;
        } else {
            speed = 20.0f;
        }

        DebugCamera_ScreenText(3, 2, "DEBUG MODE");

        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_L)) {
            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                this->actor.world.pos.y += speed;
            } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {
                this->actor.world.pos.y -= speed;
            }

            if (CHECK_BTN_ANY(sControlInput->cur.button, BTN_DUP | BTN_DLEFT | BTN_DDOWN | BTN_DRIGHT)) {
                s16 angle;
                s16 temp;

                angle = temp = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

                if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DDOWN)) {
                    angle = temp + 0x8000;
                } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DLEFT)) {
                    angle = temp + 0x4000;
                } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DRIGHT)) {
                    angle = temp - 0x4000;
                }

                this->actor.world.pos.x += speed * Math_SinS(angle);
                this->actor.world.pos.z += speed * Math_CosS(angle);
            }
        }

        Player_ZeroSpeedXZ(this);

        this->actor.gravity = 0.0f;
        this->actor.velocity.z = 0.0f;
        this->actor.velocity.y = 0.0f;
        this->actor.velocity.x = 0.0f;

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DLEFT)) {
            Flags_SetTempClear(play, play->roomCtx.curRoom.num);
        }

        Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);

        return 0;
    }

    return 1;
    #else
    return 1;
    #endif
}

void Player_BowStringPhysics(Player* this) {
    this->spinAttackTimer += this->unk_85C;
    this->unk_85C -= this->spinAttackTimer * 5.0f;
    this->unk_85C *= 0.3f;

    if (ABS(this->unk_85C) < 0.00001f) {
        this->unk_85C = 0.0f;
        if (ABS(this->spinAttackTimer) < 0.00001f) {
            this->spinAttackTimer = 0.0f;
        }
    }
}

/**
 * Updates the Bunny Hood's floppy ears' rotation and velocity.
 */
void Player_UpdateBunnyEars(Player* this) {
    Vec3s force;
    s16 angle;

    // Damping: decay by 1/8 the previous value each frame
    sBunnyEarKinematics.angVel.x -= sBunnyEarKinematics.angVel.x >> 3;
    sBunnyEarKinematics.angVel.y -= sBunnyEarKinematics.angVel.y >> 3;

    // Elastic restorative force
    sBunnyEarKinematics.angVel.x += -sBunnyEarKinematics.rot.x >> 2;
    sBunnyEarKinematics.angVel.y += -sBunnyEarKinematics.rot.y >> 2;

    // Forcing from motion relative to shape frame
    angle = this->actor.world.rot.y - this->actor.shape.rot.y;
    force.x = (s32)(this->actor.speed * -200.0f * Math_CosS(angle) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;
    force.y = (s32)(this->actor.speed * 100.0f * Math_SinS(angle) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;

    sBunnyEarKinematics.angVel.x += force.x >> 2;
    sBunnyEarKinematics.angVel.y += force.y >> 2;

    // Clamp both angular velocities to [-6000, 6000]
    if (sBunnyEarKinematics.angVel.x > 6000) {
        sBunnyEarKinematics.angVel.x = 6000;
    } else if (sBunnyEarKinematics.angVel.x < -6000) {
        sBunnyEarKinematics.angVel.x = -6000;
    }
    if (sBunnyEarKinematics.angVel.y > 6000) {
        sBunnyEarKinematics.angVel.y = 6000;
    } else if (sBunnyEarKinematics.angVel.y < -6000) {
        sBunnyEarKinematics.angVel.y = -6000;
    }

    // Add angular velocity to rotations
    sBunnyEarKinematics.rot.x += sBunnyEarKinematics.angVel.x;
    sBunnyEarKinematics.rot.y += sBunnyEarKinematics.angVel.y;

    // swivel ears outwards if bending backwards
    if (sBunnyEarKinematics.rot.x < 0) {
        sBunnyEarKinematics.rot.z = sBunnyEarKinematics.rot.x >> 1;
    } else {
        sBunnyEarKinematics.rot.z = 0;
    }
}

s32 Player_ActionChange_TryStartMeleeWeaponAttack(Player* this, PlayState* play) {
    if (Player_CanSwingBottleOrCastFishingRod(play, this) == 0) {
        if (Player_CanJumpSlash(this) != 0) {
            s32 sp24 = Player_GetMeleeAttackAnim(this);

            Player_SetupMeleeWeaponAttack(play, this, sp24);

            if (sp24 >= PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
                this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
                Player_SetupSpinAttackActor(play, this, 0);
                return 1;
            }
        } else {
            return 0;
        }
    }

    return 1;
}

static Vec3f sShockwaveRaycastPos = { 0.0f, 40.0f, 45.0f };

void Player_Action_MeleeWeaponAttack(Player* this, PlayState* play) {
    MeleeAttackAnimInfo* meleeAtkAnims = &sMeleeAttackAnims[this->meleeAttackType];

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (!Player_TryWeaponHit(play, this)) {
        Player_TryMeleeAttack(this, 0.0f, meleeAtkAnims->startFrame, meleeAtkAnims->endFrame);

        if ((this->stateFlags2 & PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK) &&
            (this->heldItemAction != PLAYER_IA_HAMMER) && LinkAnimation_OnFrame(&this->skelAnime, 0.0f)) {
            this->speedXZ = 15.0f;
            this->stateFlags2 &= ~PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
        }

        if (this->speedXZ > 12.0f) {
            Player_TrySpawnDustAtFeet(play, this);
        }

        Math_StepToF(&this->speedXZ, 0.0f, 5.0f);
        Player_DeactivateComboTimer(this);

        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (!Player_ActionChange_TryStartMeleeWeaponAttack(this, play)) {
                u8 prevMoveFlags = this->skelAnime.moveFlags;
                LinkAnimationHeader* anim;

                if (Player_CheckBattleTargeting(this)) {
                    anim = meleeAtkAnims->fightEndAnim;
                } else {
                    anim = meleeAtkAnims->endAnim;
                }

                Player_InactivateMeleeWeapon(this);
                this->skelAnime.moveFlags = 0;

                if ((anim == &gPlayerAnim_link_fighter_Lpower_jump_kiru_end) &&
                    (this->modelAnimType != PLAYER_ANIMTYPE_3)) {
                    anim = &gPlayerAnim_link_fighter_power_jump_kiru_end;
                }

                Player_StartReturnToStandStillWithAnim(this, anim, play);

                this->skelAnime.moveFlags = prevMoveFlags;
                this->stateFlags3 |= PLAYER_STATE3_ENDING_MELEE_ATTACK;
            }
        } else if (this->heldItemAction == PLAYER_IA_HAMMER) {
            if ((this->meleeAttackType == PLAYER_MELEEATKTYPE_HAMMER_FORWARD) ||
                (this->meleeAttackType == PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH)) {
                static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
                Vec3f shockwavePos;
                f32 yDistToShockwave;

                shockwavePos.y = Player_RaycastFloor(play, this, &sShockwaveRaycastPos, &shockwavePos);
                yDistToShockwave = this->actor.world.pos.y - shockwavePos.y;

                Math_ScaledStepToS(&this->actor.focus.rot.x, Math_Atan2S(45.0f, yDistToShockwave), 800);
                Player_UpdateLookAngles(this, 1);

                if ((((this->meleeAttackType == PLAYER_MELEEATKTYPE_HAMMER_FORWARD) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 7.0f)) ||
                     ((this->meleeAttackType == PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 2.0f))) &&
                    (yDistToShockwave > -40.0f) && (yDistToShockwave < 40.0f)) {
                    Player_HammerHit(play, this);
                    EffectSsBlast_SpawnWhiteShockwave(play, &shockwavePos, &zeroVec, &zeroVec);
                }
            }
        }
    }
}

void Player_Action_MeleeWeaponRebound(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_StepSpeedXZToZero(this);

    if (this->skelAnime.curFrame >= 6.0f) {
        Player_ReturnToStandStill(this, play);
    }
}

void Player_Action_ChooseFWOption(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    LinkAnimation_Update(play, &this->skelAnime);
    Player_UpdateUpperBody(this, play);

    if (this->av2.actionVar2 == 0) {
        Message_StartTextbox(play, 0x3B, &this->actor);
        this->av2.actionVar2 = 1;
        return;
    }

    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;

        if (play->msgCtx.choiceIndex == 0) {
            gSaveContext.respawnFlag = 3;
            play->transitionTrigger = TRANS_TRIGGER_START;
            play->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex;
            play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
            Interface_SetSubTimerToFinalSecond(play);
            return;
        }

        if (play->msgCtx.choiceIndex == 1) {
            gSaveContext.respawn[RESPAWN_MODE_TOP].data = -respawnData;
            gSaveContext.save.info.fw.set = 0;
            Sfx_PlaySfxAtPos(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_VANISH);
        }

        Player_SetupStandStillMorph(this, play);
        Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

void Player_Action_AppearFromFW(Player* this, PlayState* play) {
    s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;

    if (this->av2.actionVar2 > 20) {
        this->actor.draw = Player_Draw;
        this->actor.world.pos.y += 60.0f;
        Player_SetupFallFromLedge(this, play);
        return;
    }

    if (this->av2.actionVar2++ == 20) {
        gSaveContext.respawn[RESPAWN_MODE_TOP].data = respawnData + 1;
        Sfx_PlaySfxAtPos(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_WARP);
    }
}

static LinkAnimationHeader* sStartCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze1,
    &gPlayerAnim_link_magic_honoo1,
    &gPlayerAnim_link_magic_tamashii1,
};

static LinkAnimationHeader* sCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze2,
    &gPlayerAnim_link_magic_honoo2,
    &gPlayerAnim_link_magic_tamashii2,
};

static LinkAnimationHeader* sEndCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze3,
    &gPlayerAnim_link_magic_honoo3,
    &gPlayerAnim_link_magic_tamashii3,
};

static u8 sMagicSpellTimeLimits[] = { 70, 10, 10 };

static AnimSfxEntry sMagicSpellCastAnimSfx[] = {
    { NA_SE_PL_SKIP, ANIMSFX_DATA(ANIMSFX_TYPE_1, 20) },
    { NA_SE_VO_LI_SWORD_N, ANIMSFX_DATA(ANIMSFX_TYPE_4, 20) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 26) },
};

static AnimSfxEntry sMagicSpellAnimSfx[][2] = {
    {
        { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 20) },
        { NA_SE_VO_LI_MAGIC_FROL, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 30) },
    },
    {
        { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 20) },
        { NA_SE_VO_LI_MAGIC_NALE, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 44) },
    },
    {
        { NA_SE_VO_LI_MAGIC_ATTACK, ANIMSFX_DATA(ANIMSFX_TYPE_4, 20) },
        { NA_SE_IT_SWORD_SWING_HARD, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 20) },
    },
};

void Player_Action_MagicSpell(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av1.actionVar1 < 0) {
            if ((this->itemAction == PLAYER_IA_NAYRUS_LOVE) || (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
                Player_ReturnToStandStill(this, play);
                Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            if (this->av2.actionVar2 == 0) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sStartCastMagicSpellAnims[this->av1.actionVar1],
                                               0.83f);

                if (Player_SpawnMagicSpell(play, this, this->av1.actionVar1) != NULL) {
                    this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                    if ((this->av1.actionVar1 != 0) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                    }
                } else {
                    Magic_Reset(play);
                }
            } else {
                LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, sCastMagicSpellAnims[this->av1.actionVar1],
                                               0.83f);

                if (this->av1.actionVar1 == 0) {
                    this->av2.actionVar2 = -10;
                }
            }

            this->av2.actionVar2++;
        }
    } else {
        if (this->av2.actionVar2 < 0) {
            this->av2.actionVar2++;

            if (this->av2.actionVar2 == 0) {
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = 1;
                Play_SetupRespawnPoint(play, RESPAWN_MODE_TOP, 0x6FF);
                gSaveContext.save.info.fw.set = 1;
                gSaveContext.save.info.fw.pos.x = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.x;
                gSaveContext.save.info.fw.pos.y = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.y;
                gSaveContext.save.info.fw.pos.z = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.z;
                gSaveContext.save.info.fw.yaw = gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw;
                gSaveContext.save.info.fw.playerParams = 0x6FF;
                gSaveContext.save.info.fw.entranceIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex;
                gSaveContext.save.info.fw.roomIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex;
                gSaveContext.save.info.fw.tempSwchFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwchFlags;
                gSaveContext.save.info.fw.tempCollectFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags;
                this->av2.actionVar2 = 2;
            }
        } else if (this->av1.actionVar1 >= 0) {
            if (this->av2.actionVar2 == 0) {
                Player_ProcessAnimSfxList(this, sMagicSpellCastAnimSfx);
            } else if (this->av2.actionVar2 == 1) {
                Player_ProcessAnimSfxList(this, sMagicSpellAnimSfx[this->av1.actionVar1]);
                if ((this->av1.actionVar1 == 2) && LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                    this->stateFlags1 &= ~(PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE);
                }
            } else if (sMagicSpellTimeLimits[this->av1.actionVar1] < this->av2.actionVar2++) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sEndCastMagicSpellAnims[this->av1.actionVar1],
                                               0.83f);
                this->yaw = this->actor.shape.rot.y;
                this->av1.actionVar1 = -1;
            }
        }
    }

    Player_StepSpeedXZToZero(this);
}

void Player_Action_PulledByHookshot(Player* this, PlayState* play) {
    f32 temp;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_hook_fly_wait);
    }

    Math_Vec3f_Sum(&this->actor.world.pos, &this->actor.velocity, &this->actor.world.pos);

    if (Player_FinishHookshotMove(this)) {
        Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
        Player_ProcessSceneCollision(play, this);

        temp = this->actor.world.pos.y - this->actor.floorHeight;
        if (temp > 20.0f) {
            temp = 20.0f;
        }

        this->actor.world.rot.x = this->actor.shape.rot.x = 0;
        this->actor.world.pos.y -= temp;
        this->speedXZ = 1.0f;
        this->actor.velocity.y = 0.0f;
        Player_SetupFallFromLedge(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
        this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;
        this->stateFlags1 |= PLAYER_STATE1_END_HOOKSHOT_MOVE;
        return;
    }

    if ((this->skelAnime.animation != &gPlayerAnim_link_hook_fly_start) || (4.0f <= this->skelAnime.curFrame)) {
        this->actor.gravity = 0.0f;
        Math_ScaledStepToS(&this->actor.shape.rot.x, this->actor.world.rot.x, 0x800);
        Player_RequestRumble(this, 100, 2, 100, 0);
    }
}

void Player_Action_CastFishingRod(Player* this, PlayState* play) {
    if ((this->av2.actionVar2 != 0) && ((this->spinAttackTimer != 0.0f) || (this->unk_85C != 0.0f))) {
        f32 updateScale = R_UPDATE_RATE * 0.5f;

        this->skelAnime.curFrame += this->skelAnime.playSpeed * updateScale;
        if (this->skelAnime.curFrame >= this->skelAnime.animLength) {
            this->skelAnime.curFrame -= this->skelAnime.animLength;
        }

        LinkAnimation_BlendToJoint(play, &this->skelAnime, &gPlayerAnim_link_fishing_wait, this->skelAnime.curFrame,
                                   (this->spinAttackTimer < 0.0f) ? &gPlayerAnim_link_fishing_reel_left
                                                                  : &gPlayerAnim_link_fishing_reel_right,
                                   5.0f, fabsf(this->spinAttackTimer), this->blendTable);
        LinkAnimation_BlendToMorph(play, &this->skelAnime, &gPlayerAnim_link_fishing_wait, this->skelAnime.curFrame,
                                   (this->unk_85C < 0.0f) ? &gPlayerAnim_link_fishing_reel_up
                                                          : &gPlayerAnim_link_fishing_reel_down,
                                   5.0f, fabsf(this->unk_85C), sFishingBlendTable);
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 0.5f);
    } else if (LinkAnimation_Update(play, &this->skelAnime)) {
        this->unk_860 = 2;
        Player_AnimPlayLoop(play, this, &gPlayerAnim_link_fishing_wait);
        this->av2.actionVar2 = 1;
    }

    Player_StepSpeedXZToZero(this);

    if (this->unk_860 == 0) {
        Player_SetupStandStillMorph(this, play);
    } else if (this->unk_860 == 3) {
        Player_SetupAction(play, this, Player_Action_ReleaseCaughtFish, 0);
        Player_AnimChangeOnceMorph(play, this, &gPlayerAnim_link_fishing_fish_catch);
    }
}

void Player_Action_ReleaseCaughtFish(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->unk_860 == 0)) {
        Player_StartReturnToStandStillWithAnim(this, &gPlayerAnim_link_fishing_fish_catch_end, play);
    }
}

static void (*csModePlaybackFuncs[])(PlayState*, Player*, void*) = {
    NULL,
    Player_Cutscene_AnimPlaybackType0,
    Player_Cutscene_AnimPlaybackType1,
    Player_Cutscene_AnimPlaybackType2,
    Player_Cutscene_AnimPlaybackType3,
    Player_Cutscene_AnimPlaybackType4,
    Player_Cutscene_AnimPlaybackType5,
    Player_Cutscene_AnimPlaybackType6,
    Player_Cutscene_AnimPlaybackType7,
    Player_Cutscene_AnimPlaybackType8,
    Player_Cutscene_AnimPlaybackType9,
    Player_Cutscene_AnimPlaybackType10,
    Player_Cutscene_AnimPlaybackType11,
    Player_Cutscene_AnimPlaybackType12,
    Player_Cutscene_AnimPlaybackType13,
    Player_Cutscene_AnimPlaybackType14,
    Player_Cutscene_AnimPlaybackType15,
    Player_Cutscene_AnimPlaybackType16,
    Player_Cutscene_AnimPlaybackType17,
};

static AnimSfxEntry sGetUpFromDekuTreeStoryAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_5, 34) },
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 45) },
    { NA_SE_PL_CALM_HIT, ANIMSFX_DATA(ANIMSFX_TYPE_1, 51) },
    { NA_SE_PL_CALM_HIT, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 64) },
};

static AnimSfxEntry sSurprisedStumbleBackFallAnimSfx[] = {
    { NA_SE_VO_LI_SURPRISE, ANIMSFX_DATA(ANIMSFX_TYPE_4, 3) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 15) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 24) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 30) },
    { NA_SE_VO_LI_FALL_L, -ANIMSFX_DATA(ANIMSFX_TYPE_4, 31) },
};

static AnimSfxEntry sFallToKneeAnimSfx[] = {
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 10) },
};

static CutsceneModeEntry sCutsceneModeInitFuncs[PLAYER_CSACTION_MAX] = {
    { 0, NULL },                                         // PLAYER_CSACTION_NONE
    { -1, Player_Cutscene_InitIdle },                    // PLAYER_CSACTION_1
    { 2, &gPlayerAnim_link_demo_goma_furimuki },         // PLAYER_CSACTION_TURN_AROUND_SURPRISED_SHORT
    { 0, NULL },                                         // PLAYER_CSACTION_3
    { 0, NULL },                                         // PLAYER_CSACTION_4
    { 3, &gPlayerAnim_link_demo_bikkuri },               // PLAYER_CSACTION_SURPRISED
    { 0, NULL },                                         // PLAYER_CSACTION_6
    { 0, NULL },                                         // PLAYER_CSACTION_END
    { -1, Player_Cutscene_InitIdle },                    // PLAYER_CSACTION_WAIT
    { 2, &gPlayerAnim_link_demo_furimuki },              // PLAYER_CSACTION_TURN_AROUND_SURPRISED_LONG
    { -1, Player_Cutscene_InitEnterWarp },               // PLAYER_CSACTION_ENTER_WARP
    { 3, &gPlayerAnim_link_demo_warp },                  // PLAYER_CSACTION_RAISED_BY_WARP
    { -1, Player_Cutscene_InitFightStance },             // PLAYER_CSACTION_FIGHT_STANCE
    { 7, &gPlayerAnim_clink_demo_get1 },                 // PLAYER_CSACTION_START_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_get2 },                 // PLAYER_CSACTION_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_get3 },                 // PLAYER_CSACTION_END_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_standup },              // PLAYER_CSACTION_GET_UP_FROM_DEKU_TREE_STORY
    { 7, &gPlayerAnim_clink_demo_standup_wait },         // PLAYER_CSACTION_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_Cutscene_InitSwordPedestal },           // PLAYER_CSACTION_SWORD_INTO_PEDESTAL
    { 2, &gPlayerAnim_link_demo_baru_op1 },              // PLAYER_CSACTION_REACT_TO_QUAKE
    { 2, &gPlayerAnim_link_demo_baru_op3 },              // PLAYER_CSACTION_END_REACT_TO_QUAKE
    { 0, NULL },                                         // PLAYER_CSACTION_21
    { -1, Player_Cutscene_InitWarpToSages },             // PLAYER_CSACTION_WARP_TO_SAGES
    { 3, &gPlayerAnim_link_demo_jibunmiru },             // PLAYER_CSACTION_LOOK_AT_SELF
    { 9, &gPlayerAnim_link_normal_back_downA },          // PLAYER_CSACTION_KNOCKED_TO_GROUND
    { 2, &gPlayerAnim_link_normal_back_down_wake },      // PLAYER_CSACTION_GET_UP_FROM_GROUND
    { -1, Player_Cutscene_InitPlayOcarina },             // PLAYER_CSACTION_START_PLAY_OCARINA
    { 2, &gPlayerAnim_link_normal_okarina_end },         // PLAYER_CSACTION_END_PLAY_OCARINA
    { 3, &gPlayerAnim_link_demo_get_itemA },             // PLAYER_CSACTION_GET_ITEM
    { -1, Player_Cutscene_InitIdle },                    // PLAYER_CSACTION_IDLE_2
    { 2, &gPlayerAnim_link_normal_normal2fighter_free }, // PLAYER_CSACTION_DRAW_AND_BRANDISH_SWORD
    { 0, NULL },                                         // PLAYER_CSACTION_CLOSE_EYES
    { 0, NULL },                                         // PLAYER_CSACTION_OPEN_EYES
    { 5, &gPlayerAnim_clink_demo_atozusari },            // PLAYER_CSACTION_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_Cutscene_InitSwimIdle },                // PLAYER_CSACTION_SURFACE_FROM_DIVE
    { -1, Player_Cutscene_InitGetItemInWater },          // PLAYER_CSACTION_GET_ITEM_IN_WATER
    { 5, &gPlayerAnim_clink_demo_bashi },                // PLAYER_CSACTION_GENTLE_KNOCKBACK_INTO_SIT
    { 16, &gPlayerAnim_link_normal_hang_up_down },       // PLAYER_CSACTION_GRABBED_AND_CARRIED_BY_NECK
    { -1, Player_Cutscene_InitSleepingRestless },        // PLAYER_CSACTION_SLEEPING_RESTLESS
    { -1, Player_Cutscene_InitSleeping },                // PLAYER_CSACTION_SLEEPING
    { 6, &gPlayerAnim_clink_op3_okiagari },              // PLAYER_CSACTION_AWAKEN
    { 6, &gPlayerAnim_clink_op3_tatiagari },             // PLAYER_CSACTION_GET_OFF_BED
    { -1, Player_Cutscene_InitBlownBackward },           // PLAYER_CSACTION_BLOWN_BACKWARD
    { 5, &gPlayerAnim_clink_demo_miokuri },              // PLAYER_CSACTION_STAND_UP_AND_WATCH
    { -1, Player_Cutscene_InitIdle3 },                   // PLAYER_CSACTION_IDLE_3
    { -1, Player_Cutscene_InitStop },                    // PLAYER_CSACTION_STOP
    { -1, Player_Cutscene_InitSetDraw },                 // PLAYER_CSACTION_STOP_2
    { 5, &gPlayerAnim_clink_demo_nozoki },               // PLAYER_CSACTION_LOOK_THROUGH_PEEPHOLE
    { 5, &gPlayerAnim_clink_demo_koutai },               // PLAYER_CSACTION_STEP_BACK_CAUTIOUSLY
    { -1, Player_Cutscene_InitIdle },                    // PLAYER_CSACTION_IDLE_4
    { 5, &gPlayerAnim_clink_demo_koutai_kennuki },       // PLAYER_CSACTION_DRAW_SWORD_CHILD
    { 5, &gPlayerAnim_link_demo_kakeyori },              // PLAYER_CSACTION_JUMP_TO_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_link_demo_kakeyori_mimawasi },     // PLAYER_CSACTION_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_link_demo_kakeyori_miokuri },      // PLAYER_CSACTION_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { 3, &gPlayerAnim_link_demo_furimuki2 },             // PLAYER_CSACTION_TURN_AROUND_SLOWLY
    { 3, &gPlayerAnim_link_demo_kaoage },                // PLAYER_CSACTION_END_SHIELD_EYES_WITH_HAND
    { 4, &gPlayerAnim_link_demo_kaoage_wait },           // PLAYER_CSACTION_SHIELD_EYES_WITH_HAND
    { 3, &gPlayerAnim_clink_demo_mimawasi },             // PLAYER_CSACTION_LOOK_AROUND_SURPRISED
    { 3, &gPlayerAnim_link_demo_nozokikomi },            // PLAYER_CSACTION_INSPECT_GROUND_CAREFULLY
    { 6, &gPlayerAnim_kolink_odoroki_demo },             // PLAYER_CSACTION_STARTLED_BY_GORONS_FALLING
    { 6, &gPlayerAnim_link_shagamu_demo },               // PLAYER_CSACTION_FALL_TO_KNEE
    { 14, &gPlayerAnim_link_okiru_demo },                // PLAYER_CSACTION_FLAT_ON_BACK
    { 3, &gPlayerAnim_link_okiru_demo },                 // PLAYER_CSACTION_RAISE_FROM_FLAT_ON_BACK
    { 5, &gPlayerAnim_link_fighter_power_kiru_start },   // PLAYER_CSACTION_START_SPIN_ATTACK
    { 16, &gPlayerAnim_demo_link_nwait },                // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_IDLE
    { 15, &gPlayerAnim_demo_link_tewatashi },            // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 15, &gPlayerAnim_demo_link_orosuu },               // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 3, &gPlayerAnim_d_link_orooro },                   // PLAYER_CSACTION_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_d_link_imanodare },                // PLAYER_CSACTION_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_link_hatto_demo },                 // PLAYER_CSACTION_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { 6, &gPlayerAnim_o_get_mae },                       // PLAYER_CSACTION_START_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_o_get_ato },                       // PLAYER_CSACTION_END_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_om_get_mae },                      // PLAYER_CSACTION_START_LEARN_OCARINA_SONG_CHILD
    { 6, &gPlayerAnim_nw_modoru },                       // PLAYER_CSACTION_END_LEARN_OCARINA_SONG_CHILD
    { 3, &gPlayerAnim_link_demo_gurad },                 // PLAYER_CSACTION_RESIST_DARK_MAGIC
    { 3, &gPlayerAnim_link_demo_look_hand },             // PLAYER_CSACTION_TRIFORCE_HAND_RESONATES
    { 4, &gPlayerAnim_link_demo_sita_wait },             // PLAYER_CSACTION_STARE_DOWN_STARTLED
    { 3, &gPlayerAnim_link_demo_ue },                    // PLAYER_CSACTION_LOOK_UP_STARTLED
    { 3, &gPlayerAnim_Link_muku },                       // PLAYER_CSACTION_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 3, &gPlayerAnim_Link_miageru },                    // PLAYER_CSACTION_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 6, &gPlayerAnim_Link_ha },                         // PLAYER_CSACTION_SURPRISED_DEFENSE
    { 3, &gPlayerAnim_L_1kyoro },                        // PLAYER_CSACTION_START_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_L_2kyoro },                        // PLAYER_CSACTION_END_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_L_sagaru },                        // PLAYER_CSACTION_START_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_L_bouzen },                        // PLAYER_CSACTION_LOOK_UP_DEFENSE_IDLE
    { 3, &gPlayerAnim_L_kamaeru },                       // PLAYER_CSACTION_END_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_L_hajikareru },                    // PLAYER_CSACTION_START_SWORD_KNOCKED_FROM_HAND
    { 3, &gPlayerAnim_L_ken_miru },                      // PLAYER_CSACTION_SWORD_KNOCKED_FROM_HAND_IDLE
    { 3, &gPlayerAnim_L_mukinaoru },                     // PLAYER_CSACTION_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_Cutscene_InitSpinAttackIdle },          // PLAYER_CSACTION_SPIN_ATTACK_IDLE
    { 3, &gPlayerAnim_link_wait_itemD1_20f },            // PLAYER_CSACTION_INSPECT_WEAPON
    { -1, Player_Cutscene_InitDoNothing },               // PLAYER_CSACTION_91
    { -1, Player_Cutscene_InitKnockedToGroundDamaged },  // PLAYER_CSACTION_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 3, &gPlayerAnim_link_normal_wait_typeB_20f },      // PLAYER_CSACTION_REACT_TO_HEAT
    { -1, Player_Cutscene_InitGetSwordBack },            // PLAYER_CSACTION_GET_SWORD_BACK
    { 3, &gPlayerAnim_link_demo_kousan },                // PLAYER_CSACTION_CAUGHT_BY_GUARD
    { 3, &gPlayerAnim_link_demo_return_to_past },        // PLAYER_CSACTION_GET_SWORD_BACK_2
    { 3, &gPlayerAnim_link_last_hit_motion1 },           // PLAYER_CSACTION_START_GANON_KILL_COMBO
    { 3, &gPlayerAnim_link_last_hit_motion2 },           // PLAYER_CSACTION_END_GANON_KILL_COMBO
    { 3, &gPlayerAnim_link_demo_zeldamiru },             // PLAYER_CSACTION_WATCH_ZELDA_STUN_GANON
    { 3, &gPlayerAnim_link_demo_kenmiru1 },              // PLAYER_CSACTION_START_LOOK_AT_SWORD_GLOW
    { 3, &gPlayerAnim_link_demo_kenmiru2 },              // PLAYER_CSACTION_LOOK_AT_SWORD_GLOW_IDLE
    { 3, &gPlayerAnim_link_demo_kenmiru2_modori },       // PLAYER_CSACTION_END_LOOK_AT_SWORD_GLOW
};

static CutsceneModeEntry sCutsceneModeUpdateFuncs[PLAYER_CSACTION_MAX] = {
    { 0, NULL },                                          // PLAYER_CSACTION_NONE
    { -1, Player_Cutscene_Idle },                         // PLAYER_CSACTION_1
    { -1, Player_Cutscene_TurnAroundSurprisedShort },     // PLAYER_CSACTION_TURN_AROUND_SURPRISED_SHORT
    { -1, Player_Cutscene_Unk3Update },                   // PLAYER_CSACTION_3
    { -1, Player_Cutscene_Unk4Update },                   // PLAYER_CSACTION_4
    { 11, NULL },                                         // PLAYER_CSACTION_SURPRISED
    { -1, Player_Cutscene_Unk6Update },                   // PLAYER_CSACTION_6
    { -1, Player_Cutscene_Finish },                       // PLAYER_CSACTION_END
    { -1, Player_Cutscene_Wait },                         // PLAYER_CSACTION_WAIT
    { -1, Player_Cutscene_TurnAroundSurprisedLong },      // PLAYER_CSACTION_TURN_AROUND_SURPRISED_LONG
    { -1, Player_Cutscene_EnterWarp },                    // PLAYER_CSACTION_ENTER_WARP
    { -1, Player_Cutscene_RaisedByWarp },                 // PLAYER_CSACTION_RAISED_BY_WARP
    { -1, Player_Cutscene_FightStance },                  // PLAYER_CSACTION_FIGHT_STANCE
    { 11, NULL },                                         // PLAYER_CSACTION_START_GET_SPIRITUAL_STONE
    { 11, NULL },                                         // PLAYER_CSACTION_GET_SPIRITUAL_STONE
    { 11, NULL },                                         // PLAYER_CSACTION_END_GET_SPIRITUAL_STONE
    { 18, sGetUpFromDekuTreeStoryAnimSfx },               // PLAYER_CSACTION_GET_UP_FROM_DEKU_TREE_STORY
    { 11, NULL },                                         // PLAYER_CSACTION_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_Cutscene_SwordPedestal },                // PLAYER_CSACTION_SWORD_INTO_PEDESTAL
    { 12, &gPlayerAnim_link_demo_baru_op2 },              // PLAYER_CSACTION_REACT_TO_QUAKE
    { 11, NULL },                                         // PLAYER_CSACTION_END_REACT_TO_QUAKE
    { 0, NULL },                                          // PLAYER_CSACTION_21
    { -1, Player_Cutscene_WarpToSages },                  // PLAYER_CSACTION_WARP_TO_SAGES
    { 11, NULL },                                         // PLAYER_CSACTION_LOOK_AT_SELF
    { -1, Player_Cutscene_KnockedToGround },              // PLAYER_CSACTION_KNOCKED_TO_GROUND
    { 11, NULL },                                         // PLAYER_CSACTION_GET_UP_FROM_GROUND
    { 17, &gPlayerAnim_link_normal_okarina_swing },       // PLAYER_CSACTION_START_PLAY_OCARINA
    { 11, NULL },                                         // PLAYER_CSACTION_END_PLAY_OCARINA
    { 11, NULL },                                         // PLAYER_CSACTION_GET_ITEM
    { 11, NULL },                                         // PLAYER_CSACTION_IDLE_2
    { -1, Player_Cutscene_DrawAndBrandishSword },         // PLAYER_CSACTION_DRAW_AND_BRANDISH_SWORD
    { -1, Player_Cutscene_CloseEyes },                    // PLAYER_CSACTION_CLOSE_EYES
    { -1, Player_Cutscene_OpenEyes },                     // PLAYER_CSACTION_OPEN_EYES
    { 18, sSurprisedStumbleBackFallAnimSfx },             // PLAYER_CSACTION_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_Cutscene_SurfaceFromDive },              // PLAYER_CSACTION_SURFACE_FROM_DIVE
    { 11, NULL },                                         // PLAYER_CSACTION_GET_ITEM_IN_WATER
    { 11, NULL },                                         // PLAYER_CSACTION_GENTLE_KNOCKBACK_INTO_SIT
    { 11, NULL },                                         // PLAYER_CSACTION_GRABBED_AND_CARRIED_BY_NECK
    { 11, NULL },                                         // PLAYER_CSACTION_SLEEPING_RESTLESS
    { -1, Player_Cutscene_Sleeping },                     // PLAYER_CSACTION_SLEEPING
    { -1, Player_Cutscene_Awaken },                       // PLAYER_CSACTION_AWAKEN
    { -1, Player_Cutscene_GetOffBed },                    // PLAYER_CSACTION_GET_OFF_BED
    { -1, Player_Cutscene_BlownBackward },                // PLAYER_CSACTION_BLOWN_BACKWARD
    { 13, &gPlayerAnim_clink_demo_miokuri_wait },         // PLAYER_CSACTION_STAND_UP_AND_WATCH
    { -1, Player_Cutscene_Idle2 },                        // PLAYER_CSACTION_IDLE_3
    { 0, NULL },                                          // PLAYER_CSACTION_STOP
    { 0, NULL },                                          // PLAYER_CSACTION_STOP_2
    { 11, NULL },                                         // PLAYER_CSACTION_LOOK_THROUGH_PEEPHOLE
    { -1, Player_Cutscene_StepBackCautiously },           // PLAYER_CSACTION_STEP_BACK_CAUTIOUSLY
    { -1, Player_Cutscene_Wait },                         // PLAYER_CSACTION_IDLE_4
    { -1, Player_Cutscene_DrawSwordChild },               // PLAYER_CSACTION_DRAW_SWORD_CHILD
    { 13, &gPlayerAnim_link_demo_kakeyori_wait },         // PLAYER_CSACTION_JUMP_TO_ZELDAS_CRYSTAL
    { -1, Player_Cutscene_DesperateLookAtZeldasCrystal }, // PLAYER_CSACTION_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 13, &gPlayerAnim_link_demo_kakeyori_miokuri_wait }, // PLAYER_CSACTION_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { -1, Player_Cutscene_TurnAroundSlowly },             // PLAYER_CSACTION_TURN_AROUND_SLOWLY
    { 11, NULL },                                         // PLAYER_CSACTION_END_SHIELD_EYES_WITH_HAND
    { 11, NULL },                                         // PLAYER_CSACTION_SHIELD_EYES_WITH_HAND
    { 12, &gPlayerAnim_clink_demo_mimawasi_wait },        // PLAYER_CSACTION_LOOK_AROUND_SURPRISED
    { -1, Player_Cutscene_InspectGroundCarefully },       // PLAYER_CSACTION_INSPECT_GROUND_CAREFULLY
    { 11, NULL },                                         // PLAYER_CSACTION_STARTLED_BY_GORONS_FALLING
    { 18, sFallToKneeAnimSfx },                           // PLAYER_CSACTION_FALL_TO_KNEE
    { 11, NULL },                                         // PLAYER_CSACTION_FLAT_ON_BACK
    { 11, NULL },                                         // PLAYER_CSACTION_RAISE_FROM_FLAT_ON_BACK
    { 11, NULL },                                         // PLAYER_CSACTION_START_SPIN_ATTACK
    { 11, NULL },                                         // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_IDLE
    { -1, Player_Cutscene_StartPassOcarina },             // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 17, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSACTION_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 12, &gPlayerAnim_d_link_orowait },                  // PLAYER_CSACTION_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 12, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSACTION_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 11, NULL },                                         // PLAYER_CSACTION_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { -1, Player_Cutscene_LearnOcarinaSong },             // PLAYER_CSACTION_START_LEARN_OCARINA_SONG_ADULT
    { 17, &gPlayerAnim_sude_nwait },                      // PLAYER_CSACTION_END_LEARN_OCARINA_SONG_ADULT
    { -1, Player_Cutscene_LearnOcarinaSong },             // PLAYER_CSACTION_START_LEARN_OCARINA_SONG_CHILD
    { 17, &gPlayerAnim_sude_nwait },                      // PLAYER_CSACTION_END_LEARN_OCARINA_SONG_CHILD
    { 12, &gPlayerAnim_link_demo_gurad_wait },            // PLAYER_CSACTION_RESIST_DARK_MAGIC
    { 12, &gPlayerAnim_link_demo_look_hand_wait },        // PLAYER_CSACTION_TRIFORCE_HAND_RESONATES
    { 11, NULL },                                         // PLAYER_CSACTION_STARE_DOWN_STARTLED
    { 12, &gPlayerAnim_link_demo_ue_wait },               // PLAYER_CSACTION_LOOK_UP_STARTLED
    { 12, &gPlayerAnim_Link_m_wait },                     // PLAYER_CSACTION_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 13, &gPlayerAnim_Link_ue_wait },                    // PLAYER_CSACTION_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 12, &gPlayerAnim_Link_otituku_w },                  // PLAYER_CSACTION_SURPRISED_DEFENSE
    { 12, &gPlayerAnim_L_kw },                            // PLAYER_CSACTION_START_HALF_TURN_SURPRISED
    { 11, NULL },                                         // PLAYER_CSACTION_END_HALF_TURN_SURPRISED
    { 11, NULL },                                         // PLAYER_CSACTION_START_LOOK_UP_DEFENSE
    { 11, NULL },                                         // PLAYER_CSACTION_LOOK_UP_DEFENSE_IDLE
    { 11, NULL },                                         // PLAYER_CSACTION_END_LOOK_UP_DEFENSE
    { -1, Player_Cutscene_SwordKnockedFromHand },         // PLAYER_CSACTION_START_SWORD_KNOCKED_FROM_HAND
    { 11, NULL },                                         // PLAYER_CSACTION_SWORD_KNOCKED_FROM_HAND_IDLE
    { 12, &gPlayerAnim_L_kennasi_w },                     // PLAYER_CSACTION_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_Cutscene_SpinAttackIdle },               // PLAYER_CSACTION_SPIN_ATTACK_IDLE
    { -1, Player_Cutscene_InspectWeapon },                // PLAYER_CSACTION_INSPECT_WEAPON
    { -1, Player_Cutscene_DoNothing },                    // PLAYER_CSACTION_91
    { -1, Player_Cutscene_KnockedToGroundDamaged },       // PLAYER_CSACTION_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 11, NULL },                                         // PLAYER_CSACTION_REACT_TO_HEAT
    { 11, NULL },                                         // PLAYER_CSACTION_GET_SWORD_BACK
    { 11, NULL },                                         // PLAYER_CSACTION_CAUGHT_BY_GUARD
    { -1, Player_Cutscene_GetSwordBack },                 // PLAYER_CSACTION_GET_SWORD_BACK_2
    { -1, Player_Cutscene_GanonKillCombo },               // PLAYER_CSACTION_START_GANON_KILL_COMBO
    { -1, Player_Cutscene_GanonKillCombo },               // PLAYER_CSACTION_END_GANON_KILL_COMBO
    { 12, &gPlayerAnim_link_demo_zeldamiru_wait },        // PLAYER_CSACTION_WATCH_ZELDA_STUN_GANON
    { 12, &gPlayerAnim_link_demo_kenmiru1_wait },         // PLAYER_CSACTION_START_LOOK_AT_SWORD_GLOW
    { 12, &gPlayerAnim_link_demo_kenmiru2_wait },         // PLAYER_CSACTION_LOOK_AT_SWORD_GLOW_IDLE
    { 12, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSACTION_END_LOOK_AT_SWORD_GLOW
};

void Player_AnimChangeOnceMorphZeroRootYawSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ZeroRootLimbYaw(this);
    Player_AnimChangeOnceMorph(play, this, anim);
    Player_ZeroSpeedXZ(this);
}

void Player_AnimChangeOnceMorphAdjustedZeroRootYawSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ZeroRootLimbYaw(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, PLAYER_ANIM_ADJUSTED_SPEED, 0.0f, Animation_GetLastFrame(anim),
                         ANIMMODE_ONCE, -8.0f);
    Player_ZeroSpeedXZ(this);
}

void Player_AnimChangeLoopMorphAdjustedZeroRootYawSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ZeroRootLimbYaw(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, PLAYER_ANIM_ADJUSTED_SPEED, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f);
    Player_ZeroSpeedXZ(this);
}

void Player_Cutscene_AnimPlaybackType0(PlayState* play, Player* this, void* anim) {
    Player_ZeroSpeedXZ(this);
}

void Player_Cutscene_AnimPlaybackType1(PlayState* play, Player* this, void* anim) {
    Player_AnimChangeOnceMorphZeroRootYawSpeed(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType13(PlayState* play, Player* this, void* anim) {
    Player_ZeroRootLimbYaw(this);
    Player_AnimChangeFreeze(play, this, anim);
    Player_ZeroSpeedXZ(this);
}

void Player_Cutscene_AnimPlaybackType2(PlayState* play, Player* this, void* anim) {
    Player_AnimChangeOnceMorphAdjustedZeroRootYawSpeed(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType3(PlayState* play, Player* this, void* anim) {
    Player_AnimChangeLoopMorphAdjustedZeroRootYawSpeed(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType4(PlayState* play, Player* this, void* anim) {
    Player_AnimReplaceNormalPlayOnceAdjusted(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType5(PlayState* play, Player* this, void* anim) {
    Player_AnimReplacePlayOnce(play, this, anim,
                               ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
}

void Player_Cutscene_AnimPlaybackType6(PlayState* play, Player* this, void* anim) {
    Player_AnimReplaceNormalPlayLoopAdjusted(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType7(PlayState* play, Player* this, void* anim) {
    Player_AnimReplacePlayLoop(play, this, anim,
                               ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
}

void Player_Cutscene_AnimPlaybackType8(PlayState* play, Player* this, void* anim) {
    Player_AnimPlayOnce(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType9(PlayState* play, Player* this, void* anim) {
    Player_AnimPlayLoop(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType14(PlayState* play, Player* this, void* anim) {
    Player_AnimPlayOnceAdjusted(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType15(PlayState* play, Player* this, void* anim) {
    Player_AnimPlayLoopAdjusted(play, this, anim);
}

void Player_Cutscene_AnimPlaybackType10(PlayState* play, Player* this, void* anim) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_AnimPlaybackType11(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimChangeLoopMorphAdjustedZeroRootYawSpeed(play, this, anim);
        this->av2.actionVar2 = 1;
    }
}

void Player_Cutscene_AnimPlaybackType16(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_FinishAnimMovement(this);
        Player_AnimPlayLoopAdjusted(play, this, anim);
    }
}

void Player_Cutscene_AnimPlaybackType12(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimReplaceNormalPlayLoopAdjusted(play, this, anim);
        this->av2.actionVar2 = 1;
    }
}

void Player_Cutscene_AnimPlaybackType17(PlayState* play, Player* this, void* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_ProcessAnimSfxList(this, arg2);
}

void Player_Cutscene_LookAtTargetActor(Player* this) {
    if ((this->csActor == NULL) || (this->csActor->update == NULL)) {
        this->csActor = NULL;
    }

    this->targetActor = this->csActor;

    if (this->targetActor != NULL) {
        this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_Cutscene_InitSwimIdle(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    Player_AnimPlayLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_Cutscene_SurfaceFromDive(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->actor.gravity = 0.0f;

    if (this->av1.actionVar1 == 0) {
        if (Player_TryDive(play, this, NULL)) {
            this->av1.actionVar1 = 1;
        } else {
            Player_PlaySwimAnim(play, this, NULL, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->shapePitchOffset, -10000, 800);
            Player_UpdateSwimMovement(this, &this->actor.velocity.y, 4.0f, this->yaw);
        }
        return;
    }

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->av1.actionVar1 == 1) {
            Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim_wait);
        } else {
            Player_AnimPlayLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
        }
    }

    Player_SetVerticalWaterSpeed(this);
    Player_UpdateSwimMovement(this, &this->speedXZ, 0.0f, this->actor.shape.rot.y);
}

void Player_Cutscene_Idle(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_LookAtTargetActor(this);

    if (Player_IsSwimming(this)) {
        Player_Cutscene_SurfaceFromDive(play, this, NULL);
        return;
    }

    LinkAnimation_Update(play, &this->skelAnime);

    if (Player_IsShootingHookshot(this) || (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_UpdateUpperBody(this, play);
        return;
    }

    if ((this->interactRangeActor != NULL) && (this->interactRangeActor->textId == 0xFFFF)) {
        Player_ActionChange_TryGetItemOrCarry(this, play);
    }
}

void Player_Cutscene_TurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_InitIdle(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimationHeader* anim;

    if (Player_IsSwimming(this)) {
        Player_Cutscene_InitSwimIdle(play, this, NULL);
        return;
    }

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_nwait, this->modelAnimType);

    if ((this->cueId == PLAYER_CUEID_6) || (this->cueId == PLAYER_CUEID_46)) {
        Player_AnimPlayOnce(play, this, anim);
    } else {
        Player_ZeroRootLimbYaw(this);
        LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, Animation_GetLastFrame(anim),
                             ANIMMODE_LOOP, -4.0f);
    }

    Player_ZeroSpeedXZ(this);
}

void Player_Cutscene_Wait(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (Player_TryShootingGalleryPlay(play, this) == 0) {
        if ((this->csAction == PLAYER_CSACTION_IDLE_4) && (play->csCtx.state == CS_STATE_IDLE)) {
            Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_END);
            return;
        }

        if (Player_IsSwimming(this) != 0) {
            Player_Cutscene_SurfaceFromDive(play, this, NULL);
            return;
        }

        LinkAnimation_Update(play, &this->skelAnime);

        if (Player_IsShootingHookshot(this) || (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            Player_UpdateUpperBody(this, play);
        }
    }
}

static AnimSfxEntry sTurnAroundSurprisedAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 42) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 48) },
};

void Player_Cutscene_TurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_ProcessAnimSfxList(this, sTurnAroundSurprisedAnimSfx);
}

void Player_Cutscene_InitEnterWarp(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->stateFlags1 &= ~PLAYER_STATE1_AWAITING_THROWN_BOOMERANG;

    this->yaw = this->actor.shape.rot.y = this->actor.world.rot.y =
        Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (this->speedXZ <= 0.0f) {
        this->speedXZ = 0.1f;
    } else if (this->speedXZ > 2.5f) {
        this->speedXZ = 2.5f;
    }
}

void Player_Cutscene_EnterWarp(PlayState* play, Player* this, CsCmdActorCue* cue) {
    f32 sp1C = 2.5f;

    Player_ApproachCsMovePos(play, this, &sp1C, 10);

    if (play->sceneId == SCENE_JABU_JABU_BOSS) {
        if (this->av2.actionVar2 == 0) {
            if (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) {
                return;
            }
        } else {
            if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
                return;
            }
        }
    }

    this->av2.actionVar2++;
    if (this->av2.actionVar2 > 20) {
        this->csAction = PLAYER_CSACTION_RAISED_BY_WARP;
    }
}

void Player_Cutscene_InitFightStance(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_SetupBattleTargetStandStill(this, play);
}

void Player_Cutscene_FightStance(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_LookAtTargetActor(this);

    if (this->av2.actionVar2 != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_AnimPlayLoop(play, this, Player_GetFightingRightAnim(this));
            this->av2.actionVar2 = 0;
        }

        Player_ResetLRBlendWeight(this);
    } else {
        Player_PlayBlendedFightAnims(play, this);
    }
}

void Player_Cutscene_Unk3Update(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_CsMovement(play, this, cue, 0.0f, 0, 0);
}

void Player_Cutscene_Unk4Update(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_CsMovement(play, this, cue, 0.0f, 0, 1);
}

// unused
static LinkAnimationHeader* D_80855190[] = {
    &gPlayerAnim_link_demo_back_to_past,
    &gPlayerAnim_clink_demo_goto_future,
};

static Vec3f D_80855198 = { -1.0f, 70.0f, 20.0f };

void Player_Cutscene_InitSwordPedestal(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Math_Vec3f_Copy(&this->actor.world.pos, &D_80855198);
    this->actor.shape.rot.y = -0x8000;
    Player_AnimPlayOnceAdjusted(play, this, this->ageProperties->unk_9C);
    Player_AnimReplaceApplyFlags(play, this,
                                 ANIM_REPLACE_APPLY_FLAG_9 | ANIM_FLAG_0 | ANIM_FLAG_UPDATE_Y | ANIM_FLAG_PLAYER_2 |
                                     ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_PLAYER_7);
}

static SwordPedestalSfx sSwordPedestalSfx[] = {
    { NA_SE_IT_SWORD_PUTAWAY_STN, 0 },
    { NA_SE_IT_SWORD_STICK_STN, NA_SE_VO_LI_SWORD_N },
};

static AnimSfxEntry sStepOntoPedestalAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 29) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_8, 39) },
};

void Player_Cutscene_SwordPedestal(PlayState* play, Player* this, CsCmdActorCue* cue) {
    SwordPedestalSfx* swordPedestalSfx;
    Gfx** dLists;

    LinkAnimation_Update(play, &this->skelAnime);

    if ((LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 70.0f)) ||
        (!LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 87.0f))) {
        swordPedestalSfx = &sSwordPedestalSfx[gSaveContext.save.linkAge];
        this->interactRangeActor->parent = &this->actor;

        if (!LINK_IS_ADULT) {
            dLists = gPlayerLeftHandBgsDLs;
        } else {
            dLists = gPlayerLeftHandClosedDLs;
        }
        this->leftHandDLists = dLists + gSaveContext.save.linkAge;

        Player_PlaySfx(this, swordPedestalSfx->swordSfx);
        if (!LINK_IS_ADULT) {
            Player_PlayVoiceSfxForAge(this, swordPedestalSfx->voiceSfx);
        }
    } else if (LINK_IS_ADULT) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 66.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
        }
    } else {
        Player_ProcessAnimSfxList(this, sStepOntoPedestalAnimSfx);
    }
}

void Player_Cutscene_InitWarpToSages(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_demo_warp, -(2.0f / 3.0f), 12.0f, 12.0f,
                         ANIMMODE_ONCE, 0.0f);
}

static AnimSfxEntry D_808551B4[] = {
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_5, 30) },
};

void Player_Cutscene_WarpToSages(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);

    this->av2.actionVar2++;

    if (this->av2.actionVar2 >= 180) {
        if (this->av2.actionVar2 == 180) {
            LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_okarina_warp_goal, (2.0f / 3.0f), 10.0f,
                                 Animation_GetLastFrame(&gPlayerAnim_link_okarina_warp_goal), ANIMMODE_ONCE, -8.0f);
        }
        Player_ProcessAnimSfxList(this, D_808551B4);
    }
}

void Player_Cutscene_KnockedToGround(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->av2.actionVar2 == 0) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_back_downB);
        this->av2.actionVar2 = 1;
    }

    if (this->av2.actionVar2 != 0) {
        Player_StepSpeedXZToZero(this);
    }
}

void Player_Cutscene_InitPlayOcarina(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimChangeOnceMorphAdjustedZeroRootYawSpeed(play, this, &gPlayerAnim_link_normal_okarina_start);
    Player_SetOcarinaItemAction(this);
    Player_SetModels(this, Player_ActionToModelGroup(this, this->itemAction));
}

static AnimSfxEntry D_808551B8[] = {
    { NA_SE_IT_SWORD_PICKOUT, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 12) },
};

void Player_Cutscene_DrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Player_DrawCsSword(play, this, 0);
    } else {
        Player_ProcessAnimSfxList(this, D_808551B8);
    }
}

void Player_Cutscene_CloseEyes(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 0, 1);
}

void Player_Cutscene_OpenEyes(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 2, 1);
}

void Player_Cutscene_InitGetItemInWater(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimReplacePlayOnceAdjusted(play, this, &gPlayerAnim_link_swimer_swim_get,
                                       ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
}

void Player_Cutscene_InitSleeping(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimReplacePlayOnce(play, this, &gPlayerAnim_clink_op3_negaeri,
                               ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
    // Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_GROAN);
}

void Player_Cutscene_Sleeping(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimReplacePlayLoop(play, this, &gPlayerAnim_clink_op3_wait2,
                                   ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                       ANIM_FLAG_PLAYER_7);
    }
}

void Player_Cutscene_PlayLoopAdjustedAnimWithSfx(PlayState* play, Player* this, LinkAnimationHeader* anim,
                                                 AnimSfxEntry* arg3) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoopAdjusted(play, this, anim);
        this->av2.actionVar2 = 1;
    } else if (this->av2.actionVar2 == 0) {
        Player_ProcessAnimSfxList(this, arg3);
    }
}

void Player_Cutscene_InitSleepingRestless(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->actor.shape.shadowDraw = NULL;
    Player_Cutscene_AnimPlaybackType7(play, this, &gPlayerAnim_clink_op3_wait1);
}

static AnimSfxEntry sAwakenAnimSfx[] = {
    { NA_SE_VO_LI_BREATH_REST, ANIMSFX_DATA(ANIMSFX_TYPE_4, 35) },
    { NA_SE_PL_SLIPDOWN, ANIMSFX_DATA(ANIMSFX_TYPE_1, 55) },
    { NA_SE_PL_SLIPDOWN, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 65) },
};

void Player_Cutscene_Awaken(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimReplacePlayLoop(play, this, &gPlayerAnim_clink_op3_wait3,
                                   ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                       ANIM_FLAG_PLAYER_7);
        this->av2.actionVar2 = 1;
    } else if (this->av2.actionVar2 == 0) {
        Player_ProcessAnimSfxList(this, sAwakenAnimSfx);
        if (LinkAnimation_OnFrame(&this->skelAnime, 240.0f)) {
            this->actor.shape.shadowDraw = ActorShadow_DrawFeet;
        }
    }
}

static AnimSfxEntry sGetOffBedAnimSfx[] = {
    { NA_SE_PL_LAND + SURFACE_SFX_OFFSET_WOOD, ANIMSFX_DATA(ANIMSFX_TYPE_1, 67) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_9, 84) },
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_9, 90) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_9, 96) },
};

void Player_Cutscene_GetOffBed(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_ProcessAnimSfxList(this, sGetOffBedAnimSfx);
}

void Player_Cutscene_InitBlownBackward(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimReplacePlayOnceAdjusted(play, this, &gPlayerAnim_clink_demo_futtobi,
                                       ANIM_FLAG_0 | ANIM_FLAG_PLAYER_2 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE |
                                           ANIM_FLAG_PLAYER_7);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_Cutscene_SetTweenedPos(PlayState* play, Player* this, CsCmdActorCue* cue) {
    f32 startX = cue->startPos.x;
    f32 startY = cue->startPos.y;
    f32 startZ = cue->startPos.z;

    f32 distX = cue->endPos.x - startX;
    f32 distY = cue->endPos.y - startY;
    f32 distZ = cue->endPos.z - startZ;

    f32 cuePercentComplete = (f32)(play->csCtx.curFrame - cue->startFrame) / (f32)(cue->endFrame - cue->startFrame);

    this->actor.world.pos.x = distX * cuePercentComplete + startX;
    this->actor.world.pos.y = distY * cuePercentComplete + startY;
    this->actor.world.pos.z = distZ * cuePercentComplete + startZ;
}

static AnimSfxEntry sBlownBackwardAnimSfx[] = {
    { NA_SE_PL_BOUND, ANIMSFX_DATA(ANIMSFX_TYPE_2, 20) },
    { NA_SE_PL_BOUND, -ANIMSFX_DATA(ANIMSFX_TYPE_2, 30) },
};

void Player_Cutscene_BlownBackward(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_SetTweenedPos(play, this, cue);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_ProcessAnimSfxList(this, sBlownBackwardAnimSfx);
}

void Player_Cutscene_RaisedByWarp(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (cue != NULL) {
        Player_Cutscene_SetTweenedPos(play, this, cue);
    }
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_InitIdle3(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimChangeOnceMorph(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_nwait, this->modelAnimType));
    Player_ZeroSpeedXZ(this);
}

void Player_Cutscene_Idle2(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_InitStop(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_AnimReplaceApplyFlags(play, this, ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE | ANIM_FLAG_PLAYER_7);
}

void Player_Cutscene_InitSetDraw(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->actor.draw = Player_Draw;
}

void Player_Cutscene_DrawSwordChild(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimReplaceNormalPlayLoopAdjusted(play, this, &gPlayerAnim_clink_demo_koutai_wait);
        this->av2.actionVar2 = 1;
    } else if (this->av2.actionVar2 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_DrawCsSword(play, this, 1);
        }
    }
}

static AnimSfxEntry sTurnAroundSlowlyAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 10) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 24) },
};

void Player_Cutscene_TurnAroundSlowly(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_PlayLoopAdjustedAnimWithSfx(play, this, &gPlayerAnim_link_demo_furimuki2_wait,
                                                sTurnAroundSlowlyAnimSfx);
}

static AnimSfxEntry sInspectGroundCarefullyAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_8, 15) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_8, 35) },
};

void Player_Cutscene_InspectGroundCarefully(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_PlayLoopAdjustedAnimWithSfx(play, this, &gPlayerAnim_link_demo_nozokikomi_wait,
                                                sInspectGroundCarefullyAnimSfx);
}

void Player_Cutscene_StartPassOcarina(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_AnimPlayLoopAdjusted(play, this, &gPlayerAnim_demo_link_twait);
        this->av2.actionVar2 = 1;
    }

    if ((this->av2.actionVar2 != 0) && (play->csCtx.curFrame >= 900)) {
        this->rightHandType = PLAYER_MODELTYPE_LH_OPEN;
    } else {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
    }
}

void Player_Cutscene_AnimPlaybackType12PlaySfx(PlayState* play, Player* this, LinkAnimationHeader* anim,
                                               AnimSfxEntry* arg3) {
    Player_Cutscene_AnimPlaybackType12(play, this, anim);
    if (this->av2.actionVar2 == 0) {
        Player_ProcessAnimSfxList(this, arg3);
    }
}

static AnimSfxEntry sStepBackCautiouslyAnimSfx[] = {
    { 0, ANIMSFX_DATA(ANIMSFX_TYPE_6, 15) },
    { 0, -ANIMSFX_DATA(ANIMSFX_TYPE_6, 33) },
};

void Player_Cutscene_StepBackCautiously(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_clink_demo_koutai_wait,
                                              sStepBackCautiouslyAnimSfx);
}

static AnimSfxEntry sDesperateLookAtZeldasCrystalAnimSfx[] = {
    { NA_SE_PL_KNOCK, -ANIMSFX_DATA(ANIMSFX_TYPE_1, 78) },
};

void Player_Cutscene_DesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Cutscene_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_link_demo_kakeyori_wait,
                                              sDesperateLookAtZeldasCrystalAnimSfx);
}

void Player_Cutscene_InitSpinAttackIdle(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_SetSpinAttackAnims(play, this);
}

void Player_Cutscene_SpinAttackIdle(PlayState* play, Player* this, CsCmdActorCue* cue) {
    sControlInput->press.button |= BTN_B;

    Player_Action_ChargeSpinAttack(this, play);
}

void Player_Cutscene_InspectWeapon(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_Action_ChargeSpinAttack(this, play);
}

void Player_Cutscene_InitDoNothing(PlayState* play, Player* this, CsCmdActorCue* cue) {
}

void Player_Cutscene_DoNothing(PlayState* play, Player* this, CsCmdActorCue* cue) {
}

void Player_Cutscene_InitKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->stateFlags3 |= PLAYER_STATE3_MIDAIR;
    this->speedXZ = 2.0f;
    this->actor.velocity.y = -1.0f;

    Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_back_downA);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

static void (*sCsKnockedToGroundDamagedFuncs[])(Player* this, PlayState* play) = {
    Player_Action_Knockback,
    Player_Action_DownFromKnockback,
    Player_Action_GetUpFromKnockback,
};

void Player_Cutscene_KnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorCue* cue) {
    sCsKnockedToGroundDamagedFuncs[this->av2.actionVar2](this, play);
}

void Player_Cutscene_InitGetSwordBack(PlayState* play, Player* this, CsCmdActorCue* cue) {
    Player_DrawCsSword(play, this, 0);
    Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_demo_return_to_past);
}

void Player_Cutscene_SwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorCue* cue) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
        this->heldItemAction = this->itemAction = PLAYER_IA_NONE;
        this->heldItemId = ITEM_NONE;
        this->modelGroup = this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_IA_NONE);
        this->leftHandDLists = gPlayerLeftHandOpenDLs;
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_MASTER;
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SWORD);
    }
}

static LinkAnimationHeader* sLearnOcarinaSongAnims[] = {
    &gPlayerAnim_L_okarina_get,
    &gPlayerAnim_om_get,
};

static Vec3s D_80855210[2][2] = {
    { { -200, 700, 100 }, { 800, 600, 800 } },
    { { -200, 500, 0 }, { 600, 400, 600 } },
};

void Player_Cutscene_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorCue* cue) {
    static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    static Color_RGBA8 primColor = { 255, 255, 255, 0 };
    static Color_RGBA8 envColor = { 0, 128, 128, 0 };
    s32 linkAge = gSaveContext.save.linkAge;
    Vec3f sparklePos;
    Vec3f randOffsetSparklePos;
    Vec3s* ptr;

    Player_Cutscene_AnimPlaybackType12(play, this, sLearnOcarinaSongAnims[linkAge]);

    if (this->rightHandType != PLAYER_MODELTYPE_RH_FF) {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
        return;
    }

    ptr = D_80855210[gSaveContext.save.linkAge];

    randOffsetSparklePos.x = ptr[0].x + Rand_CenteredFloat(ptr[1].x);
    randOffsetSparklePos.y = ptr[0].y + Rand_CenteredFloat(ptr[1].y);
    randOffsetSparklePos.z = ptr[0].z + Rand_CenteredFloat(ptr[1].z);

    SkinMatrix_Vec3fMtxFMultXYZ(&this->shieldMf, &randOffsetSparklePos, &sparklePos);

    EffectSsKiraKira_SpawnDispersed(play, &sparklePos, &zeroVec, &zeroVec, &primColor, &envColor, 600, -10);
}

void Player_Cutscene_GetSwordBack(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_Cutscene_Finish(play, this, cue);
    } else if (this->av2.actionVar2 == 0) {
        Item_Give(play, ITEM_SWORD_MASTER);
        Player_DrawCsSword(play, this, 0);
    } else {
        Player_PlaySwordSwingSfx(this);
    }
}

void Player_Cutscene_GanonKillCombo(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_TryMeleeAttack(this, 0.0f, 99.0f, this->skelAnime.endFrame - 8.0f);
    }

    if (this->heldItemAction != PLAYER_IA_SWORD_MASTER) {
        Player_DrawCsSword(play, this, 1);
    }
}

void Player_Cutscene_Finish(PlayState* play, Player* this, CsCmdActorCue* cue) {
    if (Player_IsSwimming(this)) {
        Player_SetupSwimIdle(play, this);
        Player_ResetSubCam(play, this);
    } else {
        Player_ClearLookAndAttention(this, play);
        if (!Player_ActionChange_TrySpeakOrCheck(this, play)) {
            Player_ActionChange_TryGetItemOrCarry(this, play);
        }
    }

    this->csAction = PLAYER_CSACTION_NONE;
    this->attentionMode = 0;
}

void Player_Cutscene_SetPosAndYaw(PlayState* play, Player* this, CsCmdActorCue* cue) {
    this->actor.world.pos.x = cue->startPos.x;
    this->actor.world.pos.y = cue->startPos.y;

    if ((play->sceneId == SCENE_KOKIRI_FOREST) && !LINK_IS_ADULT) {
        this->actor.world.pos.y -= 1.0f;
    }

    this->actor.world.pos.z = cue->startPos.z;
    this->yaw = this->actor.shape.rot.y = cue->rot.y;
}

void Player_Cutscene_SetPosAndYawIfOutsideStartRange(PlayState* play, Player* this, CsCmdActorCue* cue) {
    f32 dx = cue->startPos.x - (s32)this->actor.world.pos.x;
    f32 dy = cue->startPos.y - (s32)this->actor.world.pos.y;
    f32 dz = cue->startPos.z - (s32)this->actor.world.pos.z;
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    s16 yawDiff = (s16)cue->rot.y - this->actor.shape.rot.y;

    if ((this->speedXZ == 0.0f) && ((dist > 50.0f) || (ABS(yawDiff) > 0x4000))) {
        Player_Cutscene_SetPosAndYaw(play, this, cue);
    }

    this->skelAnime.moveFlags = 0;
    Player_ZeroRootLimbYaw(this);
}

void Player_CsModePlayback(PlayState* play, Player* this, CsCmdActorCue* cue, CutsceneModeEntry* csMode) {
    if (csMode->type > 0) {
        csModePlaybackFuncs[csMode->type](play, this, csMode->ptr);
    } else if (csMode->type < 0) {
        csMode->func(play, this, cue);
    }

    if ((sPrevSkelAnimeMoveFlags & ANIM_FLAG_PLAYER_2) && !(this->skelAnime.moveFlags & ANIM_FLAG_PLAYER_2)) {
        this->skelAnime.morphTable[0].y /= this->ageProperties->unk_08;
        sPrevSkelAnimeMoveFlags = 0;
    }
}

void Player_Cutscene_DetatchHeldActor(PlayState* play, Player* this, s32 csAction) {
    if ((csAction != PLAYER_CSACTION_IDLE) && (csAction != PLAYER_CSACTION_WAIT) &&
        (csAction != PLAYER_CSACTION_IDLE_4) && (csAction != PLAYER_CSACTION_END)) {
        Player_DetachHeldActor(play, this);
    }
}

void Player_Cutscene_Unk6Update(PlayState* play, Player* this, CsCmdActorCue* cueUnused) {
    CsCmdActorCue* cue = play->csCtx.playerCue;
    s32 pad;
    s32 csAction;

    if (play->csCtx.state == CS_STATE_STOP) {
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_END);
        this->cueId = PLAYER_CUEID_NONE;
        Player_ZeroSpeedXZ(this);
        return;
    }

    if (cue == NULL) {
        this->actor.flags &= ~ACTOR_FLAG_6;
        return;
    }

    if (this->cueId != cue->id) {
        csAction = sCueToCsActionMap[cue->id];

        if (csAction >= PLAYER_CSACTION_NONE) {
            if ((csAction == PLAYER_CSACTION_3) || (csAction == PLAYER_CSACTION_4)) {
                Player_Cutscene_SetPosAndYawIfOutsideStartRange(play, this, cue);
            } else {
                Player_Cutscene_SetPosAndYaw(play, this, cue);
            }
        }

        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;

        Player_FinishAnimMovement(this);
        osSyncPrintf("TOOL MODE=%d\n", csAction);
        Player_Cutscene_DetatchHeldActor(play, this, ABS(csAction));
        Player_CsModePlayback(play, this, cue, &sCutsceneModeInitFuncs[ABS(csAction)]);

        this->av2.actionVar2 = 0;
        this->av1.actionVar1 = 0;
        this->cueId = cue->id;
    }

    csAction = sCueToCsActionMap[this->cueId];
    Player_CsModePlayback(play, this, cue, &sCutsceneModeUpdateFuncs[ABS(csAction)]);
}

void Player_Action_CsAction(Player* this, PlayState* play) {
    if (this->csAction != this->prevCsAction) {
        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;

        Player_FinishAnimMovement(this);
        this->prevCsAction = this->csAction;
        osSyncPrintf("DEMO MODE=%d\n", this->csAction);
        Player_Cutscene_DetatchHeldActor(play, this, this->csAction);
        Player_CsModePlayback(play, this, NULL, &sCutsceneModeInitFuncs[this->csAction]);
    }

    Player_CsModePlayback(play, this, NULL, &sCutsceneModeUpdateFuncs[this->csAction]);
}

int Player_IsDroppingFish(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (Player_Action_DropItemFromBottle == this->actionFunc) && (this->itemAction == PLAYER_IA_BOTTLE_FISH);
}

s32 Player_StartFishing(PlayState* play) {
    Player* this = GET_PLAYER(play);

    Player_StopCarryingActor(play, this);
    Player_UseItem(play, this, ITEM_FISHING_POLE);
    return 1;
}

s32 Player_TryRestrainedByEnemy(PlayState* play, Player* this) {
    if (!Player_InBlockingCsMode(play, this) && (this->invincibilityTimer >= 0) && !Player_IsShootingHookshot(this) &&
        !(this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        Player_StopCarryingActor(play, this);
        Player_SetupAction(play, this, Player_Action_RestrainedByEnemy, 0);
        Player_AnimPlayOnce(play, this, &gPlayerAnim_link_normal_re_dead_attack);
        this->stateFlags2 |= PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        Player_ClearAttentionModeAndStopMoving(this);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HELD);
        return true;
    }

    return false;
}

/**
 * Tries to starts a cutscene action specified by `csAction`.
 * A cutscene action will only start if player is not already in another form of cutscene.
 *
 * No actors will be halted over the duration of the cutscene action.
 *
 * @return  true if successful starting a `csAction`, false if not
 */
s32 Player_TryCsAction(PlayState* play, Actor* actor, s32 csAction) {
    Player* this = GET_PLAYER(play);

    if (!Player_InBlockingCsMode(play, this)) {
        Player_StopCarryingActor(play, this);
        Player_SetupAction(play, this, Player_Action_CsAction, 0);
        this->csAction = csAction;
        this->csActor = actor;
        Player_ClearAttentionModeAndStopMoving(this);
        return true;
    }

    return false;
}

void Player_SetupStandStillMorph(Player* this, PlayState* play) {
    Player_SetupAction(play, this, Player_Action_StandStill, 1);
    Player_AnimChangeOnceMorph(play, this, Player_GetStandStillAnim(this));
    this->yaw = this->actor.shape.rot.y;
}

s32 Player_InflictDamageAndCheckForDeath(PlayState* play, s32 damage) {
    Player* this = GET_PLAYER(play);

    if (!Player_InBlockingCsMode(play, this) && !Player_InflictDamage(play, this, damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        return 1;
    }

    return 0;
}

// Start talking with the given actor
void Player_StartTalkToActor(PlayState* play, Actor* actor) {
    Player* this = GET_PLAYER(play);
    s32 pad;

    if ((this->talkActor != NULL) || /* (actor == this->naviActor) || */
        CHECK_FLAG_ALL(actor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18)) {
        actor->flags |= ACTOR_FLAG_TALK;
    }

    this->talkActor = actor;
    this->exchangeItemId = EXCH_ITEM_NONE;

    if (actor->textId == 0xFFFF) {
        Player_SetCsActionWithHaltedActors(play, actor, PLAYER_CSACTION_IDLE);
        actor->flags |= ACTOR_FLAG_TALK;
        Player_TryUnequipItem(play, this);
    } else {
        if (this->actor.flags & ACTOR_FLAG_TALK) {
            this->actor.textId = 0;
        } else {
            this->actor.flags |= ACTOR_FLAG_TALK;
            this->actor.textId = actor->textId;
        }

        if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
            s32 sp24 = this->av2.actionVar2;

            Player_TryUnequipItem(play, this);
            Player_SetupTalkToActor(play, this);

            this->av2.actionVar2 = sp24;
        } else {
            if (Player_IsSwimming(this)) {
                Player_SetupMiniCs(play, this, Player_SetupTalkToActor);
                Player_AnimChangeLoopSlowMorph(play, this, &gPlayerAnim_link_swimer_swim_wait);
            } else if ((actor->category != ACTORCAT_NPC) || (this->heldItemAction == PLAYER_IA_FISHING_POLE)) {
                Player_SetupTalkToActor(play, this);

                if (!Player_CheckBattleTargeting(this)) {
                    if (/* (actor != this->naviActor) &&  */(actor->xzDistToPlayer < 40.0f)) {
                        Player_AnimPlayOnceAdjusted(play, this, &gPlayerAnim_link_normal_backspace);
                    } else {
                        Player_AnimPlayLoop(play, this, Player_GetStandStillAnim(this));
                    }
                }
            } else {
                Player_SetupMiniCs(play, this, Player_SetupTalkToActor);
                Player_AnimPlayOnceAdjusted(play, this,
                                            (actor->xzDistToPlayer < 40.0f) ? &gPlayerAnim_link_normal_backspace
                                                                            : &gPlayerAnim_link_normal_talk_free);
            }

            if (this->skelAnime.animation == &gPlayerAnim_link_normal_backspace) {
                Player_AnimReplaceApplyFlags(play, this, ANIM_FLAG_0 | ANIM_FLAG_PLAYER_SETMOVE | ANIM_FLAG_NO_MOVE);
            }

            Player_ClearAttentionModeAndStopMoving(this);
        }

        this->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE;
    }

    /* if ((this->naviActor == this->talkActor) && ((this->talkActor->textId & 0xFF00) != 0x200)) {
        this->naviActor->flags |= ACTOR_FLAG_TALK;
        Player_SetUseItemCam(play, 0xB);
    } */
}

void Player_OffsetBoosterFlames(Vec3f* flame, f32 xOffset, f32 yOffset, f32 zOffset) {
    flame->x -= xOffset;
    flame->y += yOffset;
    flame->z -= zOffset;
}

void Player_SpawnBoosterEffects(PlayState* play, Player* this) {
    Color_RGBA8 flamePrimColor = { 255, 255, 0, 255 };
    Color_RGBA8 flameEnvColor = { 255, 0, 0, 255 };
    Vec3f rightFlame = this->bodyPartsPos[PLAYER_BODYPART_R_HAND];
    Vec3f leftFlame = this->bodyPartsPos[PLAYER_BODYPART_L_HAND];
    Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    f32 scaleY = -0.35f;

    // Initial flame effects
    func_8002836C(play, &rightFlame, &zeroVec, &zeroVec,
                &flamePrimColor, &flameEnvColor, 200.0f, 0, 5);
    func_8002836C(play, &leftFlame, &zeroVec, &zeroVec,
                &flamePrimColor, &flameEnvColor, 200.0f, 0, 5);

    if (this->speedXZ > 10.0f) {
        f32 xOffset = Math_SinS(this->actor.shape.rot.y) * (this->speedXZ / 2.5f);
        f32 zOffset = Math_CosS(this->actor.shape.rot.y) * (this->speedXZ / 2.5f);
        u8 numFlames = 3;
        u8 i;

        for (i = 0; i < numFlames; i++) {
            Player_OffsetBoosterFlames(&rightFlame, xOffset, this->actor.velocity.y * scaleY, zOffset);
            Player_OffsetBoosterFlames(&leftFlame, xOffset, this->actor.velocity.y * scaleY, zOffset);

            func_8002836C(play, &rightFlame, &zeroVec, &zeroVec, &flamePrimColor, &flameEnvColor, 200.0f, 0, 5);
            func_8002836C(play, &leftFlame, &zeroVec, &zeroVec, &flamePrimColor, &flameEnvColor, 200.0f, 0, 5);
        }
    }
}


s32 Player_CheckBoosterJumpBonk(Player* this, PlayState* play, f32 speedTarget) {
    Actor* cylinderOc = NULL;
    DynaPolyActor* wallPolyActor;

    if (speedTarget <= this->speedXZ) {
        // If interacting with a wall and close to facing it
        // Added negative speedXZ check to prevent bugs where players end up going backward...?
        if (((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sShapeYawToTouchedWall < 0x1C00)) ||
            ((this->cylinder.base.ocFlags1 & OC1_HIT) &&
             (cylinderOc = this->cylinder.base.oc,
              ((cylinderOc->id == ACTOR_EN_WOOD02) &&
               (ABS((s16)(this->actor.world.rot.y - cylinderOc->yawTowardsPlayer)) > 0x6000)))) || this->speedXZ < 0) {
            if (!((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF))) {

                if (cylinderOc != NULL) {
                    cylinderOc->home.rot.y = 1;
                } else if (this->actor.wallBgId != BGCHECK_SCENE) {
                    wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
                    if ((wallPolyActor != NULL) && (wallPolyActor->actor.id == ACTOR_BOOST_BREAKWALL)) {
                        wallPolyActor->actor.home.rot.z = 1;
                        return false;
                    }
                }

                if (sYDistToFloor > 10.0f) {
                    this->boostCooldownTimer = 10;
                    this->actor.colChkInfo.damage = 4;
                    Player_TakeColliderDamage(play, this, 1, 9.0f, 5.0f, this->actor.shape.rot.y, 20);
                } else {
                    Player_SetupAction(play, this, Player_Action_Rolling, 0);
                    Player_AnimPlayOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_hip_down, this->modelAnimType));
                    this->av2.actionVar2 = 1;
                }

                this->stateFlags3 &= ~PLAYER_STATE3_USING_BOOSTERS;
                this->speedXZ = -1.0f;
                Player_RequestQuake(play, 33267, 3, 12);
                Player_RequestRumble(this, 255, 20, 150, 0);
                Player_PlaySfx(this, NA_SE_PL_BODY_HIT);
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                return true;
            }
        }
    }
    return false;
}

#define PLAYER_BOOSTER_ACTIVE_TIME 1 * 20

void Player_Action_FinishBoosterJump(Player* this, PlayState* play) {
    u8 animDone = LinkAnimation_Update(play, &this->skelAnime);

    Math_SmoothStepToF(&this->speedXZ, -40.0f, 1.0f, 5.0f, 0.0f);

    if (animDone && !(this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WATER))) {
        // Clear state flag, will start midair behavior
        this->stateFlags3 &= ~PLAYER_STATE3_USING_BOOSTERS;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        this->stateFlags3 &= ~PLAYER_STATE3_USING_BOOSTERS;
        Player_ZeroSpeedXZ(this);
        Player_SetupRolling(this, play);
    }
}

s32 Player_TryFinishBoosterJump(Player* this, PlayState* play) {
    if (this->boostActiveTimer == 0 || this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        this->boostActiveTimer = 0;
        this->boostCooldownTimer = 10;
        if (!(this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WATER))) {
            Player_AnimPlayOnce(play, this, &gLinkAdultSkelFinishboosterjumpAnim);
        }
        Player_SetupActionKeepItemAction(play, this, Player_Action_FinishBoosterJump, 0);
        this->stateFlags3 |= PLAYER_STATE3_USING_BOOSTERS;
        return 1;
    }

    return 0;
}

void Player_Action_BoosterJump(Player* this, PlayState* play) {
    u8 animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (animDone && this->skelAnime.animation == &gLinkAdultSkelStartboosterjumpAnim) {
        Player_AnimPlayLoop(play, this, &gLinkAdultSkelBoosterjumpAnim);
    }

    this->actor.gravity = -1.2f;
    DECR(this->boostActiveTimer);
    Math_SmoothStepToF(&this->speedXZ, 40.0f, 1.0f, 5.0f, 0.0f);
    Math_SmoothStepToF(&play->mainCamera.fov, 70.0f, 1.0f, 2.0f, 0.0f);
    Player_CheckBoosterJumpBonk(this, play, 10.0f);
    Player_TryFinishBoosterJump(this, play);
    Player_PlaySfx(this, NA_SE_EV_FIRE_PILLAR - SFX_FLAG);
}

void Player_Action_StartBoosterJump(Player* this, PlayState* play) {
    if (this->skelAnime.animation != &gLinkAdultSkelStartboosterjumpAnim) {
        LinkAnimation_Change(play, &this->skelAnime, &gLinkAdultSkelStartboosterjumpAnim, 1.0f, 0.0f, Animation_GetLastFrame(&gLinkAdultSkelStartboosterjumpAnim), ANIMMODE_ONCE, 2.0f);
    }

    LinkAnimation_Update(play, &this->skelAnime);
    
    if (this->skelAnime.curFrame >= 4.0f) {
        Player_PlaySfx(this, NA_SE_EN_TWINROBA_FIRE_EXP - SFX_FLAG);
        Player_SetupActionKeepItemAction(play, this, Player_Action_BoosterJump, 0);
        this->stateFlags3 |= PLAYER_STATE3_USING_BOOSTERS;
        this->boostActiveTimer = PLAYER_BOOSTER_ACTIVE_TIME;
        this->actor.velocity.y = 15.0f;
        this->yaw = this->actor.shape.rot.y;
    }
}

s32 Player_TryStartBoosterJump(Player* this, PlayState* play) {
    u8 isHoldingForward = ABS(sControlStickAngle) < DEG_TO_BINANG(45.0f);
    
    if (this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING || (Player_CheckTargeting(this) && !isHoldingForward && (sControlStickMagnitude > 0.0f))) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
        return 0;
    }
    
    Player_SetupActionKeepItemAction(play, this, Player_Action_StartBoosterJump, 0);
    Player_ZeroSpeedXZ(this);
    this->stateFlags3 |= PLAYER_STATE3_USING_BOOSTERS;
    return 1;
}

s32 Player_UpperAction_WearBoosters(Player* this, PlayState* play) {
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        DECR(this->boostCooldownTimer);
    }

    if (Player_TryStartTargetingDefense(play, this) || this->boostCooldownTimer > 0 || this->stateFlags1 & (PLAYER_STATE1_CLIMBING_ONTO_LEDGE)) {
        return 0;
    }

    if (sUseHeldItem) {
        return Player_TryStartBoosterJump(this, play);
    }

    return 0;
}
