#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "vectorMath.h"

typedef enum {
    MATERIAL_DIFFUSE,
    MATERIAL_MIRROR,
    MATERIAL_GLASS
} MaterialType;

typedef struct {
    Vector color;
    Vector emission;
    MaterialType type;
    double indexOfRefraction;
} Material;

typedef struct {
    Point p1, p2, p3;
    Vector edge1, edge2, edge3;
    Vector normal;
    int materialId;
} Triangle;

typedef struct {
    Point center;
    double radius;
    int materialId;
} Sphere;

typedef struct {
    Triangle * triangles;
    int numTriangles;


    Sphere * spheres;
    int numSpheres;

    
    Material * materials;
    int numMaterials;


    Point boundingBoxMin;
    Point boundingBoxMax;
    Point lightVertex;
    Vector lightEdge1;
    Vector lightEdge2;
    Vector lightNormal;
    double lightArea;
    int lightMaterialId;
    int hasLight;
} Scene;

Triangle createTriangle (Point p1, Point p2, Point p3, int materialId);
Sphere createSphere (Point center, double radius, int materialId);
Material createMaterial (Vector color, Vector emission, MaterialType type, double indexOfRefraction);

void addTriangle (Scene * scene, Triangle triangle);
void addSphere (Scene * scene, Sphere sphere);
void addMaterial (Scene * scene, Material material);
void freeScene (Scene * scene);
void detectLight (Scene * scene);

#endif
