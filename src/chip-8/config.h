#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#ifndef SDL_AudioDeviceID
typedef uint32_t SDL_AudioDeviceID;
#endif

// Constantes del sistema CHIP-8
#define MEMORY_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define STACK_SIZE 16
#define REGISTER_COUNT 16
#define KEY_COUNT 16
#define FONTSET_SIZE 80 // 16 caracteres * 5 bytes cada uno
#define ROM_LOAD_ADDRESS 0x200

// Configuraciones de visualización
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320
#define DEFAULT_PIXEL_COLOR 0x00FF00FF  // Verde con transparencia completa

// Configuraciones de emulación
#define DEFAULT_SPEED 5  // Retardo en milisegundos entre instrucciones
#define TIMER_FREQ 60    // Frecuencia de actualización de timers (60Hz)

// Configuraciones de audio
#define AUDIO_SAMPLE_RATE 44100 // Frecuencia de muestreo para el audio
#define AUDIO_SAMPLES     512 // Tamaño del buffer de audio
#define AUDIO_FREQUENCY   440.0 // Frecuencia del beep (La4)
#define AUDIO_VOLUME      28000 // Volumen del beep 

// Niveles de depuración
typedef enum {
    DEBUG_NONE,     // Sin depuración
    DEBUG_OPCODES,  // Muestra los opcodes ejecutados
    DEBUG_VERBOSE   // Información completa de depuración
} DebugLevel;

// Estado del dispositivo de audio
typedef struct {
    SDL_AudioDeviceID dev;
    double            phase;
    volatile bool              active;
} BeepState;

// Configuración global
typedef struct {
    DebugLevel debugLevel;
    int clockSpeed;
    bool enableSound;
    uint32_t pixelColor;
    BeepState  beep;
} Config;

#endif // CONFIG_H