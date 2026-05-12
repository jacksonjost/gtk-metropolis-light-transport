#include "display.h"
#include "constants.h"

int main (int argc, char ** argv) {
    GtkDisplay * display = createDisplay (IMG_WIDTH, IMG_HEIGHT);
    runDisplay (display, argc, argv);
    cleanDisplay (display);
    return 0;
}
