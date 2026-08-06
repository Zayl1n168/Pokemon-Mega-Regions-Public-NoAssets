#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "overworld.h"

// Global shader and program handle
DVLB_s* vshader_dvlb = NULL;
shaderProgram_s program;

C3D_RenderTarget* topTarget = NULL;

void writeHardwareLog(const char* format, ...) {
    FILE* f = fopen("sdmc:/pokemon_mega_debug.log", "a");
    if (!f) return;

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fprintf(f, "\n");
    fflush(f);
    fclose(f);
}

static void initShader(void) {
    writeHardwareLog("[SYS INIT] Parsing Shader DVLB binary...");
    vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    if (!vshader_dvlb) {
        writeHardwareLog("[SYS CRITICAL] Failed to parse vshader.shbin!");
        return;
    }

    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);
    writeHardwareLog("[SYS INIT] Vertex Shader bound successfully.");
}

int main(int argc, char* argv[]) {
    // Clear old log file
    FILE* f = fopen("sdmc:/pokemon_mega_debug.log", "w");
    if (f) {
        fprintf(f, "==================================================\n");
        fprintf(f, "=== POKEMON MEGA UNBUFFERED TRACE SESSION ===\n");
        fprintf(f, "==================================================\n");
        fclose(f);
    }

    writeHardwareLog("[SYS INIT] Initializing system graphics...");
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    romfsInit();

    topTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (topTarget) {
        C3D_RenderTargetSetOutput(topTarget, GFX_TOP, GFX_LEFT,
            GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
            GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    }

    initShader();
    Overworld_Init();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        Overworld_Update(kHeld);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(topTarget, C3D_CLEAR_ALL, 0x68B0E8FF, 0);
        C3D_FrameDrawOn(topTarget);

        Overworld_DrawTop(topTarget);

        C3D_FrameEnd(0);
    }

    Overworld_Free();
    
    if (vshader_dvlb) DVLB_Free(vshader_dvlb);
    shaderProgramFree(&program);

    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
