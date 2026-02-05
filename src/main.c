#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

static void createTestFile () {
    FILE* ptr; 
    ptr = fopen("image/test.ppm", "wb");
    if (ptr == NULL) {
        printf("Failed to create file\n");
        return;
    }

    int width = 500;
    int height = 500;

    fprintf(ptr, "P6\n%d %d\n255\n", width, height);
    unsigned char colors [3];
    memset(colors, 0, 3 * sizeof(unsigned char));
    
    for (int y = 0; y < height; ++ y) {
        for (int x = 0; x < width; ++ x) {
            colors [0] = (unsigned char)(x + y) % 256;    
            colors [1] = (unsigned char)(x + y) % 256;
            colors [2] = (unsigned char)(x + y) % 256;

            fwrite(colors, 1, 3, ptr);
        }
    }

    fclose(ptr);
}


static void activate(GtkApplication * app, gpointer user_data) {
    GtkWidget * window;
    GtkWidget * box;
    GtkWidget * picture;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    gtk_window_set_child(GTK_WINDOW(window), box);

    picture = gtk_picture_new_for_filename("image/test.ppm");
    gtk_box_append(GTK_BOX(box), picture);

    gtk_window_present(GTK_WINDOW (window));
}

int main (int argc, char ** argv) {
    createTestFile();

    GtkApplication * app;

    int status;
    app = gtk_application_new("com.test.metroLT", G_APPLICATION_DEFAULT_FLAGS); 
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}