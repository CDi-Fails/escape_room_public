#include "ultra64.h"
#include "z64.h"
#include "macros.h"
#include "test_map_scene.h"
#include "segment_symbols.h"
#include "command_macros_base.h"
#include "z64cutscene_commands.h"
#include "variables.h"

/**
 * Header Child Day (Default)
*/
#define LENGTH_TEST_MAP_ROOM_1_HEADER00_OBJECTLIST 2
SceneCmd test_map_room_1_header00[] = {
    SCENE_CMD_ECHO_SETTINGS(0x00),
    SCENE_CMD_ROOM_BEHAVIOR(0x00, 0x00, false, false),
    SCENE_CMD_SKYBOX_DISABLES(false, false),
    SCENE_CMD_TIME_SETTINGS(0xFF, 0xFF, 10),
    SCENE_CMD_ROOM_SHAPE(&test_map_room_1_shapeHeader),
    SCENE_CMD_OBJECT_LIST(LENGTH_TEST_MAP_ROOM_1_HEADER00_OBJECTLIST, test_map_room_1_header00_objectList),
    SCENE_CMD_END(),
};

s16 test_map_room_1_header00_objectList[LENGTH_TEST_MAP_ROOM_1_HEADER00_OBJECTLIST] = {
    OBJECT_SEED,
    OBJECT_GIANT_PLANT,
};

RoomShapeNormal test_map_room_1_shapeHeader = {
    ROOM_SHAPE_TYPE_NORMAL,
    ARRAY_COUNT(test_map_room_1_shapeDListEntry),
    test_map_room_1_shapeDListEntry,
    test_map_room_1_shapeDListEntry + ARRAY_COUNT(test_map_room_1_shapeDListEntry)
};

RoomShapeDListsEntry test_map_room_1_shapeDListEntry[1] = {
    { test_map_room_1_entry_0_opaque, NULL },
};

Gfx test_map_room_1_entry_0_opaque[] = {
	gsSPDisplayList(test_map_dl_Plane_002_mesh_layer_Opaque),
	gsSPEndDisplayList(),
};

Vtx test_map_dl_Plane_002_mesh_layer_Opaque_vtx_cull[8] = {
	{{ {-1040, -120, 475}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-1040, 280, 475}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-1040, 280, 25}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-1040, -120, 25}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-520, -120, 475}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-520, 280, 475}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-520, 280, 25}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {-520, -120, 25}, 0, {0, 0}, {0, 0, 0, 0} }},
};

