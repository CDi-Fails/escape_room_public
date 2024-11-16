#ifndef PAUSE_HELPERS_H
#define PAUSE_HELPERS_H

Gfx* Pause_LoadTexRGBA32(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_LoadTexRGBA16(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_LoadTexI8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_LoadTexIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_LoadTexI4(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_LoadTexIA4(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight);
Gfx* Pause_DrawTexRectWithDeltas(Gfx* displayListHead, s16 x, s16 y, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy);
Gfx* Pause_DrawTexRect(Gfx* displayListHead, s16 x, s16 y, s16 rectWidth, s16 rectHeight);

#endif