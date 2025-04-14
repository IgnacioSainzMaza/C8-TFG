#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include "chip8.h"

// Estructura para gestionar la visualización
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    char windowTitle[256];
} Display;

// Funciones de visualización
bool displayInit(Display* display, const char* title);
void displayRender(Display* display, Chip8* chip8, const char* colorArg);
void displayCleanup(Display* display);

#endif // DISPLAY_H