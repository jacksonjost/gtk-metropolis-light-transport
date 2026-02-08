#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <math.h>

typedef struct {
    double x, y, z;
} Vector;

typedef struct {
    double x, y, z;
} Point;

Vector addVector (Vector a, Vector b);
Vector subtractVector (Vector a, Vector b);
Vector scaleVector (Vector a, double scale);
double dotProduct (Vector a, Vector b);
Vector crossProduct (Vector a, Vector b);
double vectorLengthSquared (Vector a);
double vectorLength (Vector a);
Vector normalizeVector(Vector a);

Vector getVector (Point a, Point b);
double getDistance(Point a, Point b);
Point movePoint (Point a, Vector v);

#endif