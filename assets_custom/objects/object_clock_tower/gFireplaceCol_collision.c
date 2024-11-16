#include "ultra64.h"
#include "z64.h"
#include "macros.h"

SurfaceType gFireplaceCol_polygonTypes[] = {
	{ 0x00000000, 0x00000002 },
	{ 0x00000000, 0x00000000 },
};

CollisionPoly gFireplaceCol_polygons[] = {
	{ 0x0000, 0x0001, 0x0002, 0x0000, COLPOLY_SNORMAL(2.1892647055210546e-06), COLPOLY_SNORMAL(3.253380214118806e-07), COLPOLY_SNORMAL(1.0), 0xfca6 },
	{ 0x0000, 0x0003, 0x0004, 0x0005, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(0.0), 0xfa24 },
	{ 0x0000, 0x0006, 0x0007, 0x0008, COLPOLY_SNORMAL(-0.9296943545341492), COLPOLY_SNORMAL(1.6100296207355314e-08), COLPOLY_SNORMAL(0.3683318793773651), 0xfc47 },
	{ 0x0000, 0x0009, 0x0002, 0x000a, COLPOLY_SNORMAL(0.26525694131851196), COLPOLY_SNORMAL(-0.9573870301246643), COLPOLY_SNORMAL(0.11423102021217346), 0x02bd },
	{ 0x0000, 0x000b, 0x000c, 0x0009, COLPOLY_SNORMAL(0.9184547662734985), COLPOLY_SNORMAL(2.144951771754222e-08), COLPOLY_SNORMAL(0.39552605152130127), 0xfc43 },
	{ 0x0000, 0x0001, 0x000d, 0x000e, COLPOLY_SNORMAL(-0.9278956651687622), COLPOLY_SNORMAL(1.6296990423825264e-08), COLPOLY_SNORMAL(0.37283986806869507), 0xfbf8 },
	{ 0x0000, 0x000f, 0x0000, 0x0002, COLPOLY_SNORMAL(0.9173575043678284), COLPOLY_SNORMAL(1.2950575012382615e-07), COLPOLY_SNORMAL(0.39806440472602844), 0xfbf4 },
	{ 0x0000, 0x0010, 0x000e, 0x0007, COLPOLY_SNORMAL(-0.25765371322631836), COLPOLY_SNORMAL(-0.9608302116394043), COLPOLY_SNORMAL(0.10207880288362503), 0x02cd },
	{ 0x0000, 0x0012, 0x0013, 0x0011, COLPOLY_SNORMAL(0.9701451063156128), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24252499639987946), 0xfdc7 },
	{ 0x0000, 0x0014, 0x0015, 0x0011, COLPOLY_SNORMAL(-3.178917324930808e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfb50 },
	{ 0x0000, 0x0014, 0x0017, 0x0016, COLPOLY_SNORMAL(-0.9701408743858337), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24254213273525238), 0xfdac },
	{ 0x0000, 0x000b, 0x0009, 0x000a, COLPOLY_SNORMAL(0.9184547662734985), COLPOLY_SNORMAL(1.8474112906119444e-08), COLPOLY_SNORMAL(0.39552605152130127), 0xfc43 },
	{ 0x0000, 0x0009, 0x0018, 0x000f, COLPOLY_SNORMAL(2.342497100471519e-05), COLPOLY_SNORMAL(-4.3711548158853475e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0019, 0x000a, 0x0010, COLPOLY_SNORMAL(2.0693630631285487e-06), COLPOLY_SNORMAL(-2.566959483374376e-05), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0001, 0x001a, 0x000d, COLPOLY_SNORMAL(-0.9278956651687622), COLPOLY_SNORMAL(1.2129923732118186e-07), COLPOLY_SNORMAL(0.37283986806869507), 0xfbf8 },
	{ 0x0000, 0x001b, 0x001c, 0x001d, COLPOLY_SNORMAL(0.9781559109687805), COLPOLY_SNORMAL(0.14249813556671143), COLPOLY_SNORMAL(0.15134496986865997), 0xfdad },
	{ 0x0000, 0x001f, 0x0020, 0x001e, COLPOLY_SNORMAL(-0.9238792061805725), COLPOLY_SNORMAL(-0.3826843798160553), COLPOLY_SNORMAL(-2.4913265406212304e-06), 0x0224 },
	{ 0x0000, 0x0009, 0x000f, 0x0002, COLPOLY_SNORMAL(0.2573748230934143), COLPOLY_SNORMAL(-0.9598361253738403), COLPOLY_SNORMAL(0.11168137192726135), 0x02c8 },
	{ 0x0000, 0x0012, 0x000b, 0x0013, COLPOLY_SNORMAL(1.8360661897531827e-06), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0014, 0x0011, 0x0017, COLPOLY_SNORMAL(-3.178917609147902e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfb50 },
	{ 0x0000, 0x0018, 0x0003, 0x0005, COLPOLY_SNORMAL(-1.4300510429166025e-06), COLPOLY_SNORMAL(4.359039394330466e-06), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0021, 0x0022, 0x0010, COLPOLY_SNORMAL(1.1260573273830232e-06), COLPOLY_SNORMAL(-3.669604666356463e-06), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0023, 0x0024, 0x001f, COLPOLY_SNORMAL(-1.400706537424412e-06), COLPOLY_SNORMAL(5.9549552844373466e-08), COLPOLY_SNORMAL(1.0), 0xff9c },
	{ 0x0000, 0x0013, 0x0025, 0x0026, COLPOLY_SNORMAL(0.9701451063156128), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24252501130104065), 0xfdc7 },
	{ 0x0000, 0x0027, 0x0028, 0x0029, COLPOLY_SNORMAL(-0.9804642200469971), COLPOLY_SNORMAL(0.14283573627471924), COLPOLY_SNORMAL(0.13523244857788086), 0xfdb5 },
	{ 0x0000, 0x0023, 0x0021, 0x0016, COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.634883119550068e-06), 0x019e },
	{ 0x0000, 0x002a, 0x001e, 0x0019, COLPOLY_SNORMAL(-0.38268226385116577), COLPOLY_SNORMAL(-0.9238799810409546), COLPOLY_SNORMAL(-9.798012570172432e-07), 0x033b },
	{ 0x0000, 0x0001, 0x0000, 0x001a, COLPOLY_SNORMAL(2.1892647055210546e-06), COLPOLY_SNORMAL(3.2533870353290695e-07), COLPOLY_SNORMAL(1.0), 0xfca6 },
	{ 0x0000, 0x0027, 0x002b, 0x0028, COLPOLY_SNORMAL(-0.9804641604423523), COLPOLY_SNORMAL(0.14283576607704163), COLPOLY_SNORMAL(0.13523247838020325), 0xfdb5 },
	{ 0x0000, 0x0013, 0x0026, 0x0011, COLPOLY_SNORMAL(0.9701451063156128), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24252501130104065), 0xfdc7 },
	{ 0x0000, 0x0017, 0x002c, 0x0016, COLPOLY_SNORMAL(-0.9701408743858337), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24254213273525238), 0xfdac },
	{ 0x0000, 0x0017, 0x0026, 0x002d, COLPOLY_SNORMAL(-3.1789178933649964e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfb50 },
	{ 0x0000, 0x000a, 0x0001, 0x0010, COLPOLY_SNORMAL(7.703288815719134e-07), COLPOLY_SNORMAL(-0.9280802011489868), COLPOLY_SNORMAL(0.3723803460597992), 0x028a },
	{ 0x0000, 0x0013, 0x0020, 0x001f, COLPOLY_SNORMAL(-1.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-2.7248099740972975e-06), 0x0182 },
	{ 0x0000, 0x0006, 0x0010, 0x0007, COLPOLY_SNORMAL(-0.9296943545341492), COLPOLY_SNORMAL(1.7203936053533653e-08), COLPOLY_SNORMAL(0.3683318793773651), 0xfc47 },
	{ 0x0000, 0x001e, 0x000a, 0x0019, COLPOLY_SNORMAL(-2.9380544219748117e-06), COLPOLY_SNORMAL(3.896220732713118e-06), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x002e, 0x0003, 0x0018, COLPOLY_SNORMAL(-8.765954930822772e-07), COLPOLY_SNORMAL(1.1967497812293004e-05), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x000e, 0x000d, 0x002e, COLPOLY_SNORMAL(-8.063109999056906e-05), COLPOLY_SNORMAL(-4.371043260675833e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0030, 0x001f, 0x002f, COLPOLY_SNORMAL(-1.4007117670189473e-06), COLPOLY_SNORMAL(-2.6651239295460982e-06), COLPOLY_SNORMAL(1.0), 0xff9c },
	{ 0x0000, 0x0023, 0x001f, 0x0030, COLPOLY_SNORMAL(-1.4007063100507366e-06), COLPOLY_SNORMAL(5.954944271024942e-08), COLPOLY_SNORMAL(1.0), 0xff9c },
	{ 0x0000, 0x000c, 0x0008, 0x0007, COLPOLY_SNORMAL(-8.765954930822772e-07), COLPOLY_SNORMAL(-4.371138828673793e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0003, 0x0031, 0x0004, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(0.0), 0xfa24 },
	{ 0x0000, 0x000f, 0x0018, 0x0032, COLPOLY_SNORMAL(2.342497100471519e-05), COLPOLY_SNORMAL(-4.371136341774218e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0021, 0x0030, 0x002f, COLPOLY_SNORMAL(0.9238786101341248), COLPOLY_SNORMAL(-0.3826856017112732), COLPOLY_SNORMAL(-1.4843649296381045e-06), 0x023e },
	{ 0x0000, 0x001f, 0x0033, 0x002f, COLPOLY_SNORMAL(3.3919454835995566e-06), COLPOLY_SNORMAL(8.905382856028154e-06), COLPOLY_SNORMAL(1.0), 0xff9c },
	{ 0x0000, 0x002a, 0x0019, 0x0033, COLPOLY_SNORMAL(-0.38268235325813293), COLPOLY_SNORMAL(-0.9238799214363098), COLPOLY_SNORMAL(-1.0427366987642017e-06), 0x033b },
	{ 0x0000, 0x002e, 0x0031, 0x0003, COLPOLY_SNORMAL(-0.684497058391571), COLPOLY_SNORMAL(-0.7028322815895081), COLPOLY_SNORMAL(0.19362464547157288), 0x00ef },
	{ 0x0000, 0x002e, 0x0034, 0x0031, COLPOLY_SNORMAL(-0.5221341848373413), COLPOLY_SNORMAL(-0.8273957967758179), COLPOLY_SNORMAL(0.20686225593090057), 0x023c },
	{ 0x0000, 0x0018, 0x0004, 0x0035, COLPOLY_SNORMAL(0.5201137661933899), COLPOLY_SNORMAL(-0.8242045044898987), COLPOLY_SNORMAL(0.22398331761360168), 0x0230 },
	{ 0x0000, 0x0018, 0x0005, 0x0004, COLPOLY_SNORMAL(0.6812020540237427), COLPOLY_SNORMAL(-0.6994551420211792), COLPOLY_SNORMAL(0.2161630541086197), 0x00e2 },
	{ 0x0000, 0x0009, 0x0007, 0x002e, COLPOLY_SNORMAL(-8.765954930822772e-07), COLPOLY_SNORMAL(-4.3711249730904456e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0023, 0x0030, 0x0021, COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-9.73563880879956e-14), COLPOLY_SNORMAL(-1.634883119550068e-06), 0x019e },
	{ 0x0000, 0x0010, 0x0001, 0x000e, COLPOLY_SNORMAL(-0.2704228460788727), COLPOLY_SNORMAL(-0.9565901160240173), COLPOLY_SNORMAL(0.10865923017263412), 0x02ba },
	{ 0x0000, 0x000b, 0x0020, 0x0013, COLPOLY_SNORMAL(1.8360661897531827e-06), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0020, 0x000a, 0x001e, COLPOLY_SNORMAL(-2.15301702155557e-06), COLPOLY_SNORMAL(2.4799326183710946e-06), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0022, 0x0019, 0x0010, COLPOLY_SNORMAL(7.936188922030851e-06), COLPOLY_SNORMAL(8.109776899800636e-06), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0017, 0x002d, 0x002c, COLPOLY_SNORMAL(-0.9701408743858337), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24254213273525238), 0xfdac },
	{ 0x0000, 0x001b, 0x001d, 0x0036, COLPOLY_SNORMAL(0.9781587719917297), COLPOLY_SNORMAL(0.14249014854431152), COLPOLY_SNORMAL(0.15133365988731384), 0xfdad },
	{ 0x0000, 0x0021, 0x002f, 0x0022, COLPOLY_SNORMAL(0.9238789081573486), COLPOLY_SNORMAL(-0.3826850652694702), COLPOLY_SNORMAL(-1.232626573255402e-06), 0x023e },
	{ 0x0000, 0x001f, 0x002a, 0x0033, COLPOLY_SNORMAL(-3.7281026834534714e-06), COLPOLY_SNORMAL(1.7853624285635306e-06), COLPOLY_SNORMAL(1.0), 0xff9c },
	{ 0x0000, 0x0017, 0x0011, 0x0013, COLPOLY_SNORMAL(1.2715669299723231e-06), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(2.880747729250288e-07), 0xff9c },
	{ 0x0000, 0x0017, 0x0011, 0x0026, COLPOLY_SNORMAL(-3.1789178933649964e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfb50 },
	{ 0x0000, 0x0029, 0x001d, 0x001c, COLPOLY_SNORMAL(-2.8101731004426256e-06), COLPOLY_SNORMAL(0.10994456708431244), COLPOLY_SNORMAL(-0.9939377307891846), 0xfff2 },
	{ 0x0000, 0x0035, 0x0004, 0x0031, COLPOLY_SNORMAL(1.812392156352871e-06), COLPOLY_SNORMAL(-5.245245802143472e-07), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0024, 0x0013, 0x001f, COLPOLY_SNORMAL(-1.0), COLPOLY_SNORMAL(-1.6226123634825729e-13), COLPOLY_SNORMAL(-2.7248099740972975e-06), 0x0182 },
	{ 0x0000, 0x001f, 0x001e, 0x002a, COLPOLY_SNORMAL(-0.9238792061805725), COLPOLY_SNORMAL(-0.3826843798160553), COLPOLY_SNORMAL(-2.4913265406212304e-06), 0x0224 },
	{ 0x0000, 0x000f, 0x0032, 0x0000, COLPOLY_SNORMAL(0.9173575043678284), COLPOLY_SNORMAL(1.7399941043549916e-08), COLPOLY_SNORMAL(0.3980644643306732), 0xfbf4 },
	{ 0x0000, 0x000a, 0x0002, 0x0001, COLPOLY_SNORMAL(8.15241207874351e-07), COLPOLY_SNORMAL(-0.9280797839164734), COLPOLY_SNORMAL(0.3723812997341156), 0x028a },
	{ 0x0000, 0x0013, 0x0024, 0x0023, COLPOLY_SNORMAL(9.536726111036842e-07), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-5.108758571736871e-08), 0xff9c },
	{ 0x0000, 0x0038, 0x0039, 0x0037, COLPOLY_SNORMAL(0.9701407551765442), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-0.242542564868927), 0x01f3 },
	{ 0x0000, 0x0006, 0x003a, 0x0016, COLPOLY_SNORMAL(7.786170499457512e-06), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0006, 0x0016, 0x0021, COLPOLY_SNORMAL(7.786170499457512e-06), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x000b, 0x000a, 0x0020, COLPOLY_SNORMAL(1.9110582343273563e-06), COLPOLY_SNORMAL(4.6707704370874126e-08), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0006, 0x0021, 0x0010, COLPOLY_SNORMAL(7.70666611060733e-06), COLPOLY_SNORMAL(4.6707704370874126e-08), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x003a, 0x0014, 0x0016, COLPOLY_SNORMAL(-0.9701408743858337), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24254213273525238), 0xfdac },
	{ 0x0000, 0x0023, 0x0016, 0x0013, COLPOLY_SNORMAL(9.536760217088158e-07), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-5.1091657127244616e-08), 0xff9c },
	{ 0x0000, 0x003c, 0x003d, 0x003b, COLPOLY_SNORMAL(-0.9701407551765442), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-0.2425425499677658), 0x01d8 },
	{ 0x0000, 0x003d, 0x003e, 0x002d, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(0.0), 0xfed4 },
	{ 0x0000, 0x002f, 0x0033, 0x0019, COLPOLY_SNORMAL(0.38267335295677185), COLPOLY_SNORMAL(-0.9238837361335754), COLPOLY_SNORMAL(1.0346533372285194e-06), 0x0345 },
	{ 0x0000, 0x0007, 0x000e, 0x002e, COLPOLY_SNORMAL(-8.063109999056906e-05), COLPOLY_SNORMAL(-4.370999207026216e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0009, 0x002e, 0x0018, COLPOLY_SNORMAL(-8.765954930822772e-07), COLPOLY_SNORMAL(-4.371152328985772e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0016, 0x0017, 0x0013, COLPOLY_SNORMAL(9.536760217088158e-07), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-2.6822803533832484e-07), 0xff9c },
	{ 0x0000, 0x001b, 0x002b, 0x0027, COLPOLY_SNORMAL(4.492988864512881e-06), COLPOLY_SNORMAL(0.10994494706392288), COLPOLY_SNORMAL(0.993937611579895), 0xfcd7 },
	{ 0x0000, 0x0029, 0x0028, 0x001d, COLPOLY_SNORMAL(-6.069702976674307e-06), COLPOLY_SNORMAL(0.10994794219732285), COLPOLY_SNORMAL(-0.9939374327659607), 0xfff2 },
	{ 0x0000, 0x0015, 0x0012, 0x0011, COLPOLY_SNORMAL(0.9701451063156128), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.24252499639987946), 0xfdc7 },
	{ 0x0000, 0x003f, 0x003b, 0x0025, COLPOLY_SNORMAL(-4.7682263470960606e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x0320 },
	{ 0x0000, 0x000c, 0x0007, 0x0009, COLPOLY_SNORMAL(-8.765954930822772e-07), COLPOLY_SNORMAL(-4.371132433789171e-08), COLPOLY_SNORMAL(-1.0), 0x0000 },
	{ 0x0000, 0x0038, 0x003d, 0x003c, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x044c },
	{ 0x0000, 0x0034, 0x0035, 0x0031, COLPOLY_SNORMAL(2.0693628357548732e-06), COLPOLY_SNORMAL(2.1054456738056615e-06), COLPOLY_SNORMAL(1.0), 0xfce0 },
	{ 0x0000, 0x0038, 0x0037, 0x003e, COLPOLY_SNORMAL(0.9701407551765442), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-0.2425425499677658), 0x01f3 },
	{ 0x0000, 0x003f, 0x003c, 0x003b, COLPOLY_SNORMAL(-0.9701407551765442), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-0.242542564868927), 0x01d8 },
	{ 0x0000, 0x0026, 0x0025, 0x003d, COLPOLY_SNORMAL(7.688320692977868e-06), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(1.8623865116751404e-06), 0xfed4 },
	{ 0x0000, 0x0025, 0x003b, 0x003d, COLPOLY_SNORMAL(7.6291639743431006e-06), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(1.8278763036505552e-06), 0xfed4 },
	{ 0x0000, 0x0037, 0x002c, 0x003e, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-7.947244284878252e-08), 0xfed4 },
	{ 0x0000, 0x002c, 0x002d, 0x003e, COLPOLY_SNORMAL(-5.960433213658689e-08), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(-4.4702908041927e-08), 0xfed4 },
	{ 0x0000, 0x0026, 0x003d, 0x002d, COLPOLY_SNORMAL(1.2715648836092441e-06), COLPOLY_SNORMAL(1.0), COLPOLY_SNORMAL(6.675582426396431e-06), 0xfed4 },
	{ 0x0000, 0x0022, 0x002f, 0x0019, COLPOLY_SNORMAL(0.38267654180526733), COLPOLY_SNORMAL(-0.9238823056221008), COLPOLY_SNORMAL(-4.5842449480915093e-07), 0x0345 },
	{ 0x0000, 0x001b, 0x0036, 0x002b, COLPOLY_SNORMAL(2.4546304757677717e-06), COLPOLY_SNORMAL(0.10994327068328857), COLPOLY_SNORMAL(0.9939377903938293), 0xfcd7 },
	{ 0x0000, 0x0013, 0x003f, 0x0025, COLPOLY_SNORMAL(-4.7682263470960606e-07), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x0320 },
	{ 0x0000, 0x0038, 0x003e, 0x003d, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x044c },
	{ 0x0001, 0x0016, 0x002c, 0x0037, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x0320 },
	{ 0x0001, 0x0039, 0x0016, 0x0037, COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(0.0), COLPOLY_SNORMAL(-1.0), 0x0320 },
};

