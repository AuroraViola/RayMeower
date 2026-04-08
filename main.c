#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "MeowMath.h"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static Uint64 last_time = 0;

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240

struct Vec3 cameraPos = {0, 6.5, -10};
struct Sphere s[] = {
    {.reflectionColor = {0.0, 0.0, 0.0}, .origin = {0, -5000, 40}, .radius = 4980.0, .color = {0.7, 0.7, 0.7}},
    {.reflectionColor = {0.0, 0.0, 0.0}, .origin = {-40, 0, 50}, .radius = 20.0, .color = {0.9, 0.2, 0.2}},
    {.reflectionColor = {0.0, 0.0, 0.0}, .origin = {0, 0, 50}, .radius = 20.0, .color = {0.2, 0.9, 0.2}},
    {.reflectionColor = {0.0, 0.0, 0.0}, .origin = {40, 0, 50}, .radius = 20.0, .color = {0.2, 0.2, 0.9}},
};

struct Sun sun = {.dir={0, -0.2, 1}, .color = {1.0, 1.0, 1.0}};

struct PointLight lights[] = {
};

struct InputStates {
    bool keys[1024];
    bool shift;
    float forward;
    float right;
    float up;
    float mouseVertical;
    float mouseHorizontal;
};

struct InputStates inputStates = {0};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("RayMeower", "0.0.1", "io.auroraviola.raymeower");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("RayMeower", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetWindowRelativeMouseMode(window, true);

    last_time = SDL_GetTicks();

    sun.dir = Vec3Normalize(sun.dir);

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
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        inputStates.mouseVertical += event->motion.yrel * 0.001;
        inputStates.mouseHorizontal += event->motion.xrel * 0.001;
    }
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

static inline int calculateHit(struct Ray r, struct HitPoint *hit) {
    hit->distance = INFINITY;
    int index = 0;
    for (int i = 0; i < sizeof(s)/sizeof(s[0]); i++) {
        struct HitPoint h = IntersectionSphere(r, s[i]);
        if (h.hit && h.distance < hit->distance) {
            *hit = h;
            index = i;
        }
    }
    return index;
}

static inline struct Vec3 shade(struct Ray r, int depth) {
    struct Vec3 color = {0};
    if (depth == 0) {
        return color;
    }
    color = Vec3(0.2, 0.2, 0.5);
    struct HitPoint hit = {0};
    int index = calculateHit(r, &hit);
    if (hit.hit) {
        // Apply sun contribution
        color = s[index].color;
        float d = Vec3Dot(sun.dir, hit.normal);
        if (d < 0)
            d = 0;
        color.x *= sun.color.x * d;
        color.y *= sun.color.y * d;
        color.z *= sun.color.z * d;

        // Apply sun shadows
        struct Ray shadowRay = {0};
        shadowRay.direction = Vec3Mul(sun.dir, -1);
        shadowRay.origin = Vec3Add(hit.point, Vec3Mul(shadowRay.direction, 0.1));
        struct HitPoint shadowHit = {0};
        int shadowIndex = calculateHit(shadowRay, &shadowHit);
        if (shadowHit.hit && shadowIndex != index) {
            color.x = 0;
            color.y = 0;
            color.z = 0;
        }

        // Apply lights contribution
        for (int i = 0; i < sizeof(lights)/sizeof(lights[0]); i++) {
            struct Vec3 tempColor = s[index].color;
            struct Vec3 incidentVector = Vec3Sub(hit.point, lights[i].pos);
            float distance = Vec3Length(incidentVector);
            incidentVector = Vec3Normalize(incidentVector);

            // Apply light shadows
            struct Ray shadowRay = {0};
            shadowRay.origin = lights[i].pos;
            shadowRay.direction = incidentVector;
            struct HitPoint shadowHit = {0};
            int shadowIndex = calculateHit(shadowRay, &shadowHit);
            if (shadowHit.hit && shadowIndex != index) {
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
        struct Ray reflectionRay = {0};
        struct Vec3 incidentVector = Vec3Normalize(Vec3Sub(hit.point, r.origin));
        reflectionRay.direction = Vec3Sub(incidentVector, Vec3Mul(hit.normal, 2*Vec3Dot(hit.normal, incidentVector)));
        reflectionRay.origin = Vec3Add(hit.point, Vec3Mul(reflectionRay.direction, 0.1));
        struct Vec3 refColor = shade(reflectionRay, depth - 1);
        color.x += refColor.x * s[index].reflectionColor.x;
        color.y += refColor.y * s[index].reflectionColor.y;
        color.z += refColor.z * s[index].reflectionColor.z;

        // Apply diffuse global illumination
        struct Ray diffuseRay = {0};
        diffuseRay.direction = cosWeightedRandomHemisphereDirection(Vec3Mul(hit.normal, -1));
        diffuseRay.origin = Vec3Add(hit.point, Vec3Mul(diffuseRay.direction, 0.1));
        struct Vec3 refDiffuseColor = shade(diffuseRay, depth - 1);
        color.x += refDiffuseColor.x * s[index].color.x;
        color.y += refDiffuseColor.y * s[index].color.y;
        color.z += refDiffuseColor.z * s[index].color.z;
    }
    return color;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    uint64_t t = SDL_GetTicks();
    float dt = (float)t - (float)last_time;
    dt /= 1000.0f;
    last_time = t;
    float speed = 33.0f;

    struct Vec3 posDelta = {0};

    posDelta.x = inputStates.right * speed * dt;
    posDelta.y = inputStates.up * speed * dt;
    posDelta.z = inputStates.forward * speed * dt;
    struct Mat3 rot = RotMat(inputStates.mouseHorizontal, inputStates.mouseVertical, 0);

    posDelta = Mat3Vec3Mul(rot, posDelta);
    cameraPos = Vec3Add(cameraPos, posDelta);


    for (int x = 0; x < WINDOW_WIDTH; x++) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            struct Ray r;
            r.direction.x = (float)x / (float)WINDOW_WIDTH - 0.5;
            r.direction.y = (float)y / (float)WINDOW_HEIGHT - 0.5;
            r.direction.y *= -(float)WINDOW_HEIGHT/(float)WINDOW_WIDTH;
            r.direction.z = 1.0;
            r.direction = Vec3Normalize(r.direction);
            r.direction = Mat3Vec3Mul(rot, r.direction);
            r.origin = cameraPos;

            int samples = 5;
            struct Vec3 color = {0};
            for (int i = 0; i < samples; i++) {
                color = Vec3Add(color, shade(r, 2));
            }

            color = Vec3Mul(color, 1.0/(float)samples);

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

            SDL_SetRenderDrawColor(renderer, color.x*255, color.y*255, color.z*255, SDL_ALPHA_OPAQUE);
            SDL_RenderPoint(renderer, (float)x, (float)y);
        }
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
}
