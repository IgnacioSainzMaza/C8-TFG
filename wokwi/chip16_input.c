#include "chip16_input.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_rom_sys.h>

// Tag para logging (opcional, pero útil para debug)
static const char* TAG = "CHIP16_INPUT";

// ============================================================================
// INICIALIZACIÓN DE PINES
// ============================================================================

void chip16_input_init(void) {
    // === Configurar pines de FILAS como SALIDAS (inicialmente HIGH) ===
    gpio_set_direction(KEYPAD_R1, GPIO_MODE_OUTPUT);
    gpio_set_direction(KEYPAD_R2, GPIO_MODE_OUTPUT);
    gpio_set_direction(KEYPAD_R3, GPIO_MODE_OUTPUT);
    gpio_set_direction(KEYPAD_R4, GPIO_MODE_OUTPUT);
    
    // Poner todas las filas en HIGH (inactivas)
    gpio_set_level(KEYPAD_R1, 1);
    gpio_set_level(KEYPAD_R2, 1);
    gpio_set_level(KEYPAD_R3, 1);
    gpio_set_level(KEYPAD_R4, 1);
    
    // === Configurar pines de COLUMNAS como ENTRADAS con PULL-UP ===
    gpio_set_direction(KEYPAD_C1, GPIO_MODE_INPUT);
    gpio_set_direction(KEYPAD_C2, GPIO_MODE_INPUT);
    gpio_set_direction(KEYPAD_C3, GPIO_MODE_INPUT);
    gpio_set_direction(KEYPAD_C4, GPIO_MODE_INPUT);
    
    gpio_set_pull_mode(KEYPAD_C1, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(KEYPAD_C2, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(KEYPAD_C3, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(KEYPAD_C4, GPIO_PULLUP_ONLY);
    
  // === Configurar BOTONES DE CONTROL como ENTRADAS con PULL-UP ===
    gpio_set_direction(BTN_RESET, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_MODE, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_EFFECT, GPIO_MODE_INPUT);

    // Activar pull-up interno (ahora sí funciona con GPIO 5, 22, 19)
    gpio_set_pull_mode(BTN_RESET, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BTN_MODE, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BTN_EFFECT, GPIO_PULLUP_ONLY);
    // === Configurar LEDs como SALIDAS (inicialmente apagados) ===
    gpio_set_direction(LED_MODE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_EFFECT, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_MODE, 0);
    gpio_set_level(LED_EFFECT, 0);
    
    ESP_LOGI(TAG, "Sistema de entrada inicializado");
    ESP_LOGI(TAG, "Teclado matricial 4x4 configurado");
    ESP_LOGI(TAG, "Botones de control: RESET, MODE, EFFECT");
    ESP_LOGI(TAG, "LEDs de estado configurados");
}

// ============================================================================
// SCANNING DEL TECLADO MATRICIAL
// ============================================================================

void chip16_input_scan_keypad(Chip16* chip) {
    // Array con los pines de filas para iterar fácilmente
    const uint8_t row_pins[KEYPAD_ROWS] = {KEYPAD_R1, KEYPAD_R2, KEYPAD_R3, KEYPAD_R4};
    
    // Array con los pines de columnas para iterar fácilmente
    const uint8_t col_pins[KEYPAD_COLS] = {KEYPAD_C1, KEYPAD_C2, KEYPAD_C3, KEYPAD_C4};
    
    // === Escanear cada fila ===
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // 1. Activar la fila actual (poner en LOW)
        gpio_set_level(row_pins[row], 0);
        
        // 2. Pequeño delay para estabilización de señal (1 microsegundo)
        // En un teclado matricial, esto es crucial para lecturas confiables
        esp_rom_delay_us(1);
        
        // 3. Leer cada columna
        for (int col = 0; col < KEYPAD_COLS; col++) {
            // Si la columna está en LOW, significa que la tecla está presionada
            // (porque activamos la fila en LOW y hay continuidad eléctrica)
            bool pressed = (gpio_get_level(col_pins[col]) == 0);
            
            // Obtener el valor hexadecimal de esta tecla
            uint8_t key_value = KEYPAD_MAP[row][col];
            
            // Actualizar estado en el emulador
            chip16_set_key(chip, key_value, pressed);
        }
        
        // 4. Desactivar la fila (volver a HIGH)
        gpio_set_level(row_pins[row], 1);
    }
}

// ============================================================================
// LECTURA DE BOTONES DE CONTROL CON DEBOUNCING
// ============================================================================

// Helper interno para implementar debouncing genérico
static bool read_button_debounced(uint8_t pin, bool* lastState, 
                                  uint32_t* lastDebounceTime, 
                                  uint32_t currentTime) {
    // Leer estado actual del botón
    // Los botones están en LOW cuando presionados (pull-up)
    bool currentReading = (gpio_get_level(pin) == 0);
    
    // ¿El estado cambió desde la última lectura?
    if (currentReading != *lastState) {
        // Reiniciar el temporizador de debounce
        *lastDebounceTime = currentTime;
        *lastState = currentReading;
    }
    
    // ¿Ha pasado suficiente tiempo desde el último cambio?
    if ((currentTime - *lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        // El estado es estable, retornar si está presionado
        return currentReading;
    }
    
    // Estado no estable todavía
    return false;
}

bool chip16_input_read_button_reset(ButtonState* state, uint32_t current_time_ms) {
    return read_button_debounced(BTN_RESET, 
                                 &state->lastStateReset,
                                 &state->lastDebounceTimeReset,
                                 current_time_ms);
}

bool chip16_input_read_button_mode(ButtonState* state, uint32_t current_time_ms) {
    return read_button_debounced(BTN_MODE,
                                 &state->lastStateMode,
                                 &state->lastDebounceTimeMode,
                                 current_time_ms);
}

bool chip16_input_read_button_effect(ButtonState* state, uint32_t current_time_ms) {
    // Leer estado RAW del GPIO (sin debounce)
    int raw_level = gpio_get_level(BTN_EFFECT);
    
    // LOG cada 60 llamadas (para no saturar)
    static int log_counter = 0;
    // if (log_counter++ % 60 == 0) {
    //     printf("[DEBUG] BTN_EFFECT GPIO=%d, nivel=%d\n", BTN_EFFECT, raw_level);
    // }
    
    return read_button_debounced(BTN_EFFECT,
                                 &state->lastStateEffect,
                                 &state->lastDebounceTimeEffect,
                                 current_time_ms);
}

// ============================================================================
// CONTROL DE LEDs DE ESTADO
// ============================================================================

void chip16_input_set_led_mode(bool state) {
    gpio_set_level(LED_MODE, state ? 1 : 0);
}

void chip16_input_set_led_effect(bool state) {
    gpio_set_level(LED_EFFECT, state ? 1 : 0);
}

// ============================================================================
// INICIALIZACIÓN DE ESTADO DE BOTONES
// ============================================================================

void chip16_input_init_button_state(ButtonState* state) {
    state->lastStateReset = false;
    state->lastStateMode = false;
    state->lastStateEffect = false;
    state->lastDebounceTimeReset = 0;
    state->lastDebounceTimeMode = 0;
    state->lastDebounceTimeEffect = 0;
}