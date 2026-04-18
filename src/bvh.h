#ifndef RAYMEOWER_BVH_H
#define RAYMEOWER_BVH_H

#include <stdlib.h>

#include "MeowMath.h"
#include <assert.h>

struct BVHNode {
    struct BVHNode *left;
    struct BVHNode *right;
    union {
        struct Triangle triangle;
        struct AABB aabb;
    };
};

static inline bool CompareBarycenter(struct Triangle triangle, float midpoint, int axis) {
    struct Vec3 barycenter = GetBarycenter(&triangle, 1);
    switch (axis) {
        case 0:
            return midpoint > barycenter.x;
        case 1:
            return midpoint > barycenter.y;
        case 2:
            return midpoint > barycenter.z;
    }
}
static inline void swapTriangles(struct Triangle *t1, struct Triangle *t2) {
    struct Triangle temp = *t1;
    *t1 = *t2;
    *t2 = temp;
}

static inline int Split(struct Triangle *triangles, int count, float midpoint, int axis) {
    for (int i = 0; i < count; i++) {
        if (CompareBarycenter(triangles[i], midpoint, axis)) {
            continue;
        }
        for (int j = i + 1; j < count; j++) {
            if (CompareBarycenter(triangles[j], midpoint, axis)) {
                swapTriangles(triangles + i, triangles + j);
                break;
            }
            if (j == count - 1) {
                return i;
            }
        }
    }
}

static inline struct BVHNode *BuildBVH_rec(struct Triangle *triangles, int start, int end, int depth) {
    if (start == end) {
        return NULL;
    }
    struct BVHNode *node = (struct BVHNode *)malloc(sizeof(struct BVHNode));
    if (end-start == 1) {
        node->triangle = triangles[start];
        node->left = NULL;
        node->right = NULL;
        return node;
    }
    node->aabb = GetBoundingBox(&triangles[start], end-start);
    struct Vec3 barycenter = GetBarycenter(&triangles[start], end-start);
    int axis = depth % 3;
    float midpoint;
    switch (axis) {
        case 0:
            midpoint = barycenter.x;
            break;
        case 1:
            midpoint = barycenter.y;
            break;
        case 2:
            midpoint = barycenter.z;
            break;
    }
    int splitPoint = Split(&triangles[start], end-start, midpoint, axis);
    splitPoint += start;
    if (splitPoint == start || splitPoint == end) {
        splitPoint = (start + end) / 2;
    }
    node->left = BuildBVH_rec(triangles, start, splitPoint, depth+1);
    node->right = BuildBVH_rec(triangles, splitPoint, end, depth+1);
    return node;
}

static inline struct BVHNode *BuildBVH(struct Triangle *triangles, int count) {
    return BuildBVH_rec(triangles, 0, count, 0);
}

#endif //RAYMEOWER_BVH_H