#include "ultra64.h"
#include "z64.h"
#include "macros.h"
#include "title_scene.h"
#include "segment_symbols.h"
#include "command_macros_base.h"
#include "z64cutscene_commands.h"
#include "variables.h"

/**
 * Header Child Day (Default)
*/
SceneCmd title_scene_header00[] = {
    SCENE_CMD_SOUND_SETTINGS(0x00, 0x13, 0x00),
    SCENE_CMD_ROOM_LIST(1, title_scene_roomList),
    SCENE_CMD_MISC_SETTINGS(0x00, 0x00),
    SCENE_CMD_COL_HEADER(&title_collisionHeader),
    SCENE_CMD_SPECIAL_FILES(0x00, OBJECT_GAMEPLAY_DANGEON_KEEP),
    SCENE_CMD_SKYBOX_SETTINGS(0x00, 0x01, LIGHT_MODE_SETTINGS),
    SCENE_CMD_ENTRANCE_LIST(title_scene_header00_entranceList),
    SCENE_CMD_SPAWN_LIST(1, title_scene_header00_playerEntryList),
    SCENE_CMD_ENV_LIGHT_SETTINGS(1, title_scene_header00_lightSettings),
    SCENE_CMD_CUTSCENE_DATA(Title),
    SCENE_CMD_END(),
};

RomFile title_scene_roomList[] = {
    { (u32)_title_room_0SegmentRomStart, (u32)_title_room_0SegmentRomEnd },
};

ActorEntry title_scene_header00_playerEntryList[] = {
    {
        /* Actor ID   */ ACTOR_PLAYER,
        /* Position   */ { 0, -120, 0 },
        /* Rotation   */ { DEG_TO_BINANG(0.000), DEG_TO_BINANG(0.000), DEG_TO_BINANG(0.000) },
        /* Parameters */ 0
    },
};

Spawn title_scene_header00_entranceList[] = {
    // { Spawn Actor List Index, Room Index }
    { 0, 0 },
};

EnvLightSettings title_scene_header00_lightSettings[1] = {
    // Indoor No. 1 Lighting
    {
        {     0,     0,     0 },   // Ambient Color
        {    73,    73,    73 },   // Diffuse0 Direction
        {     0,     0,     0 },   // Diffuse0 Color
        {   -73,   -73,   -73 },   // Diffuse1 Direction
        {     0,     0,     0 },   // Diffuse1 Color
        {     0,     0,     0 },   // Fog Color
        ((1 << 10) | 993),         // Blend Rate & Fog Near
        12800,                     // Fog Far
    },
};

SurfaceType title_polygonTypes[] = {
	{ 0x00000000, 0x00000000 },
};

CollisionPoly title_polygons[] = {
	{ 0x0000, 0x0000, 0x0001, 0x0002, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(7.549790126404332e-08), 0x0078 },
	{ 0x0000, 0x0000, 0x0002, 0x0003, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(7.549790126404332e-08), 0x0078 },
};

Vec3s title_vertices[4] = {
	{ -300, -120, 300 },
	{ 300, -120, 300 },
	{ 300, -120, -300 },
	{ -300, -120, -300 },
};

CollisionHeader title_collisionHeader = {
	-300,
	-120,
	-300,
	300,
	-120,
	300,
	4,
	title_vertices,
	2,
	title_polygons,
	title_polygonTypes,
	0,
	0,
	0
};

CutsceneData Title[] = {
    CS_BEGIN_CUTSCENE(2, 65535),
	CS_DESTINATION(20, 65534, 65535),
	CS_MISC_LIST(2),
		CS_MISC(30, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
		CS_MISC(31, 65534, 65535, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	CS_END(),
};

