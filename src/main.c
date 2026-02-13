#include "display.h"
#include "geometry.h"
#include "camera.h"
#include "ray.h"
#include "sceneLoader.h"
#include <windows.h>
#include <stdio.h>

#define DEFAULT_OBJ "../test_scenes/cornell_box/CornellBox-Sphere.obj"
#define DEFAULT_MTL "../test_scenes/cornell_box/CornellBox-Sphere.mtl"

PixelMap * generateTestPixelMap (int width, int height) {
    Scene * scene = initScene();
    Camera * cam = createCamera(width, height);

    //addSphere (scene, createSphere((Point){0, 0, -10}, 3.0, MATERIAL_DIFFUSE));
    //addTriangle (scene, createTriangle((Point){0.0, 2.0, -5.0}, (Point){2.0, -2.0, -5.0}, (Point){-2.0, -2.0, -5.0}, MATERIAL_DIFFUSE));

    if (!loadScene (scene, DEFAULT_OBJ, DEFAULT_MTL)) {
        fprintf (stderr, "Failed to load scene: %s\n", DEFAULT_OBJ);
        freeScene (scene);
        freeCamera (cam);
        return NULL;
    }

    fprintf (stderr, "Loaded: %d triangles, %d spheres, %d materials\n",
             scene->numTriangles, scene->numSpheres, scene->numMaterials);

    /* bounding box!!!!! */
    Point bbMin = scene->boundingBoxMin;
    Point bbMax = scene->boundingBoxMax;
    Vector extent = getVector (bbMin, bbMax);
    Point center = movePoint (bbMin, scaleVector (extent, 0.5));

    double halfWidth = extent.x * 0.5;
    double halfHeight = extent.y * 0.5;
    double maxHalf = fmax (halfWidth, halfHeight);

    double fovDegrees = 39.0;
    double tanHalfFov = tan (fovDegrees * M_PI / 360.0);
    double distance = maxHalf / tanHalfFov;

    cam->position = (Point){center.x, center.y, bbMax.z + distance};
    cam->forward = normalizeVector (getVector (cam->position, center));
    cam->right = normalizeVector (crossProduct (cam->forward, (Vector){0, 1, 0}));
    cam->up = crossProduct (cam->right, cam->forward);
    cam->halfTanFOV = tanHalfFov;
    cam->FOV = fovDegrees;

    PixelMap * newPixels = malloc (sizeof(PixelMap));

    newPixels->width = width;
    newPixels->height = height;
    newPixels->size = width * height * sizeof(unsigned char) * 4;
    newPixels->data = malloc (newPixels->size);

    for (int x = 0; x < width; ++ x) {
        for (int y = 0; y < height; ++ y) {
            Ray temp = getCameraRay(cam, x, y);
            HitRecord dummyHit;
            int index = (x + y * width) * 4;
            if (getSceneHit(scene, temp, &dummyHit)) {
                newPixels->data[index + 0] = 255; 
                newPixels->data[index + 1] = 255; 
                newPixels->data[index + 2] = 255; 
                newPixels->data[index + 3] = 255; 
            } else {
                newPixels->data[index + 0] = 0; 
                newPixels->data[index + 1] = 0; 
                newPixels->data[index + 2] = 0; 
                newPixels->data[index + 3] = 255; 
            }
        }
    }

    freeScene(scene);
    freeCamera(cam);

    return newPixels;
}

int main (int argc, char ** argv) {
    int width = 500, height = 500;    

    if (argc == 3) {
        width = strtol(argv[1], NULL, 10);
        height = strtol(argv[2], NULL, 10);
    } 

    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    PixelMap * map = generateTestPixelMap(width, height);
    QueryPerformanceCounter(&end);

    if (!map) {
        fprintf (stderr, "Failed to generate pixel map.\n");
        return 1;
    }

    double timeSpent = (double)(end.QuadPart - start.QuadPart)/frequency.QuadPart;

    fprintf(stderr,"Rendered %d x %d pixels in %f seconds.\n", width, height, timeSpent);

    GtkDisplay * display = createDisplay(width, height);
    setPixelMap(display, map);
    runDisplay(display, 0, NULL);
    cleanDisplay(display);
}