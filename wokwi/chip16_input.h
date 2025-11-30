#ifndef CHIP16_INPUT_H
#define CHIP16_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "chip16_config.h"
#include "chip16_core.h"

static const uint8_t KEYPAD_MAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {0x1, 0x2, 0x3, 0xC},  // Fila 1
    {0x4, 0x5, 0x6, 0xD},  // Fila 2
    {0x7, 0x8, 0x9, 0xE},  // Fila 3
    {0xA, 0x0, 0xB, 0xF}   // Fila 4
};

#define DEBOUNCE_DELAY_MS 20  // Tiempo de debounce en milisegundos

// Estado de los botones de control para debouncing
typedef struct {
    bool lastStateReset;
    bool lastStateMode;
    bool lastStateEffect;
    uint32_t lastDebounceTimeReset;
    uint32_t lastDebounceTimeMode;
    uint32_t lastDebounceTimeEffect;
} ButtonState;

// ============================================================================
// FUNCIONES DEL MÓDULO DE ENTRADA
// ============================================================================


#ifdef __cplusplus
extern "C" {
#endif
// Inicialización
void chip16_input_init(void);

// Inicializar estado de botones
void chip16_input_init_button_state(ButtonState* state);

// Scanning del teclado matricial
void chip16_input_scan_keypad(Chip16* chip);

// Lectura de botones de control (con debouncing)
bool chip16_input_read_button_reset(ButtonState* state, uint32_t current_time_ms);
bool chip16_input_read_button_mode(ButtonState* state, uint32_t current_time_ms);
bool chip16_input_read_button_effect(ButtonState* state, uint32_t current_time_ms);

// Control de LEDs de estado
void chip16_input_set_led_mode(bool state);
void chip16_input_set_led_effect(bool state);

#ifdef __cplusplus
}
#endif

#endif // CHIP16_INPUT_H