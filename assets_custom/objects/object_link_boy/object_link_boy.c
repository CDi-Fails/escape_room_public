#include "ultra64.h"
#include "z64.h"
#include "macros.h"
#include "object_link_boy.h"
#include "assets/misc/link_animetion/link_animetion.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

u64 gLinkAdultGauntletPlate1Tex[] = {
#include "assets_custom/objects/object_link_boy/gauntlet_plate_1.rgba16.inc.c"
};

u64 gLinkAdultGauntletPlate2Tex[] = {
#include "assets_custom/objects/object_link_boy/gauntlet_plate_2.rgba16.inc.c"
};

u64 gLinkAdultHoverBootsHeelTex[] = {
#include "assets_custom/objects/object_link_boy/hover_boots_heel.rgba16.inc.c"
};

u64 gLinkAdultHoverBootsJetTex[] = {
#include "assets_custom/objects/object_link_boy/hover_boots_jet.rgba16.inc.c"
};

u64 gLinkAdultHoverBootsFeatherTex[] = {
#include "assets_custom/objects/object_link_boy/hover_boots_feather.rgba16.inc.c"
};

u64 gLinkAdultMirrorShieldLowerDesignTex[] = {
#include "assets_custom/objects/object_link_boy/mirror_shield_lower_design.ia16.inc.c"
};

u64 gLinkAdultMirrorShieldUpperDesignTex[] = {
#include "assets_custom/objects/object_link_boy/mirror_shield_upper_design.ia8.inc.c"
};

u64 gLinkAdultHookshotMetalTex[] = {
#include "assets_custom/objects/object_link_boy/hookshot_metal.rgba16.inc.c"
};

u64 gLinkAdultBowBodyTex[] = {
#include "assets_custom/objects/object_link_boy/bow_body.i8.inc.c"
};

u64 object_link_boyTLUT_00CB40[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyTLUT_00CB40.rgba16.inc.c"
};

u8 object_link_boy_unaccounted_00CD40[] = {
    0x00, 0x00, 0x01, 0x01, 0x00, 0x29, 0x00, 0x8A, 
};

u64 object_link_boyTLUT_00CD48[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyTLUT_00CD48.rgba16.inc.c"
};

u8 object_link_boy_unaccounted_00CF48[] = {
    0x00, 0x00, 0x02, 0x01, 0x86, 0x08, 0x02, 0xFF, 
};

u64 object_link_boyTLUT_00CF50[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyTLUT_00CF50.rgba16.inc.c"
};

u64 object_link_boyTLUT_00D078[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyTLUT_00D078.rgba16.inc.c"
};

u64 gLinkAdultSwordPommelTex[] = {
#include "assets_custom/objects/object_link_boy/sword_pommel.ci8.inc.c"
};

u64 gLinkAdultIronBootTex[] = {
#include "assets_custom/objects/object_link_boy/iron_boot.ci8.inc.c"
};

u64 gLinkAdultDefaultGauntlet1Tex[] = {
#include "assets_custom/objects/object_link_boy/default_gauntlet_1.ci8.inc.c"
};

u64 gLinkAdultShieldHandleTex[] = {
#include "assets_custom/objects/object_link_boy/shield_handle.ci8.inc.c"
};

u64 gLinkAdultDefaultGauntlet2Tex[] = {
#include "assets_custom/objects/object_link_boy/default_gauntlet_2.ci8.inc.c"
};

u64 gLinkAdultHandTex[] = {
#include "assets_custom/objects/object_link_boy/hand.ci8.inc.c"
};

u64 gLinkAdultClosedHandThumbTex[] = {
#include "assets_custom/objects/object_link_boy/closed_hand_thumb.ci8.inc.c"
};

u64 gLinkAdultHylianShieldBackTex[] = {
#include "assets_custom/objects/object_link_boy/hylian_shield_back.ci8.inc.c"
};

u64 gLinkAdultClosedHandSideTex[] = {
#include "assets_custom/objects/object_link_boy/closed_hand_side.ci8.inc.c"
};

u64 gLinkAdultSheathTex[] = {
#include "assets_custom/objects/object_link_boy/sheath.ci8.inc.c"
};

u64 gLinkAdultArmOutUpperGauntletTex[] = {
#include "assets_custom/objects/object_link_boy/arm_out_upper_gauntlet.ci8.inc.c"
};

