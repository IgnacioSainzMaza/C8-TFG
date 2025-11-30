#ifndef CHIP16_CORE_H
#define CHIP16_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "chip16_config.h"

//HEXADECIMAL FONTSET
//---------------------
extern const uint8_t CHIP16_FONTSET[FONTSET_SIZE];

//CHIP-16 EMULATOR STRUCTURE
//----------------------
typedef struct{
  uint16_t V[REGISTER_COUNT];
  uint16_t I;
  uint16_t PC;
  uint16_t SP;
  uint16_t stack[STACK_SIZE];

  // Memory
  uint8_t memory[MEMORY_SIZE]; //4KB
  // === Display ===
    uint8_t gfx[CHIP16_WIDTH * CHIP16_HEIGHT];        // Framebuffer primario
    uint8_t gfxEffect[CHIP16_WIDTH * CHIP16_HEIGHT];  // Buffer con efectos
    bool drawFlag;                                     // ¿Necesita redibujar?
    
    // === Timers ===
    uint8_t delayTimer;    // Delay Timer(60Hz)
    uint8_t soundTimer;    // Sound Timer (60Hz)
    
    // === Input ===
    uint8_t key[KEY_COUNT];  // Hexadecimal Keyset Status
    
    // === Estado Interno ===
    uint16_t opcode;                // Actual Opcode
    EmuMode mode;                   // 8bit/16bit
    GraphicsEffect currentEffect;   // Active Graphic Effect
    uint8_t effectTimer;            // Effect Timer
    uint8_t colorIndex;             // Color Index
    
    // === Configuración ===
    Config config;
    
    // === Timing ===
    uint32_t lastTimerUpdate;  // Timestamp de última actualización
    
} Chip16;

#ifdef __cplusplus
extern "C" {
#endif

//INIT
void chip16_init(Chip16* chip16);

//ROMs LOAD
void chip16_load_rom(Chip16* chip16, const uint8_t* rom, const uint16_t size);

//EXECUTION
void chip16_cycle(Chip16* chip_16);
void chip16_update_timers(Chip16* chip_16, uint32_t current_time_ms);

//INPUT SET
void chip16_set_key(Chip16* chip16, uint8_t key, bool pressed);

//EFFECTS
void chip16_set_effect(Chip16* chip16, GraphicsEffect effect);
void chip16_process_effects(Chip16* chip16);

#ifdef __cplusplus
}
#endif


#endif 