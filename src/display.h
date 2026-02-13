#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>

typedef struct {
    int width;
    int height;
    unsigned char * data;
    int size;
} PixelMap;

typedef struct _GtkDisplay GtkDisplay;

GtkDisplay * createDisplay (int width, int height);
void runDisplay (GtkDisplay * self, int argc, char ** argv);
void setPixelMap (GtkDisplay * self, PixelMap * map);
void cleanDisplay (GtkDisplay * self);


#endif