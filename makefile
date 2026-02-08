COMPILER = gcc
CFLAGS = $(shell pkg-config --cflags gtk4) -Wall
LDFLAGS = -mwindows
LIBS = $(shell pkg-config --libs gtk4) -lm
TARGET = bin/main
SOURCE = src/main.c src/display.c src/vectorMath.c src/ray.c

$(TARGET): $(SOURCE)
	mkdir -p bin
	$(COMPILER) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -rf bin
# del /Q bin\main.exe 2>nul || true