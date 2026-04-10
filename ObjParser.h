#ifndef RAYMEOWER_OBJPARSER_H
#define RAYMEOWER_OBJPARSER_H

#include <stdio.h>
#include <stdlib.h>

#include "MeowMath.h"

struct Mesh {
    struct Triangle *triangles;
    int triangleCount;
    struct Material *material;
};

static inline struct Mesh ImportObj(const char *path){
    struct Vec3 *vertices = (struct Vec3 *)malloc(sizeof(struct Vec3) * 8192);
    int vertexCount = 0;

    struct Mesh m;
    m.triangles = (struct Triangle *)malloc(sizeof(struct Triangle) * 30000);
    int triangleCount = 0;

    FILE *file = fopen(path, "r");
    char current_line[256];
    while (fgets(current_line, sizeof(current_line), file) != NULL) {
        switch (current_line[0]) {
            case 'v':
                if (current_line[1] == ' ') {
                    sscanf(current_line, "v %f %f %f", &vertices[vertexCount].x, &vertices[vertexCount].y, &vertices[vertexCount].z);
                    vertexCount++;
                }
                break;
            case 'f':
                int indexs[3];
                sscanf(current_line, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &indexs[0], &indexs[1], &indexs[2]);
                m.triangles[triangleCount].vertices.c[0] = vertices[indexs[0]-1];
                m.triangles[triangleCount].vertices.c[1] = vertices[indexs[1]-1];
                m.triangles[triangleCount].vertices.c[2] = vertices[indexs[2]-1];
                triangleCount++;
                break;
        }
    }
    m.triangleCount = triangleCount;
    free(vertices);
    fclose(file);
    return m;
};

#endif //RAYMEOWER_OBJPARSER_H