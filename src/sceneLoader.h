#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include "geometry.h"
#include <stdbool.h>

bool loadScene (Scene * scene, const char * objPath, const char * mtlPath);

#endif
