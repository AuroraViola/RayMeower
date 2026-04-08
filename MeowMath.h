#ifndef RAYMEOWER_MEOWMATH_H
#define RAYMEOWER_MEOWMATH_H
#include <math.h>

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Mat3 {
    struct Vec3 c[3];
};

struct Sphere {
    struct Vec3 origin;
    float radius;
    struct Vec3 color;
    struct Vec3 reflectionColor;
};

struct Ray {
    struct Vec3 origin;
    struct Vec3 direction;
};

struct Sun {
    struct Vec3 dir;
    struct Vec3 color;
};

struct PointLight {
    struct Vec3 pos;
    struct Vec3 color;
};

struct HitPoint {
    bool hit;
    struct Vec3 point;
    struct Vec3 normal;
    float distance;
};

static inline struct Vec3 Vec3(float x, float y, float z) {
    return (struct Vec3){
        .x=x,
        .y=y,
        .z=z
    };
}

static inline struct Vec3 Vec3Add(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3){
        .x=a.x+b.x,
        .y=a.y+b.y,
        .z=a.z+b.z
    };
}

static inline struct Vec3 Vec3Sub(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3){
        .x=a.x-b.x,
        .y=a.y-b.y,
        .z=a.z-b.z
    };
}

static inline float Vec3Dot(struct Vec3 a, struct Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float Vec3Length(struct Vec3 a) {
    return sqrt(Vec3Dot(a, a));
}

static inline struct Vec3 Vec3Normalize(struct Vec3 a) {
    float l = Vec3Length(a);
    return (struct Vec3){
        .x=a.x/l,
        .y=a.y/l,
        .z=a.z/l
    };
}

static inline struct Vec3 Vec3Mul(struct Vec3 a, float b) {
    return (struct Vec3){
        .x=a.x*b,
        .y=a.y*b,
        .z=a.z*b
    };
}

static inline struct Vec3 Mat3Vec3Mul(struct Mat3 m, struct Vec3 v) {
    return (struct Vec3){
        .x=Vec3Dot(m.c[0], v),
        .y=Vec3Dot(m.c[1], v),
        .z=Vec3Dot(m.c[2], v)
    };
}

static inline struct Mat3 Mat3Mul(struct Mat3 a, struct Mat3 b) {
    return (struct Mat3){
        .c[0]=Mat3Vec3Mul(a, b.c[0]),
        .c[1]=Mat3Vec3Mul(a, b.c[1]),
        .c[2]=Mat3Vec3Mul(a, b.c[2]),
    };
}

static inline struct Mat3 RotMat(float h, float p, float b) {
    float f_sin_h = sin(h);
    float f_cos_h = cos(h);
    float f_sin_p = sin(p);
    float f_cos_p = cos(p);
    float f_sin_b = sin(b);
    float f_cos_b = cos(b);

    return (struct Mat3){
        Vec3(f_cos_h*f_cos_b+f_sin_p*f_sin_h*f_sin_b, f_sin_p*f_sin_h*f_cos_b-f_cos_h*f_sin_b, f_cos_p*f_sin_h),
        Vec3(f_cos_p*f_sin_b, f_cos_p*f_cos_b, -f_sin_p),
        Vec3(f_sin_p*f_cos_h*f_sin_b-f_sin_h*f_cos_b, f_sin_p*f_cos_h*f_cos_b+f_sin_h*f_sin_b, f_cos_p*f_cos_h),
    };
}

static inline struct HitPoint IntersectionSphere(struct Ray ray, struct Sphere sphere) {
    struct HitPoint Temp;
    struct Vec3 op = Vec3Sub(sphere.origin, ray.origin);
    float t ,eps = 1e-3;
    float b = Vec3Dot(op , ray.direction);
    float det = b * b - Vec3Dot(op,op) + sphere.radius*sphere.radius;

    if(det < 0.0) {
        Temp.hit = false; return Temp;
    }
    else
        det = sqrt(det);

    Temp.distance = (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0.0);

    if (Temp.distance == 0.0) {
        Temp.hit = false;
        return Temp;
    }
    else {
        Temp.point = Vec3Add(ray.origin, Vec3Mul(ray.direction, Temp.distance));
        Temp.normal = Vec3Normalize(Vec3Sub(sphere.origin, Temp.point));
        Temp.hit = true;
    }
    return Temp;
}
#endif //RAYMEOWER_MEOWMATH_H