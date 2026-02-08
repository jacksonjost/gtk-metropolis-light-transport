#include "display.h"
#include "ray.h"

PixelMap * generateTestPixelMap (int width, int height) {
    PixelMap * newPixels = malloc (sizeof(PixelMap));

    newPixels->width = width;
    newPixels->height = height;
    newPixels->size = width * height * sizeof(unsigned char) * 4;
    newPixels->data = malloc (newPixels->size);

    for (int x = 0; x < width; ++ x) {
        for (int y = 0; y < height; ++ y) {
            int index = (x + y * width) * 4;
            for (int offset = 0; offset < 4; ++ offset) {
                newPixels->data[index + offset] = (unsigned char) ((x + y)%100 + rand()%155);
            }
        }
    }

    return newPixels;
}

int main (int argc, char ** argv) {
    int width = 500, height = 500;    

    if (argc == 3) {
        width = strtol(argv[1], NULL, 10);
        height = strtol(argv[2], NULL, 10);
    } 
    
    GtkDisplay * display = createDisplay(width, height);
    setPixelMap(display, generateTestPixelMap(width, height));
    runDisplay(display, 0, NULL);
    cleanDisplay(display);
}