#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "chip16.h"
#include "display.h"
#include "input.h"

int main(int argc, char** argv) {
    // Verificar argumentos
    if (argc < 2) {
        printf("Uso: %s <archivo-rom> [color-pixel-hex]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Error al inicializar SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    // Crear título de ventana
    char title[256];
    snprintf(title, sizeof(title), "TFG -> Emulador CHIP-16: %s", argv[1]);
    
    // Inicializar componentes
    Chip16 chip16;
    Display display;
    memset(&chip16, 0, sizeof(Chip16));
    // Inicializar emulador
    chip16Init(&chip16);
    
    // Inicializar pantalla
    if (!displayInit(&display, title)) {
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    // Cargar ROM
    if (!chip16LoadROM(&chip16, argv[1])) {
        displayCleanup(&display);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    // Variables para control de tiempo
    Uint32 lastCycleTime = SDL_GetTicks();
    Uint32 lastTimerUpdate = lastCycleTime;
    Uint32 lastRenderTime = lastCycleTime;
    int instructionsPerSecond = 1200;  // Configuración predeterminada
    bool quit = false;
    SDL_Event event;
    
    
    // Bucle principal de emulación
    while (!quit) {
        // Procesar entrada
        quit = inputProcess(&event, &chip16, &display);
        
        // Actualizar temporizadores a 60Hz 
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTimerUpdate >= 16) {
            chip16UpdateTimers(&chip16);
            lastTimerUpdate = currentTime;
        }
        
        // Ejecutar instrucciones a velocidad constante
        int cycleTarget = (currentTime - lastCycleTime) * instructionsPerSecond / 1000;
        if (cycleTarget > 0) {
            for (int i = 0; i < cycleTarget; i++) {
                chip16Cycle(&chip16);
            }
            lastCycleTime = currentTime;
        }
        
        // Renderizar pantalla si es necesario
        if (currentTime - lastRenderTime >= 16) {
            displayRender(&display, &chip16, argc > 2 ? argv[2] : NULL);
            lastRenderTime = currentTime;
        }

        SDL_Delay(1); // Pequeña pausa para evitar uso excesivo de CPU
    }
    
    // Liberar recursos
    SDL_CloseAudioDevice(chip16.config.beepState.dev);
    displayCleanup(&display);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}