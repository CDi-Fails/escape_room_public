#include "z_demo_sa.h"
#include "z64cutscene_commands.h"

// clang-format off
static CutsceneData D_8099010C[] = {
    CS_BEGIN_CUTSCENE(29, 3001),
    CS_UNK_DATA_LIST(0x00000020, 1),
        CS_UNK_DATA(0x00010000, 0x0BB80000, 0x00000000, 0x00000000, 0xFFFFFFFC, 0x00000002, 0x00000000, 0xFFFFFFFC, 0x00000002, 0x00000000, 0x00000000, 0x00000000),
    CS_ACTOR_CUE_LIST(31, 5),
        CS_ACTOR_CUE(0x0001, 0, 612, 0x0000, 0x0000, 0x0000, 0, 216, 0, 0, 216, 0, 0.0f, 0.0f, 0.0f),
        CS_ACTOR_CUE(0x0002, 612, 613, 0x0000, 0x0000, 0x0000, 0, 216, 0, 0, 216, 0, 0.0f, 0.0f, 0.0f),
        CS_ACTOR_CUE(0x0004, 613, 684, 0x0000, 0x0000, 0x0000, 0, 216, 0, 0, 216, 0, 0.0f, 0.0f, 0.0f),
        CS_ACTOR_CUE(0x0002, 684, 732, 0x0000, 0x0000, 0x0000, 0, 216, 0, 0, 82, 0, 0.0f, -2.7916667f, 0.0f),
        CS_ACTOR_CUE(0x0003, 732, 2912, 0x0000, 0x0000, 0x0000, 0, 82, 0, 0, 82, 0, 0.0f, 0.0f, 0.0f),
    CS_PLAYER_CUE_LIST(3),
        CS_PLAYER_CUE(PLAYER_CUEID_13, 0, 261, 0x0000, 0x0000, 0x0000, 0, 6, 0, 0, 6, 0, 0.0f, 0.0f, 0.0f),
        CS_PLAYER_CUE(PLAYER_CUEID_5, 261, 600, 0x0000, 0x9555, 0x0000, 0, 6, 0, 0, 6, 0, 0.0f, 0.0f, 0.0f),
        CS_PLAYER_CUE(PLAYER_CUEID_19, 600, 1243, 0x0000, 0x1555, 0x0000, 0, 6, 0, 0, 6, 0, 0.0f, 0.0f, 0.0f),
    CS_ACTOR_CUE_LIST(43, 3),
        CS_ACTOR_CUE(0x0001, 0, 165, 0x0000, 0x0000, 0x0000, -98, 6, -169, -98, 6, -169, 0.0f, 0.0f, 0.0f),
        CS_ACTOR_CUE(0x0002, 165, 466, 0x0000, 0x0000, 0x0000, -98, 6, -169, -98, 6, -169, 0.0f, 0.0f, 0.0f),
        CS_ACTOR_CUE(0x0003, 466, 3001, 0x0000, 0x0000, 0x0000, -98, 6, -169, -98, 6, -169, 0.0f, 0.0f, 0.0f),
    CS_TRANSITION(CS_TRANS_GRAY_FILL_IN, 590, 607),
    CS_TRANSITION(CS_TRANS_GRAY_FILL_OUT, 617, 647),
    CS_TRANSITION(CS_TRANS_GRAY_FILL_IN, 875, 905),
    CS_ACTOR_CUE_LIST(49, 1),
        CS_ACTOR_CUE(0x0001, 0, 3000, 0x0000, 0x0000, 0x0000, -98, 0, 98, -98, 0, 98, 0.0f, 0.0f, 0.0f),
    CS_ACTOR_CUE_LIST(62, 1),
        CS_ACTOR_CUE(0x0004, 0, 3000, 0x0000, 0x0000, 0x0000, -35, 97, -60, -35, 97, -60, 0.0f, 0.0f, 0.0f),
    CS_DESTINATION(CS_DEST_KOKIRI_FOREST_FROM_CHAMBER_OF_SAGES, 974, 1050),
    CS_TEXT_LIST(10),
        CS_TEXT_NONE(0, 303),
        CS_TEXT(0x106A, 303, 323, 0x0000, 0x0000, 0x0000),
        CS_TEXT_NONE(323, 344),
        CS_TEXT(0x108F, 344, 394, 0x0000, 0x0000, 0x0000),
        CS_TEXT_NONE(394, 415),
        CS_TEXT(0x1090, 415, 465, 0x0000, 0x0000, 0x0000),
        CS_TEXT_NONE(465, 871),
        CS_TEXT(0x003E, 871, 875, 0x0000, 0x0000, 0x0000),
        CS_TEXT_NONE(875, 936),
        CS_TEXT(0x106B, 936, 946, 0x0000, 0x0000, 0x0000),
    CS_START_SEQ_LIST(1),
        CS_START_SEQ(NA_BGM_MEDALLION_GET, 686, 687, 0x0000, 0x00000000, 0x00000000, 0xFFFFFFC5, 0x00000057, 0x00000000, 0xFFFFFFC5, 0x00000057),
    CS_FADE_OUT_SEQ_LIST(1),
        CS_FADE_OUT_SEQ(CS_FADE_OUT_BGM_MAIN, 550, 600, 0x0000, 0x00000000, 0x00000000, 0xFFFFFFC4, 0x00000066, 0x00000000, 0xFFFFFFC4, 0x00000066),
    CS_CAM_EYE_SPLINE(0, 1241),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 170.1984f, 159, 2758, 43, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 170.1984f, 159, 2758, 43, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 170.1984f, 159, 2409, 43, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 170.1984f, 159, 202, 43, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, 158, 222, 42, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, 158, 149, 42, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, 158, 111, 42, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, 158, 111, 42, 0x0065),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, 158, 111, 42, 0x0061),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 45.399944f, 158, 111, 42, 0x0061),
    CS_CAM_EYE_SPLINE(190, 391),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -91, 18, -158, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -90, 17, -157, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -90, 31, -157, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -90, 37, -157, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -90, 37, -157, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 40.99993f, -90, 37, -157, 0x006F),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 40.99993f, -90, 37, -157, 0x006D),
    CS_CAM_EYE_SPLINE(263, 1354),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 24.399864f, 7, 97, 127, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 24.399864f, 7, 97, 127, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 24.399864f, 7, 97, 127, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 24.399864f, 7, 97, 127, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 24.399864f, 7, 97, 127, 0x0061),
    CS_CAM_EYE_SPLINE(333, 1424),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, -279, 103, 68, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, -279, 103, 68, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, -279, 103, 68, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.399944f, -279, 103, 68, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 45.399944f, -279, 103, 68, 0x0061),
    CS_CAM_EYE_SPLINE(403, 1494),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -52, 35, -83, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -52, 35, -83, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -52, 35, -83, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -52, 35, -83, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 45.39995f, -52, 35, -83, 0x0061),
    CS_CAM_EYE_SPLINE(473, 1716),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -65, 61, -111, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -65, 61, -111, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 45.39995f, -51, 74, -86, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.600006f, 0, 136, 11, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.600006f, 386, 514, 736, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 80.399765f, 579, 156, 1099, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 80.399765f, 579, 156, 1099, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 50.599964f, 579, 156, 1099, 0x0065),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 50.599964f, 579, 156, 1099, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 50.599964f, 579, 156, 1099, 0x0061),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 50.599964f, 579, 156, 1099, 0x0072),
    CS_CAM_EYE_SPLINE(609, 951),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, 13, 854, 2, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, 9, 853, 5, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, -3, 853, 5, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, -9, 853, -6, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, -2, 852, -17, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, 9, 852, -17, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, 16, 852, -6, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 60.0f, 9, 852, 5, 0x0065),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 60.0f, -3, 851, 5, 0x0061),
    CS_CAM_EYE_SPLINE_REL_TO_PLAYER(685, 1866),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 33, -27, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 33, -27, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 68, -26, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 103, -26, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 103, -26, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 103, -26, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 0, 68.599945f, 0, 103, -26, 0x006D),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 0, 68.599945f, 0, 103, -26, 0x0065),
    CS_CAM_AT_SPLINE(0, 1270),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 170.1984f, 154, 2596, 41, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 170.1984f, 154, 2596, 41, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 170.1984f, 154, 2248, 41, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 170.1984f, 154, 42, 41, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 60.2f, 82, 94, 23, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 50.999966f, 33, 79, 0, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.399944f, 30, 62, -14, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 45.399944f, 30, 62, -14, 0x0065),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.399944f, 30, 62, -14, 0x0061),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 45.399944f, 31, 62, -14, 0x0061),
    CS_CAM_AT_SPLINE(190, 420),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 50, 40.99993f, 13, 42, 20, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.99993f, 12, 47, 18, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.99993f, 11, 50, 20, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.99993f, 11, 53, 20, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.99993f, 11, 53, 20, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.99993f, 11, 53, 20, 0x006F),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 40.99993f, 11, 53, 20, 0x006D),
    CS_CAM_AT_SPLINE(263, 1383),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 24.399864f, -42, 17, -150, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 24.399864f, -42, 17, -150, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 24.399864f, -42, 17, -150, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 24.399864f, -42, 17, -150, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 24.399864f, -42, 17, -150, 0x0061),
    CS_CAM_AT_SPLINE(333, 1453),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x01, 30, 45.199944f, -26, 13, -85, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.399944f, -26, 13, -85, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 45.399944f, -26, 13, -85, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.399944f, -26, 13, -85, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 45.399944f, -26, 13, -85, 0x0061),
    CS_CAM_AT_SPLINE(403, 1523),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 30.799892f, -226, 10, -419, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 30.999893f, -226, 10, -419, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 35.59991f, -226, 10, -419, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 40.39993f, -226, 10, -418, 0x0073),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 45.39995f, -226, 10, -418, 0x0061),
    CS_CAM_AT_SPLINE(473, 1745),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.39995f, -218, -88, -396, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.39995f, -218, -88, -396, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 45.39995f, -204, -75, -370, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 65.399994f, -149, -10, -269, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 25, 70.79991f, 287, 239, 551, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 80.399765f, 570, 493, 1083, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 10, 50.399963f, 578, 492, 1097, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 50.599964f, 578, 492, 1097, 0x0065),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 50.599964f, 578, 491, 1097, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 27, 50.599964f, 578, 491, 1097, 0x0061),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 50.599964f, 578, 491, 1097, 0x0072),
    CS_CAM_AT_SPLINE(609, 1000),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 50, 60.0f, 3, 6, -6, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 40, 60.0f, 3, 6, -6, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 50.999966f, 3, 6, -6, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 20, 20.59985f, 3, 6, -6, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 51, 10.799838f, 3, 6, -6, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 50, 10.399838f, 3, 6, -6, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 50, 10.399838f, 3, 6, -6, 0x006D),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 50, 10.199839f, 3, 6, -6, 0x0065),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 50, 10.999838f, 3, 6, -6, 0x0061),
    CS_CAM_AT_SPLINE_REL_TO_PLAYER(685, 1895),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 0, 100, 5, 0x0072),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 0, 101, 6, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 1, 99, 41, 0x002F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 0, 42, 16, 0x0073),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 0, 42, 16, 0x0061),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 1000, 68.599945f, 0, 42, 16, 0x006F),
        CS_CAM_POINT(CS_CAM_CONTINUE, 0x00, 30, 68.599945f, 0, 42, 16, 0x006D),
        CS_CAM_POINT(CS_CAM_STOP, 0x00, 30, 68.599945f, 0, 42, 16, 0x0065),
    CS_END(),
};
// clang-format on