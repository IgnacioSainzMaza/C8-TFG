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
    chip16->mode = MODE_8BIT;
    
    

    // Inicializar registros y memoria
    memset(chip16->memory, 0, MEMORY_SIZE);
    memset(chip16->V, 0, REGISTER_COUNT * sizeof(uint16_t));
    memset(chip16->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(chip16->key, 0, KEY_COUNT);
    memset(chip16->stack, 0, STACK_SIZE * sizeof(uint16_t));
    memset(chip16->gfx2Buffer, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    

    chip16->opcode = 0;
    chip16->I = 0;
    chip16->PC = ROM_LOAD_ADDRESS; // Los programas comienzan en 0x200
    chip16->SP = 0;
    chip16->delayTimer = 0;
    chip16->soundTimer = 0;
    chip16->drawFlag = false;
    chip16->currentEffect = EFFECT_NONE;
    chip16->effectTimer = 0;
    chip16->colorIndex = 0;

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

    if (bytesRead != (size_t)fileSize)
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

void chip16SetEffect(Chip16* chip16, GraphicsEffects effect) {
    chip16->currentEffect = effect;
    chip16->effectTimer = 0;  // Reiniciar timer al cambiar efecto
    
    if (chip16->config.debugLevel >= DEBUG_OPCODES) {
        printf("Efecto gráfico cambiado a: %s\n", 
               effect == EFFECT_NONE ? "Ninguno" : "Ciclo de color");
    }
}

void chip16ProcessEffects(Chip16* chip16) {
    // SIEMPRE copiamos el buffer primario al de efectos
    memcpy(chip16->gfx2Buffer, chip16->gfx, 
           DISPLAY_WIDTH * DISPLAY_HEIGHT);
    
    // Si hay un efecto activo, actualizar su estado
    if (chip16->currentEffect == EFFECT_COLOR_CYCLE) {
        // Incrementar el timer
        chip16->effectTimer++;
        
        // ¿Es momento de cambiar de color?
        if (chip16->effectTimer >= COLOR_CYCLE_FRAMES) {
            chip16->effectTimer = 0;  // Reiniciar timer
            
            // Avanzar al siguiente color (con wrap-around)
            chip16->colorIndex = (chip16->colorIndex + 1) % COLOR_PALETTE_SIZE;
            
            // Debug opcional
            if (chip16->config.debugLevel >= DEBUG_VERBOSE) {
                printf("Ciclo de color: cambiando a índice %d (Color: 0x%08X)\n", 
                       chip16->colorIndex, COLOR_PALETTE[chip16->colorIndex]);
            }
        }
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

    // Variables para el uso en instrucciones
    uint16_t xPos, yPos, height, length, pattern, value, src, dst, count;
    uint8_t shift, nParams;
    uint32_t result, product;
    uint16_t returnValue, memValue;
    bool found;
    uint8_t range;
    int basePos, pixelPos, pixelX, pixelY;
    uint16_t spriteData, activePattern, mask;

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
        if ((chip16->V[x] & 0xFF) == kk)
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
            result = (uint32_t)chip16->V[x] * (uint32_t)chip16->V[y];
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

            product = (uint32_t)chip16->V[x] * (uint32_t)chip16->V[y] + (uint32_t)chip16->V[nextX] * (uint32_t)chip16->V[nextY];

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
            value = chip16->V[x];
            shift = chip16->V[y] & 0x0F;

            chip16->V[x] = (value >> shift) | (value << (16 - shift));
            break;
        case 0x2: // 9XY2: Rotación izquierda
            value = chip16->V[x];
            shift = chip16->V[y] & 0x0F;

            chip16->V[x] = (value << shift) | (value >> (16 - shift));
            break;
        case 0x3: // 9XY3: Contar bits
            value = chip16->V[x];
            count = 0;

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
        count = chip16->V[x];
        src = chip16->I;
        dst = chip16->I + count;

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
        value = chip16->V[x];
        found = false;

        for (int i=0; i<256 && (chip16->I + i+1)< MEMORY_SIZE; i+=2){
            memValue = (chip16->memory[chip16->I + i] << 8) | chip16->memory[chip16->I + i + 1];
            if(memValue == value){
                chip16->V[0xF] = i/2;
                found = true;
                break;
            }
            
        }
        if (!found) {
            chip16->V[0xF] = 0xFFFF;
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

        
        
        default:
            break;
        }
        break;
    case 0xD000: // DXYN: Dibujar sprite en posición VX, VY con N bytes
    {
        xPos = chip16->V[x] % DISPLAY_WIDTH;
        yPos = chip16->V[y] % DISPLAY_HEIGHT;
        height = n;

        chip16->V[0xF] = 0; // Reset del flag de colisión

        for (int row = 0; row < height; row++)
        {
            uint8_t spriteDataB = chip16->memory[chip16->I + row];

            for (int col = 0; col < 8; col++)
            {
                if ((spriteDataB & (0x80 >> col)) != 0)
                {
                    // Coordenadas con wrap-around
                    pixelX = (xPos + col) % DISPLAY_WIDTH;
                    pixelY = (yPos + row) % DISPLAY_HEIGHT;
                    pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);

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
        case 0x01: // E001: Llamada con parámetros
            if(chip16->SP + 4 <= STACK_SIZE)
            {
            chip16->stack[chip16->SP] = chip16->PC;
            chip16->stack[chip16->SP + 1] = chip16->V[0xD];
            chip16->stack[chip16->SP + 2] = chip16->V[0xE];
            chip16->stack[chip16->SP + 3] = chip16->V[0xF];
            chip16->SP += 4;

            nParams = y;

            chip16->V[0xD] = (chip16->V[1] >> 8) & 0xFF;
            chip16->V[0xE] = (chip16->V[1] & 0xFF);
            chip16->V[0xF] = nParams;
            chip16->PC = chip16->V[0];
            }     
            else{
                if (chip16->config.debugLevel >= DEBUG_OPCODES)
                {
                    printf("Error: Stack overflow en llamada con parámetros\n");
                }
            }
            break;
        case 0x02: // E002: Retorno con valor

            returnValue = chip16->V[y];
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
        case 0x03: // E003: Aleatorio 16 bits
            chip16->V[x] = rand() % 65536;
            break;

        case 0x04: // E004: Aleatorio en rango
            range = x + 1;
            if (chip16->V[range] > 0)
            {
                chip16->V[x] = rand() % chip16->V[range];
            }
            else
            {
                chip16->V[x] = 0;
            }
            break;

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
        case 0x01: // FX01: Dibujar Sprite 16x16
            xPos = chip16->V[2] % DISPLAY_WIDTH;
            yPos = chip16->V[3] % DISPLAY_HEIGHT;
            chip16->V[0xF] = 0; // Reset del flag de colisión

            for (int row = 0; row < 16; row++)
            {
                spriteData = (chip16->memory[chip16->I + row * 2] << 8) |
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
        
        case 0x02: // Fx02: Dibujar línea horizontal
        xPos = chip16->V[2] % DISPLAY_WIDTH;
            yPos = chip16->V[3] % DISPLAY_HEIGHT;
            length = chip16->V[4];
            pattern = chip16->V[5];

            if (length == 0 || length > DISPLAY_WIDTH - xPos)
            {
                length = DISPLAY_WIDTH - xPos;
            }

            chip16->V[0xF] = 0; // Reset del flag de colisión
            basePos = xPos + (yPos * DISPLAY_WIDTH);
            if (length <= 16)
            {
                mask = 0;
                for (int i = 0; i < length; i++)
                {
                    mask |= (0x8000 >> i);
                }

                activePattern = pattern & mask;
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

        case 0x03: // FX03: Dibujar línea vertical
            xPos = chip16->V[2] % DISPLAY_WIDTH;
            yPos = chip16->V[3] % DISPLAY_HEIGHT;
            height = chip16->V[4];
            pattern = chip16->V[5];

            if (height == 0 || height > DISPLAY_HEIGHT - yPos)
            {
                height = DISPLAY_HEIGHT - yPos;
            }

            chip16->V[0xF] = 0;
            for (int i = 0; i < height; i++)
            {
                if ((pattern & (0x8000 >> (i % 16))) != 0)
                {
                    pixelPos = xPos + ((yPos + i) % DISPLAY_HEIGHT) * DISPLAY_WIDTH;

                    if (chip16->gfx[pixelPos] == 1)
                    {
                        chip16->V[0xF] = 1;
                    }
                    chip16->gfx[pixelPos] ^= 1;
                }
            }

            chip16->drawFlag = true;
            break;
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
            // chip16->I = chip16->V[x] * 5; // Cada carácter ocupa 5 bytes

            if (chip16->mode == MODE_8BIT) {
                // Modo CHIP-8: Sprites de 5 bytes, solo dígitos 0-F
                uint8_t digit = chip16->V[x] & 0x0F;  // Limitar a 0-F
                chip16->I = digit * 5;  // Cada sprite ocupa 5 bytes
            } else {
                // Modo CHIP-16: Podría usar sprites extendidos
                uint8_t character = chip16->V[x] & 0xFF;  // Soportar más caracteres
                
                if (character <= 0x0F) {
                    // Caracteres estándar 0-F (compatibilidad)
                    chip16->I = character * 5;
                } else {
                    // Caracteres extendidos (si los implementamos)
                    // Por ejemplo, sprites de mayor resolución o caracteres ASCII
                    if (character < 128 && character >= 16) {
                        // Offset para caracteres extendidos
                        chip16->I = FONTSET_SIZE + ((character - 16) * 8);
                    } else {
                        // Caracter no válido, usar espacio en blanco o '?'
                        chip16->I = 0x0F * 5;  // Apuntar al sprite 'F' como fallback
                    }
                }
            }
            break;

        case 0x33: // FX33: Almacenar representación BCD de VX en I, I+1, I+2
        {
            if (chip16->mode == MODE_8BIT) {
                // Modo CHIP-8: 3 dígitos BCD 
                uint8_t value = chip16->V[x] & 0xFF;  
                chip16->memory[chip16->I] = value / 100;          // Centenas
                chip16->memory[chip16->I + 1] = (value / 10) % 10; // Decenas
                chip16->memory[chip16->I + 2] = value % 10;        // Unidades
            } else {
                // Modo CHIP-16: 5 dígitos BCD
                uint16_t value = chip16->V[x];
                chip16->memory[chip16->I] = value / 10000;         // Decenas de millar
                chip16->memory[chip16->I + 1] = (value / 1000) % 10; // Millares
                chip16->memory[chip16->I + 2] = (value / 100) % 10;  // Centenas
                chip16->memory[chip16->I + 3] = (value / 10) % 10;   // Decenas
                chip16->memory[chip16->I + 4] = value % 10;          // Unidades
            }
        }
        break;

        case 0x55: // FX55: Almacenar V0 a VX en memoria desde I
            
        if (chip16->mode == MODE_8BIT) {
            // Modo compatibilidad CHIP-8
            for (int i = 0; i <= x; i++) {
                chip16->memory[chip16->I + i] = chip16->V[i] & 0xFF;
            }
            
        } else {
            for (int i = 0; i <= x; i++)
            {
                // Almacenar registro de 16 bits en dos bytes consecutivos
                chip16->memory[chip16->I + (i * 2)] = (chip16->V[i] >> 8) & 0xFF; // Byte alto
                chip16->memory[chip16->I + (i * 2) + 1] = chip16->V[i] & 0xFF;    // Byte bajo
            }
            chip16->I += (x + 1) * 2;
            }
            break;

        case 0x65: // FX65: Cargar V0 a VX desde memoria desde I
        if (chip16->mode == MODE_8BIT) {
            // Modo compatibilidad CHIP-8
            for (int i = 0; i <= x; i++) {
                chip16->V[i] = chip16->memory[chip16->I + i];
            }
            
        } else {
            // Modo CHIP-16
            for (int i = 0; i <= x; i++) {
                chip16->V[i] = (chip16->memory[chip16->I + (i * 2)] << 8) |
                               chip16->memory[chip16->I + (i * 2) + 1];
            }
            chip16->I += (x + 1) * 2;
        }
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