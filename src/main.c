#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "MeowMath.h"
#include "ObjParser.h"
#include "bvh.h"
#include "nkui.h"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static Uint64 last_time = 0;

struct Settings s;

struct Vec3 cameraPos = {0, 1, 0};

static SDL_Texture *renderTexture = NULL;
static uint32_t pixel[2][3840][2160];
static int currentRender = 0;
static SDL_Mutex *mutex;

struct Mesh scene;
struct BVHNode *bvhRoot;

struct Sun sun = {.dir={-1, -1, -0.5}, .color = {5, 5, 5}};

struct PointLight lights[] = {
    {.pos= {0, 2, 0}, .color = {5.0/3, 4.5/3, 4.0/3}}
};

struct InputStates {
    bool keys[1024];
    bool shift;
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

    sun.dir = Vec3Normalize(sun.dir);

    scene = ImportObj("../Objs/Camera.obj");
    bvhRoot = BuildBVH(scene.triangles, scene.triangleCount);

    NkUiInit(window, renderer);

    inputStates.menu = false;
    s.samples = 1;
    s.depth = 2;
    s.renderSamples = 512;
    s.renderDepth = 4;
    s.skyColor = Vec3(0.5, 0.5, 0.8);
    s.width = 160;
    s.height = 144;
    s.renderWidth = 1440;
    s.renderHeight = 1080;
    s.renderMode = false;

    mutex = SDL_CreateMutex();

