#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "chip8.h"

void initChip8();
void draw();
void execute();
void cleanSDL();
uint32_t loadC8File(char *file);

SDL_Renderer *renderer;
SDL_Window *window;
SDL_Texture *screen;

int main(int argc, char **argv)
{
    uint32_t quit = 0;
    // Inicialización de SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "SDL Failed %s\n", SDL_GetError());
        return 0;
    }

    SDL_Event event;

    char title[256];
    sprintf(title, "CHIP-8-TFG: %s", argv[1]);
    // SDL_CreateWindow -> Creates a window with the specified position,dimensions and flag
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 320, 0);

    // SDL_CreateRenderer -> Creates a 2D Rendering context for a window.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // SDL_RenderSetLogicalSize -> Set a device independent resolution for a window.
    SDL_RenderSetLogicalSize(renderer, 64, 32);

    // SDL_RenderDrawColor -> Set the color used for drawing operations
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // SDL_RenderClear -> Clear the current rendering target with the drawing color.
    SDL_RenderClear(renderer);

    // SDL_CreateTexture -> Create a texturee for a rendering context.
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    // bool running = true;
    // while (running)
    // {
    //     while (SDL_PollEvent(&event))
    //     {
    //         if (event.type == SDL_QUIT)
    //         {
    //             running = false;
    //         }
    //     }
    //     // Aquí renderiza lo que se dibuja
    //     SDL_RenderClear(renderer);
    //     SDL_RenderCopy(renderer, screen, NULL, NULL);
    //     SDL_RenderPresent(renderer);
    // }

    initChip8();
    if (loadC8File(argv[1]) == 0)
    {
        cleanSDL();
        return 0;
    }
    int32_t speed = 5;

    while (!quit)
    {
        printf("Entra en el bucle de ejecución: %d\n", speed);
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    quit = 1;
                    break;

                case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;
                    case SDLK_F1:
                        initChip8();
                        if (loadC8File(argv[1]) == 0)
                        {
                            cleanSDL();
                            return 0;
                        }
                        break;
                    case SDLK_x:
                        keyboard[0] = 1;
                        break;
                    case SDLK_1:
                        keyboard[1] = 1;
                        break;
                    case SDLK_2:
                        keyboard[2] = 1;
                        break;
                    case SDLK_3:
                        keyboard[3] = 1;
                        break;
                    case SDLK_q:
                        keyboard[4] = 1;
                        break;
                    case SDLK_w:
                        keyboard[5] = 1;
                        break;
                    case SDLK_e:
                        keyboard[6] = 1;
                        break;
                    case SDLK_a:
                        keyboard[7] = 1;
                        break;
                    case SDLK_s:
                        keyboard[8] = 1;
                        break;
                    case SDLK_d:
                        keyboard[9] = 1;
                        break;
                    case SDLK_z:
                        keyboard[0XA] = 1;
                        break;
                    case SDLK_c:
                        keyboard[0XB] = 1;
                        break;
                    case SDLK_4:
                        keyboard[0XC] = 1;
                        break;
                    case SDLK_r:
                        keyboard[0XD] = 1;
                        break;
                    case SDLK_f:
                        keyboard[0XE] = 1;
                        break;
                    case SDLK_v:
                        keyboard[0XF] = 1;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                case SDLK_x:
                    keyboard[0] = 0;
                    break;
                case SDLK_1:
                    keyboard[1] = 0;
                    break;
                case SDLK_2:
                    keyboard[2] = 0;
                    break;
                case SDLK_3:
                    keyboard[3] = 0;
                    break;
                case SDLK_q:
                    keyboard[4] = 0;
                    break;
                case SDLK_w:
                    keyboard[5] = 0;
                    break;
                case SDLK_e:
                    keyboard[6] = 0;
                    break;
                case SDLK_a:
                    keyboard[7] = 0;
                    break;
                case SDLK_s:
                    keyboard[8] = 0;
                    break;
                case SDLK_d:
                    keyboard[9] = 0;
                    break;
                case SDLK_z:
                    keyboard[0XA] = 0;
                    break;
                case SDLK_c:
                    keyboard[0XB] = 0;
                    break;
                case SDLK_4:
                    keyboard[0XC] = 0;
                    break;
                case SDLK_r:
                    keyboard[0XD] = 0;
                    break;
                case SDLK_f:
                    keyboard[0XE] = 0;
                    break;
                case SDLK_v:
                    keyboard[0XF] = 0;
                    break;
                }
                break;
            }
            break;
        }
        if (speed < 0)
        {
            speed = 0;
        }
        else
        {
            SDL_Delay(speed);
        }
        if (delay_timer > 0)
            --delay_timer;

        execute();
        draw();
    }

    cleanSDL();
    return 0;
}

