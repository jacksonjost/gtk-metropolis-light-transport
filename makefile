COMPILER = gcc
CFLAGS = $(shell pkg-config --cflags gtk4 ) 
LIBS = $(shell pkg-config --libs gtk4 )
TARGET = bin/main
SOURCE = src/main.c

$(TARGET) : $(SOURCE)
	$(COMPILER) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)