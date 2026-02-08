#include "ray.h"
#include <float.h>

Triangle createTriangle (Point p1, Point p2, Point p3) {
    Triangle newTriangle;
    newTriangle.p1 = p1;
    newTriangle.p2 = p2;
    newTriangle.p3 = p3;

    newTriangle.edge1 = getVector (p1, p2);
    newTriangle.edge2 = getVector (p1, p3);
    newTriangle.edge3 = getVector (p2, p3);

    return newTriangle;
}

double triangleIntersection (Triangle triangle, Ray ray) {
    //Moller Trumbore interesection algorithm
    Vector rayCrossE2 = crossProduct(ray.vector, triangle.edge2);
    double dot = dotProduct (triangle.edge1, rayCrossE2);
    
    if (dot > -DBL_EPSILON && dot < DBL_EPSILON) {
        return -1.0;
    }

    double inverseDot = 1.0/dot;

    Vector s = getVector(ray.origin, triangle.p1);
    double u = dotProduct (s, rayCrossE2) * inverseDot; 

    if ((u < 0 && fabs(u) > DBL_EPSILON) || (u > 1 && fabs(u - 1) > DBL_EPSILON)) {
        return -1.0;
    }

    Vector sCrossE1 = crossProduct(s, triangle.edge1);
    double v = inverseDot * dotProduct(ray.vector, sCrossE1);

    if ((v < 0 && fabs(v) > DBL_EPSILON) || ((u + v) > 1 && fabs (u + v - 1 ) >DBL_EPSILON)) {
        return -1.0;
    }

    double distance = inverseDot * dotProduct (triangle.edge2, sCrossE1);

    if (distance < DBL_EPSILON) {
       return -1.0; 
    } 

    return distance; 
}

Ray getReflectedRay (Triangle triangle, Ray ray) {
    double distance = triangleIntersection (triangle, ray);

    if (distance < 0) {
        return (Ray) {{0,0,0}, {0,0,0}};
    }

    Point intersection = movePoint(ray.origin, scaleVector(ray.vector, distance));

    Vector normal = crossProduct (triangle.edge1, triangle.edge2);
    normal = normalizeVector(normal);
    Vector incomingVector = normalizeVector(ray.vector);
    if (dotProduct(incomingVector, normal) > 0) {
        normal = scaleVector(normal, -1.0);
    }

    Vector reflectedVector = subtractVector(incomingVector, scaleVector(normal, 2 * dotProduct(incomingVector, normal)));

    //Adjusting newOrigin to prevent floating point self intersection
    Point newOrigin = movePoint(intersection, scaleVector(normal, 1e-6));

    return (Ray){newOrigin, reflectedVector};
}