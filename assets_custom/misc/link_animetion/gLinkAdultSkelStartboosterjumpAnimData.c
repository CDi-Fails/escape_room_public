#include "ultra64.h"
#include "global.h"

s16 gLinkAdultSkelStartboosterjumpAnimData[] = {
	0xffc7, 0x0d31, 0x0000, 0x0000, 0x0000, 0x0000, 0xbfff, 0x02da, 
	0xbfff, 0xffff, 0xffff, 0xffff, 0x092e, 0x0c98, 0xef13, 0xff70, 
	0xfeed, 0x1ffe, 0x0792, 0x05d5, 0xb05f, 0xf999, 0xf47d, 0xedd1, 
	0xffd7, 0xffb3, 0x2085, 0xf82c, 0xfb57, 0xb19f, 0x4000, 0xf708, 
	0x4000, 0xffe7, 0xfe50, 0x34dc, 0x0362, 0xfca5, 0x6056, 0xffff, 
	0x0000, 0xffff, 0x0931, 0xed9e, 0x7892, 0xffff, 0xfe3d, 0xe5ed, 
	0xfe18, 0x01a0, 0xc0bf, 0xf540, 0x11ec, 0x77ed, 0x0000, 0x01c3, 
	0xefc5, 0x0041, 0x0207, 0xabac, 0xbfff, 0x6a68, 0xfd25, 0xffff, 
	0x0000, 0xffff, 0x0000, 0xffc7, 0x0a00, 0xfe95, 0x0d6f, 0x0000, 
	0x0000, 0xbfff, 0xeff3, 0xbfff, 0x0000, 0xffff, 0x0b28, 0x00b1, 
	0x15aa, 0xcea8, 0xfd88, 0xfe66, 0x49fa, 0x0860, 0x009c, 0xa00e, 
	0x01ea, 0xee77, 0xcd59, 0xff54, 0xff90, 0x49e1, 0xf579, 0xff6a, 
	0xa1c3, 0x4000, 0xf4be, 0x4000, 0xffd8, 0x0001, 0x4a75, 0xfff4, 
	0xfe9f, 0x5b2e, 0x0000, 0xffff, 0xffff, 0x09e4, 0xe221, 0x74d1, 
	0xfffe, 0xfe3e, 0xc849, 0xfe89, 0x05f9, 0xca67, 0xf529, 0x1d0d, 
	0x76f3, 0x0001, 0x01c2, 0xc7fe, 0xfd5c, 0xfd03, 0xc3db, 0xbfff, 
	0x6a68, 0xfd25, 0xffff, 0xffff, 0x0000, 0x0000, 0xffc7, 0x06d0, 
	0xfd2a, 0x1add, 0x0000, 0x0000, 0xbfff, 0xdd0d, 0xbfff, 0xffff, 
	0xffff, 0x1650, 0xee34, 0x15eb, 0xa98a, 0xfc7b, 0xff0d, 0x6385, 
	0x0b15, 0xfca7, 0xa0ee, 0x1355, 0xf1fb, 0xab99, 0xff0c, 0xffbb, 
	0x62ba, 0xeecc, 0x01fb, 0xa2cf, 0x4000, 0xf275, 0x4000, 0x0301, 
	0x00e5, 0x4ae1, 0x00a0, 0x0104, 0x60bd, 0xffff, 0x0000, 0xffff, 
	0x0f35, 0xc82d, 0x68fb, 0xfffd, 0xfe3d, 0xb1c2, 0x02a5, 0x06be, 
	0xd263, 0xdad0, 0x3843, 0x5604, 0x0003, 0x01c2, 0xaddc, 0xfa3d, 
	0xff82, 0xd8e7, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 
	0x0000, 0xffc7, 0x0841, 0xfe36, 0x18ee, 0x0000, 0x0000, 0xbfff, 
	0xe4a1, 0xbfff, 0xffff, 0xffff, 0x131d, 0xf657, 0x17e3, 0xb855, 
	0xfcc2, 0xfec8, 0x5ba4, 0x0bf2, 0xfe83, 0x9a43, 0x0af3, 0xee3b, 
	0xb8d4, 0xff22, 0xffa8, 0x5a43, 0xf16e, 0x002b, 0x9c3e, 0x4000, 
	0xf35f, 0x4000, 0x0163, 0x009f, 0x4d19, 0x0001, 0x0012, 0x59cd, 
	0x0000, 0xffff, 0xffff, 0xfe54, 0xddd5, 0x7ed7, 0xfffd, 0xfe3e, 
	0xbdf9, 0xff30, 0x26e7, 0xc7d1, 0xff69, 0x2004, 0x7e4f, 0x0002, 
	0x01c1, 0xb9d8, 0xfd14, 0xd9e5, 0xcf37, 0xbfff, 0x6a68, 0xfd25, 
	0x0000, 0xffff, 0xffff, 0x0000, 0xffc7, 0x0a62, 0x015e, 0x1729, 
	0x0000, 0x0000, 0xbfff, 0xf2b4, 0xbfff, 0x0000, 0xffff, 0x0c14, 
	0x000b, 0x143a, 0xcf2a, 0xfd95, 0xfe62, 0x48e7, 0x0dad, 0x0354, 
	0x95b7, 0x0222, 0xf0ff, 0xcfa4, 0xff63, 0xff8d, 0x4583, 0xf502, 
	0xfa52, 0x9762, 0x4000, 0xf514, 0x4000, 0xff9e, 0xffc8, 0x4991, 
	0x005e, 0xfe80, 0x5320, 0x0000, 0x0000, 0x0000, 0x01db, 0xebe0, 
	0x7633, 0x0000, 0xfe3c, 0xfffb, 0xf836, 0x3231, 0x9adc, 0xfd3a, 
	0x11b1, 0x7633, 0xffff, 0x01c3, 0xfffb, 0x0077, 0xcfcb, 0xa206, 
	0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 
	0x0a78, 0x055c, 0x1b99, 0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 
	0x0000, 0xffff, 0x050f, 0xfb0b, 0x0a32, 0xdbe6, 0xfe30, 0xfe58, 
	0x3c92, 0x0d1d, 0x05d0, 0xadfd, 0x05b3, 0xf83a, 0xde59, 0xff94, 
	0xff8f, 0x36fa, 0xf5e4, 0xf7b8, 0xaf66, 0x4000, 0xf5ff, 0x4000, 
	0xffb4, 0xfefb, 0x359f, 0x01a1, 0xfdc9, 0x5e35, 0xffff, 0x0000, 
	0x0000, 0xf702, 0xeda4, 0x80fc, 0x0000, 0xfe3c, 0xfffb, 0xe93b, 
	0x31df, 0x8982, 0x0675, 0x104b, 0x80fb, 0xffff, 0x01c3, 0xfffa, 
	0x0aa2, 0xd075, 0x950a, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 
	0x0000, 0x0000, 0xffc7, 0x092e, 0x075a, 0x2a2b, 0x0000, 0x0000, 
	0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 0x0119, 0xfb51, 0x0490, 
	0xea5b, 0xfeb7, 0xfe76, 0x3197, 0x092f, 0xfd27, 0xc3d7, 0x04c5, 
	0xfb4d, 0xeba7, 0xffb1, 0xff99, 0x2e1b, 0xf616, 0x0255, 0xc526, 
	0x4000, 0xf5ff, 0x4000, 0x003f, 0xfea0, 0x1d70, 0x01f1, 0xfd24, 
	0x71cd, 0xffff, 0x0000, 0xffff, 0xf00b, 0xeddf, 0x7e35, 0x0000, 
	0xfe3c, 0xfffb, 0x6513, 0x5105, 0xfb60, 0x0c68, 0x1015, 0x7e34, 
	0xffff, 0x01c3, 0xfffa, 0x0e82, 0xd250, 0x86d5, 0xbfff, 0x6a68, 
	0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 0x0887, 0x081b, 
	0x33f5, 0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 
	0x0000, 0x0907, 0x04a8, 0xfbeb, 0xffc3, 0xff59, 0x152a, 0xf869, 
	0xf47c, 0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 0xffd3, 0x14dd, 
	0x02dc, 0x0ab3, 0xd89e, 0x4000, 0xf5ff, 0x4000, 0x0115, 0xfe11, 
	0x0a85, 0x0219, 0xfbcf, 0x83de, 0xffff, 0x0000, 0xffff, 0xeb21, 
	0xee00, 0x7b6b, 0x0000, 0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 0x7363, 
	0x1081, 0x0ff6, 0x7b6b, 0xffff, 0x01c3, 0xfffa, 0x1148, 0xd460, 
	0x7e33, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 
	0xffc7, 0x0887, 0x081b, 0x33f5, 0x0000, 0x0000, 0xbfff, 0xfa48, 
	0xbfff, 0xffff, 0x0000, 0x0000, 0x0907, 0x04a8, 0xfbeb, 0xffc3, 
	0xff59, 0x152a, 0xf869, 0xf47c, 0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 
	0xffef, 0xffd3, 0x14dd, 0x02dc, 0x0ab3, 0xd89e, 0x4000, 0xf5ff, 
	0x4000, 0x0115, 0xfe11, 0x0a85, 0x0219, 0xfbcf, 0x83de, 0xffff, 
	0x0000, 0xffff, 0xeb21, 0xee00, 0x7b6b, 0x0000, 0xfe3c, 0xfffb, 
	0xe2ab, 0x2c0c, 0x7363, 0x1081, 0x0ff6, 0x7b6b, 0xffff, 0x01c3, 
	0xfffa, 0x1148, 0xd460, 0x7e33, 0xbfff, 0x6a68, 0xfd25, 0x0000, 
	0xffff, 0x0000, 0x0000, 0xffc7, 0x0887, 0x081b, 0x33f5, 0x0000, 
	0x0000, 0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 0x0000, 0x0907, 
	0x04a8, 0xfbeb, 0xffc3, 0xff59, 0x152a, 0xf869, 0xf47c, 0xd957, 
	0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 0xffd3, 0x14dd, 0x02dc, 0x0ab3, 
	0xd89e, 0x4000, 0xf5ff, 0x4000, 0x0115, 0xfe11, 0x0a85, 0x0219, 
	0xfbcf, 0x83de, 0xffff, 0x0000, 0xffff, 0xeb21, 0xee00, 0x7b6b, 
	0x0000, 0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 0x7363, 0x1081, 0x0ff6, 
	0x7b6b, 0xffff, 0x01c3, 0xfffa, 0x1148, 0xd460, 0x7e33, 0xbfff, 
	0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 0x0887, 
	0x081b, 0x33f5, 0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 0xffff, 
	0x0000, 0x0000, 0x0907, 0x04a8, 0xfbeb, 0xffc3, 0xff59, 0x152a, 
	0xf869, 0xf47c, 0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 0xffd3, 
	0x14dd, 0x02dc, 0x0ab3, 0xd89e, 0x4000, 0xf5ff, 0x4000, 0x0115, 
	0xfe11, 0x0a85, 0x0219, 0xfbcf, 0x83de, 0xffff, 0x0000, 0xffff, 
	0xeb21, 0xee00, 0x7b6b, 0x0000, 0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 
	0x7363, 0x1081, 0x0ff6, 0x7b6b, 0xffff, 0x01c3, 0xfffa, 0x1148, 
	0xd460, 0x7e33, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 
	0x0000, 0xffc7, 0x0887, 0x081b, 0x33f5, 0x0000, 0x0000, 0xbfff, 
	0xfa48, 0xbfff, 0xffff, 0x0000, 0x0000, 0x0907, 0x04a8, 0xfbeb, 
	0xffc3, 0xff59, 0x152a, 0xf869, 0xf47c, 0xd957, 0xfb4f, 0xfaf2, 
	0xfbb2, 0xffef, 0xffd3, 0x14dd, 0x02dc, 0x0ab3, 0xd89e, 0x4000, 
	0xf5ff, 0x4000, 0x0115, 0xfe11, 0x0a85, 0x0219, 0xfbcf, 0x83de, 
	0xffff, 0x0000, 0xffff, 0xeb21, 0xee00, 0x7b6b, 0x0000, 0xfe3c, 
	0xfffb, 0xe2ab, 0x2c0c, 0x7363, 0x1081, 0x0ff6, 0x7b6b, 0xffff, 
	0x01c3, 0xfffa, 0x1148, 0xd460, 0x7e33, 0xbfff, 0x6a68, 0xfd25, 
	0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 0x0887, 0x081b, 0x33f5, 
	0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 0x0000, 
	0x0907, 0x04a8, 0xfbeb, 0xffc3, 0xff59, 0x152a, 0xf869, 0xf47c, 
	0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 0xffd3, 0x14dd, 0x02dc, 
	0x0ab3, 0xd89e, 0x4000, 0xf5ff, 0x4000, 0x0115, 0xfe11, 0x0a85, 
	0x0219, 0xfbcf, 0x83de, 0xffff, 0x0000, 0xffff, 0xeb21, 0xee00, 
	0x7b6b, 0x0000, 0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 0x7363, 0x1081, 
	0x0ff6, 0x7b6b, 0xffff, 0x01c3, 0xfffa, 0x1148, 0xd460, 0x7e33, 
	0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 
	0x0887, 0x081b, 0x33f5, 0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 
	0xffff, 0x0000, 0x0000, 0x0907, 0x04a8, 0xfbeb, 0xffc3, 0xff59, 
	0x152a, 0xf869, 0xf47c, 0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 
	0xffd3, 0x14dd, 0x02dc, 0x0ab3, 0xd89e, 0x4000, 0xf5ff, 0x4000, 
	0x0115, 0xfe11, 0x0a85, 0x0219, 0xfbcf, 0x83de, 0xffff, 0x0000, 
	0xffff, 0xeb21, 0xee00, 0x7b6b, 0x0000, 0xfe3c, 0xfffb, 0xe2ab, 
	0x2c0c, 0x7363, 0x1081, 0x0ff6, 0x7b6b, 0xffff, 0x01c3, 0xfffa, 
	0x1148, 0xd460, 0x7e33, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 
	0x0000, 0x0000, 0xffc7, 0x0887, 0x081b, 0x33f5, 0x0000, 0x0000, 
	0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 0x0000, 0x0907, 0x04a8, 
	0xfbeb, 0xffc3, 0xff59, 0x152a, 0xf869, 0xf47c, 0xd957, 0xfb4f, 
	0xfaf2, 0xfbb2, 0xffef, 0xffd3, 0x14dd, 0x02dc, 0x0ab3, 0xd89e, 
	0x4000, 0xf5ff, 0x4000, 0x0115, 0xfe11, 0x0a85, 0x0219, 0xfbcf, 
	0x83de, 0xffff, 0x0000, 0xffff, 0xeb21, 0xee00, 0x7b6b, 0x0000, 
	0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 0x7363, 0x1081, 0x0ff6, 0x7b6b, 
	0xffff, 0x01c3, 0xfffa, 0x1148, 0xd460, 0x7e33, 0xbfff, 0x6a68, 
	0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 0xffc7, 0x0887, 0x081b, 
	0x33f5, 0x0000, 0x0000, 0xbfff, 0xfa48, 0xbfff, 0xffff, 0x0000, 
	0x0000, 0x0907, 0x04a8, 0xfbeb, 0xffc3, 0xff59, 0x152a, 0xf869, 
	0xf47c, 0xd957, 0xfb4f, 0xfaf2, 0xfbb2, 0xffef, 0xffd3, 0x14dd, 
	0x02dc, 0x0ab3, 0xd89e, 0x4000, 0xf5ff, 0x4000, 0x0115, 0xfe11, 
	0x0a85, 0x0219, 0xfbcf, 0x83de, 0xffff, 0x0000, 0xffff, 0xeb21, 
	0xee00, 0x7b6b, 0x0000, 0xfe3c, 0xfffb, 0xe2ab, 0x2c0c, 0x7363, 
	0x1081, 0x0ff6, 0x7b6b, 0xffff, 0x01c3, 0xfffa, 0x1148, 0xd460, 
	0x7e33, 0xbfff, 0x6a68, 0xfd25, 0x0000, 0xffff, 0x0000, 0x0000, 

};

