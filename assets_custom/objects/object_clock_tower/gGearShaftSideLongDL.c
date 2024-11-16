#include "ultra64.h"
#include "global.h"

u64 gGearShaftSideLongDL__00010041_ci4[] = {
	0x0121103456657789, 0x0121a84755557999, 0x8912ba9845c65d74, 0x90221987d655d749, 0x88a2b20a945665d7, 0x48a0aa944d655578, 0xd88a2b210846c65d, 0xd74890a094d555d4, 
	0xd578a2222a845665, 0xdd748a2b19876555, 0x55578a1110a94566, 0x5dd78a221a98d656, 0x6565d49010aa9456, 0x65dd7499a1084d56, 0x66665d78aa0a9876, 0x66657748900984d6, 
	0x56c665d749a99987, 0x6665d77489aaa80d, 0x856666dd48aa00a4, 0x76665d77789a1a09, 0x985c6c55d49a12a9, 0x876665ddd48802a0, 0x0985cc65dd780100, 0xa4d6665d7744a323, 
	0x30a85c66655789a0, 0xa87d5665d7489b33, 0x320a85c665574490, 0x0a4476665d748023, 0x3e20a85c6655d74a, 0x22a94d6655d748ab, 0xe3b1a985cc665d78, 0x1bb09476655d7892, 
	0x1bbb19885ccc65d4, 0x9132a9845665748a, 0x8a2b1a8985cc665d, 0x492bb2a976565748, 0x74a02209886fcc65, 0x74803e20875665d7, 0x78a01b1a9985fcc6, 0x5d7413eba875665d, 
	0xd7890220aa985cc6, 0x5dd7803eb0875c65, 0x65dd49000aa985c6, 0x5dd74803e2944665, 0x6655d78a00aaa85c, 0x6dd579a1b21a9466, 0xc6655d7901110985, 0xc65d549aa1219876, 
	0x5cc665d4a2210998, 0x5c655d789022a884, 0x95fc665d4a121099, 0x95c555578a122988, 0x095fc66d74a12100, 0x1856555d480bb098, 0x0a85fc65d74a2bb3, 0x2945655d74802100, 
	0x1a995fc55d7492ee, 0x2a94565dd77491bb, 0xb0a095fc65d78923, 0xb19945655dd78abe, 0x3b11095cc5d774a2, 0x3b0894565dd7480b, 0x13321095cc5d7780, 0xbba99945665d74aa, 
	
};

u64 gGearShaftSideLongDL__00010041_pal_rgba16[] = {
	0x6b1573577b998c1f, 0x41cf290718c7398d, 0x52515a9362d583db, 0x1085314b94610843, 
};

Vtx gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_vtx_cull[8] = {
	{{ {-191, -191, -4688}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-191, -191, 4459}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-191, 191, 4459}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-191, 191, -4688}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {191, -191, -4688}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {191, -191, 4459}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {191, 191, 4459}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {191, 191, -4688}, 0, {0, 0}, {0, 0, 0, 0} }},
};

Vtx gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_vtx_0[10] = {
	{{ {-191, 0, 4459}, 0, {-7760, 1523}, {251, 247, 201, 255} }},
	{{ {0, -191, -4688}, 0, {8558, 2543}, {137, 134, 110, 255} }},
	{{ {0, -191, 4459}, 0, {-7760, 2543}, {138, 134, 111, 255} }},
	{{ {-191, 0, -4688}, 0, {8558, 1523}, {251, 247, 201, 255} }},
	{{ {0, 191, 4459}, 0, {-7760, 503}, {230, 231, 189, 255} }},
	{{ {0, 191, -4688}, 0, {8558, 503}, {230, 231, 189, 255} }},
	{{ {191, 0, 4459}, 0, {-7760, -517}, {81, 93, 81, 255} }},
	{{ {191, 0, -4688}, 0, {8558, -517}, {80, 92, 80, 255} }},
	{{ {0, -191, 4459}, 0, {-7760, -1537}, {138, 134, 111, 255} }},
	{{ {0, -191, -4688}, 0, {8558, -1537}, {137, 134, 110, 255} }},
};

Gfx gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_tri_0[] = {
	gsSPVertex(gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_vtx_0 + 0, 10, 0),
	gsSP2Triangles(0, 1, 2, 0, 0, 3, 1, 0),
	gsSP2Triangles(4, 3, 0, 0, 4, 5, 3, 0),
	gsSP2Triangles(6, 5, 4, 0, 6, 7, 5, 0),
	gsSP2Triangles(8, 7, 6, 0, 8, 9, 7, 0),
	gsSPEndDisplayList(),
};

Gfx mat_gGearShaftSideLongDL_f3dlite_material_073_layerOpaque[] = {
	gsDPPipeSync(),
	gsDPSetCombineLERP(TEXEL0, 0, SHADE, 0, 0, 0, 0, 1, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED),
	gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_SHADING_SMOOTH),
	gsSPSetOtherMode(G_SETOTHERMODE_H, 4, 20, G_AD_NOISE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_RGBA16 | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE),
	gsSPSetOtherMode(G_SETOTHERMODE_L, 0, 32, G_AC_NONE | G_ZS_PIXEL | G_RM_FOG_SHADE_A | G_RM_AA_ZB_OPA_SURF2),
	gsSPTexture(65535, 65535, 0, 0, 1),
	gsDPSetPrimColor(0, 0, 225, 225, 225, 255),
	gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, gGearShaftSideLongDL__00010041_pal_rgba16),
	gsDPSetTile(0, 0, 0, 256, 5, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0),
	gsDPLoadTLUTCmd(5, 15),
	gsDPSetTextureImage(G_IM_FMT_CI, G_IM_SIZ_16b, 1, gGearShaftSideLongDL__00010041_ci4),
	gsDPSetTile(G_IM_FMT_CI, G_IM_SIZ_16b, 0, 0, 7, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0, G_TX_WRAP | G_TX_NOMIRROR, 0, 0),
	gsDPLoadBlock(7, 0, 0, 255, 1024),
	gsDPSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 2, 0, 0, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, 0),
	gsDPSetTileSize(0, 0, 0, 124, 124),
	gsSPEndDisplayList(),
};

Gfx gGearShaftSideLongDL[] = {
	gsSPClearGeometryMode(G_LIGHTING),
	gsSPVertex(gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_vtx_cull + 0, 8, 0),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPCullDisplayList(0, 7),
	gsSPDisplayList(mat_gGearShaftSideLongDL_f3dlite_material_073_layerOpaque),
	gsSPDisplayList(gGearShaftSideLongDL_gGearShaftSideLongDL_mesh_layer_Opaque_tri_0),
	gsSPEndDisplayList(),
};

