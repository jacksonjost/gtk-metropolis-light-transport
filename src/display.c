#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "mlt.h"
#include "constants.h"

struct _GtkDisplay {
    GtkApplication * app;
    GtkWidget * window;
    GtkWidget * picture;

    GtkWidget * statusLabel;
    GtkWidget * renderBtn;

    GtkWidget * sppScale;
    GtkWidget * depthScale;
    GtkWidget * largeStepScale;

    GtkWidget * sppValue;
    GtkWidget * depthValue;
    GtkWidget * largeStepValue;

    int width;
    int height;

    PixelMap * pixelMap;
    RenderState * state;

    GThread * renderThread;
    guint timerId;
};

static void setControlsSensitive (GtkDisplay * self, gboolean sensitive) {
    gtk_widget_set_sensitive (self->sppScale, sensitive);
    gtk_widget_set_sensitive (self->depthScale, sensitive);
    gtk_widget_set_sensitive (self->largeStepScale, sensitive);
}

static void updateTexture (GtkDisplay * self) {
    int mutations = self->state->mutationsDone;

    if (mutations > 0 && self->state->normalizationB > 0.0) {
        imageToRGBA (self->state->image, self->pixelMap->data,
                     self->state->normalizationB, mutations);

        GBytes * bytes = g_bytes_new (self->pixelMap->data, self->pixelMap->size);
        GdkTexture * texture = gdk_memory_texture_new (
            self->pixelMap->width, self->pixelMap->height,
            GDK_MEMORY_R8G8B8A8, bytes, self->pixelMap->width * 4);
        gtk_picture_set_paintable (GTK_PICTURE (self->picture), GDK_PAINTABLE (texture));
        g_bytes_unref (bytes);
        g_object_unref (texture);
    }

    char status[256];
    if (self->state->renderingDone) {
        double spp = (double) mutations / (IMG_WIDTH * IMG_HEIGHT);
        double accept = mutations > 0 ? 100.0 * self->state->accepted / mutations : 0.0;
        snprintf (status, sizeof (status), "Complete | %.1f spp | %.1f%% acceptance", spp, accept);
    } else if (self->state->bootstrapping) {
        snprintf (status, sizeof (status), "Bootstrap...");
    } else if (mutations > 0) {
        double pct = 100.0 * mutations / self->state->totalMutations;
        double accept = 100.0 * self->state->accepted / mutations;
        snprintf (status, sizeof (status), "%.0f%% | %.1f%% acceptance", pct, accept);
    } else {
        status[0] = '\0';
    }
    gtk_label_set_text (GTK_LABEL (self->statusLabel), status);
}

static gpointer renderWorker (gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;
    renderMlt (self->state);
    return NULL;
}

