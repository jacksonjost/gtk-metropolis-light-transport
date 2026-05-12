#include "mlt.h"
#include "ray.h"
#include "sceneLoader.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* image buffer */

ImageBuffer * createImageBuffer (int width, int height) {
    ImageBuffer * image = malloc (sizeof (ImageBuffer));
    image->width = width;
    image->height = height;
    image->data = calloc ((size_t) width * height * 3, sizeof (double));
    return image;
}

void freeImageBuffer (ImageBuffer * image) {
    if (!image) return;
    free (image->data);
    free (image);
}

static void addImageSample (ImageBuffer * image, double px, double py, Vector color) {
    int ix = (int) px;
    int iy = (int) py;
    if (ix < 0 || ix >= image->width || iy < 0 || iy >= image->height) return;
    int offset = (iy * image->width + ix) * 3;
    image->data[offset] += color.x;
    image->data[offset + 1] += color.y;
    image->data[offset + 2] += color.z;
}

static unsigned char tonemap (double v) {
    if (v < 0.0) v = 0.0;
    v = v / (1.0 + v);
    v = pow (v, 1.0 / 2.2);
    int iv = (int) (v * 255.0 + 0.5);
    return (unsigned char) (iv > 255 ? 255 : iv);
}

void imageToRGBA (const ImageBuffer * image, unsigned char * rgba, double normalization, int mutationsDone) {
    if (mutationsDone <= 0) mutationsDone = 1;
    double scale = normalization * (double) (image->width * image->height) / (double) mutationsDone;

    for (int y = 0; y < image->height; ++ y) {
        for (int x = 0; x < image->width; ++ x) {
            int s = (y * image->width + x) * 3;
            int d = (y * image->width + x) * 4;
            rgba[d]     = tonemap (image->data[s]     * scale);
            rgba[d + 1] = tonemap (image->data[s + 1] * scale);
            rgba[d + 2] = tonemap (image->data[s + 2] * scale);
            rgba[d + 3] = 255;
        }
    }
}

/* primary sample space */

static double getSample (PrimarySamples * ps, int index) {
    if (index >= MAX_PRIMARY_SAMPLES) return 0.5;
    if (index >= ps->maxUsed) ps->maxUsed = index + 1;
    return ps->values[index];
}

static void initPrimarySamples (PrimarySamples * ps, Seed * seed) {
    for (int i = 0; i < MAX_PRIMARY_SAMPLES; ++ i) {
        ps->values[i] = randomDouble (seed);
    }
    ps->maxUsed = 0;
}

static double mutateValue (double v, Seed * seed) {
    double s1 = 1.0 / 1024.0;
    double s2 = 1.0 / 64.0;
    double dv = s2 * exp (-log (s2 / s1) * randomDouble (seed));
    if (randomDouble (seed) < 0.5) {
        v += dv;
        if (v >= 1.0) v -= 1.0;
    } else {
        v -= dv;
        if (v < 0.0) v += 1.0;
    }
    return v;
}

static void mutatePrimarySamples (const PrimarySamples * src, PrimarySamples * dst,
                                  Seed * seed, bool largeStep) {
    if (largeStep) {
        for (int i = 0; i < MAX_PRIMARY_SAMPLES; ++ i) {
            dst->values[i] = randomDouble (seed);
        }
    } else {
        for (int i = 0; i < src->maxUsed; ++ i) {
            dst->values[i] = mutateValue (src->values[i], seed);
        }
        for (int i = src->maxUsed; i < MAX_PRIMARY_SAMPLES; ++ i) {
            dst->values[i] = randomDouble (seed);
        }
    }
    dst->maxUsed = 0;
}

/* sampling helpers */

static Vector cosineSampleHemisphere (double u1, double u2, Vector normal) {
    double r = sqrt (u1);
    double theta = 2.0 * M_PI * u2;
    double x = r * cos (theta);
    double y = r * sin (theta);
    double z = sqrt (fmax (0.0, 1.0 - u1));

    Vector w = normal;
    Vector helper = (fabs (w.x) > 0.9) ? (Vector){0, 1, 0} : (Vector){1, 0, 0};
    Vector u = normalizeVector (crossProduct (helper, w));
    Vector v = crossProduct (w, u);

    return normalizeVector (addVector (addVector (
        scaleVector (u, x), scaleVector (v, y)), scaleVector (w, z)));
}