// Función que se encarga de inicializar todas las variables
void initChip8()
{
    delay_timer = 0;
    sound_timer = 0;
    opcode = 0;
    PC = 0x200;
    I = 0;
    sp = 0;

    // memset es la asignación de memoria utilizada para las variables de tipo array.
    memset(stack, 0, 16);
    memset(memory, 0, MEMORY);
    memset(v, 0, 16);
    memset(gfx, 0, 2048);
    memset(keyboard, 0, 16);

    // memcpy lo que hace es copiar en memory lo que hay en la variable fontset.
    memcpy(memory, fontset, 80 * sizeof(int8_t));
}

// Función de dibujo
void draw()
{
    uint32_t pixels[64 * 32];
    unsigned int x, y;

    if (drawflag)
    {
        memset(pixels, 0, (64 * 32) * 4);
        for (x = 0; x < 64; x++)
        {
            for (y = 0; y < 32; y++)
            {

                if (gfx[(x) + ((y) * 64)] == 1)
                {
                    pixels[(x) + ((y) * 64)] = UINT32_MAX;
                }
            }
        }

        SDL_UpdateTexture(screen, NULL, pixels, 64 * sizeof(uint32_t));
        SDL_Rect position;
        position.x = 0;
        position.y = 0;
        position.w = 64;
        position.h = 32;
        SDL_RenderCopy(renderer, screen, NULL, &position);
        SDL_RenderPresent(renderer);
    }
    drawflag = false;
}

