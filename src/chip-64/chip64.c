#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip64.h"

// ============================================================================
// FUNCIONES AUXILIARES INTERNAS
// ============================================================================

/**
 * @brief Obtiene la máscara apropiada según el modo de emulación
 *
 * Esto permite que las operaciones respeten el tamaño de bits activo:
 * - MODE_8BIT:  0xFF (255)
 * - MODE_16BIT: 0xFFFF (65535)
 * - MODE_64BIT: 0xFFFFFFFFFFFFFFFF (2^64 - 1)
 */
static inline uint64_t getMask(Chip64 *chip64)
{
    switch (chip64->mode)
    {
    case MODE_8BIT:
        return 0xFF;
    case MODE_16BIT:
        return 0xFFFF;
    case MODE_64BIT:
        return 0xFFFFFFFFFFFFFFFF;
    default:
        return 0xFFFFFFFFFFFFFFFF;
    }
}

/**
 * @brief Enmascara un valor al tamaño del modo actual
 */
static inline uint64_t maskValue(Chip64 *chip64, uint64_t value)
{
    return value & getMask(chip64);
}

/**
 * @brief Obtiene el valor máximo representable según el modo
 */
static inline uint64_t getMaxValue(Chip64 *chip64)
{
    return getMask(chip64);
}

// ============================================================================
// FUNCIONES PÚBLICAS - DIMENSIONES DEL DISPLAY
// ============================================================================

uint16_t chip64GetDisplayWidth(Chip64 *chip64)
{
    // En modo 64-bit con alta resolución: 128 píxeles
    // En modos compatibles: 64 píxeles
    return (chip64->mode == MODE_64BIT && chip64->config.highResMode)
               ? DISPLAY_WIDTH
               : (DISPLAY_WIDTH / 2);
}

uint16_t chip64GetDisplayHeight(Chip64 *chip64)
{
    return (chip64->mode == MODE_64BIT && chip64->config.highResMode)
               ? DISPLAY_HEIGHT
               : (DISPLAY_HEIGHT / 2);
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

void chip64Init(Chip64 *chip64)
{
    // Inicializar configuración predeterminada
    chip64->config.debugLevel = DEBUG_NONE;
    chip64->config.clockSpeed = DEFAULT_SPEED;
    chip64->config.enableSound = true;
    chip64->config.pixelColor = DEFAULT_PIXEL_COLOR;
    chip64->config.highResMode = false; // Modo alta resolución por defecto
    chip64->config.colorMode = false;   // Modo monocromo por defecto
    chip64->mode = MODE_8BIT;

    // Inicializar registros y memoria
    memset(chip64->memory, 0, MEMORY_SIZE);
    memset(chip64->V, 0, REGISTER_COUNT * sizeof(uint64_t));
    memset(chip64->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(chip64->gfx2Buffer, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(chip64->key, 0, KEY_COUNT);
    memset(chip64->stack, 0, STACK_SIZE * sizeof(uint16_t));

    chip64->opcode = 0;
    chip64->I = 0;
    chip64->PC = ROM_LOAD_ADDRESS; // Los programas comienzan en 0x200
    chip64->SP = 0;
    chip64->delayTimer = 0;
    chip64->soundTimer = 0;
    chip64->drawFlag = false;
    chip64->currentEffect = EFFECT_NONE;
    chip64->effectTimer = 0;
    chip64->colorIndex = 0;

    // Cargar fuente en memoria
    memcpy(chip64->memory, chip64_fontset, FONTSET_SIZE);
    // Cargar paleta de colores por defecto
    memcpy(chip64->palette, DEFAULT_PALETTE, sizeof(DEFAULT_PALETTE));

    // Inicializar semilla para números aleatorios
    srand(time(NULL));

    // --- Debug ---
    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("╔════════════════════════════════════════╗\n");
        printf("║     CHIP-64 INICIALIZADO               ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ Memoria:    %5dKB (%d bytes)   ║\n", MEMORY_SIZE / 1024, MEMORY_SIZE);
        printf("║ Display:    %dx%-2d (%s)         ║\n",
               chip64GetDisplayWidth(chip64), chip64GetDisplayHeight(chip64),
               chip64->config.highResMode ? "Hi-Res" : "Compat");
        printf("║ Registros:  %d (V0-V%d)              ║\n",
               REGISTER_COUNT, REGISTER_COUNT - 1);
        printf("║ Colores:    %s                      ║\n",
               chip64->config.colorMode ? "16 colores" : "Monocromo ");
        printf("║ Modo:       %s                   ║\n",
               chip64->mode == MODE_8BIT ? "8-bit (CHIP-8) " : chip64->mode == MODE_16BIT ? "16-bit (CHIP-16)"
                                                                                          : "64-bit (CHIP-64)");
        printf("║ PC inicial: 0x%04X                    ║\n", chip64->PC);
        printf("╚════════════════════════════════════════╝\n");
    }
}

void chip64UpdateTimers(Chip64 *chip64)
{
    if (chip64->delayTimer > 0)
    {
        chip64->delayTimer--;
    }
    if (chip64->soundTimer > 0)
    {
        chip64->soundTimer--;
        // Aquí se podría agregar código para manejar el sonido
        printf("BEEP!\n");
    }
}

void chip64SetKey(Chip64 *chip64, uint8_t key, uint8_t value)
{
    if (key < KEY_COUNT)
    {
        chip64->key[key] = value;
    }
}

void chip64SetMode(Chip64 *chip64, EmuMode mode)
{
    chip64->mode = mode;

    // Ajustar configuración según modo
    if (mode == MODE_64BIT)
    {
        chip64->config.highResMode = true; // 128×64 en modo 64-bit
        chip64->config.colorMode = true;   // 16 colores en modo 64-bit
    }
    else
    {
        chip64->config.highResMode = false; // 64×32 en modos compatibles
        chip64->config.colorMode = false;   // Monocromo en modos compatibles
    }

    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("⚙️  Modo cambiado: %s (Display: %dx%d)\n",
               mode == MODE_8BIT ? "8-bit" : mode == MODE_16BIT ? "16-bit"
                                                                : "64-bit",
               chip64GetDisplayWidth(chip64), chip64GetDisplayHeight(chip64));
    }
}

void chip64SetColorMode(Chip64 *chip64, bool enable)
{
    chip64->config.colorMode = enable;

    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("🎨 Modo color: %s\n", enable ? "16 colores" : "Monocromo");
    }
}