    SDL_Thread *renderThread = SDL_CreateThread(RenderThread, "RenderThread", NULL);

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

static inline struct HitPoint BVHHit(struct Ray ray, struct BVHNode *node, int depth, int *materialIndex, struct Triangle **t) {
    if (node->left == NULL && node->right == NULL) {
        *materialIndex = node->triangle.materialIndex;
        *t = &node->triangle;
        return IntersectionTriangle(ray, node->triangle);
    }
    struct HitPoint hit = {0};
    hit.distance = INFINITY;
    bool intersection = IntersectionAABB(ray, node->aabb);
#if 0
    if (depth == 9) {
        return (struct HitPoint) {.hit = intersection, .normal = {0, 0, 1}};
    }
#endif
    if (intersection) {
        struct HitPoint h = {0};
        int hitIndex;
        struct Triangle *temp_t = NULL;
        if (node->left != NULL) {
            h = BVHHit(ray, node->left, depth + 1, &hitIndex, &temp_t);
            if (h.hit && h.distance < hit.distance) {
                *materialIndex = hitIndex;
                *t = temp_t;
                hit = h;
            }
        }
        if (node->right != NULL) {
            h = BVHHit(ray, node->right, depth + 1, &hitIndex, &temp_t);
            if (h.hit && h.distance < hit.distance) {
                *materialIndex = hitIndex;
                *t = temp_t;
                hit = h;
            }
        }
    }
    return hit;
}

static inline int calculateHit(struct Ray r, struct HitPoint *hit) {
    hit->distance = INFINITY;
    int index = 0;
    for (int i = 0; i < scene.triangleCount; i++) {
        struct HitPoint h = IntersectionTriangle(r, scene.triangles[i]);
        if (h.hit && h.distance < hit->distance) {
            *hit = h;
            index = i;
        }
    }
    return index;
}

static inline struct Vec3 Shade(struct Ray r, int depth) {
    struct Vec3 color = {0};
    if (depth == 0) {
        return color;
    }
    color = s.skyColor;
    struct HitPoint hit = {0};
    //index = calculateHit(r, &hit);
    int index = -1;
    struct Triangle *triangleID = NULL;
    hit = BVHHit(r, bvhRoot, 0, &index, &triangleID);
    if (hit.hit) {
        struct Vec3 hitcolor = scene.material[index].color;
        if (scene.material[index].texture != NULL) {
            struct Vec3 uvs[3];
            uvs[0] = Vec2ToVec3(triangleID->uv[0]);
            uvs[1] = Vec2ToVec3(triangleID->uv[1]);
            uvs[2] = Vec2ToVec3(triangleID->uv[2]);
            struct Vec2 uv = Vec3ToVec2(InterpolateAttribute(hit, uvs));
            hitcolor = SampleTexture(scene.material[index].texture, uv);
        }
        // Apply sun contribution
        color = hitcolor;
        float d = Vec3Dot(sun.dir, hit.normal);
        if (d < 0)
            d = 0;
        color.x *= sun.color.x * d;
        color.y *= sun.color.y * d;
        color.z *= sun.color.z * d;

        // Apply sun shadows
        struct Ray shadowRay = {0};
        shadowRay.direction = Vec3Mul(sun.dir, -1);
        shadowRay.origin = Vec3Add(hit.point, Vec3Mul(shadowRay.direction, 0.001));
        struct HitPoint shadowHit = {0};
        int dummy = 0;
        struct Triangle *shadowTriangleID = NULL;
        shadowHit = BVHHit(shadowRay, bvhRoot, 0, &dummy, &shadowTriangleID);
        if (shadowHit.hit && shadowTriangleID != triangleID) {
            color.x = 0;
            color.y = 0;
            color.z = 0;
        }

        // Apply lights contribution
        for (int i = 0; i < sizeof(lights)/sizeof(lights[0]); i++) {
            struct Vec3 tempColor = hitcolor;
            struct Vec3 incidentVector = Vec3Sub(hit.point, lights[i].pos);
            float distance = Vec3Length(incidentVector);
            incidentVector = Vec3Normalize(incidentVector);

            // Apply light shadows
            struct Ray shadowRay = {0};
            shadowRay.origin = lights[i].pos;
            shadowRay.direction = incidentVector;
            struct HitPoint shadowHit = {0};
            struct Triangle *shadowTriangleID = NULL;
            shadowHit = BVHHit(shadowRay, bvhRoot, 0, &dummy, &shadowTriangleID);
            if (shadowHit.hit && shadowTriangleID != triangleID) {
                continue;
            }

            float d = Vec3Dot(incidentVector, hit.normal) / (distance * distance);
            if (d < 0)
                d = 0;
            tempColor.x *= lights[i].color.x * d;
            tempColor.y *= lights[i].color.y * d;
            tempColor.z *= lights[i].color.z * d;
            color = Vec3Add(color, tempColor);
        }

        // Apply Reflections
        if (scene.material[index].reflectionColor.x != 0 &&
            scene.material[index].reflectionColor.y != 0 &&
            scene.material[index].reflectionColor.z != 0) {
            struct Ray reflectionRay = {0};
            struct Vec3 incidentVector = Vec3Normalize(Vec3Sub(hit.point, r.origin));
            reflectionRay.direction = Vec3Sub(incidentVector, Vec3Mul(hit.normal, 2*Vec3Dot(hit.normal, incidentVector)));
            reflectionRay.origin = Vec3Add(hit.point, Vec3Mul(reflectionRay.direction, 0.001));
            struct Vec3 refColor = Shade(reflectionRay, depth - 1);
            color.x += refColor.x * scene.material[index].reflectionColor.x;
            color.y += refColor.y * scene.material[index].reflectionColor.y;
            color.z += refColor.z * scene.material[index].reflectionColor.z;
        }

        // Apply diffuse global illumination
        struct Ray diffuseRay = {0};
        diffuseRay.direction = cosWeightedRandomHemisphereDirection(Vec3Mul(hit.normal, -1));
        diffuseRay.origin = Vec3Add(hit.point, Vec3Mul(diffuseRay.direction, 0.001));
        struct Vec3 refDiffuseColor = Shade(diffuseRay, depth - 1);
        color.x += refDiffuseColor.x * hitcolor.x;
        color.y += refDiffuseColor.y * hitcolor.y;
        color.z += refDiffuseColor.z * hitcolor.z;

        // Apply emission
        color.x += scene.material[index].emissionColor.x;
        color.y += scene.material[index].emissionColor.y;
        color.z += scene.material[index].emissionColor.z;
    }
    return color;
}

static inline struct Vec3 renderPixel(int width, int height, int x, int y, struct Vec3 cameraPos, struct Mat3 rot, int samples, int depth) {
    struct Ray r;
    r.origin = cameraPos;
    struct Vec3 color = {0};
    for (int i = 0; i < samples; i++) {
        struct Vec3 rv2 = {0};
        rv2.x = (float)SDL_rand(1000000) / 1000000.0f;
        rv2.y = (float)SDL_rand(1000000) / 1000000.0f;
        if (samples == 1) {
            rv2.x = 0;
            rv2.y = 0;
        }

        r.direction.x = ((float)x + rv2.x) / (float)width - 0.5;
        r.direction.y = ((float)y + rv2.y) / (float)height - 0.5;
        r.direction.y *= -(float)height/(float)width;
        r.direction.z = 0.5;
        r.direction = Vec3Normalize(r.direction);
        r.direction = Mat3Vec3Mul(rot, r.direction);
        color = Vec3Add(color, Shade(r, depth));
    }

    return Vec3Mul(color, 1.0/(float)samples);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    NkUiDraw(&s);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    uint64_t t = SDL_GetTicks();
    float dt = (float)t - (float)last_time;
    dt /= 1000.0f;
    if (dt > 100.0f)
        dt = 100.0f;
    last_time = t;
    float speed = 3.0f;

    struct Vec3 posDelta = {0};

    posDelta.x = inputStates.right * speed * dt;
    posDelta.y = inputStates.up * speed * dt;
    posDelta.z = inputStates.forward * speed * dt;
    struct Mat3 rot = RotMat(inputStates.mouseHorizontal, inputStates.mouseVertical, 0);

    posDelta = Mat3Vec3Mul(rot, posDelta);
    cameraPos = Vec3Add(cameraPos, posDelta);

    int height = s.height;
    int width = s.width;
    if (s.renderMode) {
        height = s.renderHeight;
        width = s.renderWidth;
    }
    SDL_FRect fr = {0, 0, width, height};
    SDL_Rect r = {0, 0, width, height};


    uint32_t *pixels;
    int pitch;
    SDL_LockTexture(renderTexture, &r, (void**)&pixels, &pitch);
    SDL_LockMutex(mutex);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            ((uint32_t*)((void*)pixels + y * pitch))[x] = pixel[s.renderMode ? currentRender : !currentRender][x][y];
        }
    }
    SDL_UnlockMutex(mutex);
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

