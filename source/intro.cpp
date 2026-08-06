#include "intro.h"
#include "globals.h"
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>

// -----------------------------------------------------------------------------
// EXTERNAL LINKAGE REFERENCE MAPS (Instantiated in main.cpp)
// -----------------------------------------------------------------------------
// Resolved duplicate definition hazard by utilizing pure extern maps here
extern GameState currentState;
extern GameState nextState;
extern float fadeAlpha;
extern int selectedGender;
extern char playerName[16];

// -----------------------------------------------------------------------------
// LOCAL SCENE CONTEXT ASSETS
// -----------------------------------------------------------------------------
C2D_SpriteSheet titleSheet = NULL;
C2D_SpriteSheet arceusSheet = NULL;
C2D_SpriteSheet arceusSpriteSheet = NULL;
C2D_SpriteSheet playerMSheet = NULL;
C2D_SpriteSheet playerFSheet = NULL;

const char* arceusLines[] = {
    "Welcome traveller... or should I say the one who wanders?",
    "You have transcended the very fabric of space and time to be here.",
    "Long has it been since a soul from your world has stepped into this realm,",
    "a world where the boundaries of reality are as thin as a whisper.",
    "Go fourth, seek the truth of this land.",
    "May your journey bring balance to the chaos that stirs...",
    "Good luck."
};

int totalLines = 7; 
int currentLine = 0;
int charactersToShow = 0;
int textTimer = 0;
bool lineFinished = false;
bool stateInitialized = false;

// Optimization: Single persistent heap tracking vector for all 2D text operations
static C2D_TextBuf dynamicBuf = NULL;

// -----------------------------------------------------------------------------
// TEXT UTILITY RENDERING WRAPPERS
// -----------------------------------------------------------------------------
void drawStaticText(float x, float y, const char* text, u32 color, float size) {
    if (!text || text[0] == '\0') return;

    // FIXED: Lazy-initialize the single shared buffer array if missing
    if (!dynamicBuf) {
        dynamicBuf = C2D_TextBufNew(1024);
    }
    
    C2D_Text sText;
    // Parse using the persistent workspace to eliminate real-time heap allocations
    C2D_TextParse(&sText, dynamicBuf, text);
    C2D_TextOptimize(&sText);
    C2D_DrawText(&sText, C2D_WithColor, x, y, 0.9f, size, size, color);
}

void drawArceusText(float x, float y, const char* text, u32 color, float size, int count) {
    if (!text || count <= 0) return;
    
    if (!dynamicBuf) {
        dynamicBuf = C2D_TextBufNew(1024);
    }

    C2D_Text tText;
    char temp[1024]; 

    int toCopy = (count > (int)strlen(text)) ? (int)strlen(text) : count;
    if (toCopy >= 1024) toCopy = 1023;
    
    memcpy(temp, text, toCopy);
    temp[toCopy] = '\0'; 

    C2D_TextParse(&tText, dynamicBuf, temp);
    C2D_TextOptimize(&tText);

    C2D_DrawText(&tText, C2D_WithColor | C2D_WordWrap, x, y, 1.0f, size, size, color, 280.0f);
}

// -----------------------------------------------------------------------------
// SCENE LIFECYCLE MANAGEMENT
// -----------------------------------------------------------------------------
void Intro_LoadAssets() {
    titleSheet = C2D_SpriteSheetLoad("romfs:/title_bg.t3x");
    arceusSheet = C2D_SpriteSheetLoad("romfs:/arceus_bg.t3x");
    arceusSpriteSheet = C2D_SpriteSheetLoad("romfs:/arceus_sprite.t3x");
    playerMSheet = C2D_SpriteSheetLoad("romfs:/player_m.t3x");
    playerFSheet = C2D_SpriteSheetLoad("romfs:/player_f.t3x");
}

void Intro_Update(u32 kDown) {
    // FIXED: Explicitly handle STATE_DISCLAIMER loop evaluation to unblock the boot freeze
    if (currentState == STATE_DISCLAIMER) {
        if (kDown & KEY_A) {
            currentState = STATE_TITLE;
        }
        return;
    }

    if (currentState == STATE_TITLE) {
        if (!stateInitialized) { playMusic("romfs:/theme.wav"); stateInitialized = true; }
        if (kDown & KEY_A) { nextState = STATE_CHARACTER_SELECT; currentState = STATE_FADE_OUT; stateInitialized = false; }
    }
    else if (currentState == STATE_CHARACTER_SELECT) {
        if (!stateInitialized) { playMusic("romfs:/selection.wav"); stateInitialized = true; }
        if (kDown & KEY_LEFT) selectedGender = 0;
        if (kDown & KEY_RIGHT) selectedGender = 1;
        if (kDown & KEY_A) { nextState = STATE_ARCEUS_INTRO; currentState = STATE_FADE_OUT; stateInitialized = false; }
    }
    else if (currentState == STATE_ARCEUS_INTRO) {
        textTimer++;
        if (textTimer >= 3) {
            if (charactersToShow < (int)strlen(arceusLines[currentLine])) charactersToShow++;
            else lineFinished = true;
            textTimer = 0;
        }
        if (lineFinished && (kDown & KEY_A)) {
            currentLine++;
            if (currentLine >= totalLines) { 
                nextState = STATE_OVERWORLD; 
                currentState = STATE_FADE_OUT; 
            }
            else { 
                charactersToShow = 0; 
                lineFinished = false; 
            }
        }
    }
}

