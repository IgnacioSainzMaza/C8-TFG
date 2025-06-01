#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// Constantes del sistema CHIP-8
#define MEMORY_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define STACK_SIZE 16
#define REGISTER_COUNT 16
#define KEY_COUNT 16
#define FONTSET_SIZE 80
#define ROM_LOAD_ADDRESS 0x200

// Configuraciones de visualización
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320
#define DEFAULT_PIXEL_COLOR 0x00FF00FF  // Magenta

// Configuraciones de emulación
#define DEFAULT_SPEED 5  // Retardo en milisegundos entre instrucciones
#define TIMER_FREQ 60    // Frecuencia de actualización de timers (60Hz)


// Niveles de depuración
typedef enum {
    DEBUG_NONE,     // Sin depuración
    DEBUG_OPCODES,  // Muestra los opcodes ejecutados
    DEBUG_VERBOSE   // Información completa de depuración
} DebugLevel;

//Configuracion de modo de emulación
typedef enum {
    MODE_8BIT, //Compatibilidad con CHIP-8
    MODE_16BIT //Modo extendido para CHIP-16 (16bit)
} EmuMode;

typedef enum{
    EFFECT_NONE = 0,
    EFFECT_COLOR_CYCLE = 1
} GraphicsEffects;

// Paleta de colores para el ciclo (formato RGBA: 0xRRGGBBAA)
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

#define COLOR_CYCLE_FRAMES 10 // Índice del color por defecto en la paleta (A 60 Hz cambia 10 veces por segundo) ¿REVISAR?
// Configuración global
typedef struct {
    DebugLevel debugLevel;
    int clockSpeed;
    bool enableSound;
    uint32_t pixelColor;
} Config;

#endif // CONFIG_H