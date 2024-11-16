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
SceneCmd victory_scene_header00[] = {
    SCENE_CMD_SOUND_SETTINGS(0x00, 0x13, 0x52),
    SCENE_CMD_ROOM_LIST(1, victory_scene_roomList),
    SCENE_CMD_MISC_SETTINGS(0x30, 0x00),
    SCENE_CMD_COL_HEADER(&victory_collisionHeader),
    SCENE_CMD_SPECIAL_FILES(0x00, OBJECT_INVALID),
    SCENE_CMD_SKYBOX_SETTINGS(0x00, 0x00, LIGHT_MODE_SETTINGS),
    SCENE_CMD_ENTRANCE_LIST(victory_scene_header00_entranceList),
    SCENE_CMD_SPAWN_LIST(1, victory_scene_header00_playerEntryList),
    SCENE_CMD_ENV_LIGHT_SETTINGS(1, victory_scene_header00_lightSettings),
    SCENE_CMD_END(),
};

RomFile victory_scene_roomList[] = {
    { (u32)_victory_room_0SegmentRomStart, (u32)_victory_room_0SegmentRomEnd },
};

ActorEntry victory_scene_header00_playerEntryList[] = {
    {
        /* Actor ID   */ ACTOR_PLAYER,
        /* Position   */ { 0, -120, 0 },
        /* Rotation   */ { DEG_TO_BINANG(0.000), DEG_TO_BINANG(0.000), DEG_TO_BINANG(0.000) },
        /* Parameters */ 0
    },
};

Spawn victory_scene_header00_entranceList[] = {
    // { Spawn Actor List Index, Room Index }
    { 0, 0 },
};

EnvLightSettings victory_scene_header00_lightSettings[1] = {
    // Indoor No. 1 Lighting
    {
        {   255,   255,   255 },   // Ambient Color
        {    73,    73,    73 },   // Diffuse0 Direction
        {   255,   255,   255 },   // Diffuse0 Color
        {   -73,   -73,   -73 },   // Diffuse1 Direction
        {   255,   255,   255 },   // Diffuse1 Color
        {   255,   255,   255 },   // Fog Color
        ((1 << 10) | 993),         // Blend Rate & Fog Near
        12800,                     // Fog Far
    },
};

Vec3s victory_camPosData[3] = {
	{ 0, -45, 0 },
	{ 0, 32768, 0 },
	{ 6000, -1, -1 },
};

BgCamInfo victory_camData[1] = {
	{ CAM_SET_PREREND_FIXED, 3, &victory_camPosData[0] },
};

SurfaceType victory_polygonTypes[] = {
	{ 0x00000000, 0x00000000 },
};

CollisionPoly victory_polygons[] = {
	{ 0x0000, 0x0000, 0x0001, 0x0002, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(7.549790126404332e-08), 0x0078 },
	{ 0x0000, 0x0000, 0x0002, 0x0003, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(7.549790126404332e-08), 0x0078 },
};

Vec3s victory_vertices[4] = {
	{ -300, -120, 300 },
	{ 300, -120, 300 },
	{ 300, -120, -300 },
	{ -300, -120, -300 },
};

CollisionHeader victory_collisionHeader = {
	-300,
	-120,
	-300,
	300,
	-120,
	300,
	4,
	victory_vertices,
	2,
	victory_polygons,
	victory_polygonTypes,
	victory_camData,
	0,
	0
};

