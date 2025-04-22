#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip16.h"

// Inicialización del emulador CHIP-16
void chip16Init(Chip16 *chip16)
{
    // Inicializar configuración predeterminada
    chip16->config.debugLevel = DEBUG_NONE;
    chip16->config.clockSpeed = DEFAULT_SPEED;
    chip16->config.enableSound = true;
    chip16->config.pixelColor = DEFAULT_PIXEL_COLOR;

    // Inicializar registros y memoria
    memset(chip16->memory, 0, MEMORY_SIZE);
    memset(chip16->V, 0, REGISTER_COUNT * sizeof(uint16_t));
    memset(chip16-> gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(chip16->key, 0, KEY_COUNT);
    memset(chip16->stack, 0, STACK_SIZE * sizeof(uint16_t));

    chip16->opcode = 0;
    chip16->I = 0;
    chip16->PC = ROM_LOAD_ADDRESS; // Los programas comienzan en 0x200
    chip16->SP = 0;
    chip16->delayTimer = 0;
    chip16->soundTimer = 0;
    chip16->drawFlag = false;

    // Cargar fuente en memoria
    memcpy(chip16->memory, chip16_fontset, FONTSET_SIZE);

    // Inicializar semilla para números aleatorios
    srand(time(NULL));
}

// Cargar ROM desde archivo
bool chip16LoadROM(Chip16 *chip16, const char *filename)
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
    size_t bytesRead = fread(&chip16->memory[ROM_LOAD_ADDRESS], 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize)
    {
        fprintf(stderr, "Error: No se pudo leer el archivo completo\n");
        return false;
    }

    return true;
}

// Actualizar temporizadores
void chip16UpdateTimers(Chip16 *chip16)
{
    if (chip16->delayTimer > 0)
    {
        chip16->delayTimer--;
    }

    if (chip16->soundTimer > 0)
    {
        if (chip16->config.enableSound && chip16->soundTimer == 1)
        {
            // Aquí se implementaría la reproducción de sonido
            printf("BEEP!\n");
        }
        chip16->soundTimer--;
    }
}

// Establecer estado de una tecla
void chip16SetKey(Chip16 *chip16, uint8_t key, uint8_t value)
{
    if (key < KEY_COUNT)
    {
        chip16->key[key] = value;
    }
}

