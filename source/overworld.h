#ifndef OVERWORLD_H
#define OVERWORLD_H

#include <3ds.h>
#include <citro3d.h>

/**
 * @brief Initializes the 3D overworld scene.
 * Allocates linear VRAM for vertex buffer data (pc_vertices & fs_vertices)
 * and configures the PICA200 GPU attribute loaders and TexEnv fallback states.
 */
void Overworld_Init(void);

/**
 * @brief Updates overworld logic based on user input.
 * @param kHeld Bitmask of keys currently being held down this frame.
 */
void Overworld_Update(u32 kHeld);

/**
 * @brief Renders the overworld geometry to the specified target.
 * Sets up the projection and modelView matrices, binds the vertex buffer,
 * and executes draw arrays calls for the Pokémon Center and Pokémon Mart.
 * * @param target The Citro3D render target (usually the top left screen layout).
 */
void Overworld_DrawTop(C3D_RenderTarget* target);

/**
 * @brief Frees all allocated memory resources for the overworld.
 * Releases the combined vertex buffer object (VBO) memory block from VRAM.
 */
void Overworld_Free(void);

#endif // OVERWORLD_H