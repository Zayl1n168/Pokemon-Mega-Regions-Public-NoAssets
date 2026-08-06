#ifndef INTRO_H
#define INTRO_H

#include <3ds.h>
#include <citro2d.h>

void Intro_LoadAssets();
void Intro_Update(u32 kDown);
void Intro_DrawTop(C3D_RenderTarget* target);
void Intro_DrawBottom(C3D_RenderTarget* target);
void Intro_UnloadAssets();

#endif