Vtx test_map_dl_Plane_002_mesh_layer_Opaque_vtx_0[57] = {
	{{ {-520, -20, 280}, 0, {15344, 23331}, {0, 0, 129, 255} }},
	{{ {-520, -120, 280}, 0, {15344, 23331}, {0, 0, 129, 255} }},
	{{ {-540, -120, 280}, 0, {15344, 23331}, {0, 0, 129, 255} }},
	{{ {-540, -20, 280}, 0, {15344, 23331}, {0, 0, 129, 255} }},
	{{ {-520, -20, 220}, 0, {15344, 22717}, {0, 129, 0, 255} }},
	{{ {-520, -20, 280}, 0, {15344, 23331}, {0, 129, 0, 255} }},
	{{ {-540, -20, 280}, 0, {15344, 23331}, {0, 129, 0, 255} }},
	{{ {-540, -20, 220}, 0, {15344, 22717}, {0, 129, 0, 255} }},
	{{ {-520, -120, 220}, 0, {15344, 22717}, {0, 0, 127, 255} }},
	{{ {-520, -20, 220}, 0, {15344, 22717}, {0, 0, 127, 255} }},
	{{ {-540, -20, 220}, 0, {15344, 22717}, {0, 0, 127, 255} }},
	{{ {-540, -120, 220}, 0, {15344, 22717}, {0, 0, 127, 255} }},
	{{ {-520, -120, 220}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-540, -120, 280}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-520, -120, 280}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-540, -120, 220}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 223}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 277}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-540, -20, 280}, 0, {15344, 23331}, {129, 0, 0, 255} }},
	{{ {-540, 280, 475}, 0, {15344, 25328}, {129, 0, 0, 255} }},
	{{ {-540, 280, 25}, 0, {15344, 20720}, {129, 0, 0, 255} }},
	{{ {-540, -20, 220}, 0, {15344, 22717}, {129, 0, 0, 255} }},
	{{ {-540, -20, 220}, 0, {15344, 22717}, {129, 0, 0, 255} }},
	{{ {-540, -120, 25}, 0, {15344, 20720}, {129, 0, 0, 255} }},
	{{ {-540, -120, 220}, 0, {15344, 22717}, {129, 0, 0, 255} }},
	{{ {-540, -120, 280}, 0, {15344, 23331}, {129, 0, 0, 255} }},
	{{ {-540, -120, 475}, 0, {15344, 25328}, {129, 0, 0, 255} }},
	{{ {-540, 280, 475}, 0, {15344, 25328}, {129, 0, 0, 255} }},
	{{ {-540, -20, 280}, 0, {15344, 23331}, {129, 0, 0, 255} }},
	{{ {-540, -120, 475}, 0, {15344, 25328}, {0, 127, 0, 255} }},
	{{ {-540, -120, 280}, 0, {15344, 23331}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 277}, 0, {15344, 23300}, {0, 127, 0, 255} }},
	{{ {-540, -120, 475}, 0, {15344, 25328}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 277}, 0, {15344, 23300}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 475}, 0, {15344, 25328}, {0, 127, 0, 255} }},
	{{ {-540, 280, 475}, 0, {15344, 25328}, {0, 0, 129, 255} }},
	{{ {-540, -120, 475}, 0, {15344, 25328}, {0, 0, 129, 255} }},
	{{ {-1040, -120, 475}, 0, {15344, 25328}, {0, 0, 129, 255} }},
	{{ {-1040, 280, 475}, 0, {15344, 25328}, {0, 0, 129, 255} }},
	{{ {-540, -120, 220}, 0, {15344, 22717}, {0, 127, 0, 255} }},
	{{ {-540, -120, 25}, 0, {15344, 20720}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 25}, 0, {15344, 20720}, {0, 127, 0, 255} }},
	{{ {-1040, -120, 223}, 0, {15344, 22748}, {0, 127, 0, 255} }},
	{{ {-540, 280, 25}, 0, {15344, 20720}, {0, 129, 0, 255} }},
	{{ {-540, 280, 475}, 0, {15344, 25328}, {0, 129, 0, 255} }},
	{{ {-1040, 280, 475}, 0, {15344, 25328}, {0, 129, 0, 255} }},
	{{ {-1040, 280, 25}, 0, {15344, 20720}, {0, 129, 0, 255} }},
	{{ {-540, -120, 25}, 0, {15344, 20720}, {0, 0, 127, 255} }},
	{{ {-540, 280, 25}, 0, {15344, 20720}, {0, 0, 127, 255} }},
	{{ {-1040, 280, 25}, 0, {15344, 20720}, {0, 0, 127, 255} }},
	{{ {-1040, -120, 25}, 0, {15344, 20720}, {0, 0, 127, 255} }},
	{{ {-1040, 280, 475}, 0, {15344, 25328}, {127, 0, 0, 255} }},
	{{ {-1040, -120, 475}, 0, {15344, 25328}, {127, 0, 0, 255} }},
	{{ {-1040, -120, 277}, 0, {15344, 22717}, {127, 0, 0, 255} }},
	{{ {-1040, -120, 223}, 0, {15344, 22748}, {127, 0, 0, 255} }},
	{{ {-1040, 280, 25}, 0, {15344, 20720}, {127, 0, 0, 255} }},
	{{ {-1040, -120, 25}, 0, {15344, 20720}, {127, 0, 0, 255} }},
};

Gfx test_map_dl_Plane_002_mesh_layer_Opaque_tri_0[] = {
	gsSPVertex(test_map_dl_Plane_002_mesh_layer_Opaque_vtx_0 + 0, 32, 0),
	gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
	gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
	gsSP2Triangles(8, 9, 10, 0, 8, 10, 11, 0),
	gsSP2Triangles(12, 13, 14, 0, 12, 15, 13, 0),
	gsSP2Triangles(13, 15, 16, 0, 13, 16, 17, 0),
	gsSP2Triangles(18, 19, 20, 0, 18, 20, 21, 0),
	gsSP2Triangles(22, 20, 23, 0, 22, 23, 24, 0),
	gsSP2Triangles(25, 26, 27, 0, 25, 27, 28, 0),
	gsSP1Triangle(29, 30, 31, 0),
	gsSPVertex(test_map_dl_Plane_002_mesh_layer_Opaque_vtx_0 + 32, 25, 0),
	gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0),
	gsSP2Triangles(3, 5, 6, 0, 7, 8, 9, 0),
	gsSP2Triangles(7, 9, 10, 0, 11, 12, 13, 0),
	gsSP2Triangles(11, 13, 14, 0, 15, 16, 17, 0),
	gsSP2Triangles(15, 17, 18, 0, 19, 20, 21, 0),
	gsSP2Triangles(19, 21, 22, 0, 22, 23, 19, 0),
	gsSP1Triangle(22, 24, 23, 0),
	gsSPEndDisplayList(),
};

Gfx test_map_dl_Plane_002_mesh_layer_Opaque[] = {
	gsSPClearGeometryMode(G_LIGHTING),
	gsSPVertex(test_map_dl_Plane_002_mesh_layer_Opaque_vtx_cull + 0, 8, 0),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPCullDisplayList(0, 7),
	gsSPDisplayList(mat_test_map_dl_Floor_layerOpaque),
	gsSPDisplayList(test_map_dl_Plane_002_mesh_layer_Opaque_tri_0),
	gsSPEndDisplayList(),
};

