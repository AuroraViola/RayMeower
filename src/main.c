#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "MeowMath.h"
#include "ObjParser.h"
#include "bvh.h"
#include "nkui.h"
#include "vk.h"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static Uint64 last_time = 0;

struct Settings s;

struct Vec3 cameraPos = {0, 1, 0};

static SDL_Texture *renderTexture = NULL;
static uint32_t pixel[2][3840][2160];
static struct Vec3 tempFrameBuffer[3840][2160] = {0};
static int currentRender = 0;

struct Scene scene;

struct InputStates {
    bool keys[1024];
    bool shift;
    bool screenKey;
    float forward;
    float right;
    float up;
    float mouseVertical;
    float mouseHorizontal;
    bool menu;
};

struct InputStates inputStates = {0};

static int SDLCALL RenderThread(void *ptr);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

    SDL_SetAppMetadata("RayMeower", "0.0.1", "io.auroraviola.raymeower");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("RayMeower", 320, 240, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_SetWindowResizable(window, true);

    renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 3840, 2160);
    SDL_SetTextureScaleMode(renderTexture, SDL_SCALEMODE_NEAREST);

    last_time = SDL_GetTicks();


    scene.mesh = ImportObj("../Objs/Test.obj");
    scene.bvhRoot = BuildBVH(scene.mesh.triangles, scene.mesh.triangleCount);

    scene.sun = (struct Sun){.dir={0, -1, 0}, .color = {1.0, 1.0, 1.0}, .intensity = 5.0};
    scene.sun.dir = Vec3Normalize(scene.sun.dir);
    scene.sun.angle = 0.275;
    scene.lightsCount = 0;

    NkUiInit(window, renderer, &scene);

    inputStates.menu = false;
    s.depth = 6;
    s.skyColor = Vec3(0.5, 0.5, 0.8);
    s.width = 1024;
    s.height = 768;
    s.selectedMaterial = 0;
    s.sunElevation = 30;
    s.sunRotation = 40;
    s.cumSamples = 1;
    struct Mat3 sunRotationMat = RotMat((s.sunRotation * PI / 180), 0, (s.sunElevation * PI / 180));
    scene.sun.dir = Mat3Vec3Mul(sunRotationMat, Vec3(-1, 0, 0));

    CreateVk(3840, 2160);
    struct LinearBVH linearBVH = LinearizeBVH(scene.bvhRoot);
    UpdateBvhBuffer(linearBVH.buffer);
    UploadMaterials(scene.mesh.material, scene.mesh.materialCount);
    scene.mesh.materialGpu = gpuMaterials;
    UploadPointLights(scene.lights, scene.lightsCount);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        if (event->key.key < 1024) {
            inputStates.keys[event->key.key] = SDL_EVENT_KEY_DOWN == event->type;
        }
        else if (event->key.key == SDLK_LSHIFT) {
            inputStates.shift = SDL_EVENT_KEY_DOWN == event->type;
        }
        else if (event->key.key == SDLK_F2) {
            inputStates.screenKey = SDL_EVENT_KEY_DOWN == event->type;
        }
        inputStates.forward = 0;
        inputStates.right = 0;
        inputStates.up = 0;
        if (inputStates.keys[SDLK_W]) {
            inputStates.forward += 1;
        }
        if (inputStates.keys[SDLK_S]) {
            inputStates.forward -= 1;
        }
        if (inputStates.keys[SDLK_D]) {
            inputStates.right += 1;
        }
        if (inputStates.keys[SDLK_A]) {
            inputStates.right -= 1;
        }
        if (inputStates.keys[SDLK_SPACE]) {
            inputStates.up += 1;
        }
        if (inputStates.shift) {
            inputStates.up -= 1;
        }
        if (inputStates.keys[SDLK_ESCAPE]) {
            inputStates.menu = !inputStates.menu;
            SDL_SetWindowRelativeMouseMode(window, !inputStates.menu);
        }
        if (inputStates.screenKey) {
            FILE *f = fopen("./image.ppm", "w");
            fprintf(f, "P3\n%d %d\n%d\n", s.width, s.height, 255);
            for (int i = 0; i < s.width * s.height; i++) {
                uint32_t color = PackColor(Reinhard(frameBuffer[i], 1.5));
                fprintf(f,"%d %d %d ", (color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff);
            }
            fclose(f);
        }
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && !inputStates.menu) {
        inputStates.mouseVertical += event->motion.yrel * 0.001;
        inputStates.mouseHorizontal += event->motion.xrel * 0.001;
    }
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    nk_sdl_handle_event(ctx, event);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    uint64_t t = SDL_GetTicksNS();
    float dt = (float)t - (float)last_time;
    dt /= 1000000000.0f;
    last_time = t;
    float speed = 6.0f;

    nk_input_end(ctx);

    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);
    NkFpsDraw(dt, w, h);
    if (inputStates.menu)
        NkMenuDraw(&s, &scene);

    if (dt > 0.1f)
        dt = 0.1f;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    s.cumFact = 1.0/s.cumSamples;
    if (!inputStates.menu) {
        static float prevMouseHorizontal = 0.0f;
        static float prevMouseVertical = 0.0f;
        struct Vec3 posDelta = {0};
        posDelta.x = inputStates.right * speed * dt;
        posDelta.y = inputStates.up * speed * dt;
        posDelta.z = inputStates.forward * speed * dt;
        struct Mat3 rot = RotMat(inputStates.mouseHorizontal, inputStates.mouseVertical, 0);
        if (prevMouseHorizontal != inputStates.mouseHorizontal || prevMouseVertical != inputStates.mouseVertical || posDelta.x != 0 || posDelta.y != 0 || posDelta.z != 0) {
            prevMouseVertical = inputStates.mouseVertical;
            prevMouseHorizontal = inputStates.mouseHorizontal;
            s.cumFact = 1;
            s.cumSamples = 1;
        }
        posDelta = Mat3Vec3Mul(rot, posDelta);
        cameraPos = Vec3Add(cameraPos, posDelta);
    }
    else {
        s.cumFact = 1;
        s.cumSamples = 1;
    }
    s.cumSamples++;


    SDL_FRect fr = {0, 0, s.width, s.height};
    SDL_Rect r = {0, 0, s.width, s.height};

    RunVk(s.width, s.height, cameraPos, RotMat(inputStates.mouseHorizontal, inputStates.mouseVertical, 0), (uint32_t)SDL_rand(100000), &scene, &s);
    uint32_t *pixels;
    int pitch;
    SDL_LockTexture(renderTexture, &r, (void**)&pixels, &pitch);
    for (int x = 0; x < s.width; x++) {
        for (int y = 0; y < s.height; y++) {
            // TODO: make this a compute shader
            struct Vec3 color = frameBuffer[x+y*s.width];
            ((uint32_t*)((void*)pixels + y * pitch))[x] = PackColor(Reinhard(color, 1.5));
        }
    }
    SDL_UnlockTexture(renderTexture);
    SDL_RenderTexture(renderer, &renderTexture[0], &fr, NULL);

    nk_sdl_render(ctx, AA);
    nk_sdl_update_TextInput(ctx);

    SDL_RenderPresent(renderer);

    nk_input_begin(ctx);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
}