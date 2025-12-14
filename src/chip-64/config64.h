#ifndef CONFIG64_H
#define CONFIG64_H

#include <stdint.h>
#include <stdbool.h>

// Constantes del sistema CHIP-64
// -- Memoria --
#define MEMORY_SIZE 65536 // 64 KB de memoria - Mejora 
#define ROM_LOAD_ADDRESS 0x200
#define FONTSET_SIZE 80

// -- Pantalla --
#define DISPLAY_WIDTH 128 // Mejora: Resolución más alta
#define DISPLAY_HEIGHT 64 // Mejora: Resolución más alta

// -- Colores --
#define COLOR_DEPTH 4              // 4 bits = 16 colores
#define MAX_COLORS 16              // Paleta de 16 colores

// -- Pila y registros --
#define STACK_SIZE 16
#define REGISTER_COUNT 32 // Mejora a 32 registros V0-V31 para CHIP-64
#define REG_VF 31   // Registro VF en CHIP-64
#define KEY_COUNT 16

// -- Visualización --
#define WINDOW_WIDTH (DISPLAY_WIDTH * 8)    // 1024 pixels
#define WINDOW_HEIGHT (DISPLAY_HEIGHT * 8)  // 512 pixels
#define DEFAULT_PIXEL_COLOR 0x00FF00FF  // Magenta

// -- Tiempos y velocidad --
#define DEFAULT_SPEED 5  // Retardo en milisegundos entre instrucciones
#define TIMER_FREQ 60    // Frecuencia de actualización de timers (60Hz)

// -- Sprites --
#define SPRITE_8x8_SIZE 8         // Sprite estándar CHIP-8: 8×N
#define SPRITE_16x16_SIZE 32      // Sprite CHIP-16: 16×16 = 32 bytes
#define SPRITE_32x32_SIZE 128     // Sprite CHIP-64: 32×32 = 128 bytes (NUEVO)

// Enumeraciones
// -- Niveles de depuración --
typedef enum {
    DEBUG_NONE,     // Sin depuración
    DEBUG_OPCODES,  // Muestra los opcodes ejecutados
    DEBUG_VERBOSE   // Información completa de depuración
} DebugLevel;

// -- Configuración de modo de emulación --
typedef enum {
    MODE_8BIT, //Compatibilidad con CHIP-8
    MODE_16BIT, //Modo extendido para CHIP-16 (16bit)
    MODE_64BIT  //Modo extendido para CHIP-64 (64bit)
} EmuMode;

// -- Efectos gráficos --
typedef enum{
    EFFECT_NONE = 0,
    EFFECT_COLOR_CYCLE = 1
} GraphicsEffects;


// Formato RGB565: 5 bits Rojo, 6 bits Verde, 5 bits Azul
// Compatible con displays TFT como ILI9341
typedef uint16_t Color16;

// Paleta por defecto: Estilo NES/Retro
static const Color16 DEFAULT_PALETTE[MAX_COLORS] = {
    0x0000,  // 0:  Negro
    0xFFFF,  // 1:  Blanco
    0xF800,  // 2:  Rojo
    0x07E0,  // 3:  Verde
    0x001F,  // 4:  Azul
    0xFFE0,  // 5:  Amarillo
    0xF81F,  // 6:  Magenta
    0x07FF,  // 7:  Cyan
    0x7BEF,  // 8:  Gris claro
    0x4208,  // 9:  Gris oscuro
    0xFC00,  // 10: Naranja
    0x8410,  // 11: Marrón
    0x7C1F,  // 12: Púrpura
    0x03EF,  // 13: Verde agua
    0xFD20,  // 14: Rosa
    0xBDF7   // 15: Celeste
};


// Paleta para ciclo de colores (formato RGBA: 0xRRGGBBAA)
#define COLOR_PALETTE_SIZE 8
static const uint32_t COLOR_PALETTE[COLOR_PALETTE_SIZE] = {
    0x00FF00FF,  // Verde puro
    0x00FFFFFF,  // Cyan
    0x0000FFFF,  // Azul puro
    0xFF00FFFF,  // Magenta
    0xFFFF00FF,  // Amarillo
    0xFF8000FF,  // Naranja
    0xFF0000FF,  // Rojo puro
    0xFFFFFFFF   // Blanco
};

// Frames para cambiar de color (a 60 Hz)
#define COLOR_CYCLE_FRAMES 10

// Configuración global
typedef struct {
    DebugLevel debugLevel;   // Nivel de depuración activo
    int clockSpeed;          // Velocidad del reloj (delay entre instrucciones)
    bool enableSound;        // ¿Sonido habilitado?
    uint32_t pixelColor;     // Color de píxeles activos
    bool colorMode;       // Modo de color habilitado // true = 16 colores, false = monocromo
    bool highResMode;     // Modo de alta resolución habilitado // true = 128×64, false = 64×32 (compatibilidad)
} Config64;

#endif // CONFIG64_H