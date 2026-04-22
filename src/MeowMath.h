#ifndef RAYMEOWER_MEOWMATH_H
#define RAYMEOWER_MEOWMATH_H
#include <math.h>
#include <stdint.h>
#include <SDL3_image/SDL_image.h>

#define PI 3.14159265

// Avoid funcion call
#define fmin(a,b) (((a) < (b)) ? (a) : (b))
#define fmax(a,b) (((a) > (b)) ? (a) : (b))
#define fabs(a) fmax(a, -a)

struct Vec2 {
    float x;
    float y;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Mat3 {
    struct Vec3 c[3];
};

struct Material {
    const char *name;
    struct Vec3 color;
    float metallic;
    float roughness;
    float transmissive;
    struct Vec3 emissionColor;
    float emissionIntensity;
    float ior;
    SDL_Surface *texture;
    SDL_Surface *roughnessMap;
    SDL_Surface *normalMap;
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

struct Triangle {
    struct Mat3 vertices;
    struct Vec2 uv[3];
    uint16_t materialIndex;
};

struct Sun {
    struct Vec3 dir;
    struct Vec3 color;
    float intensity;
    float angle;
};

struct PointLight {
    struct Vec3 pos;
    struct Vec3 color;
    float intensity;
};


struct HitPoint {
    bool hit;
    struct Vec3 point;
    struct Vec3 normal;
    struct Vec3 barycentric;
    float distance;
};

struct AABB {
    struct Vec3 min;
    struct Vec3 max;
};

struct Mesh {
    struct Triangle *triangles;
    int triangleCount;
    struct Material *material;
    int materialCount;
};

struct Scene {
    struct Mesh mesh;
    struct BVHNode *bvhRoot;
    struct Sun sun;
    struct PointLight *lights;
    int lightsCount;
};

static inline struct Vec3 Vec3(float x, float y, float z) {
    return (struct Vec3){
        .x=x,
        .y=y,
        .z=z
    };
}

static inline struct Vec2 Vec2(float x, float y) {
    return (struct Vec2){
        .x=x,
        .y=y
    };
}

static inline struct Vec3 Vec2ToVec3(struct Vec2 a) {
    return (struct Vec3){
        .x=a.x,
        .y=a.y,
        .z=0
    };
}

static inline struct Vec2 Vec3ToVec2(struct Vec3 a) {
    return (struct Vec2){
        .x=a.x,
        .y=a.y
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

static inline struct Vec3 Vec3DivScalar(struct Vec3 a, float b) {
    return (struct Vec3){
        .x=a.x/b,
        .y=a.y/b,
        .z=a.z/b
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

static inline struct Vec3 Reflect(struct Vec3 v, struct Vec3 n) {
    return Vec3Sub(v, Vec3Mul(n, 2*Vec3Dot(n, v)));
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

static inline struct Mat3 Mat3Transpose(struct Mat3 a) {
    return (struct Mat3){
        .c[0]=Vec3(a.c[0].x, a.c[1].x, a.c[2].x),
        .c[1]=Vec3(a.c[0].y, a.c[1].y, a.c[2].y),
        .c[2]=Vec3(a.c[0].z, a.c[1].z, a.c[2].z)
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

static inline struct HitPoint IntersectionTriangleFast(struct Ray ray, struct Triangle triangle) {
    struct HitPoint hit;
    hit.hit = false;
    hit.barycentric = Vec3(0, 1, 0);
    const float epsilon = 1e-5;

    struct Vec3 edge1 = Vec3Sub(triangle.vertices.c[1], triangle.vertices.c[0]);
    struct Vec3 edge2 = Vec3Sub(triangle.vertices.c[2], triangle.vertices.c[0]);

    // Backface culling, assuming CCW-wound triangles.
    struct Vec3 normal = Vec3Cross(edge1, edge2); // No need to normalize
    //if (Vec3Dot(normal, ray.direction) > 0)
        normal = Vec3Mul(normal, -1);

    struct Vec3 ray_cross_e2 = Vec3Cross(ray.direction, edge2);
    float det = Vec3Dot(edge1, ray_cross_e2);

    if (fabs(det) < epsilon)
        return hit; // Ray is parallel to triangle

    float inv_det = 1.0 / det;
    struct Vec3 s = Vec3Sub(ray.origin, triangle.vertices.c[0]);
    float u = inv_det * Vec3Dot(s, ray_cross_e2);

    if (u < -epsilon || u - 1 > epsilon)
        return hit; // Ray passes outside edge2's bounds

    struct Vec3 s_cross_e1 = Vec3Cross(s, edge1);
    float v = inv_det * Vec3Dot(ray.direction, s_cross_e1);

    if (v < -epsilon || u + v - 1 > epsilon)
        return hit; // Ray passes outside edge1's bounds

    // The ray line intersects with the triangle.
    // We compute t to find where on the ray the intersection is.
    float t = inv_det * Vec3Dot(edge2, s_cross_e1);

    if (t > epsilon) {// Ray intersection
        hit.point = Vec3Add(ray.origin, Vec3Mul(ray.direction, t));
        hit.hit = true;
        hit.normal = Vec3Mul(Vec3Normalize(normal), -1);
        hit.distance = t;
        hit.barycentric = Vec3(1 - u - v, u, v);
        return hit;
    }
    // This means that there is a line intersection but not a ray intersection.
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

static inline struct Vec3 InterpolateAttribute(struct HitPoint hit, struct Vec3 v[3]) {
    v[0] = Vec3Mul(v[0], hit.barycentric.x);
    v[1] = Vec3Mul(v[1], hit.barycentric.y);
    v[2] = Vec3Mul(v[2], hit.barycentric.z);
    return Vec3Add(v[0], Vec3Add(v[1], v[2]));
}

static inline struct Vec3 SampleTexture(SDL_Surface *surface, struct Vec2 uv) {
    uint32_t x = uv.x * surface->w;
    uint32_t y = (1.0-uv.y) * surface->h;
    x %= surface->w;
    y %= surface->h;

    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
    int pixelSize = details->bytes_per_pixel;
    void *pixel = surface->pixels + x * pixelSize + y * surface->pitch;

    // Gray-scale RGB (used in roughness map)
    if (pixelSize == 1) {
        uint8_t value = *(uint8_t*)pixel;
        return Vec3(value / 255.0, value / 255.0, value / 255.0);
    }
    uint32_t packedPixel = *(uint32_t*)pixel;
    uint8_t rgb[3];
    SDL_GetRGB(packedPixel, details, NULL, &rgb[0], &rgb[1], &rgb[2]);
    return Vec3(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
}

static inline float FresnelDielectric(float cosTheta, float ior) {
    if (cosTheta < 0) {
        cosTheta = -cosTheta;
        ior = 1/ior;
    }
    float sin2ThetaI = 1 - cosTheta * cosTheta;
    float sin2ThetaT = sin2ThetaI / (ior * ior);
    if (sin2ThetaT >= 1) {
        return 1.0f;
    }
    float cosThetaT = sqrt(1 - sin2ThetaT);

    float rParl = (ior * cosTheta - cosThetaT) /
                   (ior * cosTheta + cosThetaT);
    float rPerp = (cosTheta - ior * cosThetaT) /
                   (cosTheta + ior * cosThetaT);
    return ((rParl * rParl) + (rPerp * rPerp)) / 2;
}

static inline struct Vec3 Refract(struct Vec3 direction, struct Vec3 normal, float ior) {
    direction = Vec3Mul(direction, -1);
    float cosTheta = -Vec3Dot(direction, normal);
    normal = Vec3Mul(normal, -1);
    if (cosTheta < 0) {
        normal = Vec3Mul(normal, -1);
        cosTheta = -cosTheta;
        ior = 1/ior;
    }
    float sin2ThetaI = 1 - cosTheta * cosTheta;
    float sin2ThetaT = sin2ThetaI / (ior * ior);
    if (sin2ThetaT >= 1) {
        // Total internal reflection handled in Fresnel
        sin2ThetaT = 1;
    }
    float cosThetaT = sqrt(1 - sin2ThetaT);
    return Vec3Normalize(Vec3Add(Vec3DivScalar(direction, -ior), Vec3Mul(normal, (cosTheta / ior - cosThetaT))));
}

static inline float Lerp(float a, float b, float fact) {
    return a + fact * (b - a);
}

// http://jcgt.org/published/0007/04/01/paper.pdf by Eric Heitz
// Input Ve: view direction
// Input alpha_x, alpha_y: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
static inline struct Vec3 sampleGgxVndf(struct Vec3 ve, struct Vec2 alpha, struct Vec2 U) {
    struct Vec3 Vh = Vec3Normalize(Vec3(alpha.x * ve.x, alpha.y * ve.y, ve.z));
    float lensq = Vec3Length(Vec3(Vh.x, Vh.y, 0));

    struct Vec3 T1 = lensq > 0.0 ? Vec3Mul(Vec3(-Vh.y, Vh.x, 0.0), (1/sqrt(lensq))) : Vec3(1.0, 0.0, 0.0);
    struct Vec3 T2 = Vec3Cross(Vh, T1);

    float r = sqrt(U.x);
    float phi = 2.0 * PI * U.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    struct Vec3 Nh = Vec3Add(Vec3Add(Vec3Mul(T1, t1), Vec3Mul(T2, t2)), Vec3Mul(Vh, sqrt(fmax(0.0, 1.0 - t1 * t1 - t2 * t2))));

    struct Vec3 Ne = Vec3Normalize(Vec3(alpha.x * Nh.x, alpha.y * Nh.y, fmax(0.0, Nh.z)));
    return Ne;
}

static inline struct Mat3 Tbn(struct Vec3 n) {
    struct Vec3 u;
    if(fabs(n.z) > 0.0) {
        u = Vec3DivScalar(Vec3(0.0, -n.z, n.y), Vec3Length(Vec3(n.y, n.z, 0.0)));
    }
    else {
        u = Vec3DivScalar(Vec3(n.y, -n.x, 0.0), Vec3Length(Vec3(n.x, n.y, 0.0)));
    }

    struct Mat3 TBN;

    TBN.c[0] = u;
    TBN.c[1] = Vec3Cross(n, u);
    TBN.c[2] = n;

    return TBN;
}

static inline struct Mat3 TbnUv(struct Vec3 n, struct Triangle *t) {
    struct Vec3 edge1 = Vec3Sub(t->vertices.c[1], t->vertices.c[0]);
    struct Vec3 edge2 = Vec3Sub(t->vertices.c[2], t->vertices.c[0]);
    struct Vec3 deltaUv1 = Vec3Sub(Vec2ToVec3(t->uv[1]), Vec2ToVec3(t->uv[0]));
    struct Vec3 deltaUv2 = Vec3Sub(Vec2ToVec3(t->uv[2]), Vec2ToVec3(t->uv[0]));


    float f = 1.0f / (deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y);

    struct Vec3 tangent;
    struct Vec3 bitangent;

    tangent.x = f * (deltaUv2.y * edge1.x - deltaUv1.y * edge2.x);
    tangent.y = f * (deltaUv2.y * edge1.y - deltaUv1.y * edge2.y);
    tangent.z = f * (deltaUv2.y * edge1.z - deltaUv1.y * edge2.z);

    bitangent.x = f * (-deltaUv2.x * edge1.x + deltaUv1.x * edge2.x);
    bitangent.y = f * (-deltaUv2.x * edge1.y + deltaUv1.x * edge2.y);
    bitangent.z = f * (-deltaUv2.x * edge1.z + deltaUv1.x * edge2.z);

    struct Mat3 TBN;

    TBN.c[0] = Vec3Normalize(tangent);
    TBN.c[1] = Vec3Normalize(bitangent);
    TBN.c[2] = n;

    return TBN;
}

static inline struct Vec2 UniformRandomCirclePoint(float radius, struct Vec2 u) {
    float alpha = u.x * 2 * PI;
    radius = sqrt(u.y) * radius;
    return (struct Vec2){
        .x = radius * sin(alpha),
        .y = radius * cos(alpha)
    };
}

#endif //RAYMEOWER_MEOWMATH_H