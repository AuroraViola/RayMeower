#ifndef RAYMEOWER_MEOWMATH_H
#define RAYMEOWER_MEOWMATH_H
#include <math.h>
#include <stdint.h>

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Mat3 {
    struct Vec3 c[3];
};

struct Material {
    struct Vec3 color;
    struct Vec3 reflectionColor;
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

struct Plane {
    struct Vec3 origin;
    struct Vec3 normal;
};

struct Triangle {
    struct Mat3 vertices;
    uint16_t materialIndex;
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

struct AABB {
    struct Vec3 min;
    struct Vec3 max;
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

static inline struct Vec3 Vec3Div(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3){
        .x=a.x/b.x,
        .y=a.y/b.y,
        .z=a.z/b.z
    };
}

static inline struct Vec3 Vec3Cross(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3){
        .x=a.y*b.z-a.z*b.y,
        .y=a.z*b.x-a.x*b.z,
        .z=a.x*b.y-a.y*b.x
    };
}

static inline struct Vec3 Vec3Min(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3) {
        .x=a.x<b.x?a.x:b.x,
        .y=a.y<b.y?a.y:b.y,
        .z=a.z<b.z?a.z:b.z
    };
}

static inline struct Vec3 Vec3Max(struct Vec3 a, struct Vec3 b) {
    return (struct Vec3) {
        .x=a.x>b.x?a.x:b.x,
        .y=a.y>b.y?a.y:b.y,
        .z=a.z>b.z?a.z:b.z
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

static inline struct Vec3 cosWeightedRandomHemisphereDirection(struct Vec3 n) {
    struct Vec3 rv2 = {0};
    rv2.x = (float)SDL_rand(1000000) / 1000000.0f;
    rv2.y = (float)SDL_rand(1000000) / 1000000.0f;

    struct Vec3 uu = Vec3Normalize(Vec3Cross(n, Vec3(0.0, 1.0, 1.0)));
    struct Vec3 vv = Vec3Normalize(Vec3Cross(uu, n));

    float ra = sqrt(rv2.y);
    float rx = ra * cos(6.2831 * rv2.x);
    float ry = ra * sin(6.2831 * rv2.x);
    float rz = sqrt(1.0 - rv2.y);
    struct Vec3 rr = Vec3Add(Vec3Add(Vec3Mul(uu, rx), Vec3Mul(vv, ry)), Vec3Mul(n, rz));

    return Vec3Normalize(rr);
}

static inline struct HitPoint IntersectionPlane(struct Ray ray, struct Plane plane) {
    struct HitPoint res = {0};
    float denom = Vec3Dot(ray.direction, plane.normal);
    if (denom < 1e-3) {
        return res;
    }
    float nom = Vec3Dot(Vec3Sub(plane.origin, ray.origin), plane.normal);
    res.distance = nom/denom;
    if (res.distance >= 0) {
        res.hit = true;
    }
    res.normal = plane.normal;
    res.point = Vec3Add(ray.origin, Vec3Mul(ray.direction, res.distance));
    return res;
}

static inline float TriangleDoubleArea(struct Triangle t) {
    struct Vec3 v1 = Vec3Sub(t.vertices.c[1], t.vertices.c[0]);
    struct Vec3 v2 = Vec3Sub(t.vertices.c[2], t.vertices.c[0]);
    return Vec3Length(Vec3Cross(v1, v2));
}

static inline struct Vec3 TriangleNormal(struct Triangle t) {
    struct Vec3 v1 = Vec3Sub(t.vertices.c[1], t.vertices.c[0]);
    struct Vec3 v2 = Vec3Sub(t.vertices.c[2], t.vertices.c[0]);
    return Vec3Normalize(Vec3Cross(v1, v2));
}

static inline struct Vec3 PointTriangleIntersection(struct Vec3 p, struct Triangle t) {
    float a = TriangleDoubleArea(t);
    struct Triangle t1 = {.vertices = {t.vertices.c[0], t.vertices.c[1], p}};
    struct Triangle t2 = {.vertices = {t.vertices.c[1], t.vertices.c[2], p}};
    struct Triangle t3 = {.vertices = {t.vertices.c[0], t.vertices.c[2], p}};
    float a1 = TriangleDoubleArea(t1);
    float a2 = TriangleDoubleArea(t2);
    float a3 = TriangleDoubleArea(t3);

    if (a < (a1 + a2 + a3) - 1e-5) {
        return Vec3(-1, -1, -1);
    }

    return Vec3(a1/a, a2/a, a3/a);
}

static inline struct HitPoint IntersectionTriangle(struct Ray ray, struct Triangle triangle) {
    struct Plane plane;
    plane.normal = TriangleNormal(triangle);

    if (Vec3Dot(ray.direction, plane.normal) < 0.0) {
        plane.normal = Vec3Mul(plane.normal, -1);
    }

    plane.origin = triangle.vertices.c[0];

    struct HitPoint hit = IntersectionPlane(ray, plane);
    if (!hit.hit) {
        return hit;
    }

    struct Vec3 barycentric = PointTriangleIntersection(hit.point, triangle);
    if (barycentric.x < 0) {
        hit.hit = false;
    }
    return hit;
}

static inline struct AABB GetBoundingBox(struct Triangle *triangle, int count) {
    struct AABB box;
    box.min = triangle->vertices.c[0];
    box.max = triangle->vertices.c[0];

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < 3; j++) {
            box.min.x = fmin(triangle[i].vertices.c[j].x, box.min.x);
            box.min.y = fmin(triangle[i].vertices.c[j].y, box.min.y);
            box.min.z = fmin(triangle[i].vertices.c[j].z, box.min.z);
            box.max.x = fmax(triangle[i].vertices.c[j].x, box.max.x);
            box.max.y = fmax(triangle[i].vertices.c[j].y, box.max.y);
            box.max.z = fmax(triangle[i].vertices.c[j].z, box.max.z);
        }
    }
    box.min.x += -1e-3;
    box.min.y += -1e-3;
    box.min.z += -1e-3;
    box.max.x += 1e-3;
    box.max.y += 1e-3;
    box.max.z += 1e-3;

    return box;
}

