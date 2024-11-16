#include "ultra64.h"
#include "z64.h"
#include "macros.h"

SurfaceType gGearSlotCol_polygonTypes[] = {
	{ 0x00000000, 0x00000002 },
};

CollisionPoly gGearSlotCol_polygons[] = {
	{ 0x0000, 0x0000, 0x0001, 0x0002, COLPOLY_SNORMAL(-1.1059423233976406e-13), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-3.8146797720628456e-08), 0xfd12 },
	{ 0x0000, 0x0000, 0x0002, 0x0003, COLPOLY_SNORMAL(-9.604285682723868e-14), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-3.81467906152011e-08), 0xfd12 },
};

Vec3s gGearSlotCol_vertices[4] = {
	{ 3125, 750, -3125 },
	{ -3125, 750, -3125 },
	{ -3125, 750, 3125 },
	{ 3125, 750, 3125 },
};

CollisionHeader gGearSlotCol_collisionHeader = {
	-3125,
	750,
	-3125,
	3125,
	750,
	3125,
	4,
	gGearSlotCol_vertices,
	2,
	gGearSlotCol_polygons,
	gGearSlotCol_polygonTypes,
	0,
	0,
	0
};

