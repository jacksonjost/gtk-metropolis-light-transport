#include "ray.h"
#include <float.h>

double triangleIntersection (Triangle triangle, Ray ray) {
    //Moller Trumbore intersection algorithm
    Vector rayCrossE2 = crossProduct (ray.vector, triangle.edge2);
    double dot = dotProduct (triangle.edge1, rayCrossE2);
    
    if (dot > -DBL_EPSILON && dot < DBL_EPSILON) {
        return -1.0;
    }

    double inverseDot = 1.0 / dot;

    Vector s = getVector (triangle.p1, ray.origin);
    double u = dotProduct (s, rayCrossE2) * inverseDot; 

    if ((u < 0 && fabs (u) > DBL_EPSILON) || (u > 1 && fabs (u - 1) > DBL_EPSILON)) {
        return -1.0;
    }

    Vector sCrossE1 = crossProduct (s, triangle.edge1);
    double v = inverseDot * dotProduct (ray.vector, sCrossE1);

    if ((v < 0 && fabs (v) > DBL_EPSILON) || ((u + v) > 1 && fabs (u + v - 1) > DBL_EPSILON)) {
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

    Point intersection = movePoint (ray.origin, scaleVector (ray.vector, distance));

    Vector normal = crossProduct (triangle.edge1, triangle.edge2);
    normal = normalizeVector (normal);
    Vector incomingVector = normalizeVector (ray.vector);
    if (dotProduct (incomingVector, normal) > 0) {
        normal = scaleVector (normal, -1.0);
    }

    Vector reflectedVector = subtractVector (incomingVector, scaleVector (normal, 2 * dotProduct (incomingVector, normal)));

    //Adjusting newOrigin to prevent floating point self intersection
    Point newOrigin = movePoint (intersection, scaleVector (normal, 1e-6));

    return (Ray){newOrigin, reflectedVector};
}

int getTriangleHit (Triangle triangle, Ray ray, double minDist, double maxDist, HitRecord * record) {
    //Moller Trumbore intersection algorithm
    Vector rayCrossE2 = crossProduct (ray.vector, triangle.edge2);
    double det = dotProduct (triangle.edge1, rayCrossE2);

    if (det > -DBL_EPSILON && det < DBL_EPSILON) {
        return 0;
    }

    double inverseDet = 1.0 / det;

    Vector s = getVector (triangle.p1, ray.origin);
    double u = dotProduct (s, rayCrossE2) * inverseDet;

    if ((u < 0 && fabs (u) > DBL_EPSILON) || (u > 1 && fabs (u - 1) > DBL_EPSILON)) {
        return 0;
    }

    Vector sCrossE1 = crossProduct (s, triangle.edge1);
    double v = inverseDet * dotProduct (ray.vector, sCrossE1);

    if ((v < 0 && fabs (v) > DBL_EPSILON) || ((u + v) > 1 && fabs (u + v - 1) > DBL_EPSILON)) {
        return 0;
    }

    double distance = inverseDet * dotProduct (triangle.edge2, sCrossE1);

    if (distance < minDist || distance > maxDist) {
        return 0;
    }

    record->distance = distance;
    record->intersection = movePoint (ray.origin, scaleVector (ray.vector, distance));
    record->normal = triangle.normal;
    record->materialId = triangle.materialId;
    return 1;
}

int getSphereHit (Sphere sphere, Ray ray, double minDist, double maxDist, HitRecord * record) {
    Vector originToCenter = getVector (sphere.center, ray.origin);
    double a = dotProduct (ray.vector, ray.vector);
    double halfB = dotProduct (originToCenter, ray.vector);
    double c = dotProduct (originToCenter, originToCenter) - sphere.radius * sphere.radius;
    double discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0) {
        return 0;
    }

    double sqrtDisc = sqrt (discriminant);
    double distance = (-halfB - sqrtDisc) / a;

    if (distance < minDist || distance > maxDist) {
        distance = (-halfB + sqrtDisc) / a;
        if (distance < minDist || distance > maxDist) {
            return 0;
        }
    }

    record->distance = distance;
    record->intersection = movePoint (ray.origin, scaleVector (ray.vector, distance));
    record->normal = scaleVector (getVector (sphere.center, record->intersection), 1.0 / sphere.radius);
    record->materialId = sphere.materialId;
    return 1;
}

int getSceneHit (Scene scene, Ray ray, HitRecord * record) {
    double closest = 1e20;
    int hit = 0;
    HitRecord temp;

    for (int i = 0; i < scene.numSpheres; ++ i) {
        if (getSphereHit (scene.spheres[i], ray, RAY_EPSILON, closest, &temp)) {
            closest = temp.distance;
            *record = temp;
            hit = 1;
        }
    }

    for (int i = 0; i < scene.numTriangles; ++ i) {
        if (getTriangleHit (scene.triangles[i], ray, RAY_EPSILON, closest, &temp)) {
            closest = temp.distance;
            *record = temp;
            hit = 1;
        }
    }

    return hit;
}