static Vector evalDirectLight (Scene * scene, Point hitPoint, Vector normal,
                               Vector albedo, double u1, double u2) {
    if (!scene->hasLight) return (Vector){0, 0, 0};

    Point lightPoint = movePoint (scene->lightVertex,
        addVector (scaleVector (scene->lightEdge1, u1),
                   scaleVector (scene->lightEdge2, u2)));
    Vector toLight = getVector (hitPoint, lightPoint);
    double distSq = vectorLengthSquared (toLight);
    double dist = sqrt (distSq);
    Vector lightDir = scaleVector (toLight, 1.0 / dist);

    double cosSurface = dotProduct (normal, lightDir);
    if (cosSurface <= 0.0) return (Vector){0, 0, 0};

    double cosLight = dotProduct (scene->lightNormal, negateVector (lightDir));
    if (cosLight <= 0.0) return (Vector){0, 0, 0};

    Ray shadow = { movePoint (hitPoint, scaleVector (normal, RAY_EPSILON)), lightDir };
    HitRecord shadowHit;
    if (getSceneHitBVH (scene, shadow, &shadowHit)) {
        if (shadowHit.distance < dist - RAY_EPSILON * 2.0 &&
            shadowHit.materialId != scene->lightMaterialId) {
            return (Vector){0, 0, 0};
        }
    } else {
        return (Vector){0, 0, 0};
    }

    Vector Le = scene->materials[scene->lightMaterialId].emission;
    double geom = cosSurface * cosLight * scene->lightArea / distSq;
    return scaleVector (multiplyVector (Le, albedo), geom / M_PI);
}

/* path tracer driven by primary samples */

static PathResult tracePathMLT (RenderState * state, PrimarySamples * ps) {
    PathResult result = { (Vector){0, 0, 0}, 0.0, 0.0 };
    int idx = 0;

    result.px = getSample (ps, idx ++) * IMG_WIDTH;
    result.py = getSample (ps, idx ++) * IMG_HEIGHT;
    Ray ray = getCameraRay (state->camera, result.px, result.py);

    Vector throughput = {1, 1, 1};
    int countEmission = 1;

    for (int depth = 0; depth < state->maxDepth; ++ depth) {
        HitRecord hit;
        if (!getSceneHitBVH (state->scene, ray, &hit)) break;

        Material * mat = &state->scene->materials[hit.materialId];
        double nDotD = dotProduct (ray.vector, hit.normal);
        Vector normal = (nDotD < 0.0) ? hit.normal : negateVector (hit.normal);

        if (maxComponent (mat->emission) > 0.0) {
            if (countEmission) {
                result.color = addVector (result.color, multiplyVector (throughput, mat->emission));
            }
            break;
        }

        if (mat->type == MATERIAL_DIFFUSE) {
            double ul1 = getSample (ps, idx ++);
            double ul2 = getSample (ps, idx ++);
            Vector direct = evalDirectLight (state->scene, hit.intersection, normal, mat->color, ul1, ul2);
            result.color = addVector (result.color, multiplyVector (throughput, direct));

            if (depth >= 3) {
                double rr = fmax (0.05, maxComponent (mat->color));
                if (getSample (ps, idx ++) >= rr) break;
                throughput = scaleVector (throughput, 1.0 / rr);
            }

            double ub1 = getSample (ps, idx ++);
            double ub2 = getSample (ps, idx ++);
            Vector newDir = cosineSampleHemisphere (ub1, ub2, normal);
            throughput = multiplyVector (throughput, mat->color);
            ray.origin = movePoint (hit.intersection, scaleVector (normal, RAY_EPSILON));
            ray.vector = newDir;
            countEmission = 0;

        } else if (mat->type == MATERIAL_MIRROR) {
            ray.vector = normalizeVector (reflectVector (ray.vector, normal));
            ray.origin = movePoint (hit.intersection, scaleVector (normal, RAY_EPSILON));
            throughput = multiplyVector (throughput, mat->color);
            countEmission = 1;

            if (depth >= 3) {
                double rr = fmax (0.05, maxComponent (mat->color));
                if (getSample (ps, idx ++) >= rr) break;
                throughput = scaleVector (throughput, 1.0 / rr);
            }

        } else if (mat->type == MATERIAL_GLASS) {
            int entering = (nDotD < 0.0);
            Vector outwardN = entering ? hit.normal : negateVector (hit.normal);
            double eta = entering ? (1.0 / mat->indexOfRefraction) : mat->indexOfRefraction;
            double cosI = fabs (nDotD);
            double sin2T = eta * eta * (1.0 - cosI * cosI);

            if (sin2T > 1.0) {
                ray.vector = normalizeVector (reflectVector (ray.vector, outwardN));
                ray.origin = movePoint (hit.intersection, scaleVector (outwardN, RAY_EPSILON));
            } else {
                double cosT = sqrt (1.0 - sin2T);
                double R0 = (1.0 - mat->indexOfRefraction) / (1.0 + mat->indexOfRefraction);
                R0 = R0 * R0;
                double cosAir = entering ? cosI : cosT;
                double fresnel = R0 + (1.0 - R0) * pow (1.0 - cosAir, 5.0);

                if (getSample (ps, idx ++) < fresnel) {
                    ray.vector = normalizeVector (reflectVector (ray.vector, outwardN));
                    ray.origin = movePoint (hit.intersection, scaleVector (outwardN, RAY_EPSILON));
                } else {
                    Vector refracted = addVector (scaleVector (ray.vector, eta),
                                                  scaleVector (outwardN, eta * cosI - cosT));
                    ray.vector = normalizeVector (refracted);
                    ray.origin = movePoint (hit.intersection, scaleVector (outwardN, -RAY_EPSILON));
                }
            }
            countEmission = 1;

            if (depth >= 5) {
                double rr = 0.95;
                if (getSample (ps, idx ++) >= rr) break;
                throughput = scaleVector (throughput, 1.0 / rr);
            }
        }
    }

    return result;
}