u64 gLinkAdultSwordGuardTex[] = {
#include "assets_custom/objects/object_link_boy/sword_guard.ci8.inc.c"
};

u64 gLinkAdultSheathBandTex[] = {
#include "assets_custom/objects/object_link_boy/sheath_band.ci8.inc.c"
};

u64 gLinkAdultSwordEmblemTex[] = {
#include "assets_custom/objects/object_link_boy/sword_emblem.ci8.inc.c"
};

u64 gLinkAdultHookshotHandleTex[] = {
#include "assets_custom/objects/object_link_boy/hookshot_handle.ci8.inc.c"
};

u64 gLinkAdultHookshotDesignTex[] = {
#include "assets_custom/objects/object_link_boy/hookshot_design.ci8.inc.c"
};

u64 gLinkAdultArmOutSleeveTex[] = {
#include "assets_custom/objects/object_link_boy/arm_out_sleeve.ci8.inc.c"
};

Vtx object_link_boyVtx_00EFB8[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_00EFB8.vtx.inc"
};

#include "gLinkAdultLeftHandNearDL.c"
#include "gLinkAdultLeftHandClosedNearDL.c"
#include "gLinkAdultSheathDL.c"
#include "gLinkAdultSwordBladeDL.c"
#include "gLinkAdultSwordHiltDL.c"
#include "gLinkAdultRightHandNearDL.c"
#include "gLinkAdultRightHandClosedNearDL.c"
#include "gLinkAdultShieldDL.c"
#include "gLinkAdultMirrorShieldDL.c"
#include "gLinkAdultMirrorShieldDecalDL.c"
#include "gLinkAdultBowDL.c"
#include "gLinkAdultLeftHandOutNearDL.c"
#include "gLinkAdultHookshotDL.c"
#include "gLinkAdultOcarinaDL.c"
#include "gLinkAdultRightArmOutNearDL.c"
#include "gLinkAdultLeftArmOutNearDL.c"
#include "gLinkAdultRightHandOutNearDL.c"
#include "gLinkAdultRightHandFPSHoldDL.c"
#include "gLinkAdultFPSHookshotDL.c"
#include "gLinkAdultLeftBoosterDL.c"
#include "bottle.c"

Mtx gHiltOnBackMtx = gdSPDefMtx( 
    1, 0, 0, -812,                    
    0, 1, 0, -313,                    
    0, 0, 1, 0,                    
    0, 0, 0, 1              
);

Mtx gShieldOnBackMtx = gdSPDefMtx( 
    -1, 0, 0, 812,                     
    0, -1, 0, 0,                     
    0, 0, 1, 0,                      
    0, 0, 0, 1                     
);

Mtx gMirrorShieldOnBackMtx = gdSPDefMtx( 
    -1, 0, 0, 812,                     
    0, -1, 0, 100,                     
    0, 0, 1, 0,                      
    0, 0, 0, 1                     
);

Mtx gBoosterRightMtx = gdSPDefMtx( 
    1, 0, 0, 0,                     
    0, -1, 0, 0,                     
    0, 0, -1, 0,                      
    0, 0, 0, 1                     
);

Gfx gLinkAdultRightBoosterDL[] = {
    gsSPMatrix(&gBoosterRightMtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW),
    gsSPDisplayList(&gLinkAdultLeftBoosterDL),
    gsSPPopMatrix(G_MTX_MODELVIEW),
    gsSPEndDisplayList()
};

// Shield on back
Gfx gLinkAdultShieldOnBackDL[] = {
    gsSPMatrix(&gShieldOnBackMtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW),
    gsSPDisplayList(&gLinkAdultShieldDL),
    gsSPPopMatrix(G_MTX_MODELVIEW),
    gsSPEndDisplayList()
};

// Mirror Shield on back
Gfx gLinkAdultMirrorShieldOnBackDL[] = {
    gsSPMatrix(&gMirrorShieldOnBackMtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW),
    gsSPDisplayList(&gLinkAdultMirrorShieldDL),
    gsSPDisplayList(&gLinkAdultMirrorShieldDecalDL),
    gsSPPopMatrix(G_MTX_MODELVIEW),
    gsSPEndDisplayList()
};

// Sword
Gfx gLinkAdultSwordDL[] = {
    gsSPDisplayList(&gLinkAdultSwordHiltDL),
    gsSPBranchList(&gLinkAdultSwordBladeDL)
};

