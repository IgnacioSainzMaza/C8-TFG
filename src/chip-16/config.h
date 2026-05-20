#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

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

// Configuraciones de audio
#define AUDIO_SAMPLE_RATE 44100 // Frecuencia de muestreo para el audio
#define AUDIO_SAMPLES     512 // Tamaño del buffer de audio
#define AUDIO_FREQUENCY   440.0 // Frecuencia de la nota A4 (La4) para el sonido del buzzer
#define AUDIO_VOLUME      28000 // Volumen del sonido (ajustable entre 0 y 32767)


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

#define COLOR_CYCLE_FRAMES 60 // Cambiar de color cada 60 frames (aprox. cada segundo a 60Hz)

// Estado del dispositivo de audio
typedef struct {
    SDL_AudioDeviceID dev;
    double            phase;
    volatile bool     active;
} BeepState;


// Configuración global
typedef struct {
    DebugLevel debugLevel;
    int clockSpeed;
    bool enableSound;
    uint32_t pixelColor;
    BeepState beepState;
} Config;

#endif // CONFIG_H