#include "display.h"
#include "geometry.h"
#include "camera.h"
#include "ray.h"
#include <windows.h>
#include <stdio.h>
PixelMap * generateTestPixelMap (int width, int height) {
    Scene * scene = initScene();
    Camera * cam = createCamera(width, height);

    addSphere (scene, createSphere((Point){0, 0, -10}, 3.0, MATERIAL_DIFFUSE));
    addTriangle (scene, createTriangle((Point){0.0, 2.0, -5.0}, (Point){2.0, -2.0, -5.0}, (Point){-2.0, -2.0, -5.0}, MATERIAL_DIFFUSE));

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

    double timeSpent = (double)(end.QuadPart - start.QuadPart)/frequency.QuadPart;

    fprintf(stderr,"Rendered %d x %d pixels in %f seconds.\n", width, height, timeSpent);

    GtkDisplay * display = createDisplay(width, height);
    setPixelMap(display, map);
    runDisplay(display, 0, NULL);
    cleanDisplay(display);
}