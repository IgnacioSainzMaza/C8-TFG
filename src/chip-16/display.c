#include <stdio.h>
#include <string.h>
#include "display.h"

// Inicializar el subsistema de visualización
bool displayInit(Display* display, const char* title) {
    // Copiar título de la ventana
    strncpy(display->windowTitle, title, 255);
    display->windowTitle[255] = '\0';
    display->dualWindowMode = false;
    display->debugWindow = NULL;
    display->debugRenderer = NULL;
    display->debugTexture = NULL;
    
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

void displayToggleDualWindow(Display* display, Chip16* chip16) {
    if (!display->dualWindowMode) {
        // Activar modo dual - crear segunda ventana
        char debugTitle[256];
        snprintf(debugTitle, sizeof(debugTitle), "%s - Buffer Original (Sin Efectos)", 
                 display->windowTitle);
        
        // Crear ventana de debug a la derecha de la principal
        int mainX, mainY;
        SDL_GetWindowPosition(display->window, &mainX, &mainY);
        
        display->debugWindow = SDL_CreateWindow(
            debugTitle,
            mainX + WINDOW_WIDTH,  // 20 pixels de separación
            mainY,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            0
        );
        
        if (display->debugWindow == NULL) {
            fprintf(stderr, "Error al crear ventana de debug: %s\n", SDL_GetError());
            return;
        }
        
        // Crear renderer de debug
        display->debugRenderer = SDL_CreateRenderer(
            display->debugWindow,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );
        
        if (display->debugRenderer == NULL) {
            SDL_DestroyWindow(display->debugWindow);
            display->debugWindow = NULL;
            return;
        }
        
        SDL_RenderSetLogicalSize(display->debugRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        
        // Crear textura de debug
        display->debugTexture = SDL_CreateTexture(
            display->debugRenderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT
        );
        
        if (display->debugTexture == NULL) {
            SDL_DestroyRenderer(display->debugRenderer);
            SDL_DestroyWindow(display->debugWindow);
            display->debugRenderer = NULL;
            display->debugWindow = NULL;
            return;
        }
        
        display->dualWindowMode = true;
        printf("Modo ventana dual: ACTIVADO\n");
        
        // Forzar redibujado
        chip16->drawFlag = true;
        
    } else {
        // Desactivar modo dual - destruir segunda ventana
        if (display->debugTexture) {
            SDL_DestroyTexture(display->debugTexture);
            display->debugTexture = NULL;
        }
        if (display->debugRenderer) {
            SDL_DestroyRenderer(display->debugRenderer);
            display->debugRenderer = NULL;
        }
        if (display->debugWindow) {
            SDL_DestroyWindow(display->debugWindow);
            display->debugWindow = NULL;
        }
        
        display->dualWindowMode = false;
        printf("Modo ventana dual: DESACTIVADO\n");
    }
}

// Renderizar el estado actual del emulador
void displayRender(Display* display, Chip16* chip16, const char* colorArg) {
    if (!chip16->drawFlag) {
        return;  // No hay necesidad de actualizar la pantalla
    }

    chip16ProcessEffects(chip16);
    
    // Determinar el color del pixel
    uint32_t pixelColor;
    // if (colorArg != NULL) {
    //     // Convertir argumento de color de string a uint32
    //     pixelColor = strtoul(colorArg, NULL, 16);
    // } else {
    //     // Usar color predeterminado
    //     pixelColor = chip16->config.pixelColor;
    // }

     // === NUEVO: Lógica de selección de color según el efecto ===
     if (chip16->currentEffect == EFFECT_COLOR_CYCLE) {
        // Si el efecto está activo, usar el color de la paleta
        pixelColor = COLOR_PALETTE[chip16->colorIndex];
    } else if (colorArg != NULL) {
        // Si no hay efecto pero hay argumento de línea de comandos
        pixelColor = strtoul(colorArg, NULL, 16);
    } else {
        // Color por defecto
        pixelColor = chip16->config.pixelColor;
    }
    
    // Crear buffer de pixels para la textura
    uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    memset(pixels, 0, sizeof(pixels));  // Limpiar buffer con negro
    
    // Convertir el estado de gfx[] a pixeles con color
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int index = x + (y * DISPLAY_WIDTH);
            if (chip16->gfx2Buffer[index] == 1) {
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
    

    if (display->dualWindowMode && display->debugWindow) {
        // Limpiar buffer de pixels
        memset(pixels, 0, sizeof(pixels));
        
        // Determinar color para la ventana de debug (siempre sin efectos)
        uint32_t debugColor;
        if (colorArg != NULL) {
            debugColor = strtoul(colorArg, NULL, 16);
        } else {
            debugColor = chip16->config.pixelColor;
        }
        
        // Usar gfx[] original para la ventana de debug
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                int index = x + (y * DISPLAY_WIDTH);
                if (chip16->gfx[index] == 1) {  // <-- Nota: usamos gfx, no effectBuffer
                    pixels[index] = debugColor;
                }
            }
        }
        
        SDL_UpdateTexture(display->debugTexture, NULL, pixels, DISPLAY_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(display->debugRenderer);
        SDL_RenderCopy(display->debugRenderer, display->debugTexture, NULL, NULL);
        SDL_RenderPresent(display->debugRenderer);
    }
    // Restablecer flag de dibujo
    chip16->drawFlag = false;
}

// Liberar recursos del subsistema de visualización
void displayCleanup(Display* display) {

    // Limpiar ventana de debug si existe
    if (display->debugTexture != NULL) {
        SDL_DestroyTexture(display->debugTexture);
    }
    if (display->debugRenderer != NULL) {
        SDL_DestroyRenderer(display->debugRenderer);
    }
    if (display->debugWindow != NULL) {
        SDL_DestroyWindow(display->debugWindow);
    }

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