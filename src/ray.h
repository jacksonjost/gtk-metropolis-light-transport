#ifndef RAY_H
#define RAY_H

#include "vectorMath.h"

typedef struct {
    Point origin;
    Vector vector;
} Ray;


typedef struct {
    Point p1, p2, p3;
    Vector edge1, edge2, edge3;
} Triangle;

Triangle createTriangle (Point p1, Point p2, Point p3);
double triangleIntersection (Triangle triangle, Ray ray);
Ray getReflectedRay (Triangle triangle, Ray ray);

#endif