void chip64SetPalette(Chip64 *chip64, const Color16 *newPalette)
{
    memcpy(chip64->palette, newPalette, sizeof(Color16) * MAX_COLORS);

    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("🎨 Paleta actualizada\n");
    }
}

void chip64SetEffect(Chip64 *chip64, GraphicsEffects effect)
{
    chip64->currentEffect = effect;
    chip64->effectTimer = 0; // Reiniciar timer al cambiar efecto

    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("Efecto gráfico: ");
        switch (effect)
        {
        case EFFECT_NONE:
            printf("Ninguno\n");
            break;
        case EFFECT_COLOR_CYCLE:
            printf("Ciclo de colores\n");
            break;
        // case EFFECT_FADE:
        //     printf("Fade (no implementado)\n");
        //     break;
        // case EFFECT_SHAKE:
        //     printf("Screen shake (no implementado)\n");
        //     break;
        default:
            printf("Desconocido\n");
            break;
        }
    }
}

void chip64ProcessEffects(Chip64 *chip64)
{
    memcpy(chip64->gfx2Buffer, chip64->gfx, (DISPLAY_HEIGHT * DISPLAY_WIDTH));

    // Si hay efecto activo, actualizar su estado
    switch (chip64->currentEffect)
    {
    case EFFECT_NONE:
        // Sin procesamiento adicional
        break;

    case EFFECT_COLOR_CYCLE:
        // Actualizar timer del ciclo de colores
        chip64->effectTimer++;

        if (chip64->effectTimer >= COLOR_CYCLE_FRAMES)
        {
            chip64->effectTimer = 0;
            chip64->colorIndex = (chip64->colorIndex + 1) % COLOR_PALETTE_SIZE;

            if (chip64->config.debugLevel >= DEBUG_VERBOSE)
            {
                printf("🎨 Ciclo de color: índice %d (Color: 0x%08X)\n",
                       chip64->colorIndex, COLOR_PALETTE[chip64->colorIndex]);
            }
        }

        // El renderizador debe usar colorIndex para elegir el color
        // de la paleta COLOR_PALETTE_RGBA al dibujar píxeles activos
        break;

        // case EFFECT_FADE:
        //     // TODO: Implementar fade in/out
        //     // Podría modificar la intensidad de los píxeles en gfx2Buffer
        //     break;

        // case EFFECT_SHAKE:
        //     // TODO: Implementar screen shake
        //     // Podría desplazar aleatoriamente los píxeles en gfx2Buffer
        //     break;

    default:
        break;
    }
}

void chip64GetStatus(Chip64 *chip64, char *buffer, size_t bufferSize)
{
    snprintf(buffer, bufferSize,
             "PC=0x%04X I=0x%016llX SP=%d Mode=%s Res=%dx%d",
             chip64->PC,
             (unsigned long long)chip64->I,
             chip64->SP,
             chip64->mode == MODE_8BIT ? "8bit" : chip64->mode == MODE_16BIT ? "16bit"
                                                                             : "64bit",
             chip64GetDisplayWidth(chip64),
             chip64GetDisplayHeight(chip64));
}

