#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 

#include "overworld.h"

extern void writeHardwareLog(const char* format, ...);
extern shaderProgram_s program;

typedef struct {
    char vboName[64];
    char t3xName[64];
    C3D_Tex texture;
    bool hasValidTexture;
    void* vboData;
    size_t vertexCount;
    size_t vboSizeBytes;
    size_t texSizeBytes;
    float posX, posY, posZ;
} OverworldModel;

#define MAX_SCENE_OBJECTS 64
static OverworldModel sceneModels[MAX_SCENE_OBJECTS];
static int activeObjectCount = 0;

static bool assetsLoaded = false;
static u64 frameCounter = 0;
static int uLoc_projection = -1;
static int uLoc_modelView  = -1;
static C3D_Mtx projection;

static C3D_Tex fallbackWhiteTex;

// Camera state
static float camPosX = 0.0f;
static float camPosY = 10.0f;  
static float camPosZ = 40.0f;  
static float camPitch = 15.0f; 

// Global scale factor for world models
static const float GLOBAL_MODEL_SCALE = 0.02f;

static void clamp(float* value, float min, float max) {
    *value = fmaxf(*value, min);
    *value = fminf(*value, max);
}

static void createFallbackTexture(void) {
    writeHardwareLog("[TEX INIT] Allocating fallback white texture...");
    u32* texData = (u32*)linearAlloc(sizeof(u32) * 64);
    if (texData) {
        for (int i = 0; i < 64; i++) texData[i] = 0xFFFFFFFF;
        C3D_TexInit(&fallbackWhiteTex, 8, 8, GPU_RGBA8);
        C3D_TexUpload(&fallbackWhiteTex, texData);
        C3D_TexSetFilter(&fallbackWhiteTex, GPU_NEAREST, GPU_NEAREST);
        linearFree(texData);
        writeHardwareLog("[TEX INIT] Fallback white texture allocated.");
    }
}

static bool loadTexture(C3D_Tex* tex, const char* path, size_t* outTexSize) {
    *outTexSize = 0;
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    *outTexSize = size;

    if (size < 512) {
        fclose(f);
        return false;
    }

    void* buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return false;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    tex->data = NULL; 
    Tex3DS_Texture t3x = Tex3DS_TextureImport(buffer, size, tex, NULL, false);
    free(buffer);

    if (!t3x) {
        if (tex->data) { 
            free(tex->data);
            tex->data = NULL;
        }
        return false;
    }
    Tex3DS_TextureFree(t3x); 

    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);
    return true;
}

static void RegisterSceneAsset(const char* vboName, const char* t3xName, float x, float y, float z) {
    if (activeObjectCount >= MAX_SCENE_OBJECTS) return;

    OverworldModel* model = &sceneModels[activeObjectCount];
    snprintf(model->vboName, sizeof(model->vboName), "%s", vboName);
    snprintf(model->t3xName, sizeof(model->t3xName), "%s", t3xName);
    model->vboData = NULL; 
    model->texture.data = NULL;
    model->hasValidTexture = false;
    model->posX = x; model->posY = y; model->posZ = z;

    char vboPath[128], t3xPath[128];
    snprintf(vboPath, sizeof(vboPath), "romfs:/gfx/%s", vboName);
    snprintf(t3xPath, sizeof(t3xPath), "romfs:/gfx/%s", t3xName);

    FILE* f = fopen(vboPath, "rb");
    if (!f) {
        writeHardwareLog("[ASSET FAIL] Could not open %s", vboPath);
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    rewind(f);

    model->vboSizeBytes = fileSize;
    model->vertexCount = fileSize / (sizeof(float) * 5);

    // Ensure 8-byte aligned linear memory allocation for VBO
    size_t alignedSize = (fileSize + 15) & ~15;
    model->vboData = linearAlloc(alignedSize);
    if (!model->vboData) {
        fclose(f);
        return;
    }

    fread(model->vboData, 1, fileSize, f);
    fclose(f);

    model->hasValidTexture = loadTexture(&model->texture, t3xPath, &model->texSizeBytes);

    writeHardwareLog("[ASSET REGISTERED] Obj #%d: %s (%zu verts) | Tex: %s (%s)", 
                       activeObjectCount, model->vboName, model->vertexCount,
                       model->t3xName, model->hasValidTexture ? "LOADED" : "FALLBACK");

    activeObjectCount++;
}

void Overworld_Init(void) {
    if (assetsLoaded) return;

    writeHardwareLog("=== OVERWORLD_INIT START ===");

    createFallbackTexture();

    RegisterSceneAsset("mst_okl_baked.vbo",       "mst_okl_baked.t3x",       -5.0f, 0.0f, -5.0f);
    RegisterSceneAsset("mst_rdh_baked.vbo",       "mst_rdh_baked.t3x",        5.0f, 0.0f, -5.0f);
    RegisterSceneAsset("mst_grh_baked.vbo",       "mst_grh_baked.t3x",        5.0f, 0.0f,  2.0f);
    RegisterSceneAsset("poke_c_flower_baked.vbo", "poke_c_flower_baked.t3x",  0.0f, 0.0f,  0.0f);
    RegisterSceneAsset("map15_01kokage_baked.vbo","map15_01kokage_baked.t3x", -7.0f, 0.0f, -10.0f);
    RegisterSceneAsset("map15_02kokage_baked.vbo","map15_02kokage_baked.t3x",  7.0f, 0.0f, -10.0f);
    RegisterSceneAsset("board_a_baked.vbo",       "board_a_baked.t3x",        -2.0f, 0.0f, -2.0f);

    if (program.vertexShader) {
        uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
        uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
    }

    // Standard Citro3D Perspective Projection
    Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.1f, 1000.0f, false);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_NONE);

    assetsLoaded = true;
    writeHardwareLog("=== OVERWORLD_INIT END ===");
}

