#include <stdio.h>
#include <stdlib.h>
#include <string.h>         
#include <SDL2/SDL.h>
#include "chip16.h"
#include "display.h"
#include "input.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Error al inicializar SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    char title[256];
    snprintf(title, sizeof(title), "TFG -> Emulador CHIP-16: %s", argv[1]);
    
    Chip16 chip16;
    Display display;
    memset(&chip16, 0, sizeof(Chip16));
    chip16Init(&chip16);
    if (chip16.config.enableSound) {
    if (!chip16AudioInit(&chip16)) {
        fprintf(stderr, "Advertencia: audio no disponible\n");
        chip16.config.enableSound = false;
    }
}

    // Leer argumento de modo (argv[3], opcional)
    if (argc >= 4) {
        if (strcmp(argv[3], "8") == 0) {
            chip16.mode = MODE_8BIT;
            printf("Modo de emulación: CHIP-8 (8 bits)\n");
        } else if (strcmp(argv[3], "16") == 0) {
            chip16.mode = MODE_16BIT;
            printf("Modo de emulación: CHIP-16 (16 bits)\n");
        } else {
            fprintf(stderr, "Advertencia: modo '%s' no reconocido. Usando 16 bits.\n", argv[3]);
        }
    }

    // Leer argumento de color (argv[2], opcional)  // 
    const char* colorArg = NULL;
    if (argc > 2 && strcmp(argv[2], "-") != 0) {
        colorArg = argv[2];
    }
    
    if (!displayInit(&display, title)) {
        printf("Error en displayInit\n");  // 
        SDL_Quit();
        return EXIT_FAILURE;
    }
    printf("DisplayInit completado\n"); 
    const char* romPath = argv[1];
    
    if (!chip16LoadROM(&chip16, romPath)) {
        chip16AudioCleanup(&chip16);
        displayCleanup(&display);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    Uint32 lastCycleTime   = SDL_GetTicks();
    Uint32 lastTimerUpdate = lastCycleTime;
    Uint32 lastRenderTime  = lastCycleTime;
    int instructionsPerSecond = 1200;
    bool quit = false;
    SDL_Event event;
    
    while (!quit) {
        quit = inputProcess(&event, &chip16, &display, romPath);  
        
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTimerUpdate >= 16) {
            chip16UpdateTimers(&chip16);
            lastTimerUpdate = currentTime;
        }
        
        int cycleTarget = (currentTime - lastCycleTime) * instructionsPerSecond / 1000;
        if (cycleTarget > 0) {
            for (int i = 0; i < cycleTarget; i++) {
                chip16Cycle(&chip16);
            }
            lastCycleTime = currentTime;
        }
        
        if (currentTime - lastRenderTime >= 16) {
            displayRender(&display, &chip16, colorArg);  // <- CAMBIAR
            lastRenderTime = currentTime;
        }

        SDL_Delay(1);
    }
    
    chip16AudioCleanup(&chip16);
    displayCleanup(&display);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}