Vec3s gFireplaceCol_vertices[64] = {
	{ 757, 1338, 858 },
	{ -767, 1044, 858 },
	{ 757, 1044, 858 },
	{ -1192, 1500, 0 },
	{ 955, 1500, 800 },
	{ 1209, 1500, 0 },
	{ -708, 0, 800 },
	{ -1025, 1021, 0 },
	{ -1025, 0, 0 },
	{ 1042, 1021, 0 },
	{ 698, 1021, 800 },
	{ 698, 0, 800 },
	{ 1042, 0, 0 },
	{ -1112, 1338, 0 },
	{ -1112, 1044, 0 },
	{ 1130, 1044, 0 },
	{ -708, 1021, 800 },
	{ 286, 100, 1200 },
	{ 386, 0, 800 },
	{ 386, 100, 800 },
	{ -314, 0, 1200 },
	{ 286, 0, 1200 },
	{ -414, 100, 800 },
	{ -314, 100, 1200 },
	{ 1042, 1338, 0 },
	{ -14, 900, 800 },
	{ -767, 1338, 858 },
	{ 290, 1500, 648 },
	{ 367, 1500, 152 },
	{ 269, 2100, 219 },
	{ 269, 783, 800 },
	{ 386, 500, 100 },
	{ 386, 500, 800 },
	{ -414, 500, 800 },
	{ -297, 783, 800 },
	{ -414, 100, 100 },
	{ 386, 100, 100 },
	{ 386, 300, 800 },
	{ 286, 300, 1200 },
	{ -290, 1500, 648 },
	{ -262, 2100, 219 },
	{ -359, 1500, 152 },
	{ 269, 783, 100 },
	{ -212, 2100, 581 },
	{ -414, 300, 800 },
	{ -314, 300, 1200 },
	{ -1025, 1338, 0 },
	{ -297, 783, 100 },
	{ -414, 500, 100 },
	{ -965, 1500, 800 },
	{ 1130, 1338, 0 },
	{ -14, 900, 100 },
	{ -708, 1338, 800 },
	{ 698, 1338, 800 },
	{ 213, 2100, 581 },
	{ -314, 300, 800 },
	{ -239, 100, 1100 },
	{ -314, 100, 800 },
	{ -414, 0, 800 },
	{ 286, 300, 800 },
	{ 211, 100, 1100 },
	{ 211, 300, 1100 },
	{ -239, 300, 1100 },
	{ 286, 100, 800 },
};

CollisionHeader gFireplaceCol_collisionHeader = {
	-1192,
	0,
	0,
	1209,
	2100,
	1200,
	64,
	gFireplaceCol_vertices,
	102,
	gFireplaceCol_polygons,
	gFireplaceCol_polygonTypes,
	0,
	0,
	0
};