// Ejecutar un ciclo de emulación
void chip16Cycle(Chip16 *chip16)
{
    // Extraer opcode (2 bytes)
    chip16->opcode = (chip16->memory[chip16->PC] << 8) | chip16->memory[chip16->PC + 1];

    // Incrementar PC antes de ejecutar
    chip16->PC += 2;

    // Variables para decodificación del opcode
    uint8_t x = (chip16->opcode & 0x0F00) >> 8;
    uint8_t y = (chip16->opcode & 0x00F0) >> 4;
    uint8_t n = chip16->opcode & 0x000F;
    uint8_t kk = chip16->opcode & 0x00FF;
    uint16_t nnn = chip16->opcode & 0x0FFF;

    // Depuración si está habilitada
    if (chip16->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("Ejecutando opcode: 0x%04X en PC=0x%04X\n", chip16->opcode, chip16->PC - 2);
    }

    // Decodificar e implementar opcode
    switch (chip16->opcode & 0xF000)
    {
    case 0x0000:
        switch (kk)
        {
        case 0x00E0: // 00E0: Limpiar pantalla
            memset(chip16->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            chip16->drawFlag = true;
            break;

        case 0x00EE: // 00EE: Retornar de subrutina
            chip16->SP--;
            chip16->PC = chip16->stack[chip16->SP];
            break;

        default:
            if (chip16->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("Opcode desconocido: 0x%04X\n", chip16->opcode);
            }
        }
        break;

    case 0x1000: // 1NNN: Saltar a dirección NNN
        chip16->PC = nnn;
        break;

    case 0x2000: // 2NNN: Llamar subrutina en NNN
        chip16->stack[chip16->SP] = chip16->PC;
        chip16->SP++;
        chip16->PC = nnn;
        break;

    case 0x3000: // 3XKK: Saltar siguiente instrucción si VX == KK
        if (chip16->V[x] & 0xFF == kk)
        {
            chip16->PC += 2;
        }
        break;

    case 0x4000: // 4XKK: Saltar siguiente instrucción si VX != KK
        if (chip16->V[x] != kk)
        {
            chip16->PC += 2;
        }
        break;

    case 0x5000: // 5XY0: Saltar siguiente instrucción si VX == VY
        if (n == 0 && chip16->V[x] == chip16->V[y])
        {
            chip16->PC += 2;
        }
        break;

    case 0x6000: // 6XKK: Establecer VX = KK
        chip16->V[x] = kk;
        break;

    case 0x7000: // 7XKK: Establecer VX = VX + KK
        chip16->V[x] += kk;
        break;

    case 0x8000:
        switch (n)
        {
        case 0x0: // 8XY0: Establecer VX = VY
            chip16->V[x] = chip16->V[y];
            break;

        case 0x1: // 8XY1: Establecer VX = VX OR VY
            chip16->V[x] |= chip16->V[y];
            break;

        case 0x2: // 8XY2: Establecer VX = VX AND VY
            chip16->V[x] &= chip16->V[y];
            break;

        case 0x3: // 8XY3: Establecer VX = VX XOR VY
            chip16->V[x] ^= chip16->V[y];
            break;

        case 0x4: // 8XY4: Establecer VX = VX + VY, VF = carry
        {
            int sum = chip16->V[x] + chip16->V[y];
            chip16->V[0xF] = (sum > 0xFFFF) ? 1 : 0;
            chip16->V[x] = sum & 0xFFFF;
        }
        break;

        case 0x5: // 8XY5: Establecer VX = VX - VY, VF = not borrow
            chip16->V[0xF] = (chip16->V[x] > chip16->V[y]) ? 1 : 0;
            chip16->V[x] =(chip16->V[x] - chip16->V[y]) & 0xFFFF; 
            break;

        case 0x6: // 8XY6: Desplazar VX a la derecha, VF = bit menos significativo

            chip16->V[0xF] = chip16->V[x] & 0x1;
            chip16->V[x] >>= 1;

            break;

        case 0x7: // 8XY7: Establecer VX = VY - VX, VF = not borrow
            chip16->V[0xF] = (chip16->V[y] > chip16->V[x]) ? 1 : 0;
            chip16->V[x] = chip16->V[y] - chip16->V[x];
            break;

        case 0xE: // 8XYE: Desplazar VX a la izquierda, VF = bit más significativo

            chip16->V[0xF] = (chip16->V[x] & 0x8000) >> 15;
            chip16->V[x] <<= 1;

            break;
        }
        break;

    case 0x9000: // 9XY0: Saltar siguiente instrucción si VX != VY
        if (n == 0 && chip16->V[x] != chip16->V[y])
        {
            chip16->PC += 2;
        }
        break;

    case 0xA000: // ANNN: Establecer I = NNN
        chip16->I = nnn;
        break;

    case 0xB000: // BNNN: Saltar a dirección NNN + V0
        chip16->PC = nnn + (chip16->V[0] & 0xFFFF);
        break;

    case 0xC000: // CXKK: Establecer VX = random byte AND KK
        chip16->V[x] = (rand() % 256) & kk; //No se cambia a 65536 para mantener compatibilidad con programas existentes
        break;

    case 0xD000: // DXYN: Dibujar sprite en posición VX, VY con N bytes
    {
        uint16_t xPos = chip16->V[x] % DISPLAY_WIDTH;
        uint16_t yPos = chip16->V[y] % DISPLAY_HEIGHT;
        uint16_t height = n;

        chip16->V[0xF] = 0; // Reset del flag de colisión

        for (int row = 0; row < height; row++)
        {
            uint8_t spriteData = chip16->memory[chip16->I + row];

            for (int col = 0; col < 8; col++)
            {
                if ((spriteData & (0x80 >> col)) != 0)
                {
                    // Coordenadas con wrap-around
                    int pixelX = (xPos + col) % DISPLAY_WIDTH;
                    int pixelY = (yPos + row) % DISPLAY_HEIGHT;
                    int pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);

                    // Comprobar colisión
                    if (chip16->gfx[pixelPos] == 1)
                    {
                        chip16->V[0xF] = 1;
                    }

                    // XOR con el pixel existente
                    chip16->gfx[pixelPos] ^= 1;
                }
            }
        }

        chip16->drawFlag = true;
    }
    break;

    case 0xE000:
        switch (kk)
        {
        case 0x9E: // EX9E: Saltar siguiente instrucción si tecla VX está presionada
            if (chip16->key[chip16->V[x]] != 0)
            {
                chip16->PC += 2;
            }
            break;

        case 0xA1: // EXA1: Saltar siguiente instrucción si tecla VX no está presionada
            if (chip16->key[chip16->V[x]] == 0)
            {
                chip16->PC += 2;
            }
            break;
        }
        break;

    case 0xF000:
        switch (kk)
        {
        case 0x07: // FX07: Establecer VX = valor del delay timer
            chip16->V[x] = chip16->delayTimer;
            break;

        case 0x0A: // FX0A: Esperar presión de tecla, almacenar en VX
        {
            bool keyPressed = false;

            for (int i = 0; i < KEY_COUNT; i++)
            {
                if (chip16->key[i])
                {
                    chip16->V[x] = i;
                    keyPressed = true;
                    break;
                }
            }

            // Si no se presionó tecla, repetir instrucción
            if (!keyPressed)
            {
                chip16->PC -= 2;
            }
        }
        break;

        case 0x15: // FX15: Establecer delay timer = VX
            chip16->delayTimer = chip16->V[x];
            break;

        case 0x18: // FX18: Establecer sound timer = VX
            chip16->soundTimer = chip16->V[x];
            break;

        case 0x1E: // FX1E: Establecer I = I + VX
            chip16->I += chip16->V[x];
            break;

        case 0x29:                      // FX29: Establecer I = dirección del carácter en VX
            chip16->I = chip16->V[x] * 5; // Cada carácter ocupa 5 bytes
            break;

        case 0x33: // FX33: Almacenar representación BCD de VX en I, I+1, I+2
        {
            uint16_t value = chip16->V[x];
            chip16->memory[chip16->I] = value / 10000;
            chip16->memory[chip16->I + 1] = (value / 1000) % 10;
            chip16->memory[chip16->I + 2] = (value / 100) % 10;
            chip16->memory[chip16->I + 3] = (value / 10) % 10;
            chip16->memory[chip16->I + 4] = value % 10;
        }
        break;

        case 0x55: // FX55: Almacenar V0 a VX en memoria desde I
            for (int i = 0; i <= x; i++)
            {
                // Almacenar registro de 16 bits en dos bytes consecutivos
                chip16->memory[chip16->I + (i * 2)] = (chip16->V[i] >> 8) & 0xFF; // Byte alto
                chip16->memory[chip16->I + (i * 2) + 1] = chip16->V[i] & 0xFF;    // Byte bajo
            }
            chip16->I += (x + 1) * 2;
            break;

        case 0x65: // FX65: Cargar V0 a VX desde memoria desde I
            for (int i = 0; i <= x; i++)
            {
                // Cargar dos bytes consecutivos como un valor de 16 bits
                chip16->V[i] = (chip16->memory[chip16->I + (i * 2)] << 8) |
                              chip16->memory[chip16->I + (i * 2) + 1];
            }
            chip16->I += (x + 1) * 2;
            break;
        }
        break;

    default:
        if (chip16->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("Opcode desconocido: 0x%04X\n", chip16->opcode);
        }
    }
}