CC=g++
CFLAGS=-c -Wall -O3 -mtune=native -march=native
CFLAGS+=`pkg-config --cflags gtk+-3.0 gdk-pixbuf-2.0`
LDFLAGS+=`pkg-config --libs gtk+-3.0 gdk-pixbuf-2.0`
SOURCES=main.cpp patchmatchapp.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) patchmatch

patchmatch: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
