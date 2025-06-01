#include "input.h"

// Procesar eventos de entrada
bool inputProcess(SDL_Event* event, Chip16* chip16, Display* display) {
    bool quit = false;
    
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                quit = true;
                break;
                
            case SDL_KEYDOWN:
                if (event->key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                } else if (event->key.keysym.sym == SDLK_F1) {
                    // Reiniciar emulador
                    chip16Init(chip16);
                    return false;  // Continuar ejecución
                } 
                else if (event->key.keysym.sym == SDLK_F2) {
                    // Toggle del efecto de ciclo de color
                    if (chip16->currentEffect == EFFECT_COLOR_CYCLE) {
                        chip16SetEffect(chip16, EFFECT_NONE);
                        printf("Efecto de ciclo de color: DESACTIVADO\n");
                    } else {
                        chip16SetEffect(chip16, EFFECT_COLOR_CYCLE);
                        printf("Efecto de ciclo de color: ACTIVADO\n");
                    }
                }

                else if (event->key.keysym.sym == SDLK_F3) {
                    // Toggle del modo ventana dual
                    displayToggleDualWindow(display, chip16);
                }

                else {
                    // Mapear otras teclas al teclado CHIP-8
                    inputMapKey(event, chip16, true);
                }
                break;
                
            case SDL_KEYUP:
                // Mapear tecla liberada al teclado CHIP-8
                inputMapKey(event, chip16, false);
                break;
        }
    }
    
    return quit;
}

// Mapear teclas SDL a teclas CHIP-8
void inputMapKey(SDL_Event* event, Chip16* chip16, bool keyDown) {
    uint8_t keyValue = keyDown ? 1 : 0;
    SDL_Keycode key = event->key.keysym.sym;
    
    // Mapeo basado en la disposición original del teclado hexadecimal CHIP-8
    // 1 2 3 C    ->    1 2 3 4
    // 4 5 6 D    ->    Q W E R
    // 7 8 9 E    ->    A S D F
    // A 0 B F    ->    Z X C V
    
    switch (key) {
        case SDLK_x: chip16SetKey(chip16, 0x0, keyValue); break;  // 0
        case SDLK_1: chip16SetKey(chip16, 0x1, keyValue); break;  // 1
        case SDLK_2: chip16SetKey(chip16, 0x2, keyValue); break;  // 2
        case SDLK_3: chip16SetKey(chip16, 0x3, keyValue); break;  // 3
        case SDLK_q: chip16SetKey(chip16, 0x4, keyValue); break;  // 4
        case SDLK_w: chip16SetKey(chip16, 0x5, keyValue); break;  // 5
        case SDLK_e: chip16SetKey(chip16, 0x6, keyValue); break;  // 6
        case SDLK_a: chip16SetKey(chip16, 0x7, keyValue); break;  // 7
        case SDLK_s: chip16SetKey(chip16, 0x8, keyValue); break;  // 8
        case SDLK_d: chip16SetKey(chip16, 0x9, keyValue); break;  // 9
        case SDLK_z: chip16SetKey(chip16, 0xA, keyValue); break;  // A
        case SDLK_c: chip16SetKey(chip16, 0xB, keyValue); break;  // B
        case SDLK_4: chip16SetKey(chip16, 0xC, keyValue); break;  // C
        case SDLK_r: chip16SetKey(chip16, 0xD, keyValue); break;  // D
        case SDLK_f: chip16SetKey(chip16, 0xE, keyValue); break;  // E
        case SDLK_v: chip16SetKey(chip16, 0xF, keyValue); break;  // F
    }
}