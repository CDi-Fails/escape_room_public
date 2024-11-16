#include "ultra64.h"
#include "z64.h"
#include "macros.h"
#include "victory_scene.h"
#include "segment_symbols.h"
#include "command_macros_base.h"
#include "z64cutscene_commands.h"
#include "variables.h"

/**
 * Header Child Day (Default)
*/
SceneCmd victory_room_0_header00[] = {
    SCENE_CMD_ECHO_SETTINGS(0x00),
    SCENE_CMD_ROOM_BEHAVIOR(0x00, 0x00, false, false),
    SCENE_CMD_SKYBOX_DISABLES(false, true),
    SCENE_CMD_TIME_SETTINGS(0xFF, 0xFF, 10),
    SCENE_CMD_ROOM_SHAPE(&victory_room_0_shapeHeader),
    SCENE_CMD_END(),
};

RoomShapeImageSingle victory_room_0_shapeHeader = {
    { ROOM_SHAPE_TYPE_IMAGE, ROOM_SHAPE_IMAGE_AMOUNT_SINGLE, &victory_room_0_shapeDListEntry },
    victory_bg_victory_background_jpg,
    0x00000000,
    NULL,
    320, 240,
    G_IM_FMT_RGBA, G_IM_SIZ_16b,
    0x0000, 0x0000
};

RoomShapeDListsEntry victory_room_0_shapeDListEntry = { victory_room_0_entry_0_opaque, NULL };

Gfx victory_room_0_entry_0_opaque[] = {
	gsSPDisplayList(victory_dl_Floor_mesh_layer_Opaque),
	gsSPEndDisplayList(),
};

Vtx victory_dl_Floor_mesh_layer_Opaque_vtx_cull[8] = {
	{{ {-300, -120, 300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-300, -120, 300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-300, -120, -300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-300, -120, -300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {300, -120, 300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {300, -120, 300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {300, -120, -300}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {300, -120, -300}, 0, {0, 0}, {0, 0, 0, 0} }},
};

Vtx victory_dl_Floor_mesh_layer_Opaque_vtx_0[4] = {
	{{ {-300, -120, 300}, 0, {-16, 1008}, {0, 127, 0, 255} }},
	{{ {300, -120, 300}, 0, {1008, 1008}, {0, 127, 0, 255} }},
	{{ {300, -120, -300}, 0, {1008, -16}, {0, 127, 0, 255} }},
	{{ {-300, -120, -300}, 0, {-16, -16}, {0, 127, 0, 255} }},
};

Gfx victory_dl_Floor_mesh_layer_Opaque_tri_0[] = {
	gsSPVertex(victory_dl_Floor_mesh_layer_Opaque_vtx_0 + 0, 4, 0),
	gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
	gsSPEndDisplayList(),
};

Gfx mat_victory_dl_floor_mat_layerOpaque[] = {
	gsDPPipeSync(),
	gsDPSetCombineLERP(0, 0, 0, SHADE, 0, 0, 0, 1, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED),
	gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_LIGHTING | G_SHADING_SMOOTH),
	gsSPSetOtherMode(G_SETOTHERMODE_H, 4, 20, G_AD_NOISE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE),
	gsSPSetOtherMode(G_SETOTHERMODE_L, 0, 32, G_AC_NONE | G_ZS_PIXEL | G_RM_FOG_SHADE_A | G_RM_AA_ZB_OPA_SURF2),
	gsSPTexture(65535, 65535, 0, 0, 1),
	gsDPSetPrimColor(0, 0, 0, 0, 0, 255),
	gsSPEndDisplayList(),
};

Gfx victory_dl_Floor_mesh_layer_Opaque[] = {
	gsSPClearGeometryMode(G_LIGHTING),
	gsSPVertex(victory_dl_Floor_mesh_layer_Opaque_vtx_cull + 0, 8, 0),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPCullDisplayList(0, 7),
	gsSPDisplayList(mat_victory_dl_floor_mat_layerOpaque),
	gsSPDisplayList(victory_dl_Floor_mesh_layer_Opaque_tri_0),
	gsSPEndDisplayList(),
};

u64 victory_bg_victory_background_jpg[SCREEN_WIDTH * SCREEN_HEIGHT / 4] = {
#include "assets_custom/scenes/victory/victory_bg_victory_background_jpg.jpg.inc.c"
};

