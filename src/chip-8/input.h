#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "chip8.h"

// Manejo de entrada del usuario
bool inputProcess(SDL_Event* event, Chip8* chip8);
void inputMapKey(SDL_Event* event, Chip8* chip8, bool keyDown);

#endif // INPUT_H