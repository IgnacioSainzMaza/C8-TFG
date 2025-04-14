#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "chip8.h"
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
    snprintf(title, sizeof(title), "CHIP-8 Emulator: %s", argv[1]);
    
    // Inicializar componentes
    Chip8 chip8;
    Display display;
    
    // Inicializar emulador
    chip8Init(&chip8);
    
    // Inicializar pantalla
    if (!displayInit(&display, title)) {
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    // Cargar ROM
    if (!chip8LoadROM(&chip8, argv[1])) {
        displayCleanup(&display);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    // Variables para control de tiempo
    Uint32 lastCycleTime = SDL_GetTicks();
    Uint32 lastTimerUpdate = lastCycleTime;
    int instructionsPerSecond = 700;  // Configuración predeterminada
    bool quit = false;
    SDL_Event event;
    
    // Bucle principal de emulación
    while (!quit) {
        // Procesar entrada
        quit = inputProcess(&event, &chip8);
        
        // Actualizar temporizadores a 60Hz (cada ~16.67ms)
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTimerUpdate >= 16) {
            chip8UpdateTimers(&chip8);
            lastTimerUpdate = currentTime;
        }
        
        // Ejecutar instrucciones a velocidad constante
        int cycleTarget = (currentTime - lastCycleTime) * instructionsPerSecond / 1000;
        if (cycleTarget > 0) {
            for (int i = 0; i < cycleTarget; i++) {
                chip8Cycle(&chip8);
            }
            lastCycleTime = currentTime;
        }
        
        // Renderizar pantalla si es necesario
        displayRender(&display, &chip8, argc > 2 ? argv[2] : NULL);
        
        // Pequeña pausa para evitar uso excesivo de CPU
        SDL_Delay(1);
    }
    
    // Liberar recursos
    displayCleanup(&display);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}