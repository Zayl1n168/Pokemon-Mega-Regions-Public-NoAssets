#ifndef GLOBALS_H
#define GLOBALS_H

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

// -----------------------------------------------------------------------------
// ENGINE STATE MACHINE DEFINITIONS
// -----------------------------------------------------------------------------
typedef enum {
    STATE_DISCLAIMER,
    STATE_TITLE,
    STATE_FADE_OUT,
    STATE_CHARACTER_SELECT,
    STATE_NAME_INPUT,
    STATE_ARCEUS_INTRO, 
    STATE_OVERWORLD,    // The grass world!
    STATE_SAVING
} GameState;

// Shared state machine variables (Must be explicitly instantiated in main.cpp)
extern GameState currentState;
extern GameState nextState;
extern float fadeAlpha;
extern int selectedGender;
extern char playerName[16]; // Matched to 16 bytes for safe structural stack alignment

// -----------------------------------------------------------------------------
// HARDWARE GRAPHICS & AUDIO ENGINE GLOBALS
// -----------------------------------------------------------------------------
// Shared shader program pipeline initialized in main.cpp
extern shaderProgram_s program; 

// Shared audio streaming trackers managed by the hardware sound mixer
extern u8* audioBuffer;
extern u32 audioBufferSize;

// Shared engine background music controllers
void playMusic(const char* path);

// -----------------------------------------------------------------------------
// TEXT UTILITY RENDERING WRAPPERS
// -----------------------------------------------------------------------------
void drawStaticText(float x, float y, const char* text, u32 color, float size);
void drawArceusText(float x, float y, const char* text, u32 color, float size, int count);

#endif // GLOBALS_H
