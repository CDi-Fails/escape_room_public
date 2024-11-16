#include "ultra64.h"
#include "z64.h"
#include "macros.h"

SurfaceType gPlantPotCol_polygonTypes[] = {
	{ 0x00000000, 0x00000002 },
	{ 0x00000000, 0x00000000 },
};

CollisionPoly gPlantPotCol_polygons[] = {
	{ 0x0000, 0x0000, 0x0001, 0x0002, COLPOLY_SNORMAL(-0.786209762096405), COLPOLY_SNORMAL(-0.23585966229438782), COLPOLY_SNORMAL(-0.571178138256073), 0xf6c8 },
	{ 0x0000, 0x0000, 0x0002, 0x0003, COLPOLY_SNORMAL(-0.7862098813056946), COLPOLY_SNORMAL(-0.2358604073524475), COLPOLY_SNORMAL(-0.57117760181427), 0xf6c8 },
	{ 0x0000, 0x0004, 0x0005, 0x0006, COLPOLY_SNORMAL(0.9717878699302673), COLPOLY_SNORMAL(-0.23585660755634308), COLPOLY_SNORMAL(-4.194340363028459e-06), 0xf6fc },
	{ 0x0000, 0x0004, 0x0006, 0x0007, COLPOLY_SNORMAL(0.9717878699302673), COLPOLY_SNORMAL(-0.23585659265518188), COLPOLY_SNORMAL(-4.204649485473055e-06), 0xf6fc },
	{ 0x0000, 0x0008, 0x0004, 0x0007, COLPOLY_SNORMAL(0.300345778465271), COLPOLY_SNORMAL(-0.23586048185825348), COLPOLY_SNORMAL(0.92420893907547), 0xf699 },
	{ 0x0000, 0x0008, 0x0007, 0x0009, COLPOLY_SNORMAL(0.30034565925598145), COLPOLY_SNORMAL(-0.23586022853851318), COLPOLY_SNORMAL(0.9242091178894043), 0xf699 },
	{ 0x0000, 0x0005, 0x0000, 0x0003, COLPOLY_SNORMAL(0.3003070056438446), COLPOLY_SNORMAL(-0.2358579933643341), COLPOLY_SNORMAL(-0.9242221713066101), 0xf719 },
	{ 0x0000, 0x0005, 0x0003, 0x0006, COLPOLY_SNORMAL(0.3003070056438446), COLPOLY_SNORMAL(-0.23585805296897888), COLPOLY_SNORMAL(-0.9242221713066101), 0xf719 },
	{ 0x0000, 0x0001, 0x0008, 0x0009, COLPOLY_SNORMAL(-0.7861931324005127), COLPOLY_SNORMAL(-0.23585689067840576), COLPOLY_SNORMAL(0.5712020397186279), 0xf679 },
	{ 0x0000, 0x0001, 0x0009, 0x0002, COLPOLY_SNORMAL(-0.7861936092376709), COLPOLY_SNORMAL(-0.23585611581802368), COLPOLY_SNORMAL(0.5712018013000488), 0xf679 },
	{ 0x0001, 0x0006, 0x0003, 0x0002, COLPOLY_SNORMAL(-2.8432383558651964e-08), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-8.750483004860143e-08), 0xf63c },
	{ 0x0001, 0x0009, 0x0007, 0x0002, COLPOLY_SNORMAL(2.843082214099013e-08), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-8.750429003612226e-08), 0xf63c },
	{ 0x0001, 0x0007, 0x0006, 0x0002, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(0.0), 0xf63c },
};

Vec3s gPlantPotCol_vertices[10] = {
	{ -979, 0, -2784 },
	{ -3052, 0, 69 },
	{ -3802, 2500, 69 },
	{ -1211, 2500, -3498 },
	{ 2375, 0, 1832 },
	{ 2375, 0, -1694 },
	{ 2982, 2500, -2135 },
	{ 2982, 2500, 2273 },
	{ -979, 0, 2922 },
	{ -1210, 2500, 3636 },
};

CollisionHeader gPlantPotCol_collisionHeader = {
	-3802,
	0,
	-3498,
	2982,
	2500,
	3636,
	10,
	gPlantPotCol_vertices,
	13,
	gPlantPotCol_polygons,
	gPlantPotCol_polygonTypes,
	0,
	0,
	0
};

