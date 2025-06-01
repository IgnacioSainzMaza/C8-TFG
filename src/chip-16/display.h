#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include "chip16.h"

// Estructura para gestionar la visualización
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    char windowTitle[256];

    SDL_Window* debugWindow;        // Ventana de debug
    SDL_Renderer* debugRenderer;    // Renderer de debug
    SDL_Texture* debugTexture;      // Textura de debug
    char debugWindowTitle[256]; // Título de la ventana de debug
    bool dualWindowMode;            // ¿Modo doble ventana activo?
} Display;

// Funciones de visualización
bool displayInit(Display* display, const char* title);
void displayRender(Display* display, Chip16* chip16, const char* colorArg);
void displayCleanup(Display* display);
void displayToggleDualWindow(Display* display, Chip16* chip16);

#endif // DISPLAY_H