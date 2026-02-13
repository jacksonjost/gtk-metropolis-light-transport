#include "sceneLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PARSED_MATERIALS 64 /* temp, can modify if needed*/

typedef struct {
    char name[128];
    Vector diffuseColor;
    Vector specularColor;
    Vector emissionColor;
    double refractiveIndex;
    int illuminationModel;
} ParsedMaterial;


static int containsIgnoreCase (const char * haystack, const char * needle) {
    size_t needleLength = strlen (needle);
    for (size_t i = 0; haystack[i]; ++ i) {
        int match = 1;
        for (size_t j = 0; j < needleLength && haystack[i + j]; ++ j) {
            if (tolower ((unsigned char) haystack[i + j]) !=
                tolower ((unsigned char) needle[j])) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

static void getDirectoryFromPath (const char * path, char * directory, int directorySize) {
    strncpy (directory, path, directorySize - 1);
    directory[directorySize - 1] = '\0';
    char * last = NULL;
    for (char * p = directory; *p; ++ p) {
        if (*p == '/' || *p == '\\') last = p;
    }
    if (last) last[1] = '\0';
    else directory[0] = '\0';
}

/* MTL parser */

static int parseMtlFile (const char * path, ParsedMaterial * materials, int maxMaterials) {
    FILE * file = fopen (path, "r");
    if (!file) return 0;

    int count = -1;
    char line[512];
    while (fgets (line, sizeof (line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        if (strncmp (line, "newmtl ", 7) == 0) {
            count ++;
            if (count >= maxMaterials) { count --; continue; }
            memset (&materials[count], 0, sizeof (ParsedMaterial));
            materials[count].refractiveIndex = 1.0;
            sscanf (line + 7, "%127s", materials[count].name);
        } else if (count >= 0 && count < maxMaterials) {
            double r, g, b;
            if (sscanf (line, " Kd %lf %lf %lf", &r, &g, &b) == 3) {
                materials[count].diffuseColor = (Vector){r, g, b};
            } else if (sscanf (line, " Ks %lf %lf %lf", &r, &g, &b) == 3) {
                materials[count].specularColor = (Vector){r, g, b};
            } else if (sscanf (line, " Ke %lf %lf %lf", &r, &g, &b) == 3) {
                materials[count].emissionColor = (Vector){r, g, b};
            } else if (sscanf (line, " Ni %lf", &r) == 1) {
                materials[count].refractiveIndex = r;
            } else {
                sscanf (line, " illum %d", &materials[count].illuminationModel);
            }
        }
    }
    fclose (file);
    return count + 1;
}

static int findParsedMaterialIndex (const ParsedMaterial * materials, int count, const char * name) {
    for (int i = 0; i < count; ++ i) {
        if (strcmp (materials[i].name, name) == 0) return i;
    }
    return -1;
}

/* material conversion */

static int loadAndConvertMaterials (Scene * scene, const char * mtlPath,
                                    ParsedMaterial * parsed, int * outCount) {
    int numParsed = parseMtlFile (mtlPath, parsed, MAX_PARSED_MATERIALS);
    *outCount = numParsed;

    for (int i = 0; i < numParsed; ++ i) {
        MaterialType type;
        Vector color;
        double indexOfRefraction = 1.0;

        if (parsed[i].illuminationModel == 5) {
            type = MATERIAL_MIRROR;
            color = parsed[i].specularColor;
        } else if (parsed[i].illuminationModel == 7) {
            type = MATERIAL_GLASS;
            color = (maxComponent (parsed[i].specularColor) > 0.01)
                ? parsed[i].specularColor : (Vector){0.95, 0.95, 0.95};
            indexOfRefraction = (parsed[i].refractiveIndex > 1.0)
                ? parsed[i].refractiveIndex : 1.5;
        } else {
            type = MATERIAL_DIFFUSE;
            color = parsed[i].diffuseColor;
        }

        addMaterial (scene, createMaterial (color, parsed[i].emissionColor, type, indexOfRefraction));
    }

    return numParsed;
}

/* sphere detection */

static int compareInts (const void * a, const void * b) {
    return *(const int *) a - *(const int *) b;
}

static void flushSphereGroup (Scene * scene, const Point * vertices,
                              int * indices, int numIndices, int materialId) {
    if (numIndices == 0 || materialId < 0) return;

    qsort (indices, numIndices, sizeof (int), compareInts);
    int numUnique = 0;
    for (int i = 0; i < numIndices; ++ i) {
        if (i == 0 || indices[i] != indices[i - 1]) numUnique ++;
    }

    Point center = {0, 0, 0};
    int prev = -1;
    for (int i = 0; i < numIndices; ++ i) {
        if (indices[i] != prev) {
            center.x += vertices[indices[i]].x;
            center.y += vertices[indices[i]].y;
            center.z += vertices[indices[i]].z;
            prev = indices[i];
        }
    }
    center.x /= numUnique;
    center.y /= numUnique;
    center.z /= numUnique;

    double radiusSum = 0;
    prev = -1;
    for (int i = 0; i < numIndices; ++ i) {
        if (indices[i] != prev) {
            radiusSum += vectorLength (getVector (center, vertices[indices[i]]));
            prev = indices[i];
        }
    }
    double radius = radiusSum / numUnique;

    if (radius > 1e-6) {
        addSphere (scene, createSphere (center, radius, materialId));
    }
}

/* OBJ loader */

bool loadScene (Scene * scene, const char * objPath, const char * mtlPath) {
    scene->boundingBoxMin = (Point){1e20, 1e20, 1e20};
    scene->boundingBoxMax = (Point){-1e20, -1e20, -1e20};

    char directory[512];
    getDirectoryFromPath (objPath, directory, sizeof (directory));

    FILE * file = fopen (objPath, "r");
    if (!file) return false;

    Point * vertices = NULL;
    int numVertices = 0;
    int verticesCapacity = 0;

    ParsedMaterial parsed[MAX_PARSED_MATERIALS];
    int numParsed = 0;

    /* load MTL from path */
    int mtlLoaded = 0;
    if (mtlPath && mtlPath[0]) {
        numParsed = loadAndConvertMaterials (scene, mtlPath, parsed, &numParsed);
        mtlLoaded = 1;
    }

    /* current parsing state */
    int currentMaterial = 0;
    char currentGroup[128] = "";
    int inSphere = 0;
    int sphereMaterial = -1;

    int * sphereVertices = NULL;
    int numSphereVertices = 0;
    int sphereVerticesCapacity = 0;

    char line[512];
    while (fgets (line, sizeof (line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        /* mtllib to autoload MTL from OBJ directory */
        if (!mtlLoaded && strncmp (line, "mtllib ", 7) == 0) {
            char mtlName[256];
            sscanf (line + 7, "%255s", mtlName);
            char autoPath[768];
            snprintf (autoPath, sizeof (autoPath), "%s%s", directory, mtlName);
            numParsed = loadAndConvertMaterials (scene, autoPath, parsed, &numParsed);
            mtlLoaded = 1;
        }

        /* vertex */
        else if (line[0] == 'v' && (line[1] == ' ' || line[1] == '\t')) {
            Point vertex;
            if (sscanf (line + 2, "%lf %lf %lf", &vertex.x, &vertex.y, &vertex.z) == 3) {
                if (numVertices >= verticesCapacity) {
                    verticesCapacity = verticesCapacity ? verticesCapacity * 2 : 4096;
                    vertices = (Point *) realloc (vertices, verticesCapacity * sizeof (Point));
                }
                vertices[numVertices ++] = vertex;

                if (vertex.x < scene->boundingBoxMin.x) scene->boundingBoxMin.x = vertex.x;
                if (vertex.y < scene->boundingBoxMin.y) scene->boundingBoxMin.y = vertex.y;
                if (vertex.z < scene->boundingBoxMin.z) scene->boundingBoxMin.z = vertex.z;
                if (vertex.x > scene->boundingBoxMax.x) scene->boundingBoxMax.x = vertex.x;
                if (vertex.y > scene->boundingBoxMax.y) scene->boundingBoxMax.y = vertex.y;
                if (vertex.z > scene->boundingBoxMax.z) scene->boundingBoxMax.z = vertex.z;
            }
        }

        /* group */
        else if (line[0] == 'g' && (line[1] == ' ' || line[1] == '\t')) {
            if (inSphere && numSphereVertices > 0) {
                flushSphereGroup (scene, vertices, sphereVertices, numSphereVertices, sphereMaterial);
            }
            numSphereVertices = 0;

            sscanf (line + 2, "%127s", currentGroup);
            inSphere = containsIgnoreCase (currentGroup, "sphere");
            if (inSphere) sphereMaterial = currentMaterial;
        }

        /* matereial assignment */
        else if (strncmp (line, "usemtl ", 7) == 0) {
            char materialName[128];
            sscanf (line + 7, "%127s", materialName);
            int index = findParsedMaterialIndex (parsed, numParsed, materialName);
            if (index >= 0) currentMaterial = index;
            if (inSphere) sphereMaterial = currentMaterial;
        }

        /* face */
        else if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
            int vertexIndices[8];
            int count = 0;
            const char * p = line + 2;

            while (count < 8) {
                while (*p == ' ' || *p == '\t') p ++;
                if (!*p || *p == '\n' || *p == '\r') break;
                int v = atoi (p);
                if (v == 0) break;
                vertexIndices[count ++] = (v > 0) ? v - 1 : numVertices + v;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p ++;
            }

            if (inSphere) {
                for (int i = 0; i < count; ++ i) {
                    if (numSphereVertices >= sphereVerticesCapacity) {
                        sphereVerticesCapacity = sphereVerticesCapacity ? sphereVerticesCapacity * 2 : 8192;
                        sphereVertices = (int *) realloc (sphereVertices, sphereVerticesCapacity * sizeof (int));
                    }
                    sphereVertices[numSphereVertices ++] = vertexIndices[i];
                }
            } else if (count >= 3 && currentMaterial >= 0) {
                addTriangle (scene, createTriangle (
                    vertices[vertexIndices[0]],
                    vertices[vertexIndices[1]],
                    vertices[vertexIndices[2]],
                    currentMaterial));
                if (count >= 4) {
                    addTriangle (scene, createTriangle (
                        vertices[vertexIndices[2]],
                        vertices[vertexIndices[3]],
                        vertices[vertexIndices[0]],
                        currentMaterial));
                }
            }
        }
    }

    /* flush any sphere group stuff left */
    if (inSphere && numSphereVertices > 0) {
        flushSphereGroup (scene, vertices, sphereVertices, numSphereVertices, sphereMaterial);
    }

    fclose (file);
    free (vertices);
    free (sphereVertices);

    detectLight (scene);
    return (scene->numTriangles > 0 || scene->numSpheres > 0);
}
