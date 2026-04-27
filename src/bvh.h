#ifndef RAYMEOWER_BVH_H
#define RAYMEOWER_BVH_H

#include <stdlib.h>

#include "MeowMath.h"
#include "vulkan/vulkan.h"
#include <assert.h>

static int CreateBuffer(int size, VkBufferUsageFlags usage, VkBuffer *buffer, void **pp, bool deviceLocal);

struct BVHNode {
    struct BVHNode *left;
    struct BVHNode *right;
    union {
        struct Triangle triangle;
        struct AABB aabb;
    };
};

struct BVHNodeLinear {
    int left;
    int right;
    struct Triangle triangle;
    struct AABB aabb;
};

struct LinearBVH {
    struct BVHNodeLinear *nodes;
    VkBuffer buffer;
    int size;
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

static inline int explore(struct BVHNode *node) {
    int nodes = 0;
    if (node->left != NULL) {
        nodes += explore(node->left);
    }
    if (node->right != NULL) {
        nodes += explore(node->right);
    }
    return nodes + 1;
}

static inline void LinearizeBVH_rec(struct BVHNode *root, struct BVHNodeLinear *bvh, int *index) {
    if (root->left == NULL && root->right == NULL) {
        bvh[*index].triangle = root->triangle;
    }
    else {
        bvh[*index].aabb = root->aabb;
    }
    int current_index = *index;
    bvh[current_index].left = -1;
    bvh[current_index].right = -1;
    if (root->left != NULL) {
        *index = (*index) + 1;
        bvh[current_index].left = *index;
        LinearizeBVH_rec(root->left, bvh, index);
    }
    if (root->right != NULL) {
        *index = (*index) + 1;
        bvh[current_index].right = *index;
        LinearizeBVH_rec(root->right, bvh, index);
    }
}

static inline struct LinearBVH LinearizeBVH(struct BVHNode *root) {
    struct LinearBVH bvh;
    bvh.size = explore(root);
    //bvh.nodes = malloc(sizeof(struct BVHNodeLinear)* bvh.size);
    CreateBuffer(bvh.size * sizeof(struct BVHNodeLinear), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &bvh.buffer, (void**)&bvh.nodes, true);
    int index = 0;
    LinearizeBVH_rec(root, bvh.nodes, &index);
    return bvh;
}

#endif //RAYMEOWER_BVH_H