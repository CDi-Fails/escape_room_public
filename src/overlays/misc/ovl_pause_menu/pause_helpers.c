#include "global.h"

typedef struct {
    s16 width;
    s16 height;
} TextureInfo;

static TextureInfo currentTexture = { 0, 0 };

Gfx* Pause_LoadTexRGBA32(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock(displayListHead++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, textureWidth, textureHeight, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_LoadTexRGBA16(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock(displayListHead++, texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, textureWidth, textureHeight, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_LoadTexI8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock(displayListHead++, texture, G_IM_FMT_I, G_IM_SIZ_8b, textureWidth, textureHeight, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_LoadTexIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock(displayListHead++, texture, G_IM_FMT_IA, G_IM_SIZ_8b, textureWidth, textureHeight, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_LoadTexI4(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock_4b(displayListHead++, texture, G_IM_FMT_I, textureWidth, textureHeight, 0, G_TX_NOMIRROR | G_TX_WRAP,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_LoadTexIA4(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight) {
    gDPLoadTextureBlock_4b(displayListHead++, texture, G_IM_FMT_IA, textureWidth, textureHeight, 0, G_TX_NOMIRROR | G_TX_WRAP,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    currentTexture.width = textureWidth;
    currentTexture.height = textureHeight;
    
    return displayListHead;
}

Gfx* Pause_DrawTexRectWithDeltas(Gfx* displayListHead, s16 x, s16 y, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy) {
    gSPTextureRectangle(displayListHead++, x << 2, y << 2, (x + rectWidth) << 2, (y + rectHeight) << 2, G_TX_RENDERTILE,
                        0, 0, dsdx, dtdy);

    return displayListHead;
}

Gfx* Pause_DrawTexRect(Gfx* displayListHead, s16 x, s16 y, s16 rectWidth, s16 rectHeight) {
    u16 auto_dsdx = (1024 * currentTexture.width) / rectWidth; // Scale factor for width
    u16 auto_dtdy = (1024 * currentTexture.height) / rectHeight; // Scale factor for height

    return Pause_DrawTexRectWithDeltas(displayListHead, x, y, rectWidth, rectHeight, auto_dsdx, auto_dtdy);
}