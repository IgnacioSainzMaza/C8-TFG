#ifndef CHIP16_H
#define CHIP16_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Definición del conjunto de fuentes en formato de sprites hexadecimales
static const uint8_t chip16_fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


typedef struct {
    uint16_t opcode;              // Opcode actual
    uint8_t memory[MEMORY_SIZE];  // Memoria del sistema
    uint16_t V[REGISTER_COUNT];    // Registros V0-VF
    uint16_t I;                   // Registro índice
    uint16_t PC;                  // Program Counter
    uint8_t gfx[DISPLAY_WIDTH * DISPLAY_HEIGHT];  // Memoria de pantalla
    uint8_t delayTimer;           // Timer de retardo
    uint8_t soundTimer;           // Timer de sonido
    uint16_t stack[STACK_SIZE];   // Pila para guardar direcciones de retorno
    uint16_t SP;                  // Stack Pointer
    uint8_t key[KEY_COUNT];       // Estado del teclado
    bool drawFlag;                // Bandera para indicar si hay que actualizar la pantalla
    Config config;                // Configuración del emulador
    EmuMode mode; 

    //2o buffer
    uint8_t gfx2Buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // Buffer de pantalla secundario
    GraphicsEffects currentEffect; // Efecto gráfico actual
    uint8_t effectTimer; // Temporizador para efectos gráficos
    uint8_t colorIndex; // Índice del color actual en el ciclo de colores
} Chip16;

// Funciones principales del emulador
void chip16Init(Chip16* chip16);
bool chip16LoadROM(Chip16* chip16, const char* filename);
void chip16Cycle(Chip16* chip16);
void chip16UpdateTimers(Chip16* chip16);
void chip16SetKey(Chip16* chip16, uint8_t key, uint8_t value);
void chip16SetEffect(Chip16* chip16, GraphicsEffects effect);
void chip16ProcessEffects(Chip16* chip16);

#endif // CHIP16_H