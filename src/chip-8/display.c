#include <stdio.h>
#include <string.h>
#include "display.h"

// Inicializar el subsistema de visualización
bool displayInit(Display* display, const char* title) {
    // Copiar título de la ventana
    strncpy(display->windowTitle, title, 255);
    display->windowTitle[255] = '\0';
    
    // Crear ventana SDL
    display->window = SDL_CreateWindow(
        display->windowTitle,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );
    
    if (display->window == NULL) {
        fprintf(stderr, "Error al crear ventana SDL: %s\n", SDL_GetError());
        return false;
    }
    
    // Crear renderer
    display->renderer = SDL_CreateRenderer(
        display->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (display->renderer == NULL) {
        fprintf(stderr, "Error al crear renderer SDL: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        return false;
    }
    
    // Configurar tamaño lógico del renderer
    SDL_RenderSetLogicalSize(display->renderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Establecer color de fondo (negro)
    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
    SDL_RenderClear(display->renderer);
    
    // Crear textura para el framebuffer
    display->texture = SDL_CreateTexture(
        display->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );
    
    if (display->texture == NULL) {
        fprintf(stderr, "Error al crear textura SDL: %s\n", SDL_GetError());
        SDL_DestroyRenderer(display->renderer);
        SDL_DestroyWindow(display->window);
        return false;
    }
    
    return true;
}

// Renderizar el estado actual del emulador
void displayRender(Display* display, Chip8* chip8, const char* colorArg) {
    if (!chip8->drawFlag) {
        return;  // No hay necesidad de actualizar la pantalla
    }
    
    // Determinar el color del pixel
    uint32_t pixelColor;
    if (colorArg != NULL) {
        // Convertir argumento de color de string a uint32
        pixelColor = strtoul(colorArg, NULL, 16);
    } else {
        // Usar color predeterminado
        pixelColor = chip8->config.pixelColor;
    }
    
    // Crear buffer de pixels para la textura
    uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    memset(pixels, 0, sizeof(pixels));  // Limpiar buffer con negro
    
    // Convertir el estado de gfx[] a pixeles con color
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int index = x + (y * DISPLAY_WIDTH);
            if (chip8->gfx[index] == 1) {
                pixels[index] = pixelColor;
            }
        }
    }
    
    // Actualizar textura con nuevos datos
    SDL_UpdateTexture(display->texture, NULL, pixels, DISPLAY_WIDTH * sizeof(uint32_t));
    
    // Renderizar
    SDL_RenderClear(display->renderer);
    SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
    SDL_RenderPresent(display->renderer);
    
    // Restablecer flag de dibujo
    chip8->drawFlag = false;
}

// Liberar recursos del subsistema de visualización
void displayCleanup(Display* display) {
    if (display->texture != NULL) {
        SDL_DestroyTexture(display->texture);
    }
    
    if (display->renderer != NULL) {
        SDL_DestroyRenderer(display->renderer);
    }
    
    if (display->window != NULL) {
        SDL_DestroyWindow(display->window);
    }
}