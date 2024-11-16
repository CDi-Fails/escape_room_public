#include "ultra64.h"
#include "z64.h"
#include "macros.h"

SurfaceType gAirVentCrackedDL_polygonTypes[] = {
	{ 0x00000000, 0x00000002 },
};

CollisionPoly gAirVentCrackedDL_polygons[] = {
	{ 0x0000, 0x0001, 0x0002, 0x0000, COLPOLY_SNORMAL(-1.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), 0x0000 },
	{ 0x0000, 0x0003, 0x0001, 0x0000, COLPOLY_SNORMAL(-1.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), 0x0000 },
};

Vec3s gAirVentCrackedDL_vertices[4] = {
	{ 0, 6500, -6500 },
	{ 0, -6500, 6500 },
	{ 0, 6500, 6500 },
	{ 0, -6500, -6500 },
};

CollisionHeader gAirVentCrackedDL_collisionHeader = {
	0,
	-6500,
	-6500,
	0,
	6500,
	6500,
	4,
	gAirVentCrackedDL_vertices,
	2,
	gAirVentCrackedDL_polygons,
	gAirVentCrackedDL_polygonTypes,
	0,
	0,
	0
};