// Ejecutar un ciclo de emulación
void chip64Cycle(Chip64 *chip64)
{
    // Extraer opcode (2 bytes)
    chip64->opcode = (chip64->memory[chip64->PC] << 8) |
                     chip64->memory[chip64->PC + 1];

    // Incrementar PC antes de ejecutar
    chip64->PC += 2;

    // Variables para decodificación del opcode
    uint8_t x = (chip64->opcode & 0x0F00) >> 8;
    uint8_t y = (chip64->opcode & 0x00F0) >> 4;
    uint8_t n = chip64->opcode & 0x000F;
    uint8_t kk = chip64->opcode & 0x00FF;
    uint16_t nnn = chip64->opcode & 0x0FFF;

    // Variables para el uso en instrucciones
    // --- Valores y operandos generales ---
    uint64_t value; // Valor temporal genérico para cálculos y operaciones
    uint64_t src;   // Dirección fuente en operaciones de memoria (MEMCPY, MEMSRCH)
    uint64_t dst;   // Dirección destino en operaciones de memoria (MEMCPY)
    uint64_t count; // Contador para loops (copias, búsquedas, bits activos)

    // --- Coordenadas y dimensiones para gráficos ---
    uint64_t xPos;   // Posición X en display para operaciones de dibujo
    uint64_t yPos;   // Posición Y en display para operaciones de dibujo
    uint64_t height; // Altura de sprites (DRW) o líneas verticales (VLINE)
    uint64_t width;  // Ancho de sprites (reservado para futuro uso)

    // --- Datos de gráficos y patrones ---
    uint64_t memValue;   // Valor leído de memoria (búsquedas, load/store)
    uint64_t spriteData; // Datos de sprite de 16/32 bits (DRAW16, líneas)
    uint64_t pattern;    // Patrón de bits para dibujar líneas (HLINE, VLINE)
    uint64_t length;     // Longitud de líneas horizontales (HLINE)

    // --- Operaciones aritméticas de alta precisión ---
    __uint128_t result;  // Resultado de sumas/restas para detectar carry/borrow
    __uint128_t product; // Producto de multiplicaciones para detectar overflow

    // --- Operaciones de bits y control ---
    uint8_t shift;       // Cantidad de bits para rotaciones (ROR, ROL) y shifts
    uint8_t range;       // Límite superior para números aleatorios en rango (RNDR)
    uint8_t spriteDataB; // Datos de sprite de 8 bits (DRW estándar)

    // --- Flags de estado ---
    bool found;      // Flag: ¿se encontró el valor en búsqueda? (MEMSRCH)
    bool keyPressed; // Flag: ¿se detectó una tecla presionada? (FX0A)

    // --- Cálculos de píxeles individuales ---
    int pixelX;   // Coordenada X del píxel actual (con wrapping)
    int pixelY;   // Coordenada Y del píxel actual (con wrapping)
    int pixelPos; // Posición lineal en framebuffer (pixelX + pixelY * WIDTH)
    int basePos;  // Posición base para líneas horizontales (optimización)

    // --- Dimensiones efectivas del display ---
    uint16_t dispWidth;  // Ancho del display según modo (64 o 128)
    uint16_t dispHeight; // Altura del display según modo (32 o 64)

    dispHeight = chip64GetDisplayHeight(chip64);
    dispWidth = chip64GetDisplayWidth(chip64);

    // Depuración si está habilitada
    if (chip64->config.debugLevel >= DEBUG_OPCODES)
    {
        printf("Ejecutando opcode: 0x%04X en PC=0x%04X\n", chip64->opcode, chip64->PC - 2);
    }

    // Decodificar e implementar opcode
    switch (chip64->opcode & 0xF000)
    {
    case 0x0000:
        switch (kk)
        {
        case 0xE0:
            // 00E0: Limpiar pantalla
            memset(chip64->gfx, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            chip64->drawFlag = true;
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("CLS\n");
            }
            break;

        case 0xEE:
            // 00EE: Retornar de subrutina
            chip64->SP--;
            chip64->PC = chip64->stack[chip64->SP];
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("RET (→ 0x%04X)\n", chip64->PC);
            }
            break;

        default:
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("Opcode desconocido: 0x%04X\n", chip64->opcode);
            }
            break;
        }

    case 0x1000: // 1NNN: Saltar a dirección NNN
        chip64->PC = nnn;
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("JP 0x%03X\n", nnn);
        }
        break;

    case 0x2000: // 2NNN: Llamar subrutina en NNN
        chip64->stack[chip64->SP] = chip64->PC;
        chip64->SP++;
        chip64->PC = nnn;
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("CALL 0x%03X (SP=%d)\n", nnn, chip64->SP);
        }
        break;

    case 0x3000: // 3XKK: Saltar siguiente instrucción si VX == KK
        if ((chip64->V[x] & 0xFF) == kk)
        {
            chip64->PC += 2;
        }
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("SE V%X, 0x%02X (V%X=%llu)\n", x, kk, x,
                   (unsigned long long)(chip64->V[x] & 0xFF));
        }
        break;

    case 0x4000: // 4XKK: Saltar siguiente instrucción si VX != KK
        if (maskValue(chip64, chip64->V[x]) != kk)
        {
            chip64->PC += 2;
        }
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("SNE V%X, 0x%02X\n", x, kk);
        }
        break;

    case 0x5000:
        switch (n)
        {
        case 0x0: // 5XY0: SE Vx, Vy - Saltar si Vx == Vy
            if (chip64->V[x] == chip64->V[y])
            {
                chip64->PC += 2;
            }
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SE V%X, V%X\n", x, y);
            }
            break;

        case 0x1: // 5XY1: MUL Vx, Vy - Multiplicación
            result = (__uint128_t)chip64->V[x] * (__uint128_t)chip64->V[y];
            chip64->V[x] = maskValue(chip64, (uint64_t)result);
            chip64->V[REG_VF] = (result > getMask(chip64)) ? 1 : 0;
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("MUL V%X, V%X (overflow=%d)\n", x, y, chip64->V[REG_VF]);
            }
            break;

        case 0x2: // 5XY2: DIV Vx, Vy - División
            if (chip64->V[y] != 0)
            {
                chip64->V[REG_VF] = chip64->V[x] % chip64->V[y]; // Resto
                chip64->V[x] = chip64->V[x] / chip64->V[y];      // Cociente
            }
            else
            {
                chip64->V[x] = getMask(chip64); // División por cero - ERROR
                chip64->V[REG_VF] = 0;
            }
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("DIV V%X, V%X (resto=V%X)\n", x, y, REG_VF);
            }
            break;

        case 0x3: // 5XY3: VADD Vx, Vy - Suma vectorial 2D
        {
            uint8_t nextX = (x + 1) % REGISTER_COUNT;
            uint8_t nextY = (y + 1) % REGISTER_COUNT;
            chip64->V[x] = maskValue(chip64, chip64->V[x] + chip64->V[y]);
            chip64->V[nextX] = maskValue(chip64, chip64->V[nextX] + chip64->V[nextY]);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("VADD V%X, V%X (2D vector)\n", x, y);
            }
        }
        break;

        case 0x4: // 5XY4: DOT Vx, Vy - Producto escalar 2D
        {
            uint8_t nextX = (x + 1) % REGISTER_COUNT;
            uint8_t nextY = (y + 1) % REGISTER_COUNT;
            product = (__uint128_t)chip64->V[x] * (__uint128_t)chip64->V[y] +
                      (__uint128_t)chip64->V[nextX] * (__uint128_t)chip64->V[nextY];
            chip64->V[x] = maskValue(chip64, (uint64_t)product);
            chip64->V[REG_VF] = maskValue(chip64, (uint64_t)(product >> 64));
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("DOT V%X, V%X (2D scalar product)\n", x, y);
            }
        }
        break;
        }
        break;

    case 0x6000: // 6XKK: LD Vx, byte --> VX = KK
        chip64->V[x] = kk;
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("LD V%X, 0x%02X\n", x, kk);
        }
        break;

    case 0x7000: // 7XKK: ADD Vx, KK --> VX = VX + KK
        chip64->V[x] = maskValue(chip64, chip64->V[x] + kk);
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("ADD V%X, 0x%02X\n", x, kk);
        }
        break;

    case 0x8000:
        switch (n)
        {
        case 0x0: // 8XY0: LD Vx, Vy --> VX = VY
            chip64->V[x] = chip64->V[y];
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("LD V%X, V%X\n", x, y);
            }
            break;
        case 0x1: // 8XY1: OR Vx, Vy --> VX = VX OR VY
            chip64->V[x] |= chip64->V[y];
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("OR V%X, V%X\n", x, y);
            }
            break;
        case 0x2: // 8XY2: AND Vx, Vy --> VX = VX AND VY
            chip64->V[x] &= chip64->V[y];
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("AND V%X, V%X\n", x, y);
            }
            break;
        case 0x3: // 8XY3: XOR Vx, Vy --> VX = VX XOR VY
            chip64->V[x] ^= chip64->V[y];
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("XOR V%X, V%X\n", x, y);
            }
            break;
        case 0x4: // 8XY4: ADD Vx, Vy --> VX = VX + VY, VF = carry
            result = (__uint128_t)chip64->V[x] + (__uint128_t)chip64->V[y];
            chip64->V[REG_VF] = (result > getMask(chip64)) ? 1 : 0;
            chip64->V[x] = maskValue(chip64, (uint64_t)result);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)

            {
                printf("ADD V%X, V%X (carry=%d)\n", x, y, chip64->V[REG_VF]);
            }
            break;
        case 0x5: // 8XY5: SUB Vx, Vy --> VX = VX - VY, VF = NOT borrow
            chip64->V[REG_VF] = (chip64->V[x] >= chip64->V[y]) ? 1 : 0;
            chip64->V[x] = maskValue(chip64, chip64->V[x] - chip64->V[y]);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SUB V%X, V%X (NOT borrow=%d)\n", x, y, chip64->V[REG_VF]);
            }
            break;
        case 0x6: // 8XY6: SHR Vx {, Vy} --> VX = VX >> 1, VF = LSB before shift
            chip64->V[REG_VF] = chip64->V[x] & 0x1;
            chip64->V[x] >>= 1;
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SHR V%X (LSB=%d)\n", x, chip64->V[REG_VF]);
            }
            break;
        case 0x7: // 8XY7: SUBN Vx, Vy --> VX = VY - VX, VF = NOT borrow
            chip64->V[REG_VF] = (chip64->V[y] >= chip64->V[x]) ? 1 : 0;
            chip64->V[x] = maskValue(chip64, chip64->V[y] - chip64->V[x]);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SUBN V%X, V%X (NOT borrow=%d)\n", x, y, chip64->V[REG_VF]);
            }
            break;
        case 0xE: // 8XYE: SHL Vx {, Vy} --> VX = VX << 1, VF = MSB before shift
        {
            uint8_t bitPos = (chip64->mode == MODE_8BIT) ? 7 : (chip64->mode == MODE_16BIT) ? 15
                                                                                            : 63;
            chip64->V[REG_VF] = (chip64->V[x] & ((uint64_t)1 << bitPos)) >> bitPos;
            chip64->V[x] = maskValue(chip64, chip64->V[x] << 1);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SHL V%X (MSB=%d)\n", x, chip64->V[REG_VF]);
            }
        }
        break;
        }
        break;

    case 0x9000:
        switch (n)
        {
        case 0x0: // 9XY0: SNE Vx, Vy --> Saltar si VX != VY
            if (chip64->V[x] != chip64->V[y])
            {
                chip64->PC += 2;
            }
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("SNE V%X, V%X\n", x, y);
            }
            break;
        case 0x1: // 9XY1: ROR Vx {, Vy} --> Rotar VX a la derecha
            value = chip64->V[x];
            shift = chip64->V[y] & 0x3F;
            uint8_t bitWidth = (chip64->mode == MODE_8BIT) ? 8 : (chip64->mode == MODE_16BIT) ? 16
                                                                                              : 64;
            chip64->V[x] = (value >> shift) | (value << (bitWidth - shift));
            chip64->V[x] = maskValue(chip64, chip64->V[x]);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("ROR V%X, V%X (%d bits)\n", x, y, shift);
            }
            break;
        case 0x2: // 9XY2: ROL Vx {, Vy} --> Rotar VX a la izquierda
            value = chip64->V[x];
            shift = chip64->V[y] & 0x3F;
            uint8_t bitWidth = (chip64->mode == MODE_8BIT) ? 8 : (chip64->mode == MODE_16BIT) ? 16
                                                                                              : 64;
            chip64->V[x] = (value << shift) | (value >> (bitWidth - shift));
            chip64->V[x] = maskValue(chip64, chip64->V[x]);
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("ROL V%X, V%X (%d bits)\n", x, y, shift);
            }
            break;
        case 0x3: // 9XY3: POPCNT Vx --> Contar bits activos en VX
{
            value = chip64->V[x];
            count = 0;
            uint8_t maxBits = (chip64->mode == MODE_8BIT) ? 8 : (chip64->mode == MODE_16BIT) ? 16
                                                                                             : 64;
            for (int i = 0; i < maxBits; i++)
            {
                if (value & ((uint64_t)1 << i))
                {
                    count++;
                }
            }
            chip64->V[x] = count;
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("POPCNT V%X = %llu\n", x, (unsigned long long)count);
            }
        }
        break;
        }

        
    
    case 0xA000: // ANNN: LD I, addr --> I = NNN
        chip64->I = nnn;
        if (chip64->config.debugLevel >= DEBUG_OPCODES)
        {
            printf("LD I, 0x%03X\n", nnn);
        }   
        break;

    case 0xB000:
        switch(n)
        {
            case 0x0:  // BNNN: JP V0, addr
                chip64->PC = nnn + (chip64->V[0] & 0xFFFF);
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("JP V0, 0x%03X (→ 0x%04X)\n", nnn, chip64->PC);
                }
                break;

            case 0x1:  // B001: MEMCPY - Copiar bloque
                count = chip64->V[x] & 0xFFFF;
                src = chip64->I;
                dst = chip64->I + count;
                
                if (dst < MEMORY_SIZE) {
                    if (src < dst && src + count > dst) {
                        // Solapamiento: copiar hacia atrás
                        for (uint64_t i = count; i > 0; i--) {
                            chip64->memory[dst + i - 1] = chip64->memory[src + i - 1];
                        }
                    } else {
                        for (uint64_t i = 0; i < count; i++) {
                            chip64->memory[dst + i] = chip64->memory[src + i];
                        }
                    }
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("MEMCPY %llu bytes (I=0x%04llX)\n", 
                           (unsigned long long)count, (unsigned long long)src);
                }
                break;

            case 0x2:  // B002: MEMSRCH - Buscar valor en memoria
                value = chip64->V[x];
                found = false;
                
                uint8_t bytesPerValue = (chip64->mode == MODE_8BIT) ? 1 : 
                                        (chip64->mode == MODE_16BIT) ? 2 : 8;
                
                for (int i = 0; i < 256 && (chip64->I + i + bytesPerValue - 1) < MEMORY_SIZE; 
                     i += bytesPerValue) {
                    if (chip64->mode == MODE_8BIT) {
                        memValue = chip64->memory[chip64->I + i];
                    } else if (chip64->mode == MODE_16BIT) {
                        memValue = (chip64->memory[chip64->I + i] << 8) | 
                                   chip64->memory[chip64->I + i + 1];
                    } else {
                        memValue = 0;
                        for (int j = 0; j < 8; j++) {
                            memValue = (memValue << 8) | chip64->memory[chip64->I + i + j];
                        }
                    }
                    
                    if (memValue == value) {
                        chip64->V[REG_VF] = i / bytesPerValue;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    chip64->V[REG_VF] = getMask(chip64);
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("MEMSRCH V%X (found=%d)\n", x, found);
                }
                break;
        }
    break;

    case 0xC000: // CXKK: RND Vx, byte --> VX = random byte AND KK
        chip64 ->V[x] = (rand() % 256) & kk; 
        if (chip64->config.debugLevel >= DEBUG_OPCODES) {
            printf("RND V%X, 0x%02X (= 0x%02llX)\n", x, kk, 
                   (unsigned long long)(chip64->V[x] & 0xFF));
        }
    break;

    case 0xD000: // DXYN: DRW Vx, Vy, nibble --> Dibujar sprite en posición VX, VY con N bytes
        xPos = chip64->V[x] % dispWidth;
        yPos = chip64->V[y] % dispHeight;
        height = n;
        
        chip64->V[REG_VF] = 0;
        for (uint16_t row = 0; row < height; row++) {
            spriteDataB = chip64->memory[chip64->I + row];
            
            for (int col = 0; col < 8; col++) {
                if ((spriteDataB & (0x80 >> col)) != 0) {
                    pixelX = (xPos + col) % dispWidth;
                    pixelY = (yPos + row) % dispHeight;
                    pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);
                    
                    if (chip64->gfx[pixelPos] == 1) {
                        chip64->V[REG_VF] = 1;
                    }
                    
                    chip64->gfx[pixelPos] ^= 1;
                }
            }
        }
        
        chip64->drawFlag = true;
        if (chip64->config.debugLevel >= DEBUG_OPCODES) {
            printf("DRW V%X, V%X, %d (8×%d sprite)\n", x, y, n, n);
        }
    break;

    case 0xE000:
        switch (kk)
        {
        case 0x01: // E001: CALLP - Llamada con parámetros 
            if (chip64->SP + 4 <= STACK_SIZE) {
                    chip64->stack[chip64->SP] = chip64->PC;
                    chip64->stack[chip64->SP + 1] = chip64->V[0xD] & 0xFFFF;
                    chip64->stack[chip64->SP + 2] = chip64->V[0xE] & 0xFFFF;
                    chip64->stack[chip64->SP + 3] = chip64->V[REG_VF] & 0xFFFF;
                    chip64->SP += 4;
                    
                    // Configurar registros especiales
                    chip64->V[0xD] = (chip64->V[1] >> 8) & 0xFF;
                    chip64->V[0xE] = chip64->V[1] & 0xFF;
                    chip64->V[REG_VF] = y;  // Número de parámetros
                    chip64->PC = chip64->V[0] & 0xFFFF;
                    
                    if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                        printf("CALLP %d params (→ 0x%04X, SP=%d)\n", y, chip64->PC, chip64->SP);
                    }
                } else if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("⚠️  ERROR: Stack overflow en CALLP\n");
                }
                break;

        case 0x02: // E002: RETV - Retorno con valor
            {
                    uint64_t returnValue = chip64->V[y];
                    
                    if (chip64->SP >= 4) {
                        chip64->SP -= 4;
                        chip64->V[REG_VF] = chip64->stack[chip64->SP + 3];
                        chip64->V[0xE] = chip64->stack[chip64->SP + 2];
                        chip64->V[0xD] = chip64->stack[chip64->SP + 1];
                        chip64->PC = chip64->stack[chip64->SP];
                        chip64->V[0] = returnValue;
                        
                        if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                            printf("RETV V%X (val=0x%llX, → 0x%04X, SP=%d)\n", 
                                   y, (unsigned long long)returnValue, chip64->PC, chip64->SP);
                        }
                    } else if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                        printf("⚠️  ERROR: Stack underflow en RETV\n");
                    }
                }
                break;

        case 0x03: // E003: RND16 - Aleatorio 16 bits completo 
            chip64->V[x] = rand() % 65536;
            if (chip64->config.debugLevel >= DEBUG_OPCODES)
            {
                printf("RND16 V%X = 0x%04llX\n", x, (unsigned long long)chip64->V[x]);
            }
            break;

        case 0x04: // E004: RNDR - Aleatorio en rango
            {
                uint8_t rangeReg = (x + 1) % REGISTER_COUNT;
                if (chip64->V[rangeReg] > 0)
                {
                    chip64->V[x] = rand() % (chip64->V[rangeReg] & 0xFFFF);
                }
                else
                {
                    chip64->V[x] = 0;
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES)
                {
                    printf("RNDR V%X (max=V%X=%llu, result=%llu)\n",
                           x, rangeReg,
                           (unsigned long long)chip64->V[rangeReg],
                           (unsigned long long)chip64->V[x]);
                }
            }
            break;

        case 0x9E: // EX9E: SKP Vx - Saltar si tecla presionada
                if (chip64->key[chip64->V[x] & 0xF] != 0) {
                    chip64->PC += 2;
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("SKP V%X (key=%llu)\n", x, 
                           (unsigned long long)(chip64->V[x] & 0xF));
                }
                break;

        case 0xA1:  // EXA1: SKNP Vx - Saltar si tecla NO presionada
                if (chip64->key[chip64->V[x] & 0xF] == 0) {
                    chip64->PC += 2;
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("SKNP V%X\n", x);
                }
                break;
        break;
        }

    case 0xF000:
        switch (kk) {
            
            case 0x01:  // F001 (FX01): DRAW16 - Dibujar sprite 16×16 (CHIP-16)
                xPos = chip64->V[2] % dispWidth;
                yPos = chip64->V[3] % dispHeight;
                chip64->V[REG_VF] = 0;
                
                for (int row = 0; row < 16; row++) {
                    spriteData = (chip64->memory[chip64->I + row * 2] << 8) |
                                 chip64->memory[chip64->I + row * 2 + 1];
                    
                    for (int col = 0; col < 16; col++) {
                        if ((spriteData & (0x8000 >> col)) != 0) {
                            pixelX = (xPos + col) % dispWidth;
                            pixelY = (yPos + row) % dispHeight;
                            pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);
                            
                            if (chip64->gfx[pixelPos] == 1) {
                                chip64->V[REG_VF] = 1;
                            }
                            
                            chip64->gfx[pixelPos] ^= 1;
                        }
                    }
                }
                
                chip64->drawFlag = true;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("DRAW16 at (%llu,%llu)\n", 
                           (unsigned long long)xPos, (unsigned long long)yPos);
                }
                break;
            
            case 0x02:  // F002 (FX02): HLINE - Línea horizontal (CHIP-16)
                xPos = chip64->V[2] % dispWidth;
                yPos = chip64->V[3] % dispHeight;
                length = chip64->V[4];
                pattern = chip64->V[5];
                
                if (length == 0 || length > dispWidth - xPos) {
                    length = dispWidth - xPos;
                }
                
                chip64->V[REG_VF] = 0;
                basePos = xPos + (yPos * DISPLAY_WIDTH);
                
                for (uint64_t i = 0; i < length; i++) {
                    if ((pattern & (0x8000 >> (i % 16))) != 0) {
                        if (chip64->gfx[basePos + i] == 1) {
                            chip64->V[REG_VF] = 1;
                        }
                        chip64->gfx[basePos + i] ^= 1;
                    }
                }
                
                chip64->drawFlag = true;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("HLINE (%llu,%llu) len=%llu\n", 
                           (unsigned long long)xPos, (unsigned long long)yPos, 
                           (unsigned long long)length);
                }
                break;
            
            case 0x03:  // F003 (FX03): VLINE - Línea vertical (CHIP-16)
                xPos = chip64->V[2] % dispWidth;
                yPos = chip64->V[3] % dispHeight;
                height = chip64->V[4];
                pattern = chip64->V[5];
                
                if (height == 0 || height > dispHeight - yPos) {
                    height = dispHeight - yPos;
                }
                
                chip64->V[REG_VF] = 0;
                
                for (uint64_t i = 0; i < height; i++) {
                    if ((pattern & (0x8000 >> (i % 16))) != 0) {
                        pixelPos = xPos + ((yPos + i) % dispHeight) * DISPLAY_WIDTH;
                        
                        if (chip64->gfx[pixelPos] == 1) {
                            chip64->V[REG_VF] = 1;
                        }
                        chip64->gfx[pixelPos] ^= 1;
                    }
                }
                
                chip64->drawFlag = true;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("VLINE (%llu,%llu) height=%llu\n", 
                           (unsigned long long)xPos, (unsigned long long)yPos, 
                           (unsigned long long)height);
                }
                break;
            
            case 0x04:  // F004 (FX04): DRAW32 - Dibujar sprite 32×32 (CHIP-64) 
                xPos = chip64->V[2] % dispWidth;
                yPos = chip64->V[3] % dispHeight;
                chip64->V[REG_VF] = 0;
                
                // Sprite 32×32 = 32 filas × 4 bytes por fila = 128 bytes
                for (int row = 0; row < 32; row++) {
                    // Leer 4 bytes (32 bits) por fila
                    uint32_t spriteRow = 
                        ((uint32_t)chip64->memory[chip64->I + row * 4] << 24) |
                        ((uint32_t)chip64->memory[chip64->I + row * 4 + 1] << 16) |
                        ((uint32_t)chip64->memory[chip64->I + row * 4 + 2] << 8) |
                        ((uint32_t)chip64->memory[chip64->I + row * 4 + 3]);
                    
                    for (int col = 0; col < 32; col++) {
                        if ((spriteRow & (0x80000000 >> col)) != 0) {
                            pixelX = (xPos + col) % dispWidth;
                            pixelY = (yPos + row) % dispHeight;
                            pixelPos = pixelX + (pixelY * DISPLAY_WIDTH);
                            
                            if (chip64->gfx[pixelPos] == 1) {
                                chip64->V[REG_VF] = 1;
                            }
                            
                            chip64->gfx[pixelPos] ^= 1;
                        }
                    }
                }
                
                chip64->drawFlag = true;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("DRAW32 at (%llu,%llu) - 32×32 sprite [CHIP-64]\n", 
                           (unsigned long long)xPos, (unsigned long long)yPos);
                }
                break;
            
                        
            case 0x07:  // FX07: LD Vx, DT
                chip64->V[x] = chip64->delayTimer;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD V%X, DT (=%d)\n", x, chip64->delayTimer);
                }
                break;
            
            case 0x0A:  // FX0A: LD Vx, K - Esperar tecla
                keyPressed = false;
                for (int i = 0; i < KEY_COUNT; i++) {
                    if (chip64->key[i]) {
                        chip64->V[x] = i;
                        keyPressed = true;
                        break;
                    }
                }
                
                if (!keyPressed) {
                    chip64->PC -= 2;  // Repetir instrucción
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD V%X, K %s\n", x, keyPressed ? "(pressed)" : "(waiting)");
                }
                break;
            
            case 0x15:  // FX15: LD DT, Vx
                chip64->delayTimer = chip64->V[x] & 0xFF;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD DT, V%X (=%d)\n", x, chip64->delayTimer);
                }
                break;
            
            case 0x18:  // FX18: LD ST, Vx
                chip64->soundTimer = chip64->V[x] & 0xFF;
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD ST, V%X (=%d)\n", x, chip64->soundTimer);
                }
                break;
            
            case 0x1E:  // FX1E: ADD I, Vx
                chip64->I += chip64->V[x];
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("ADD I, V%X (I=0x%04llX)\n", x, (unsigned long long)chip64->I);
                }
                break;
            
            case 0x29:  // FX29: LD F, Vx - Cargar sprite de fuente
                {
                    uint8_t digit = chip64->V[x] & 0x0F;
                    chip64->I = digit * 5;
                    if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                        printf("LD F, V%X (char='%X', I=0x%04llX)\n", 
                               x, digit, (unsigned long long)chip64->I);
                    }
                }
                break;
            
            case 0x33:  // FX33: LD B, Vx - Almacenar BCD
                if (chip64->mode == MODE_8BIT) {
                    uint8_t val = chip64->V[x] & 0xFF;
                    chip64->memory[chip64->I] = val / 100;
                    chip64->memory[chip64->I + 1] = (val / 10) % 10;
                    chip64->memory[chip64->I + 2] = val % 10;
                } else if (chip64->mode == MODE_16BIT) {
                    uint16_t val = chip64->V[x] & 0xFFFF;
                    chip64->memory[chip64->I] = val / 10000;
                    chip64->memory[chip64->I + 1] = (val / 1000) % 10;
                    chip64->memory[chip64->I + 2] = (val / 100) % 10;
                    chip64->memory[chip64->I + 3] = (val / 10) % 10;
                    chip64->memory[chip64->I + 4] = val % 10;
                } else {  // MODE_64BIT - 20 dígitos
                    uint64_t val = chip64->V[x];
                    for (int i = 19; i >= 0; i--) {
                        chip64->memory[chip64->I + i] = val % 10;
                        val /= 10;
                    }
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD B, V%X (BCD at I=0x%04llX)\n", x, (unsigned long long)chip64->I);
                }
                break;
            
            case 0x55:  // FX55: LD [I], Vx - Guardar registros
                if (chip64->mode == MODE_8BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip64->memory[chip64->I + i] = chip64->V[i] & 0xFF;
                    }
                } else if (chip64->mode == MODE_16BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip64->memory[chip64->I + (i * 2)] = (chip64->V[i] >> 8) & 0xFF;
                        chip64->memory[chip64->I + (i * 2) + 1] = chip64->V[i] & 0xFF;
                    }
                    chip64->I += (x + 1) * 2;
                } else {  // MODE_64BIT
                    for (int i = 0; i <= x; i++) {
                        for (int j = 0; j < 8; j++) {
                            chip64->memory[chip64->I + (i * 8) + j] = 
                                (chip64->V[i] >> (56 - j * 8)) & 0xFF;
                        }
                    }
                    chip64->I += (x + 1) * 8;
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD [I], V%X (saved V0-V%X)\n", x, x);
                }
                break;
            
            case 0x65:  // FX65: LD Vx, [I] - Cargar registros
                if (chip64->mode == MODE_8BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip64->V[i] = chip64->memory[chip64->I + i];
                    }
                } else if (chip64->mode == MODE_16BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip64->V[i] = (chip64->memory[chip64->I + (i * 2)] << 8) |
                                       chip64->memory[chip64->I + (i * 2) + 1];
                    }
                    chip64->I += (x + 1) * 2;
                } else {  // MODE_64BIT
                    for (int i = 0; i <= x; i++) {
                        chip64->V[i] = 0;
                        for (int j = 0; j < 8; j++) {
                            chip64->V[i] = (chip64->V[i] << 8) | 
                                           chip64->memory[chip64->I + (i * 8) + j];
                        }
                    }
                    chip64->I += (x + 1) * 8;
                }
                if (chip64->config.debugLevel >= DEBUG_OPCODES) {
                    printf("LD V%X, [I] (loaded V0-V%X)\n", x, x);
                }
                break;
        }
        break;
    
    
    default:
        if (chip64->config.debugLevel >= DEBUG_OPCODES) {
            printf("OPCODE DESCONOCIDO: 0x%04X en PC=0x%04X\n", 
                   chip64->opcode, chip64->PC - 2);
        }
        break;
}