static int SDLCALL RenderThread(void *ptr) {
    while (true) {
        SDL_LockMutex(mutex);
        bool renderMode = s.renderMode;
        currentRender = !currentRender;
        int currentTexture = currentRender;
        struct Mat3 rot = RotMat(inputStates.mouseHorizontal, inputStates.mouseVertical, 0);
        int height = s.height;
        int width = s.width;
        if (renderMode) {
            height = s.renderHeight;
            width = s.renderWidth;
        }
        uint32_t *pixels;
        int pitch;

        struct Vec3 cameraPosBuf = cameraPos;

        SDL_UnlockMutex(mutex);

        #pragma omp parallel for
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                struct Vec3 color = {0};
                if (renderMode) {
                    color = renderPixel(width, height, x, y, cameraPosBuf, rot, s.renderSamples, s.renderDepth);
                }
                else {
                    color = renderPixel(width, height, x, y, cameraPosBuf, rot, s.samples, s.depth);
                }

                float exposure = 1.5;
                color.x = 1 - exp(-color.x * exposure);
                color.y = 1 - exp(-color.y * exposure);
                color.z = 1 - exp(-color.z * exposure);

                if (color.x > 1) {
                    color.x = 1;
                }
                if (color.y > 1) {
                    color.y = 1;
                }
                if (color.z > 1) {
                    color.z = 1;
                }

                pixel[currentTexture][x][y] = 0xff | ((uint32_t)(color.x*255)) << 24 | ((uint32_t)(color.y*255)) << 16 | ((uint32_t)(color.z*255)) << 8;
            }
        }

        if (renderMode) {
            FILE *f = fopen("/tmp/image.ppm", "w");         // Write image to PPM file.
            fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
            for (int i = 0; i < width * height; i++) {
                int x = i % width;
                int y = i / width;
                uint32_t color = pixel[currentTexture][x][y];
                fprintf(f,"%d %d %d ", (color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff);
            }
            fclose(f);
            s.renderMode = false;
        }
    }
}
