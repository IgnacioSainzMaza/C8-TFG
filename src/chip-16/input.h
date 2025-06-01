#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "chip16.h"
#include "display.h"

// Manejo de entrada del usuario
bool inputProcess(SDL_Event* event, Chip16* chip16, Display* display);
void inputMapKey(SDL_Event* event, Chip16* chip16, bool keyDown);

#endif // INPUT_H