void cleanSDL()
{
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void execute()
{
    uint16_t nnn;
    uint32_t i, key_d;
    uint8_t x, y, kk, n;

    opcode = memory[PC] << 8 | memory[PC + 1];
    PC += 2;
    // Se aplican máscaras para sacar los valores correspondientes
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    nnn = (opcode & 0x0FFF);
    kk = (opcode & 0x00FF);
    n = (opcode & 0x000F);

    switch (opcode & 0xF000)
    {
    // 0nnn - 00E0 - 00EE
    case 0x0000:
        // 0nnn no se añade en intérpretes modernos
        switch (opcode & 0x00FF)
        {
        // 00E0 - CLS
        case 0x00E0:
            memset(gfx, 0, 2048);
            drawflag = true;
            break;

        // 00EE - RET
        case 0x00EE:
            
            --sp;
            PC = stack[sp];
            break;

        default:
        printf("Opcode error 0xxx");
            break;
        }

        break;

    // 1nnn - JP addr
    case 0x1000:
        PC = nnn;
        break;

    // 2nnn - CALL addr
    case 0x2000:

        stack[sp] = nnn;
        ++sp;
        PC = nnn;

        break;

    // 3xkk - SE Vx, byte
    case 0x3000:
        if (v[x] == kk)
        {
            PC += 2;
        }

        break;

    // 4xkk - SNE Vx, byte
    case 0x4000:
        if (v[x] != kk)
        {
            PC += 2;
        }
        break;

    // 5xy0 - SE Vx, Vy
    case 0x5000:
        if (v[x] == v[y])
        {
            PC += 2;
        }
        break;

    // 6xkk - LD Vx, byte
    case 0x6000:
        v[x] = kk;
        break;

    // 7xkk
    case 0x7000:
        v[x] = v[x] + kk;
        break;

    // 8xy0 - 8xy1 - 8xy2 - 8xy3 - 8xy4 - 8xy5 - 8xy6 - 8xy7 - 8xyE
    case 0x8000:
        switch (n)
        {
        // 8xy0 - LD Vx,Vy
        case 0x0000:
            v[x] = v[y];
            break;

        // 8xy1 - OR Vx,Vy
        case 0x0001:
            v[x] = v[x] | v[y];
            break;

        // 8xy2 - AND Vx,Vy
        case 0x0002:
            v[x] = v[x] & v[y];
            break;

        // 8xy3 - XOR Vx,Vy
        case 0x0003:
            v[x] = v[x] ^ v[y];
            break;

        // 8xy4 - ADD Vx,Vy
        case 0x0004:
            int temp;
            temp = (int)(v[x]) + (int)(v[y]);
            if (temp > 255)
                v[0xF] = 1;
            else
                v[0xF] = 0;
            v[x] = 0x0FF & temp;
            break;

        // 8xy5 - SUB Vx,Vy
        case 0x0005:
            if (v[x] > v[y])
                v[0xF] = 1;
            else
                v[0xF] = 0;
            v[x] = v[x] - v[y];
            break;

        // 8xy6 - SHR Vx, {Vy}
        case 0x0006:
            v[0xF] = v[x] & 1;
            v[x] >>= 1;
            break;

        // 8xy7 - SUBN Vx,Vy
        case 0x0007:
            if (v[y] > v[x])
                v[0xF] = 1;
            else
                v[0xF] = 0;
            v[x] = v[y] - v[x];
            break;

        // 8xyE - SHL Vx, {Vy}
        case 0x000E:
            v[0xF] = v[x] >> 7;
            v[x] = v[x] << 1;
            break;

        default:
            break;
        }
        break;

    // 9xy0 - SNE Vx, Vy
    case 0x9000:
        if (v[x] != v[y])
            PC += 2;
        break;

    // Annn - LD I, addr
    case 0xA000:
        I = nnn;
        break;

    // Bnnn - JP V0, addr
    case 0xB000:
        PC = v[0x000] + nnn;
        break;

    // Cxkk - RND Vx, byte
    case 0xC000:
        v[x] = (rand() % 0x100) & kk;
        break;

    // Dxyn - DRW Vx,Vy, nibble
    case 0xD000:
        uint16_t xx = v[x];
			uint16_t yy = v[y];
			uint16_t height = n;
			uint8_t pixel;

			v[0xF] = 0;
			for (int yline = 0; yline < height; yline++) {
				pixel = memory[I + yline];
				for(int xline = 0; xline < 8; xline++) {
					if((pixel & (0x80 >> xline)) != 0) {
						if(gfx[(xx + xline + ((yy + yline) * 64))] == 1){
							v[0xF] = 1;                                   
						}
						gfx[xx + xline + ((yy+ yline) * 64)] ^= 1;
					}

				}

			}
			drawflag = true;
			break;
			
        break;

    // Ex9E - ExA1
    case 0xE000:
        switch (kk)
        {
        // Ex9E - SKP Vx
        case 0x009E:
            if (keyboard[v[x]] != 0)
            {
                PC += 2;
            }
            break;
        // ExA1 - SKNP Vx
        case 0x00A1:
            if (keyboard[v[x]] == 0)
            {
                PC += 2;
            }
            break;

        default:
            break;
        }

        break;

    // Fx07 - Fx0A - Fx15 - Fx18 - Fx1E - Fx29 - Fx33 - Fx55 - Fx65
    case 0xF000:
        switch (kk)
        {
        // Fx07 - LD Vx,DT
        case 0x0007:
            v[x] = delay_timer;
            break;

        // Fx0A - LD Vx,K
        case 0x000A:
            key_d = 0;
            for (i = 0; i < 16; i++)
            {
                if (keyboard[i])
                {
                    key_d = 1;
                    v[x] = i;
                }
            }
            if (key_d == 0)
            {
                PC -= 2;
            }
            break;
        // Fx15 - LD DT,Vx
        case 0x0015:
            delay_timer = v[x];
            break;
        // Fx18 - LD ST,Vx
        case 0x0018:
            sound_timer = v[x];
            break;
        // Fx1E - ADD I,Vx
        case 0x001E:
            I += v[x];
            break;
        // Fx29 - LD F,Vx
        case 0x0029:
            I = v[x] * 5;
            break;
        // Fx33 - LD B,Vx
        case 0x0033:
            int vX;
            vX = v[x];
            memory[I] = (vX - (vX % 100)) / 100;
            vX -= memory[I] * 100;
            memory[I + 1] = (vX - (vX % 10)) / 10;
            vX -= memory[I + 1] * 10;
            memory[I + 2] = vX;
            break;
        // Fx55 - LD [I],Vx
        case 0x0055:
            for (u_int8_t i = 0; i <= x; ++i)
            {
                memory[I + i] = v[i];
            }
            break;
        // Fx65 - LD Vx,[I]
        case 0x0065:
            for (u_int8_t i = 0; i <= x; ++i)
            {
                v[i] = memory[I + i];
            }
            break;
        }
        break;
    }
}

uint32_t loadC8File(char *file)
{
    // fopen - rb - Apertura de un archivo binario existente para su lectura.
    FILE *fp = fopen(file, "rb");

    if (fp == NULL)
    {
        fprintf(stderr, "This file cannot be opened\n");
        return 0;
    }

    // fseek - Seek to a certain position on STREAM. SEEK_END - Seeks from end of file
    fseek(fp, 0, SEEK_END);
    // ftell - Return the current position of STREAM.
    int size = ftell(fp);
    // SEEK_SET - Seek from beggining of file.
    fseek(fp, 0, SEEK_SET);

    fread(memory + 0x200, sizeof(uint16_t), size, fp);
    return 1;
}