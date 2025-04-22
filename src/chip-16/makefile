CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lSDL2
INCLUDES = -I/usr/include/SDL2

# Los archivos fuente están en el mismo directorio que el Makefile
SRCDIR = .
BUILDDIR = build
TARGET = chip16-emu

# Buscar todos los archivos .c en el directorio actual
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

.PHONY: all clean