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
    memset(chip16->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
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

        case 0x0001: // 0001: Llamada con parámetros
            chip16->stack[chip16->SP] = chip16->PC;
            chip16->stack[chip16->SP + 1] = chip16->V[0xD];
            chip16->stack[chip16->SP + 2] = chip16->V[0xE];
            chip16->stack[chip16->SP + 3] = chip16->V[0xF];
            chip16->SP += 4;

            uint8_t nParams = y;

            chip16->V[0xD] = (chip16->V[1] >> 8) & 0xFF;
            chip16->V[0xE] = (chip16->V[1] & 0xFF);
            chip16->V[0xF] = nParams;
            chip16->PC = chip16->V[0];
            break;

        case 0x0002: // 0002: Retorno con valor

            uint16_t returnValue = chip16->V[y];
            if (chip16->SP >= 4)
            {

                chip16->SP -= 4;
                chip16->V[0xF] = chip16->stack[chip16->SP + 3];
                chip16->V[0xE] = chip16->stack[chip16->SP + 2];
                chip16->V[0xD] = chip16->stack[chip16->SP + 1];
                chip16->PC = chip16->stack[chip16->SP];
                chip16->V[0] = returnValue;
            }
            else
            {
                if (chip16->config.debugLevel >= DEBUG_OPCODES)
                {
                    printf("Error: Stack underflow en retorno con valor\n");
                }
            }
            break;
        case 0x0003: // 0003: Dibujar sprite 16x16
            uint16_t xPos = chip16->V[2] % DISPLAY_WIDTH;
            uint16_t yPos = chip16->V[3] % DISPLAY_HEIGHT;
            chip16->V[0xF] = 0; // Reset del flag de colisión

            for (int row = 0; row < 16; row++)
            {
                uint16_t spriteData = (chip16->memory[chip16->I + row * 2] << 8) |
                                      chip16->memory[chip16->I + row * 2 + 1];

                for (int col = 0; col < 16; col++)
                {
                    if ((spriteData & (0x8000 >> col)) != 0)
                    {
                        int pixelX = (xPos + col) % DISPLAY_WIDTH;
                        int pixelY = (yPos + row) % DISPLAY_HEIGHT;
                        int pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);

                        if (chip16->gfx[pixelPos] == 1)
                        {
                            chip16->V[0xF] = 1;
                        }

                        chip16->gfx[pixelPos] ^= 1;
                    }
                }
            }

            chip16->drawFlag = true;
            break;

        case 0x0004: // 0004: Dibujar línea horizontal
            uint16_t xPos = chip16->V[2] % DISPLAY_WIDTH;
            uint16_t yPos = chip16->V[3] % DISPLAY_HEIGHT;
            uint16_t length = chip16->V[4];
            uint16_t pattern = chip16->V[5];

            if (length == 0 || length > DISPLAY_WIDTH - xPos)
            {
                length = DISPLAY_WIDTH - xPos;
            }

            chip16->V[0xF] = 0; // Reset del flag de colisión
            int basePos = xPos + (yPos * DISPLAY_WIDTH);
            if (length <= 16)
            {
                uint16_t mask = 0;
                for (int i = 0; i < length; i++)
                {
                    mask |= (0x8000 >> i);
                }

                uint16_t activePattern = pattern & mask;
                for (int i = 0; i < length; i++)
                {
                    if ((activePattern & (0x8000 >> i)) != 0)
                    {
                        if (chip16->gfx[basePos + i] == 1)
                        {
                            chip16->V[0xF] = 1;
                        }
                        chip16->gfx[basePos + i] ^= 1;
                    }
                }
            }
            else
            {
                for (int i = 0; i < length; i++)
                {
                    if ((pattern & (0x8000 >> i)) != 0)
                    {
                        if (chip16->gfx[basePos + i] == 1)
                        {
                            chip16->V[0xF] = 1;
                        }
                        chip16->gfx[basePos + i] ^= 1;
                    }
                }
            }
            chip16->drawFlag = true;
            break;

        case 0x0005: // 0005: Dibujar línea vertical
            uint16_t xPos = chip16->V[2] % DISPLAY_WIDTH;
            uint16_t yPos = chip16->V[3] % DISPLAY_HEIGHT;
            uint16_t height = chip16->V[4];
            uint16_t pattern = chip16->V[5];

            if (height == 0 || height > DISPLAY_HEIGHT - yPos)
            {
                height = DISPLAY_HEIGHT - yPos;
            }

            chip16->V[0xF] = 0;
            for (int i = 0; i < height; i++)
            {
                if ((pattern & (0x8000 >> (i % 16))) != 0)
                {
                    int pixelPos = xPos + ((yPos + i) % DISPLAY_HEIGHT) * DISPLAY_WIDTH;

                    if (chip16->gfx[pixelPos] == 1)
                    {
                        chip16->V[0xF] = 1;
                    }
                    chip16->gfx[pixelPos] ^= 1;
                }
            }

            chip16->drawFlag = true;
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

    case 0x5000:
        if (n == 0) // 5XY0: Saltar siguiente instrucción si VX == VY
        {
            if (chip16->V[x] == chip16->V[y])
            {
                chip16->PC += 2;
            }
        }

        else if (n == 1) // 5XY1: Multiplicación
        {
            uint32_t result = (uint32_t)chip16->V[x] * (uint32_t)chip16->V[y];
            chip16->V[x] = result & 0xFFFF;
        }

        else if (n == 2) // 5XY2 : División
        {
            if (chip16->V[y] != 0)
            {
                chip16->V[0xF] = chip16->V[x] % chip16->V[y]; // Resto
                chip16->V[x] = chip16->V[x] / chip16->V[y];   // Cociente
            }
            else
            {
                chip16->V[x] = 0xFFFF; // División por cero - ERROR
                chip16->V[0xF] = 0;
            }
        }

        else if (n == 3) // 5XY3 : Suma vectorial
        {
            uint16_t nextX = (x + 1) % REGISTER_COUNT;
            uint16_t nextY = (y + 1) % REGISTER_COUNT;

            chip16->V[x] += chip16->V[y];
            chip16->V[nextX] += chip16->V[nextY];
        }

        else if (n == 4) // 5XY4 : PRoducto escalar
        {
            uint16_t nextX = (x + 1) % REGISTER_COUNT;
            uint16_t nextY = (y + 1) % REGISTER_COUNT;

            uint32_t product = (uint32_t)chip16->V[x] * (uint32_t)chip16->V[y] + (uint32_t)chip16->V[nextX] * (uint32_t)chip16->V[nextY];

            chip16->V[x] = product & 0xFFFF;           // Producto escalar
            chip16->V[0xF] = (product >> 16) & 0xFFFF; // Desbordamiento
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
            chip16->V[x] = (chip16->V[x] - chip16->V[y]) & 0xFFFF;
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

    case 0x9000:
        switch (n)
        {
        case 0x0: // 9XY0: Saltar siguiente instrucción si VX != VY
            if (chip16->V[x] != chip16->V[y])
            {
                chip16->PC += 2;
            }

            break;

        case 0x1: // 9XY1: ROtación derecha
            uint16_t value = chip16->V[x];
            uint8_t shift = chip16->V[y] & 0x0F;

            chip16->V[x] = (value >> shift) | (value << (16 - shift));
            break;
        case 0x2: // 9XY2: Rotación izquierda
            uint16_t value = chip16->V[x];
            uint8_t shift = chip16->V[y] & 0x0F;

            chip16->V[x] = (value << shift) | (value >> (16 - shift));
            break;
        case 0x3: // 9XY3: Contar bits
            uint16_t value = chip16->V[x];
            uint8_t count = 0;

            for (int i = 0; i < 16; i++)
            {
                if (value & (1 << i))
                {
                    count++;
                }
            }

            chip16->V[x] = count;
            break;

        default:
            break;
        }

        break;
    case 0xA000: // ANNN: Establecer I = NNN
        chip16->I = nnn;
        break;

    case 0xB000: 
    switch (n)
    {
    case 0x0: // BNNN: Saltar a dirección NNN + V0
        chip16->PC = nnn + (chip16->V[0] & 0xFFFF);
        break;

    case 0x1: // B001: Copiar bloque de memoria
        uint16_t count = chip16->V[x];
        uint16_t src = chip16->I;
        uint16_t dst = chip16->I + count;

        if (dst<MEMORY_SIZE){
            if(src<dst && src + count > dst){
                for (int i=count-1; i>=0; i--){
                    chip16->memory[dst+i] = chip16->memory[src+i];
                }
            }
            else{
                for (int i=0; i<count; i++){
                    chip16->memory[dst+i] = chip16->memory[src+i];
                }
            }
        }
        break;
    
    case 0x2: // B002: BUscar valor en memoria
        uint16_t value = chip16->V[x];
        bool found = false;

        for (int i=0; i<256 && (chip16->I + i+1)< MEMORY_SIZE; i+=2){
            uint16_t memValue = (chip16->memory[chip16->I + i] << 8) | chip16->memory[chip16->I + i + 1];
            if(memValue == value){
                chip16->V[0xF] = i/2;
                found = true;
                break;
            }
        }
        break;
    default:
        break;
    }
    break;

    case 0xC000:
        switch (n)
        {
        case 0x0: // CXKK: Establecer VX = random byte AND KK
            chip16->V[x] = (rand() % 256) & kk; // No se cambia a 65536 para mantener compatibilidad con programas existentes
            break;

        case 0x1: // CX01: Aleatorio 16 bits
            chip16->V[x] = rand() % 65536;
            break;
        case 0x2: // CX02: Aleatorio en rango
            uint8_t range = x + 1;
            if (chip16->V[range] > 0)
            {
                chip16->V[x] = rand() % chip16->V[range];
            }
            else
            {
                chip16->V[x] = 0;
            }
            break;
        default:
            break;
        }
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

        case 0x29:                        // FX29: Establecer I = dirección del carácter en VX
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