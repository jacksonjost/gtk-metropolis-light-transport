#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"

struct _GtkDisplay{
    GtkApplication * app;
    GtkWidget * window;
    GtkWidget * picture;

    int width;
    int height;
    
    PixelMap * pixelMap;
};

static void activate(GtkApplication * app, gpointer user_data) {
    GtkDisplay * self = (GtkDisplay *) user_data;

    self->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(self->window), "GdkTexture Test");
    gtk_window_set_default_size(GTK_WINDOW(self->window), self->width, self->height);

    GBytes * pixelMapByteData = g_bytes_new_static (self->pixelMap->data, self->pixelMap->size);

    GdkTexture * texture = gdk_memory_texture_new (self->pixelMap->width, self->pixelMap->height, GDK_MEMORY_R8G8B8A8, pixelMapByteData, self->pixelMap->width * 4);

    self->picture =  gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));

    gtk_window_set_child(GTK_WINDOW(self->window), self->picture);

    gtk_window_present(GTK_WINDOW (self->window));
    g_bytes_unref(pixelMapByteData);
    g_object_unref(texture);
}

void setPixelMap (GtkDisplay * self, PixelMap * map) {
    self->pixelMap = map;
}

GtkDisplay * createDisplay (int width, int height) {
    GtkDisplay * newDisplay = malloc (sizeof(GtkDisplay));
    newDisplay->app = gtk_application_new("com.test.metroLT", G_APPLICATION_DEFAULT_FLAGS); 
    newDisplay->width = width;
    newDisplay->height = height; 

    newDisplay->window = NULL;
    newDisplay->picture = NULL;
    newDisplay->pixelMap = NULL;

    return newDisplay;
}

void runDisplay (GtkDisplay * self, int argc, char ** argv) {
    g_signal_connect(self->app, "activate", G_CALLBACK(activate), self);
    g_application_run(G_APPLICATION(self->app), argc, argv);
}

void cleanDisplay (GtkDisplay * self) {
    g_object_unref(self->app);
    if (self->pixelMap) {
        free(self->pixelMap->data);
        free(self->pixelMap);
    }
    free(self);
}