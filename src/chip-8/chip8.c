#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8.h"

// Inicialización del emulador CHIP-8
void chip8Init(Chip8 *chip8)
{
    // Inicializar configuración predeterminada
    chip8->config.debugLevel = DEBUG_NONE;
    chip8->config.clockSpeed = DEFAULT_SPEED;
    chip8->config.enableSound = true;
    chip8->config.pixelColor = DEFAULT_PIXEL_COLOR;

    // Inicializar registros y memoria
    memset(chip8->memory, 0, MEMORY_SIZE);
    memset(chip8->V, 0, REGISTER_COUNT);
    memset(chip8->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(chip8->key, 0, KEY_COUNT);
    memset(chip8->stack, 0, STACK_SIZE * sizeof(uint16_t));

    chip8->opcode = 0;
    chip8->I = 0;
    chip8->PC = ROM_LOAD_ADDRESS; // Los programas comienzan en 0x200
    chip8->SP = 0;
    chip8->delayTimer = 0;
    chip8->soundTimer = 0;
    chip8->drawFlag = false;

    // Cargar fuente en memoria
    memcpy(chip8->memory, chip8_fontset, FONTSET_SIZE);

    // Inicializar semilla para números aleatorios
    srand(time(NULL));
}

// Cargar ROM desde archivo
bool chip8LoadROM(Chip8 *chip8, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", filename);
        return false;
    }

    // Determinar tamaño del archivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Verificar que la ROM cabe en memoria
    if (fileSize > MEMORY_SIZE - ROM_LOAD_ADDRESS)
    {
        fprintf(stderr, "Error: La ROM es demasiado grande para la memoria\n");
        fclose(file);
        return false;
    }

    // Leer ROM en memoria
    size_t bytesRead = fread(&chip8->memory[ROM_LOAD_ADDRESS], 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize)
    {
        fprintf(stderr, "Error: No se pudo leer el archivo completo\n");
        return false;
    }

    return true;
}

// Actualizar temporizadores
void chip8UpdateTimers(Chip8 *chip8)
{
    if (chip8->delayTimer > 0)
    {
        chip8->delayTimer--;
    }

    if (chip8->soundTimer > 0)
    {
        if (chip8->config.enableSound && chip8->soundTimer == 1)
        {
            // Aquí se implementaría la reproducción de sonido
            printf("BEEP!\n");
        }
        chip8->soundTimer--;
    }
}

// Establecer estado de una tecla
void chip8SetKey(Chip8 *chip8, uint8_t key, uint8_t value)
{
    if (key < KEY_COUNT)
    {
        chip8->key[key] = value;
    }
}