static inline struct Vec3 GetBarycenter(struct Triangle *triangle, int count) {
    struct Vec3 barycenter = {0};
    for (int i = 0; i < count; i++) {
        barycenter.x += triangle[i].vertices.c[0].x;
        barycenter.x += triangle[i].vertices.c[1].x;
        barycenter.x += triangle[i].vertices.c[2].x;
        barycenter.y += triangle[i].vertices.c[0].y;
        barycenter.y += triangle[i].vertices.c[1].y;
        barycenter.y += triangle[i].vertices.c[2].y;
        barycenter.z += triangle[i].vertices.c[0].z;
        barycenter.z += triangle[i].vertices.c[1].z;
        barycenter.z += triangle[i].vertices.c[2].z;
    }
    barycenter.x /= count*3;
    barycenter.y /= count*3;
    barycenter.z /= count*3;
    return barycenter;
}

static inline bool IntersectionAABB(struct Ray ray, struct AABB aabb) {
    struct Vec3 vtmin = Vec3Div(Vec3Sub(aabb.min, ray.origin), ray.direction);
    struct Vec3 vtmax = Vec3Div(Vec3Sub(aabb.max, ray.origin), ray.direction);

    struct Vec3 tmin = Vec3Min(vtmin, vtmax);
    struct Vec3 tmax = Vec3Max(vtmin, vtmax);

    float enter = fmax(tmin.x, fmax(tmin.y, tmin.z));
    float exit = fmin(tmax.x, fmin(tmax.y, tmax.z));

    return exit > 0.0 && exit > enter;
}

#endif //RAYMEOWER_MEOWMATH_H