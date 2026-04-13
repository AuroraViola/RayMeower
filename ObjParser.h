#ifndef RAYMEOWER_OBJPARSER_H
#define RAYMEOWER_OBJPARSER_H

#include <stdio.h>
#include <stdlib.h>

#include "MeowMath.h"
#include <libgen.h>
#include <SDL3_image/SDL_image.h>

#define begins(a, b) strncmp(a, b, strlen(a))

struct Mesh {
    struct Triangle *triangles;
    int triangleCount;
    struct Material *material;
    int materialCount;
};

static inline void ResolvePath(const char *path, const char *filename, char *resoved) {
    char cur[256];
    strcpy(cur, path);
    sprintf(resoved, "%s/%s", dirname(cur), filename);
}

static inline struct Material *ImportMtl(const char *path, int *materialCount) {
    struct Material *materials = malloc(sizeof(struct Material) * 256);
    FILE *file = fopen(path, "r");
    *materialCount = 0;
    char current_line[256];
    while (fgets(current_line, sizeof(current_line), file) != NULL) {
        if (begins("newmtl", current_line) == 0) {
            (*materialCount)++;
            materials[(*materialCount)-1].name = malloc(256);
            sscanf(current_line, "newmtl %s", materials[(*materialCount)-1].name);
            materials[(*materialCount)-1].color = Vec3(1.0, 1.0, 1.0);
            materials[(*materialCount)-1].reflectionColor = Vec3(0.0, 0.0, 0.0);
            materials[(*materialCount)-1].emissionColor = Vec3(0.0, 0.0, 0.0);
            materials[(*materialCount)-1].texture = NULL;
        }
        if (begins("Kd", current_line) == 0) {
            sscanf(current_line, "Kd %f %f %f", &materials[(*materialCount)-1].color.x, &materials[(*materialCount)-1].color.y, &materials[(*materialCount)-1].color.z);
        }
        if (begins("Ke", current_line) == 0) {
            sscanf(current_line, "Ke %f %f %f", &materials[(*materialCount)-1].emissionColor.x, &materials[(*materialCount)-1].emissionColor.y, &materials[(*materialCount)-1].emissionColor.z);
        }
        if (begins("map_Kd", current_line) == 0) {
            char filename[256];
            sscanf(current_line, "map_Kd %s", filename);
            char texturePath[256];
            ResolvePath(path, filename, texturePath);
            materials[(*materialCount)-1].texture = IMG_Load(texturePath);
        }
    }
    return materials;
}

static inline struct Mesh ImportObj(const char *path){
    struct Vec3 *vertices = (struct Vec3 *)malloc(sizeof(struct Vec3) * 819200);
    struct Vec2 *uv = (struct Vec2 *)malloc(sizeof(struct Vec2) * 819200);
    int vertexCount = 0;
    int uvCount = 0;
    struct Mesh m;
    m.triangles = (struct Triangle *)malloc(sizeof(struct Triangle) * 1000000);
    int triangleCount = 0;
    int currentMaterial = 0;

    FILE *file = fopen(path, "r");
    char current_line[256];
    while (fgets(current_line, sizeof(current_line), file) != NULL) {
        switch (current_line[0]) {
            case 'v':
                if (current_line[1] == ' ') {
                    sscanf(current_line, "v %f %f %f", &vertices[vertexCount].x, &vertices[vertexCount].y, &vertices[vertexCount].z);
                    vertices[vertexCount].x *= -1;
                    vertexCount++;
                }
                else if (current_line[1] == 't') {
                    sscanf(current_line, "vt %f %f", &uv[uvCount].x, &uv[uvCount].y);
                    uvCount++;
                }
                break;
            case 'f':
                int indexs[3] = {0};
                int uvIndex[3] = {0};
                if (sscanf(current_line, "f %d/%d/%*d %d/%d/%*d %d/%d/%*d", &indexs[0], &uvIndex[0], &indexs[1], &uvIndex[1], &indexs[2], &uvIndex[2]) != 6);
                    sscanf(current_line, "f %d//%*d %d//%*d %d//%*d", &indexs[0], &indexs[1], &indexs[2]);
                m.triangles[triangleCount].vertices.c[0] = vertices[indexs[0]-1];
                m.triangles[triangleCount].vertices.c[1] = vertices[indexs[1]-1];
                m.triangles[triangleCount].vertices.c[2] = vertices[indexs[2]-1];
                m.triangles[triangleCount].uv[0] = uv[uvIndex[0]-1];
                m.triangles[triangleCount].uv[1] = uv[uvIndex[1]-1];
                m.triangles[triangleCount].uv[2] = uv[uvIndex[2]-1];
                m.triangles[triangleCount].materialIndex = currentMaterial;
                triangleCount++;
                break;
        }
        if (begins("mtllib", current_line) == 0) {
            char mtlfilename[256];
            sscanf(current_line, "mtllib %s", mtlfilename);
            char mtlpath[256];
            ResolvePath(path, mtlfilename, mtlpath);
            m.material = ImportMtl(mtlpath, &m.materialCount);
        }
        if (begins("usemtl", current_line) == 0) {
            char mtlname[256];
            sscanf(current_line, "usemtl %s", mtlname);
            for (int i = 0; i < m.materialCount; i++) {
                if (strcmp(mtlname, m.material[i].name) == 0) {
                    currentMaterial = i;
                    break;
                }
            }
        }
    }
    m.triangleCount = triangleCount;
    free(vertices);
    fclose(file);
    return m;
};

#endif //RAYMEOWER_OBJPARSER_H