// Sheathed sword on back
Gfx gLinkAdultSheathedSwordOnBackDL[] = {
    gsSPDisplayList(&gLinkAdultSheathDL),
    gsSPMatrix(&gHiltOnBackMtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW),
    gsSPDisplayList(&gLinkAdultSwordHiltDL),
    gsSPPopMatrix(G_MTX_MODELVIEW),
    gsSPEndDisplayList()
};

// Sheathed sword + shield
Gfx gLinkAdultHylianShieldSwordAndSheathNearDL[] = {
    gsSPDisplayList(&gLinkAdultSheathedSwordOnBackDL),
    gsSPBranchList(&gLinkAdultShieldOnBackDL),
};

// Sheath + shield
Gfx gLinkAdultHylianShieldAndSheathNearDL[] = {
    gsSPDisplayList(&gLinkAdultSheathDL),
    gsSPBranchList(&gLinkAdultShieldOnBackDL)
};

Gfx gLinkAdultMirrorShieldSwordAndSheathNearDL[] = {
    gsSPDisplayList(&gLinkAdultSheathedSwordOnBackDL),
    gsSPBranchList(&gLinkAdultMirrorShieldOnBackDL),
};

Gfx gLinkAdultMirrorShieldAndSheathNearDL[] = {
    gsSPDisplayList(&gLinkAdultSheathDL),
    gsSPBranchList(gLinkAdultMirrorShieldOnBackDL),
};

Gfx gLinkAdultLeftHandHoldingMasterSwordNearDL[] = {
    gsSPDisplayList(&gLinkAdultSwordDL),
    gsSPBranchList(&gLinkAdultLeftHandClosedNearDL)
};

Gfx gLinkAdultRightHandHoldingHylianShieldNearDL[] = {
    gsSPDisplayList(&gLinkAdultShieldDL),
    gsSPBranchList(&gLinkAdultRightHandClosedNearDL)
};

Gfx gLinkAdultRightHandHoldingBowNearDL[] = {
    gsSPDisplayList(&gLinkAdultBowDL),
    gsSPBranchList(&gLinkAdultRightHandClosedNearDL)
};

Gfx gLinkAdultMasterSwordAndSheathNearDL[] = {
    gsSPBranchList(&gLinkAdultSheathedSwordOnBackDL)
};

Gfx gLinkAdultLeftHandHoldingHammerNearDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandHoldingBgsNearDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultHandHoldingBrokenGiantsKnifeDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingMirrorShieldNearDL[] = {
    gsSPDisplayList(&gLinkAdultMirrorShieldDL),
    gsSPDisplayList(&gLinkAdultMirrorShieldDecalDL),
    gsSPBranchList(&gLinkAdultRightHandClosedNearDL)
};

Gfx gLinkAdultRightHandHoldingOotNearDL[] = {
    gsSPDisplayList(&gLinkAdultOcarinaDL),
    gsSPBranchList(&gLinkAdultRightHandClosedNearDL)
};

Gfx gLinkAdultSheathNearDL[] = {
    gsSPBranchList(&gLinkAdultSheathDL)
};

Gfx gLinkAdultRightHandHoldingHookshotNearDL[] = {
    gsSPDisplayList(&gLinkAdultFPSHookshotDL),
    gsSPBranchList(&gLinkAdultRightHandFPSHoldDL)
};

Gfx gLinkAdultLeftGauntletPlate1DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftGauntletPlate2DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftGauntletPlate3DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightGauntletPlate1DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightGauntletPlate2DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightGauntletPlate3DL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftIronBootDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightIronBootDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHoverBootDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHoverBootDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultHylianShieldSwordAndSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultHylianShieldAndSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultMirrorShieldSwordAndSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultMirrorShieldAndSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandClosedFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandHoldingMasterSwordFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandClosedFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingHylianShieldFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingBowFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultMasterSwordAndSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultSheathFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandHoldingHammerFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultLeftHandHoldingBgsFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingMirrorShieldFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingOotFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultHandHoldingBrokenGiantsKnifeFarDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultHandHoldingBottleDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingBowFirstPersonDL[] = {
    gsSPEndDisplayList()
};

