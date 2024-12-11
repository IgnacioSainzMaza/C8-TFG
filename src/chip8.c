#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "chip8.h"

void initChip8();
void draw();
void execute();
void cleanUp_SDL();

SDL_Renderer * renderer;
SDL_Window  * window;
SDL_Texture * screen;

int main(int argc, char** argv){


    //Inicialización de SDL
    if (SDL_Init(SDL_INIT_EVERYTHING)!=0){
        fprintf(stderr,"SDL Failed %s\n",SDL_GetError());
        return 0;
    }
    

    SDL_Event event;

    //SDL_CreateWindow -> Creates a window with the specified position,dimensions and flag
    window = SDL_CreateWindow(("CHIP-8-TFG"), SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 640, 320, 0);

    //SDL_CreateRenderer -> Creates a 2D Rendering context for a window.
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    //SDL_RenderSetLogicalSize -> Set a device independent resolution for a window.
    SDL_RenderSetLogicalSize(renderer,64,32);

    //SDL_RenderDrawColor -> Set the color used for drawing operations
    SDL_SetRenderDrawColor(renderer,0,0,0,255);

    //SDL_RenderClear -> Clear the current rendering target with the drawing color.
    SDL_RenderClear(renderer);
    
    //SDL_CreateTexture -> Create a texturee for a rendering context.
    screen = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,64,32);

    bool running = true;
    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT){
                running = false;
            }
        }
        //Aquí renderiza lo que se dibuja
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer,screen,NULL,NULL);
        SDL_RenderPresent(renderer);
    }

    
    return 0;
}