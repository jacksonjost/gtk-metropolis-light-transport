#include "geometry.h"
#include <stdlib.h>
#include <string.h>

Triangle createTriangle (Point p1, Point p2, Point p3, int materialId) {
    Triangle newTriangle;
    newTriangle.p1 = p1;
    newTriangle.p2 = p2;
    newTriangle.p3 = p3;
    newTriangle.edge1 = getVector (p1, p2);
    newTriangle.edge2 = getVector (p1, p3);
    newTriangle.edge3 = getVector (p2, p3);
    newTriangle.normal = normalizeVector (crossProduct (newTriangle.edge1, newTriangle.edge2));
    newTriangle.materialId = materialId;
    return newTriangle;
}

Sphere createSphere (Point center, double radius, int materialId) {
    Sphere newSphere;
    newSphere.center = center;
    newSphere.radius = radius;
    newSphere.materialId = materialId;
    return newSphere;
}

Material createMaterial (Vector color, Vector emission, MaterialType type, double indexOfRefraction) {
    Material newMaterial;
    newMaterial.color = color;
    newMaterial.emission = emission;
    newMaterial.type = type;
    newMaterial.indexOfRefraction = indexOfRefraction;
    return newMaterial;
}

void addTriangle (Scene * scene, Triangle triangle) {
    scene->numTriangles ++;
    scene->triangles = (Triangle *) realloc (scene->triangles, scene->numTriangles * sizeof (Triangle));
    scene->triangles[scene->numTriangles - 1] = triangle;
}

void addSphere (Scene * scene, Sphere sphere) {
    scene->numSpheres ++;
    scene->spheres = (Sphere *) realloc (scene->spheres, scene->numSpheres * sizeof (Sphere));
    scene->spheres[scene->numSpheres - 1] = sphere;
}

void addMaterial (Scene * scene, Material material) {
    scene->numMaterials ++;
    scene->materials = (Material *) realloc (scene->materials, scene->numMaterials * sizeof (Material));
    scene->materials[scene->numMaterials - 1] = material;
}

void freeScene (Scene * scene) {
    free (scene->spheres);
    free (scene->triangles);
    free (scene->materials);
    memset (scene, 0, sizeof (Scene));
}

void detectLight (Scene * scene) {
    scene->hasLight = 0;

    int lightMaterial = -1;
    for (int i = 0; i < scene->numMaterials; ++ i) {
        if (maxComponent (scene->materials[i].emission) > 0) {
            lightMaterial = i;
            break;
        }
    }
    if (lightMaterial < 0) return;
    scene->lightMaterialId = lightMaterial;

    int lightTriangles[8];
    int numLightTriangles = 0;
    for (int i = 0; i < scene->numTriangles && numLightTriangles < 8; ++ i) {
        if (scene->triangles[i].materialId == lightMaterial) {
            lightTriangles[numLightTriangles ++] = i;
        }
    }

    if (numLightTriangles == 2) {
        Triangle * t0 = &scene->triangles[lightTriangles[0]];
        Triangle * t1 = &scene->triangles[lightTriangles[1]];

        Point t0Verts[3] = {t0->p1, t0->p2, t0->p3};
        Point t1Verts[3] = {t1->p1, t1->p2, t1->p3};
        Point uniqueVertex = t1Verts[0];
        int found = 0;

        for (int i = 0; i < 3 && !found; ++ i) {
            int inFirstTriangle = 0;
            for (int j = 0; j < 3; ++ j) {
                Vector diff = getVector (t1Verts[i], t0Verts[j]);
                if (vectorLengthSquared (diff) < 1e-8) {
                    inFirstTriangle = 1;
                    break;
                }
            }
            if (!inFirstTriangle) {
                uniqueVertex = t1Verts[i];
                found = 1;
            }
        }

        if (found) {
            scene->lightVertex = t0->p1;
            scene->lightEdge1 = getVector (t0->p1, t0->p2);
            scene->lightEdge2 = getVector (t0->p1, uniqueVertex);
            scene->lightNormal = t0->normal;
            scene->lightArea = vectorLength (crossProduct (scene->lightEdge1, scene->lightEdge2));
            scene->hasLight = 1;
        }
    } else if (numLightTriangles == 1) {
        Triangle * t = &scene->triangles[lightTriangles[0]];
        scene->lightVertex = t->p1;
        scene->lightEdge1 = getVector (t->p1, t->p2);
        scene->lightEdge2 = getVector (t->p1, t->p3);
        scene->lightNormal = t->normal;
        scene->lightArea = 0.5 * vectorLength (crossProduct (scene->lightEdge1, scene->lightEdge2));
        scene->hasLight = 1;
    }
}
