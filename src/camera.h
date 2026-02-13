#ifndef CAMERA_H
#define CAMERA_H

#include "vectorMath.h"
#include "ray.h"

typedef struct {
    Point position;
    Vector forward;
    Vector up;
    Vector right;
    double halfTanFOV;
    double FOV;
    int imageWidth;
    int imageHeight;
} Camera;

Camera * createCamera(int imageWidth, int imageHeight);
void freeCamera(Camera * cam);
Ray getCameraRay (Camera * cam, double px, double py);

#endif