static gboolean onTimerTick (gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;
    updateTexture (self);

    if (self->state->renderingDone) {
        gtk_button_set_label (GTK_BUTTON (self->renderBtn), "Render");
        setControlsSensitive (self, TRUE);
        self->timerId = 0;
        if (self->renderThread) {
            g_thread_join (self->renderThread);
            self->renderThread = NULL;
        }
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void onRenderClicked (GtkButton * btn, gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;

    if (self->renderThread && !self->state->renderingDone) {
        self->state->stopRequested = 1;
        return;
    }

    if (self->renderThread) {
        g_thread_join (self->renderThread);
        self->renderThread = NULL;
    }

    if (!self->state->scene) {
        gtk_label_set_text (GTK_LABEL (self->statusLabel), "No scene loaded");
        return;
    }

    int spp = (int) gtk_range_get_value (GTK_RANGE (self->sppScale));
    self->state->totalMutations = spp * IMG_WIDTH * IMG_HEIGHT;
    self->state->maxDepth = (int) gtk_range_get_value (GTK_RANGE (self->depthScale));
    self->state->largeStepProb = gtk_range_get_value (GTK_RANGE (self->largeStepScale)) / 100.0;

    resetRenderState (self->state);

    gtk_button_set_label (GTK_BUTTON (btn), "Stop");
    setControlsSensitive (self, FALSE);

    self->renderThread = g_thread_new ("mlt_render", renderWorker, self);
    if (self->timerId == 0) {
        self->timerId = g_timeout_add (500, onTimerTick, self);
    }
}

static void onSppChanged (GtkRange * range, gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;
    char buf[16];
    snprintf (buf, sizeof (buf), "%d", (int) gtk_range_get_value (range));
    gtk_label_set_text (GTK_LABEL (self->sppValue), buf);
}

static void onDepthChanged (GtkRange * range, gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;
    char buf[16];
    snprintf (buf, sizeof (buf), "%d", (int) gtk_range_get_value (range));
    gtk_label_set_text (GTK_LABEL (self->depthValue), buf);
}

static void onLargeStepChanged (GtkRange * range, gpointer data) {
    GtkDisplay * self = (GtkDisplay *) data;
    char buf[16];
    snprintf (buf, sizeof (buf), "%d%%", (int) gtk_range_get_value (range));
    gtk_label_set_text (GTK_LABEL (self->largeStepValue), buf);
}

static GtkWidget * makeSliderRow (GtkDisplay * self, const char * labelText,
                                  double min, double max, double initial, double step,
                                  GtkWidget ** outScale, GtkWidget ** outValueLabel,
                                  GCallback changedCallback) {
    GtkWidget * box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start (box, 8);
    gtk_widget_set_margin_end (box, 8);

    GtkWidget * label = gtk_label_new (labelText);
    gtk_widget_set_size_request (label, 80, -1);
    gtk_label_set_xalign (GTK_LABEL (label), 0.0);
    gtk_box_append (GTK_BOX (box), label);

    GtkWidget * scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step);
    gtk_range_set_value (GTK_RANGE (scale), initial);
    gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
    gtk_widget_set_hexpand (scale, TRUE);
    gtk_box_append (GTK_BOX (box), scale);

    char buf[16];
    if (step >= 1.0) {
        snprintf (buf, sizeof (buf), "%d", (int) initial);
    } else {
        snprintf (buf, sizeof (buf), "%.0f%%", initial);
    }
    GtkWidget * valueLabel = gtk_label_new (buf);
    gtk_widget_set_size_request (valueLabel, 40, -1);
    gtk_label_set_xalign (GTK_LABEL (valueLabel), 1.0);
    gtk_box_append (GTK_BOX (box), valueLabel);

    g_signal_connect (scale, "value-changed", changedCallback, self);

    *outScale = scale;
    *outValueLabel = valueLabel;
    return box;
}

static PixelMap * createPixelMap (int width, int height) {
    PixelMap * map = malloc (sizeof (PixelMap));
    map->width = width;
    map->height = height;
    map->size = width * height * 4;
    map->data = malloc (map->size);
    memset (map->data, 0, map->size);
    for (int i = 0; i < width * height; ++ i) {
        map->data[i * 4 + 3] = 255;
    }
    return map;
}

static void activate (GtkApplication * app, gpointer user_data) {
    GtkDisplay * self = (GtkDisplay *) user_data;

    self->window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (self->window), "MLT Renderer");
    gtk_window_set_resizable (GTK_WINDOW (self->window), FALSE);

    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
    gtk_window_set_child (GTK_WINDOW (self->window), vbox);

    self->picture = gtk_picture_new ();
    gtk_widget_set_size_request (self->picture, self->width, self->height);
    gtk_box_append (GTK_BOX (vbox), self->picture);

    GBytes * initBytes = g_bytes_new (self->pixelMap->data, self->pixelMap->size);
    GdkTexture * initTexture = gdk_memory_texture_new (
        self->pixelMap->width, self->pixelMap->height,
        GDK_MEMORY_R8G8B8A8, initBytes, self->pixelMap->width * 4);
    gtk_picture_set_paintable (GTK_PICTURE (self->picture), GDK_PAINTABLE (initTexture));
    g_bytes_unref (initBytes);
    g_object_unref (initTexture);

    gtk_box_append (GTK_BOX (vbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

    gtk_box_append (GTK_BOX (vbox),
        makeSliderRow (self, "SPP", 1, 200, 200, 1,
                       &self->sppScale, &self->sppValue,
                       G_CALLBACK (onSppChanged)));
    gtk_box_append (GTK_BOX (vbox),
        makeSliderRow (self, "Max depth", 2, 16, 16, 1,
                       &self->depthScale, &self->depthValue,
                       G_CALLBACK (onDepthChanged)));
    gtk_box_append (GTK_BOX (vbox),
        makeSliderRow (self, "Large step", 5, 80, 80, 5,
                       &self->largeStepScale, &self->largeStepValue,
                       G_CALLBACK (onLargeStepChanged)));

    self->renderBtn = gtk_button_new_with_label ("Render");
    gtk_widget_set_margin_start (self->renderBtn, 8);
    gtk_widget_set_margin_end (self->renderBtn, 8);
    gtk_widget_set_margin_top (self->renderBtn, 4);
    gtk_widget_set_margin_bottom (self->renderBtn, 4);
    g_signal_connect (self->renderBtn, "clicked", G_CALLBACK (onRenderClicked), self);
    gtk_box_append (GTK_BOX (vbox), self->renderBtn);

    self->statusLabel = gtk_label_new ("");
    gtk_widget_set_margin_start (self->statusLabel, 8);
    gtk_widget_set_margin_end (self->statusLabel, 8);
    gtk_widget_set_margin_bottom (self->statusLabel, 8);
    gtk_box_append (GTK_BOX (vbox), self->statusLabel);

    if (loadRenderStateScene (self->state, DEFAULT_OBJ, DEFAULT_MTL)) {
        char msg[256];
        snprintf (msg, sizeof (msg), "Loaded: %d tri, %d sph, %d mat",
                  self->state->scene->numTriangles,
                  self->state->scene->numSpheres,
                  self->state->scene->numMaterials);
        gtk_label_set_text (GTK_LABEL (self->statusLabel), msg);
    } else {
        gtk_label_set_text (GTK_LABEL (self->statusLabel), "Failed to load scene");
        gtk_widget_set_sensitive (self->renderBtn, FALSE);
    }

    gtk_window_present (GTK_WINDOW (self->window));
}

GtkDisplay * createDisplay (int width, int height) {
    GtkDisplay * newDisplay = malloc (sizeof (GtkDisplay));
    newDisplay->app = gtk_application_new ("com.test.metroLT", G_APPLICATION_DEFAULT_FLAGS);
    newDisplay->width = width;
    newDisplay->height = height;

    newDisplay->window = NULL;
    newDisplay->picture = NULL;
    newDisplay->statusLabel = NULL;
    newDisplay->renderBtn = NULL;
    newDisplay->sppScale = NULL;
    newDisplay->depthScale = NULL;
    newDisplay->largeStepScale = NULL;
    newDisplay->sppValue = NULL;
    newDisplay->depthValue = NULL;
    newDisplay->largeStepValue = NULL;

    newDisplay->pixelMap = createPixelMap (width, height);
    newDisplay->state = createRenderState (width, height);

    newDisplay->renderThread = NULL;
    newDisplay->timerId = 0;

    return newDisplay;
}

void runDisplay (GtkDisplay * self, int argc, char ** argv) {
    g_signal_connect (self->app, "activate", G_CALLBACK (activate), self);
    g_application_run (G_APPLICATION (self->app), argc, argv);
}

void cleanDisplay (GtkDisplay * self) {
    if (self->renderThread && !self->state->renderingDone) {
        self->state->stopRequested = 1;
    }
    if (self->renderThread) {
        g_thread_join (self->renderThread);
        self->renderThread = NULL;
    }
    if (self->timerId) {
        g_source_remove (self->timerId);
        self->timerId = 0;
    }

    g_object_unref (self->app);
    freeRenderState (self->state);
    if (self->pixelMap) {
        free (self->pixelMap->data);
        free (self->pixelMap);
    }
    free (self);
}