/* render state */

RenderState * createRenderState (int width, int height) {
    RenderState * state = calloc (1, sizeof (RenderState));
    state->image = createImageBuffer (width, height);
    state->scene = NULL;
    state->camera = createCamera (width, height);
    state->seed = generateSeed ();
    state->totalMutations = 16 * width * height;
    state->maxDepth = 8;
    state->numBootstrap = BOOTSTRAP_SAMPLES;
    state->largeStepProb = 0.3;
    return state;
}

void freeRenderState (RenderState * state) {
    if (!state) return;
    freeImageBuffer (state->image);
    if (state->scene) freeScene (state->scene);
    if (state->camera) freeCamera (state->camera);
    free (state->seed);
    free (state);
}

bool loadRenderStateScene (RenderState * state, const char * objPath, const char * mtlPath) {
    if (state->scene) freeScene (state->scene);
    state->scene = initScene ();
    if (!loadScene (state->scene, objPath, mtlPath)) {
        freeScene (state->scene);
        state->scene = NULL;
        return false;
    }
    frameScene (state->scene, state->camera);
    resetRenderState (state);
    return true;
}

void resetRenderState (RenderState * state) {
    memset (state->image->data, 0,
            sizeof (double) * (size_t) state->image->width * state->image->height * 3);
    state->mutationsDone = 0;
    state->accepted = 0;
    state->renderingDone = 0;
    state->bootstrapping = 0;
    state->stopRequested = 0;
    state->normalizationB = 0.0;
}

/* MLT core */

void renderMlt (RenderState * state) {
    if (!state->scene) {
        state->renderingDone = 1;
        return;
    }

    state->bootstrapping = 1;

    double bSum = 0.0;
    PrimarySamples bestSeed;
    double bestF = 0.0;
    int hasBest = 0;

    for (int i = 0; i < state->numBootstrap; ++ i) {
        if (state->stopRequested) {
            state->renderingDone = 1;
            return;
        }
        PrimarySamples ps;
        initPrimarySamples (&ps, state->seed);
        PathResult pr = tracePathMLT (state, &ps);
        double f = luminance (pr.color);
        bSum += f;
        if (f > bestF) {
            bestF = f;
            bestSeed = ps;
            hasBest = 1;
        }
    }

    state->normalizationB = bSum / (double) state->numBootstrap;
    state->bootstrapping = 0;

    if (!hasBest || state->normalizationB <= 0.0) {
        state->renderingDone = 1;
        return;
    }

    PrimarySamples currentPs = bestSeed;
    PathResult currentPr = tracePathMLT (state, &currentPs);
    double currentF = luminance (currentPr.color);

    for (int i = 0; i < state->totalMutations; ++ i) {
        if (state->stopRequested) break;

        bool largeStep = (randomDouble (state->seed) < state->largeStepProb);

        PrimarySamples proposedPs;
        mutatePrimarySamples (&currentPs, &proposedPs, state->seed, largeStep);

        PathResult proposedPr = tracePathMLT (state, &proposedPs);
        double proposedF = luminance (proposedPr.color);

        double acceptProb = (currentF > 0.0) ? fmin (1.0, proposedF / currentF) : 1.0;

        if (currentF > 0.0) {
            addImageSample (state->image, currentPr.px, currentPr.py,
                scaleVector (currentPr.color, (1.0 - acceptProb) / currentF));
        }
        if (proposedF > 0.0) {
            addImageSample (state->image, proposedPr.px, proposedPr.py,
                scaleVector (proposedPr.color, acceptProb / proposedF));
        }

        if (randomDouble (state->seed) < acceptProb) {
            currentPs = proposedPs;
            currentPr = proposedPr;
            currentF = proposedF;
            state->accepted ++;
        }

        state->mutationsDone = i + 1;
    }

    state->renderingDone = 1;
}