// Ejecutar un ciclo de emulación
void chip8Cycle(Chip8 *chip8)
{
    // Extraer opcode (2 bytes)
    chip8->opcode = (chip8->memory[chip8->PC] << 8) | chip8->memory[chip8->PC + 1];

    // Incrementar PC antes de ejecutar
    chip8->PC += 2;

    // Variables para decodificación del opcode
    uint8_t x = (chip8->opcode & 0x0F00) >> 8;
    uint8_t y = (chip8->opcode & 0x00F0) >> 4;
    uint8_t n = chip8->opcode & 0x000F;
    uint8_t kk = chip8->opcode & 0x00FF;
    uint16_t nnn = chip8->opcode & 0x0FFF;

    // Depuración si está habilitada
    if (chip8->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("Ejecutando opcode: 0x%04X en PC=0x%04X\n", chip8->opcode, chip8->PC - 2);
    }

    // Decodificar e implementar opcode
    switch (chip8->opcode & 0xF000)
    {
    case 0x0000:
        switch (kk)
        {
        case 0x00E0: // 00E0: Limpiar pantalla
            memset(chip8->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            chip8->drawFlag = true;
            break;

        case 0x00EE: // 00EE: Retornar de subrutina
            chip8->SP--;
            chip8->PC = chip8->stack[chip8->SP];
            break;

        default:
            if (chip8->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("Opcode desconocido: 0x%04X\n", chip8->opcode);
            }
        }
        break;

    case 0x1000: // 1NNN: Saltar a dirección NNN
        chip8->PC = nnn;
        break;

    case 0x2000: // 2NNN: Llamar subrutina en NNN
        chip8->stack[chip8->SP] = chip8->PC;
        chip8->SP++;
        chip8->PC = nnn;
        break;

    case 0x3000: // 3XKK: Saltar siguiente instrucción si VX == KK
        if (chip8->V[x] == kk)
        {
            chip8->PC += 2;
        }
        break;

    case 0x4000: // 4XKK: Saltar siguiente instrucción si VX != KK
        if (chip8->V[x] != kk)
        {
            chip8->PC += 2;
        }
        break;

    case 0x5000: // 5XY0: Saltar siguiente instrucción si VX == VY
        if (n == 0 && chip8->V[x] == chip8->V[y])
        {
            chip8->PC += 2;
        }
        break;

    case 0x6000: // 6XKK: Establecer VX = KK
        chip8->V[x] = kk;
        break;

    case 0x7000: // 7XKK: Establecer VX = VX + KK
        chip8->V[x] += kk;
        break;

    case 0x8000:
        switch (n)
        {
        case 0x0: // 8XY0: Establecer VX = VY
            chip8->V[x] = chip8->V[y];
            break;

        case 0x1: // 8XY1: Establecer VX = VX OR VY
            chip8->V[x] |= chip8->V[y];
            break;

        case 0x2: // 8XY2: Establecer VX = VX AND VY
            chip8->V[x] &= chip8->V[y];
            break;

        case 0x3: // 8XY3: Establecer VX = VX XOR VY
            chip8->V[x] ^= chip8->V[y];
            break;

        case 0x4: // 8XY4: Establecer VX = VX + VY, VF = carry
        {
            int sum = chip8->V[x] + chip8->V[y];
            chip8->V[0xF] = (sum > 255) ? 1 : 0;
            chip8->V[x] = sum & 0xFF;
        }
        break;

        case 0x5: // 8XY5: Establecer VX = VX - VY, VF = not borrow
            chip8->V[0xF] = (chip8->V[x] > chip8->V[y]) ? 1 : 0;
            chip8->V[x] -= chip8->V[y];
            break;

        case 0x6: // 8XY6: Desplazar VX a la derecha, VF = bit menos significativo

            chip8->V[0xF] = chip8->V[x] & 0x1;
            chip8->V[x] >>= 1;

            break;

        case 0x7: // 8XY7: Establecer VX = VY - VX, VF = not borrow
            chip8->V[0xF] = (chip8->V[y] > chip8->V[x]) ? 1 : 0;
            chip8->V[x] = chip8->V[y] - chip8->V[x];
            break;

        case 0xE: // 8XYE: Desplazar VX a la izquierda, VF = bit más significativo

            chip8->V[0xF] = (chip8->V[x] & 0x80) >> 7;
            chip8->V[x] <<= 1;

            break;
        }
        break;

    case 0x9000: // 9XY0: Saltar siguiente instrucción si VX != VY
        if (n == 0 && chip8->V[x] != chip8->V[y])
        {
            chip8->PC += 2;
        }
        break;

    case 0xA000: // ANNN: Establecer I = NNN
        chip8->I = nnn;
        break;

    case 0xB000: // BNNN: Saltar a dirección NNN + V0
        chip8->PC = nnn + chip8->V[0];
        break;

    case 0xC000: // CXKK: Establecer VX = random byte AND KK
        chip8->V[x] = (rand() % 256) & kk;
        break;

    case 0xD000: // DXYN: Dibujar sprite en posición VX, VY con N bytes
    {
        uint16_t xPos = chip8->V[x] % DISPLAY_WIDTH;
        uint16_t yPos = chip8->V[y] % DISPLAY_HEIGHT;
        uint16_t height = n;

        chip8->V[0xF] = 0; // Reset del flag de colisión

        for (int row = 0; row < height; row++)
        {
            uint8_t spriteData = chip8->memory[chip8->I + row];

            for (int col = 0; col < 8; col++)
            {
                if ((spriteData & (0x80 >> col)) != 0)
                {
                    // Coordenadas con wrap-around
                    int pixelX = (xPos + col) % DISPLAY_WIDTH;
                    int pixelY = (yPos + row) % DISPLAY_HEIGHT;
                    int pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);

                    // Comprobar colisión
                    if (chip8->gfx[pixelPos] == 1)
                    {
                        chip8->V[0xF] = 1;
                    }

                    // XOR con el pixel existente
                    chip8->gfx[pixelPos] ^= 1;
                }
            }
        }

        chip8->drawFlag = true;
    }
    break;

    case 0xE000:
        switch (kk)
        {
        case 0x9E: // EX9E: Saltar siguiente instrucción si tecla VX está presionada
            if (chip8->key[chip8->V[x]] != 0)
            {
                chip8->PC += 2;
            }
            break;

        case 0xA1: // EXA1: Saltar siguiente instrucción si tecla VX no está presionada
            if (chip8->key[chip8->V[x]] == 0)
            {
                chip8->PC += 2;
            }
            break;
        }
        break;

    case 0xF000:
        switch (kk)
        {
        case 0x07: // FX07: Establecer VX = valor del delay timer
            chip8->V[x] = chip8->delayTimer;
            break;

        case 0x0A: // FX0A: Esperar presión de tecla, almacenar en VX
        {
            bool keyPressed = false;

            for (int i = 0; i < KEY_COUNT; i++)
            {
                if (chip8->key[i])
                {
                    chip8->V[x] = i;
                    keyPressed = true;
                    break;
                }
            }

            // Si no se presionó tecla, repetir instrucción
            if (!keyPressed)
            {
                chip8->PC -= 2;
            }
        }
        break;

        case 0x15: // FX15: Establecer delay timer = VX
            chip8->delayTimer = chip8->V[x];
            break;

        case 0x18: // FX18: Establecer sound timer = VX
            chip8->soundTimer = chip8->V[x];
            break;

        case 0x1E: // FX1E: Establecer I = I + VX
            chip8->I += chip8->V[x];
            break;

        case 0x29:                      // FX29: Establecer I = dirección del carácter en VX
            chip8->I = chip8->V[x] * 5; // Cada carácter ocupa 5 bytes
            break;

        case 0x33: // FX33: Almacenar representación BCD de VX en I, I+1, I+2
        {
            uint8_t value = chip8->V[x];
            chip8->memory[chip8->I] = value / 100;
            chip8->memory[chip8->I + 1] = (value / 10) % 10;
            chip8->memory[chip8->I + 2] = value % 10;
        }
        break;

        case 0x55: // FX55: Almacenar V0 a VX en memoria desde I
            for (int i = 0; i <= x; i++)
            {
                chip8->memory[chip8->I + i] = chip8->V[i];
            }

            break;

        case 0x65: // FX65: Cargar V0 a VX desde memoria desde I
            for (int i = 0; i <= x; i++)
            {
                chip8->V[i] = chip8->memory[chip8->I + i];
            }

            break;
        }
        break;

    default:
        if (chip8->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("Opcode desconocido: 0x%04X\n", chip8->opcode);
        }
    }
}