void Overworld_Update(u32 kHeld) {
    const float camRotSpeed = 0.5f;
    const float camMoveSpeed = 0.2f;

    if (kHeld & KEY_DUP)    camPitch += camRotSpeed;
    if (kHeld & KEY_DDOWN)  camPitch -= camRotSpeed;
    if (kHeld & KEY_DLEFT)  camPosZ  += camMoveSpeed; 
    if (kHeld & KEY_DRIGHT) camPosZ  -= camMoveSpeed;

    clamp(&camPitch, -89.0f, 89.0f);
}

void Overworld_DrawTop(C3D_RenderTarget* target) {
    frameCounter++;

    if (!assetsLoaded || activeObjectCount == 0 || uLoc_projection == -1 || uLoc_modelView == -1) return;

    C3D_DepthTest(true, GPU_LEQUAL, GPU_WRITE_ALL);

    // 1. Send Projection Matrix to GPU
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    // 2. Build Camera View Matrix using Citro3D LookAt
    C3D_Mtx viewMatrix;
    float rad = camPitch * (3.14159265f / 180.0f);
    F3Vector cameraPos = F3Vec_New(camPosX, camPosY, camPosZ);
    F3Vector cameraTarget = F3Vec_New(camPosX, camPosY - sinf(rad) * 10.0f, camPosZ - cosf(rad) * 10.0f);
    F3Vector cameraUp = F3Vec_New(0.0f, 1.0f, 0.0f);
    Mtx_LookAt(&viewMatrix, cameraPos, cameraTarget, cameraUp, false);

    // 3. Configure Vertex Attributes
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // Position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // UV

    for (int i = 0; i < activeObjectCount; i++) {
        if (!sceneModels[i].vboData || sceneModels[i].vertexCount == 0) continue;

        // Texture Binding
        if (sceneModels[i].hasValidTexture && sceneModels[i].texture.data) {
            C3D_TexBind(0, &sceneModels[i].texture);
        } else {
            C3D_TexBind(0, &fallbackWhiteTex);
        }

        // Build ModelView Matrix (Translate -> Scale)
        C3D_Mtx modelMatrix, modelView;
        Mtx_Identity(&modelMatrix);
        Mtx_Translate(&modelMatrix, sceneModels[i].posX, sceneModels[i].posY, sceneModels[i].posZ, true);
        Mtx_Scale(&modelMatrix, GLOBAL_MODEL_SCALE, GLOBAL_MODEL_SCALE, GLOBAL_MODEL_SCALE);

        // ModelView = View * Model
        Mtx_Multiply(&modelView, &viewMatrix, &modelMatrix);

        // Upload ModelView matrix to shader
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);

        // Bind VBO Buffer
        C3D_BufInfo* bufInfo = C3D_GetBufInfo();
        BufInfo_Init(bufInfo);
        BufInfo_Add(bufInfo, sceneModels[i].vboData, sizeof(float) * 5, 2, 0x10); 

        // Submit Draw Arrays to PICA200 GPU
        C3D_DrawArrays(GPU_TRIANGLES, 0, sceneModels[i].vertexCount);
    }
}

void Overworld_Free(void) {
    if (!assetsLoaded) return;
    C3D_TexDelete(&fallbackWhiteTex);
    
    for (int i = 0; i < activeObjectCount; i++) {
        if (sceneModels[i].vboData) linearFree(sceneModels[i].vboData);
        if (sceneModels[i].hasValidTexture && sceneModels[i].texture.data) {
            C3D_TexDelete(&sceneModels[i].texture);
        }
    }
    activeObjectCount = 0;
    assetsLoaded = false;
}
