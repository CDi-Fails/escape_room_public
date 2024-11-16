#include "ultra64.h"
#include "z64.h"
#include "macros.h"
#include "escape_room_scene.h"
#include "segment_symbols.h"
#include "command_macros_base.h"
#include "z64cutscene_commands.h"
#include "variables.h"

/**
 * Header Child Day (Default)
*/
#define LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_OBJECTLIST 1
#define LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_ACTORLIST 1
SceneCmd escape_room_room_12_header00[] = {
    SCENE_CMD_ECHO_SETTINGS(0x00),
    SCENE_CMD_ROOM_BEHAVIOR(0x00, 0x00, false, false),
    SCENE_CMD_SKYBOX_DISABLES(true, true),
    SCENE_CMD_TIME_SETTINGS(0xFF, 0xFF, 10),
    SCENE_CMD_ROOM_SHAPE(&escape_room_room_12_shapeHeader),
    SCENE_CMD_OBJECT_LIST(LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_OBJECTLIST, escape_room_room_12_header00_objectList),
    SCENE_CMD_ACTOR_LIST(LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_ACTORLIST, escape_room_room_12_header00_actorList),
    SCENE_CMD_END(),
};

s16 escape_room_room_12_header00_objectList[LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_OBJECTLIST] = {
    OBJECT_BOX,
};

ActorEntry escape_room_room_12_header00_actorList[LENGTH_ESCAPE_ROOM_ROOM_12_HEADER00_ACTORLIST] = {
    // Treasure Chest
    {
        /* Actor ID   */ ACTOR_EN_BOX,
        /* Position   */ { 3082, -200, -6093 },
        /* Rotation   */ { DEG_TO_BINANG(0.000), DEG_TO_BINANG(150.002), DEG_TO_BINANG(0.000) },
        /* Parameters */ 0x0643
    },
};

RoomShapeNormal escape_room_room_12_shapeHeader = {
    ROOM_SHAPE_TYPE_NORMAL,
    ARRAY_COUNT(escape_room_room_12_shapeDListEntry),
    escape_room_room_12_shapeDListEntry,
    escape_room_room_12_shapeDListEntry + ARRAY_COUNT(escape_room_room_12_shapeDListEntry)
};

RoomShapeDListsEntry escape_room_room_12_shapeDListEntry[1] = {
    { escape_room_room_12_entry_0_opaque, NULL },
};

Gfx escape_room_room_12_entry_0_opaque[] = {
	gsSPDisplayList(escape_room_dl_Room12Mesh_mesh_layer_Opaque),
	gsSPEndDisplayList(),
};

Vtx escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_cull[8] = {
	{{ {2771, -200, -5734}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {2771, 50, -5734}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {2771, 50, -6261}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {2771, -200, -6261}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {3283, -200, -5734}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {3283, 50, -5734}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {3283, 50, -6261}, 0, {0, 0}, {0, 0, 0, 0} }},
	{{ {3283, -200, -6261}, 0, {0, 0}, {0, 0, 0, 0} }},
};

Vtx escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_0[37] = {
	{{ {2953, -200, -5809}, 0, {18608, 1008}, {83, 65, 48, 255} }},
	{{ {2901, -200, -5839}, 0, {18608, 1287}, {82, 64, 47, 255} }},
	{{ {2891, -200, -5822}, 0, {18476, 1287}, {0, 0, 0, 255} }},
	{{ {2943, -200, -5792}, 0, {18476, 1008}, {0, 0, 0, 255} }},
	{{ {2953, -100, -5809}, 0, {18608, 543}, {72, 54, 47, 255} }},
	{{ {2943, -100, -5792}, 0, {18476, 543}, {0, 0, 0, 255} }},
	{{ {2901, -200, -5839}, 0, {18212, 1008}, {82, 64, 47, 255} }},
	{{ {2901, -100, -5839}, 0, {18212, 543}, {68, 50, 44, 255} }},
	{{ {2891, -100, -5822}, 0, {18344, 543}, {1, 1, 0, 255} }},
	{{ {2891, -200, -5822}, 0, {18344, 1008}, {0, 0, 0, 255} }},
	{{ {2891, -100, -5822}, 0, {18212, 636}, {1, 1, 0, 255} }},
	{{ {2901, -100, -5839}, 0, {18212, 543}, {68, 50, 44, 255} }},
	{{ {2953, -100, -5809}, 0, {18608, 543}, {72, 54, 47, 255} }},
	{{ {2943, -100, -5792}, 0, {18608, 636}, {0, 0, 0, 255} }},
	{{ {3083, -200, -5734}, 0, {7, 1008}, {0, 0, 0, 255} }},
	{{ {2953, -200, -5809}, 0, {1819, 1008}, {83, 65, 48, 255} }},
	{{ {2953, -100, -5809}, 0, {1819, 189}, {255, 199, 151, 255} }},
	{{ {3083, 50, -5734}, 0, {7, -1040}, {0, 0, 0, 255} }},
	{{ {2901, -100, -5839}, 0, {2433, 189}, {255, 199, 151, 255} }},
	{{ {2771, 50, -5914}, 0, {4148, -1040}, {0, 0, 0, 255} }},
	{{ {2771, -200, -5914}, 0, {4148, 1008}, {0, 0, 0, 255} }},
	{{ {2901, -200, -5839}, 0, {2433, 1008}, {82, 64, 47, 255} }},
	{{ {2771, 50, -5914}, 0, {-10, -1042}, {0, 0, 0, 255} }},
	{{ {2771, -200, -5914}, 0, {-10, 1010}, {0, 0, 0, 255} }},
	{{ {2871, -75, -6088}, 0, {2038, -16}, {255, 202, 153, 255} }},
	{{ {2971, -200, -6261}, 0, {4085, 1010}, {2, 2, 1, 255} }},
	{{ {2971, 50, -6261}, 0, {4085, -1042}, {0, 0, 0, 255} }},
	{{ {3283, -200, -6081}, 0, {4080, 1008}, {1, 0, 0, 255} }},
	{{ {3283, 50, -6081}, 0, {4080, -1040}, {0, 0, 0, 255} }},
	{{ {3127, -75, -6171}, 0, {2032, -16}, {255, 194, 145, 255} }},
	{{ {2971, 50, -6261}, 0, {-16, -1040}, {0, 0, 0, 255} }},
	{{ {2971, -200, -6261}, 0, {-16, 1008}, {2, 2, 1, 255} }},
	{{ {3283, -200, -6081}, 0, {-16, 1008}, {1, 0, 0, 255} }},
	{{ {3083, -200, -5734}, 0, {4080, 1008}, {0, 0, 0, 255} }},
	{{ {3183, -75, -5908}, 0, {2032, -16}, {255, 194, 145, 255} }},
	{{ {3083, 50, -5734}, 0, {4080, -1040}, {0, 0, 0, 255} }},
	{{ {3283, 50, -6081}, 0, {-16, -1040}, {0, 0, 0, 255} }},
};

