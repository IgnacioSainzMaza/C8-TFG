#ifndef CHIP16_CONFIG_H
#define CHIP16_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

//CONFIGURACIÓN MEMORIA
//---------------------------
#define MEMORY_SIZE 4096
#define ROM_LOAD_ADDRESS 0x200
#define FONTSET_SIZE 80

//CONFIGURACIÓN DE DISPLAY
//---------------------------
#define CHIP16_WIDTH 64
#define CHIP16_HEIGHT 32
#define PIXEL_SCALE 8  //320 Pixeles
// -- Dimensiones físicas display --
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

//CONFIGURACIÓN DE REGISTROS Y HW
//----------------------------
#define REGISTER_COUNT 16
#define STACK_SIZE 16
#define KEY_COUNT 16

//PINES DEL HW
//-----------------------------
//Display ILI9341
#define TFT_CS   15  
#define TFT_RST  4   
#define TFT_DC   2   
#define TFT_MOSI 23  
#define TFT_SCLK 18  
#define TFT_MISO 19  

// Teclado Matricial 4x4
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

// Pines Teclado
#define KEYPAD_R1 13
#define KEYPAD_R2 12
#define KEYPAD_R3 14
#define KEYPAD_R4 27
#define KEYPAD_C1 26
#define KEYPAD_C2 25
#define KEYPAD_C3 33
#define KEYPAD_C4 32

// Botones de Control
#define BTN_RESET  5   // Cambiar de 35 a 5
#define BTN_MODE   22  // Cambiar de 34 a 22
#define BTN_EFFECT 0  // Cambiar de 36 a 19

// LEDs de Estado
#define LED_MODE   16
#define LED_EFFECT 17

// Audio
#define BUZZER_PIN 21

//TIMING Y VELOCIDAD
//---------------------------
#define CYCLES_PER_FRAME 10        // Instrucciones por frame
#define TIMER_UPDATE_MS 16         // ~60Hz para timers (16.67ms)
#define INSTRUCTIONS_PER_SECOND 700

//PALETA COLORES
//---------------------------
#define COLOR_PALETTE_SIZE 8
static const uint16_t COLOR_PALETTE[COLOR_PALETTE_SIZE] = {
    0x07E0,  // Verde:   RGB(0, 255, 0)
    0x07FF,  // Cyan:    RGB(0, 255, 255)
    0x001F,  // Azul:    RGB(0, 0, 255)
    0xF81F,  // Magenta: RGB(255, 0, 255)
    0xFFE0,  // Amarillo: RGB(255, 255, 0)
    0xFC00,  // Naranja: RGB(255, 128, 0)
    0xF800,  // Rojo:    RGB(255, 0, 0)
    0xFFFF   // Blanco:  RGB(255, 255, 255)
};
#define DEFAULT_PIXEL_COLOR 0x001F // Azul
#define COLOR_CYCLE_FRAMES 10

//ENUMS
//---------------------------
typedef enum{
  DEBUG_NONE = 0,
  DEBUG_OPCODES = 1,
  DEBUG_VERBOSE = 2
} DebugLevel;

typedef enum{
  MODE_8BIT = 0,
  MODE_16BIT = 1 //Chip-16
} EmuMode;

typedef enum{
  EFFECT_NONE = 0,
  EFFECT_COLOR_CYCLE = 1
} GraphicsEffect;

//STRUCTS
typedef struct{
  DebugLevel debugLevel;
  uint16_t intsPerSec;
  uint16_t pixelColor;
  bool enableSound;
} Config;

#endif //CHIP16_CONFIG_H