Gfx gLinkAdultRightHandHoldingHookshotFarDL[] = {
    gsSPDisplayList(&gLinkAdultHookshotDL),
    gsSPBranchList(&gLinkAdultRightHandClosedNearDL)
};

Gfx gLinkAdultBottleDL[] = {
    gsSPBranchList(&gBottleGlassDL),
};

Vtx object_link_boyVtx_02AE70[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_02AE70.vtx.inc"
};

Gfx gLinkAdultHookshotChainDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(gLinkAdultHookshotChainTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 32, 0, G_TX_MIRROR | G_TX_WRAP,
                         G_TX_MIRROR | G_TX_WRAP, 4, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetCombineMode(G_CC_MODULATEIDECALA_PRIM, G_CC_PASS2),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_TEX_EDGE2),
    gsSPClearGeometryMode(G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPSetGeometryMode(G_FOG),
    gsDPSetPrimColor(0, 0, 255, 255, 255, 255),
    gsSPVertex(object_link_boyVtx_02AE70, 24, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 3, 1, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 7, 5, 0),
    gsSP2Triangles(8, 9, 10, 0, 8, 11, 9, 0),
    gsSP2Triangles(12, 13, 14, 0, 12, 15, 13, 0),
    gsSP2Triangles(16, 17, 18, 0, 16, 19, 17, 0),
    gsSP2Triangles(20, 21, 22, 0, 20, 23, 21, 0),
    gsSPEndDisplayList(),
};

Vtx object_link_boyVtx_02B0A8[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_02B0A8.vtx.inc"
};

Gfx gLinkAdultBowStringDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0, 0, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetCombineLERP(SHADE, 0, PRIMITIVE, 0, 0, 0, 0, 1, 0, 0, 0, COMBINED, 0, 0, 0, COMBINED),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2),
    gsSPClearGeometryMode(G_CULL_BACK | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPSetGeometryMode(G_FOG | G_LIGHTING),
    gsDPSetPrimColor(0, 0, 255, 255, 255, 255),
    gsSPVertex(object_link_boyVtx_02B0A8, 6, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(3, 2, 4, 0, 3, 4, 5, 0),
    gsSPEndDisplayList(),
};

Vtx object_link_boyVtx_02B168[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_02B168.vtx.inc"
};

Gfx gLinkAdultHookshotTipDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0x07D0, 0x09C4, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(gHilite1Tex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR |
                         G_TX_WRAP, 4, 4, G_TX_NOLOD, 15),
    gsDPSetCombineLERP(TEXEL0, 0, SHADE, 0, 0, 0, 0, 1, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2),
    gsSPSetGeometryMode(G_CULL_BACK | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsDPSetPrimColor(0, 0, 255, 255, 255, 255),
    gsSPVertex(object_link_boyVtx_02B168, 3, 0),
    gsSP1Triangle(0, 1, 2, 0),
    gsSPVertex(&object_link_boyVtx_02B168[3], 15, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0),
    gsSP2Triangles(4, 3, 6, 0, 7, 8, 9, 0),
    gsSP2Triangles(8, 7, 10, 0, 11, 12, 13, 0),
    gsSP1Triangle(12, 11, 14, 0),
    gsSPEndDisplayList(),
};

u64 gLinkAdultHookshotChainTex[] = {
#include "assets_custom/objects/object_link_boy/hookshot_chain.rgba16.inc.c"
};

Vtx object_link_boyVtx_02B738[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_02B738.vtx.inc"
};

Gfx gLinkAdultBrokenGiantsKnifeBladeDL[] = {
    gsSPEndDisplayList()
};

u64 gLinkAdultHookshotReticleTex[] = {
#include "assets_custom/objects/object_link_boy/hookshot_dot.i8.inc.c"
};

Vtx object_link_boyVtx_02CB18[] = {
#include "assets_custom/objects/object_link_boy/object_link_boyVtx_02CB18.vtx.inc"
};

Gfx gLinkAdultHookshotReticleDL[] = {
    gsSPMatrix(0x01000000, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(gLinkAdultHookshotReticleTex, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                         G_TX_NOMIRROR | G_TX_CLAMP, 6, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetPrimColor(0, 0, 255, 0, 0, 255),
    gsSPVertex(object_link_boyVtx_02CB18, 3, 0),
    gsSP1Triangle(0, 1, 2, 0),
    gsSPEndDisplayList(),
};