Gfx escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_0[] = {
	gsSPVertex(escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_0 + 0, 32, 0),
	gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
	gsSP2Triangles(3, 4, 0, 0, 3, 5, 4, 0),
	gsSP2Triangles(6, 7, 8, 0, 6, 8, 9, 0),
	gsSP2Triangles(10, 11, 12, 0, 10, 12, 13, 0),
	gsSP2Triangles(14, 15, 16, 0, 17, 14, 16, 0),
	gsSP2Triangles(17, 16, 18, 0, 19, 17, 18, 0),
	gsSP2Triangles(20, 19, 18, 0, 18, 21, 20, 0),
	gsSP2Triangles(22, 23, 24, 0, 23, 25, 24, 0),
	gsSP2Triangles(25, 26, 24, 0, 26, 22, 24, 0),
	gsSP2Triangles(27, 28, 29, 0, 28, 30, 29, 0),
	gsSP2Triangles(30, 31, 29, 0, 31, 27, 29, 0),
	gsSPVertex(escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_0 + 32, 5, 0),
	gsSP2Triangles(0, 1, 2, 0, 1, 3, 2, 0),
	gsSP2Triangles(3, 4, 2, 0, 4, 0, 2, 0),
	gsSPEndDisplayList(),
};

Vtx escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_1[5] = {
	{{ {3083, 50, -5734}, 0, {-1859, 4079}, {0, 0, 0, 255} }},
	{{ {2771, 50, -5914}, 0, {3670, 4079}, {0, 0, 0, 255} }},
	{{ {3027, 50, -5998}, 0, {906, 1008}, {255, 204, 156, 255} }},
	{{ {2971, 50, -6261}, 0, {3670, -2063}, {0, 0, 0, 255} }},
	{{ {3283, 50, -6081}, 0, {-1859, -2063}, {0, 0, 0, 255} }},
};

Gfx escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_1[] = {
	gsSPVertex(escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_1 + 0, 5, 0),
	gsSP2Triangles(0, 1, 2, 0, 1, 3, 2, 0),
	gsSP2Triangles(3, 4, 2, 0, 4, 0, 2, 0),
	gsSPEndDisplayList(),
};

Vtx escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_2[7] = {
	{{ {2971, -200, -6261}, 0, {4069, 4534}, {2, 2, 1, 255} }},
	{{ {2771, -200, -5914}, 0, {4069, -1608}, {0, 0, 0, 255} }},
	{{ {3027, -200, -5998}, 0, {1013, 1463}, {181, 138, 103, 255} }},
	{{ {2901, -200, -5839}, 0, {1522, -1608}, {82, 64, 47, 255} }},
	{{ {2953, -200, -5809}, 0, {504, -1608}, {83, 65, 48, 255} }},
	{{ {3083, -200, -5734}, 0, {-2043, -1608}, {0, 0, 0, 255} }},
	{{ {3283, -200, -6081}, 0, {-2043, 4534}, {1, 0, 0, 255} }},
};

Gfx escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_2[] = {
	gsSPVertex(escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_2 + 0, 7, 0),
	gsSP2Triangles(0, 1, 2, 0, 1, 3, 2, 0),
	gsSP2Triangles(3, 4, 2, 0, 4, 5, 2, 0),
	gsSP2Triangles(5, 6, 2, 0, 6, 0, 2, 0),
	gsSPEndDisplayList(),
};

Gfx escape_room_dl_Room12Mesh_mesh_layer_Opaque[] = {
	gsSPClearGeometryMode(G_LIGHTING),
	gsSPVertex(escape_room_dl_Room12Mesh_mesh_layer_Opaque_vtx_cull + 0, 8, 0),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPCullDisplayList(0, 7),
	gsSPDisplayList(mat_escape_room_dl_f3dlite_material_017_layerOpaque),
	gsSPDisplayList(escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_0),
	gsSPDisplayList(mat_escape_room_dl_f3dlite_material_041_layerOpaque),
	gsSPDisplayList(escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_1),
	gsSPDisplayList(mat_escape_room_dl_f3dlite_material_119_layerOpaque),
	gsSPDisplayList(escape_room_dl_Room12Mesh_mesh_layer_Opaque_tri_2),
	gsSPEndDisplayList(),
};