void Intro_DrawTop(C3D_RenderTarget* target) {
    C2D_SceneBegin(target);
    
    if (currentState == STATE_TITLE && titleSheet != NULL) {
        C2D_Image img = C2D_SpriteSheetGetImage(titleSheet, 0);
        C2D_DrawImageAt(img, 0, 0, 0.5f, NULL, 400.0f / img.subtex->width, 240.0f / img.subtex->height);
    } 
    else if (currentState == STATE_CHARACTER_SELECT) {
        C2D_DrawRectSolid(0, 0, 0.1f, 400, 240, C2D_Color32(0, 0, 0, 255));
        
        if (playerMSheet != NULL) {
            C2D_DrawImageAt(C2D_SpriteSheetGetImage(playerMSheet, 0), 60, 40, 0.6f, NULL, 0.8f, 0.8f);
            if(selectedGender == 0) C2D_DrawRectSolid(60, 175, 0.7f, 80, 3, C2D_Color32(255, 255, 255, 255));
        }
        if (playerFSheet != NULL) {
            C2D_DrawImageAt(C2D_SpriteSheetGetImage(playerFSheet, 0), 220, 40, 0.6f, NULL, 0.8f, 0.8f);
            if(selectedGender == 1) C2D_DrawRectSolid(220, 175, 0.7f, 80, 3, C2D_Color32(255, 255, 255, 255));
        }
    }
    else if (currentState == STATE_ARCEUS_INTRO && arceusSheet != NULL) {
        C2D_Image bg = C2D_SpriteSheetGetImage(arceusSheet, 0);
        C2D_DrawImageAt(bg, 0, 0, 0.5f, NULL, 400.0f / bg.subtex->width, 240.0f / bg.subtex->height);
        if (arceusSpriteSheet != NULL) {
            C2D_DrawImageAt(C2D_SpriteSheetGetImage(arceusSpriteSheet, 0), 120, 40, 0.6f, NULL, 0.7f, 0.7f);
        }
    }
}

void Intro_DrawBottom(C3D_RenderTarget* target) {
    // Clear structural text buffer references before building frame layout groupings
    if (dynamicBuf) C2D_TextBufClear(dynamicBuf);

    C2D_SceneBegin(target);
    if (currentState == STATE_DISCLAIMER) {
        drawStaticText(40, 60, "POKEMON MEGA REGIONS", C2D_Color32(255, 255, 255, 255), 0.8f);
        drawStaticText(40, 100, "This is a fan project.", C2D_Color32(200, 200, 200, 255), 0.6f);
        drawStaticText(40, 180, "Press (A) to Start", C2D_Color32(255, 255, 255, 255), 0.7f);
    }
    else if (currentState == STATE_TITLE) {
        drawStaticText(80, 100, "Press (A) to Start", C2D_Color32(255, 255, 255, 255), 0.75f);
    }
    else if (currentState == STATE_CHARACTER_SELECT) {
        drawStaticText(60, 40, "CHOOSE YOUR TRAINER", C2D_Color32(255, 255, 255, 255), 0.8f);
        drawStaticText(60, 180, "<- BOY", (selectedGender == 0) ? C2D_Color32(255, 255, 0, 255) : C2D_Color32(255, 255, 255, 255), 0.7f);
        drawStaticText(180, 180, "GIRL ->", (selectedGender == 1) ? C2D_Color32(255, 255, 0, 255) : C2D_Color32(255, 255, 255, 255), 0.7f);
    }
    else if (currentState == STATE_ARCEUS_INTRO) {
        C2D_DrawRectSolid(10, 140, 0.5f, 300, 90, C2D_Color32(255, 255, 255, 255)); 
        C2D_DrawRectSolid(12, 142, 0.55f, 296, 86, C2D_Color32(30, 30, 30, 255)); 
        drawArceusText(20, 155, arceusLines[currentLine], C2D_Color32(255, 255, 255, 255), 0.55f, charactersToShow);
    }
}

void Intro_UnloadAssets() {
    if (titleSheet) { C2D_SpriteSheetFree(titleSheet); titleSheet = NULL; }
    if (arceusSheet) { C2D_SpriteSheetFree(arceusSheet); arceusSheet = NULL; }
    if (arceusSpriteSheet) { C2D_SpriteSheetFree(arceusSpriteSheet); arceusSpriteSheet = NULL; }
    if (playerMSheet) { C2D_SpriteSheetFree(playerMSheet); playerMSheet = NULL; }
    if (playerFSheet) { C2D_SpriteSheetFree(playerFSheet); playerFSheet = NULL; }
    
    if (dynamicBuf) { C2D_TextBufDelete(dynamicBuf); dynamicBuf = NULL; }
}
