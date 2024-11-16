#include "ultra64.h"
#include "global.h"

u64 gGiHeatGearGlowDL_eff_bubble_1_i8_i8[] = {
	0x0000000000020507, 0x0705020000000000, 0x00000001080c0c0c, 0x0b0c0b0802000000, 0x0000030b0c0a1837, 0x5868491b0b040000, 0x00010b0c09072667, 0xaaccbb692a0b0200, 
	0x00080c0907053577, 0xcbfffeba480b0800, 0x010c0b0806052566, 0xa9dcedba57190c02, 0x040c090705051535, 0x6688987746180c05, 0x060c091615151515, 0x2545453526080c07, 
	0x060c192625252515, 0x1515151506080c07, 0x041c293745453525, 0x1515050507090c05, 0x011b2b4866656545, 0x35150506080a0c02, 0x00072d4977867565, 0x45251607090c0800, 
	0x00011b3c69787766, 0x462718090c0c0200, 0x0000021b3c5b5959, 0x39291b0c0b030000, 0x00000001172b2c2c, 0x2c1c0c0801000000, 0x0000000000010406, 0x0604020000000000, 
	
};

Vtx gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_vtx_cull[8] = {
	{{ {-25, -17, -28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-25, -17, 28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-25, 8, 28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-25, 8, -28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {28, -17, -28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {28, -17, 28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {28, 8, 28}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {28, 8, -28}, 0, {0, 0}, {0, 0, 0, 0} }},
};

Vtx gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_vtx_0[16] = {
	{{ {13, -1, 28}, 0, {382, 296}, {83, 34, 90, 255} }},
	{{ {27, 8, 12}, 0, {456, 24}, {108, 45, 49, 255} }},
	{{ {12, 1, 28}, 0, {382, 24}, {83, 34, 90, 255} }},
	{{ {28, 5, 12}, 0, {456, 296}, {108, 45, 49, 255} }},
	{{ {27, 8, -12}, 0, {530, 24}, {108, 45, 207, 255} }},
	{{ {28, 5, -12}, 0, {530, 296}, {108, 45, 207, 255} }},
	{{ {12, 1, -28}, 0, {604, 24}, {45, 19, 139, 255} }},
	{{ {13, -1, -28}, 0, {604, 296}, {45, 19, 139, 255} }},
	{{ {-10, -7, -28}, 0, {676, 24}, {211, 237, 139, 255} }},
	{{ {-8, -10, -28}, 0, {676, 296}, {211, 237, 139, 255} }},
	{{ {-25, -14, -12}, 0, {750, 24}, {148, 211, 207, 255} }},
	{{ {-24, -17, -12}, 0, {750, 296}, {148, 211, 207, 255} }},
	{{ {-25, -14, 12}, 0, {824, 24}, {148, 211, 49, 255} }},
	{{ {-24, -17, 12}, 0, {824, 296}, {148, 211, 49, 255} }},
	{{ {-10, -7, 28}, 0, {898, 24}, {173, 222, 90, 255} }},
	{{ {-8, -10, 28}, 0, {898, 296}, {173, 222, 90, 255} }},
};

Gfx gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_tri_0[] = {
	gsSPVertex(gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_vtx_0 + 0, 16, 0),
	gsSP2Triangles(0, 1, 2, 0, 0, 3, 1, 0),
	gsSP2Triangles(3, 4, 1, 0, 3, 5, 4, 0),
	gsSP2Triangles(5, 6, 4, 0, 5, 7, 6, 0),
	gsSP2Triangles(7, 8, 6, 0, 7, 9, 8, 0),
	gsSP2Triangles(9, 10, 8, 0, 9, 11, 10, 0),
	gsSP2Triangles(11, 12, 10, 0, 11, 13, 12, 0),
	gsSP2Triangles(13, 14, 12, 0, 13, 15, 14, 0),
	gsSPEndDisplayList(),
};

Gfx mat_gGiHeatGearGlowDL_Glow_layerTransparent[] = {
	gsDPPipeSync(),
	gsDPSetCombineLERP(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, COMBINED, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0),
	gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_LIGHTING | G_SHADING_SMOOTH),
	gsSPSetOtherMode(G_SETOTHERMODE_H, 4, 20, G_AD_NOISE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE),
	gsSPSetOtherMode(G_SETOTHERMODE_L, 0, 32, G_AC_NONE | G_ZS_PIXEL | G_RM_FOG_SHADE_A | G_RM_AA_ZB_XLU_SURF2),
	gsSPTexture(65535, 65535, 0, 0, 1),
	gsDPSetPrimColor(0, 0, 238, 64, 39, 128),
	gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b_LOAD_BLOCK, 1, gGiHeatGearGlowDL_eff_bubble_1_i8_i8),
	gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b_LOAD_BLOCK, 0, 0, 7, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0),
	gsDPLoadBlock(7, 0, 0, 127, 1024),
	gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b, 2, 0, 0, 0, G_TX_WRAP | G_TX_NOMIRROR, 4, 0, G_TX_WRAP | G_TX_NOMIRROR, 4, 0),
	gsDPSetTileSize(0, 0, 0, 60, 60),
	gsSPDisplayList(0x8000000),
	gsSPEndDisplayList(),
};

Gfx gGiHeatGearGlowDL[] = {
	gsSPClearGeometryMode(G_LIGHTING),
	gsSPVertex(gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_vtx_cull + 0, 8, 0),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPCullDisplayList(0, 7),
	gsSPDisplayList(mat_gGiHeatGearGlowDL_Glow_layerTransparent),
	gsSPDisplayList(gGiHeatGearGlowDL_gGiHeatGearGlowDL_mesh_layer_Transparent_tri_0),
	gsSPEndDisplayList(),
};

