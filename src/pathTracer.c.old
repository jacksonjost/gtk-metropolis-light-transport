#include "pathTracer.h"
#include <stdlib.h>

static Ray mirrorReflection (Vector incoming, Vector normal, Point intersection) {
    Ray reflectedRay;
    reflectedRay.vector = subtractVector(incoming, scaleVector(normal, 2 * dotProduct(normal, incoming)));
    reflectedRay.vector = normalizeVector(reflectedRay.vector);
    reflectedRay.origin = movePoint(intersection, scaleVector(normal, RAY_EPSILON));
    return reflectedRay;
}

static Ray diffuseReflection (Vector normal, Point intersection, Seed * seed) {
    Vector randVec;
    randVec.x = (randomDouble(seed)) * 2.0 - 1.0;
    randVec.y = (randomDouble(seed)) * 2.0 - 1.0;
    randVec.z = (randomDouble(seed)) * 2.0 - 1.0;

    Ray reflectedRay;
    reflectedRay.vector = normalizeVector(addVector(normal, randVec));
    reflectedRay.origin = movePoint (intersection, scaleVector(normal, RAY_EPSILON));

    return reflectedRay;
}

int tracePath (Ray ray, HitRecord * path, int totalBounces, Scene * scene, Seed * seed) {
    if (totalBounces >= MAX_BOUNCES) return totalBounces;
    
    HitRecord currentHit;
    if (!getSceneHit(scene, ray, &currentHit)) {
        return totalBounces;
    } else {
        path[totalBounces]  = currentHit;
    }

    Material mat = scene->materials[currentHit.materialId];

    Ray reflectedRay;
    
    if (mat.type == MATERIAL_MIRROR) {
        reflectedRay = mirrorReflection(ray.vector, currentHit.normal, currentHit.intersection);
    } else if (mat.type == MATERIAL_DIFFUSE){
        reflectedRay = diffuseReflection(currentHit.normal, currentHit.intersection, seed);
    } else if (mat.type == MATERIAL_GLASS) {
        double indexOfRefraction = mat.indexOfRefraction;
        double cosTheta = dotProduct (ray.vector, currentHit.normal);
        Vector glassNormal = currentHit.normal;
        double refractionRatio = 1.0/indexOfRefraction; //Assuming index of air is 1.0

        if (cosTheta > 0) {
            glassNormal = scaleVector(currentHit.normal, -1);
            refractionRatio = indexOfRefraction; 
        } else {
            cosTheta = -cosTheta;  
        }

        double internalReflectionCheck = 1.0 - refractionRatio * refractionRatio * (1.0 - cosTheta * cosTheta);

        if (internalReflectionCheck < 0) {
            //it behaves like a mirror due to total Internal Reflection
            reflectedRay = mirrorReflection(ray.vector, glassNormal, currentHit.intersection);
        } else {
            //schlick approximation
            double reflectionCoefficient = (1.0 - indexOfRefraction) / (1.0 + indexOfRefraction);
            reflectionCoefficient = reflectionCoefficient * reflectionCoefficient;

            double fresnelProbability = reflectionCoefficient + (1 - reflectionCoefficient) * pow((1 - cosTheta), 5);


            if (randomDouble(seed) < fresnelProbability) {
                //reflection
                reflectedRay = mirrorReflection(ray.vector, glassNormal, currentHit.intersection);
            } else {
                //refraction
                Vector term1 = scaleVector(ray.vector, refractionRatio);
                Vector term2 = scaleVector(glassNormal, refractionRatio * cosTheta - sqrt(internalReflectionCheck));

                reflectedRay.vector = normalizeVector(addVector(term1, term2));
                reflectedRay.origin = movePoint(currentHit.intersection, scaleVector(glassNormal, -1 * RAY_EPSILON));
            }
        }

    }

    return tracePath (reflectedRay, path, totalBounces + 1, scene, seed);
}

Vector calculatePathColor (HitRecord * path, int numHits, Scene * scene, Seed * seed) {
    Vector color = {0, 0, 0};
    Vector throughput = {1, 1, 1};

    for (int i = 0; i < numHits; ++ i) {
        HitRecord * currentHit = &(path [i]);
        Material mat = scene->materials [currentHit->materialId];

        if (i == 0) {
            color = addVector(color, multiplyVector (throughput, mat.emission));
        }

        if (scene->hasLight) {
            Vector directionToLight = getVector (currentHit->intersection, scene->lightVertex);
            double distanceToLight = vectorLength(directionToLight);
            directionToLight = normalizeVector(directionToLight);
            Vector directionFromLight = scaleVector(directionToLight, -1.0);

            double cosThetaLight = dotProduct(scene->lightNormal, directionFromLight);
            double cosThetaSurface = dotProduct (currentHit->normal, directionToLight);

            if(cosThetaLight > 0 && cosThetaSurface >0) {
                Point origin = movePoint(currentHit->intersection, scaleVector(currentHit->normal, RAY_EPSILON));
                Ray directLightRay = {origin, directionToLight};

                HitRecord directLightHit;
                if (!getSceneHit(scene, directLightRay, &directLightHit) || directLightHit.distance > (distanceToLight - RAY_EPSILON)) {
                    if (cosThetaSurface > 0) {
                        double falloff = 1.0/(distanceToLight * distanceToLight + 1);
                        double intensity = cosThetaSurface * cosThetaLight * falloff;

                        Vector directLightContribution = scaleVector(scene->materials[scene->lightMaterialId].emission, intensity);
                        Vector reflectedLight = multiplyVector(directLightContribution, mat.color);
                        color = addVector(color, multiplyVector(throughput, reflectedLight));
                    }
                }
            }
        }

        throughput = multiplyVector (throughput, mat.color);
    }

    return color;
}