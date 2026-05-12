#ifndef MLT_H
#define MLT_H

#include "geometry.h"
#include "camera.h"
#include "rand.h"
#include "constants.h"
#include <stdbool.h>

typedef struct {
    double values[MAX_PRIMARY_SAMPLES];
    int maxUsed;
} PrimarySamples;

typedef struct {
    Vector color;
    double px;
    double py;
} PathResult;

typedef struct {
    double * data;
    int width;
    int height;
} ImageBuffer;

typedef struct {
    ImageBuffer * image;
    Scene * scene;
    Camera * camera;
    Seed * seed;

    double normalizationB;
    int totalMutations;
    int maxDepth;
    int numBootstrap;
    double largeStepProb;

    volatile int mutationsDone;
    volatile int accepted;
    volatile int renderingDone;
    volatile int bootstrapping;
    volatile int stopRequested;
} RenderState;

RenderState * createRenderState (int width, int height);
void freeRenderState (RenderState * state);
bool loadRenderStateScene (RenderState * state, const char * objPath, const char * mtlPath);
void resetRenderState (RenderState * state);
void renderMlt (RenderState * state);

ImageBuffer * createImageBuffer (int width, int height);
void freeImageBuffer (ImageBuffer * image);
void imageToRGBA (const ImageBuffer * image, unsigned char * rgba, double normalization, int mutationsDone);

#endif
