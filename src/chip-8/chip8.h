#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Definición del conjunto de fuentes en formato de sprites hexadecimales
static const uint8_t chip8_fontset[FONTSET_SIZE] = {
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

// Estructura principal del emulador CHIP-8
typedef struct {
    uint16_t opcode;              // Opcode actual
    uint8_t memory[MEMORY_SIZE];  // Memoria del sistema
    uint8_t V[REGISTER_COUNT];    // Registros V0-VF
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
} Chip8;

// Funciones principales del emulador
void chip8Init(Chip8* chip8);
bool chip8LoadROM(Chip8* chip8, const char* filename);
void chip8Cycle(Chip8* chip8);
void chip8UpdateTimers(Chip8* chip8);
void chip8SetKey(Chip8* chip8, uint8_t key, uint8_t value);

#endif // CHIP8_H