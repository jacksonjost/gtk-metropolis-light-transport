#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

typedef struct {
    int width;
    int height;
    unsigned char * data;
    int size;
} PixelMap;

static PixelMap * createPixelMap (int width, int height) {
    int sizeOfRGBA = 4;
    PixelMap * newPixels = malloc (sizeof(PixelMap));

    newPixels->width = width;
    newPixels->height = height;
    newPixels->size = width * height * sizeOfRGBA;
    newPixels->data = malloc (newPixels->size);

    for (int x = 0; x < width; ++ x) {
        for (int y = 0; y < height; ++ y) {
            int index = (x + y * width ) * 4;
            for (int i = 0; i < 4; i ++) {
                newPixels->data [index + i] = (x + y)% 255;
            }
        }
    }
    return newPixels;

}


static void activate(GtkApplication * app, gpointer user_data) {
    GtkWidget * window;
    GtkWidget * picture;
    int width = 500; 
    int height = 500;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GdkTexture Test");
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);

    picture = gtk_picture_new();

    PixelMap * pixels = createPixelMap (width, height);
    // Note: the g_bytes_new_with_free_func calls the free function on the data pointer
    GBytes * pixelMapByteData = g_bytes_new_with_free_func (pixels->data, pixels->size, free, NULL);

    GdkTexture * texture = gdk_memory_texture_new (pixels->width, pixels->height, GDK_MEMORY_R8G8B8A8, pixelMapByteData, pixels->width * 4);

    gtk_picture_set_paintable (GTK_PICTURE(picture), GDK_PAINTABLE(texture));
    gtk_window_set_child(GTK_WINDOW(window), picture);

    g_bytes_unref(pixelMapByteData);
    g_object_unref(texture);
    free(pixels);

    gtk_window_present(GTK_WINDOW (window));
}

int main (int argc, char ** argv) {

    GtkApplication * app;

    int status;
    app = gtk_application_new("com.test.metroLT", G_APPLICATION_DEFAULT_FLAGS); 
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}