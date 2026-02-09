#ifndef RAY_H
#define RAY_H

#include "geometry.h"

#define RAY_EPSILON 1e-4

typedef struct {
    Point origin;
    Vector vector;
} Ray;

typedef struct {
    double distance;
    Point intersection;
    Vector normal;
    int materialId;
} HitRecord;

double triangleIntersection (Triangle triangle, Ray ray);
Ray getReflectedRay (Triangle triangle, Ray ray);

int getTriangleHit (Triangle triangle, Ray ray, double minDist, double maxDist, HitRecord * record);
int getSphereHit (Sphere sphere, Ray ray, double minDist, double maxDist, HitRecord * record);
int getSceneHit (Scene scene, Ray ray, HitRecord * record);

#endif
