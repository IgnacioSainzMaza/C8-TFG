#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <Arduino.h>

// Módulos del emulador
#include "chip16_config.h"
#include "chip16_core.h"
#include "chip16_input.h"
#include "chip16_display.h"
#include "chip16_buzzer.h"
#include "test_roms.h"


// ============================================================================
// TAG PARA LOGGING
// ============================================================================
static const char* TAG = "CHIP16_MAIN";

static const int CURRENT_ROM_INDEX = 2;
// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Instancias principales
static Chip16 chip;
static Display display;
static ButtonState buttonState;

// Control de timing
static uint64_t lastCycleTime = 0;
static uint64_t lastTimerUpdate = 0;
static uint64_t lastRenderTime = 0;

// Estadísticas de rendimiento
static uint32_t frameCount = 0;
static uint64_t lastFPSReport = 0;

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

// Obtener tiempo actual en microsegundos
static inline uint64_t get_time_us(void) {
  return esp_timer_get_time();
}

// Obtener tiempo actual en milisegundos
static inline uint32_t get_time_ms(void) {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// Reportar estadísticas de rendimiento
void report_performance(void) {
  uint64_t now = get_time_us();
  uint64_t elapsed = now - lastFPSReport;

  if (elapsed >= 1000000) {  // Cada segundo
    float fps = (float)frameCount * 1000000.0f / (float)elapsed;

    printf(TAG, "=== Estadísticas ===");
    printf(TAG, "FPS: %.2f", fps);
    printf(TAG, "PC: 0x%04X | I: 0x%04X | SP: %d",
             chip.PC, chip.I, chip.SP);
    printf(TAG, "Timers: DT=%d ST=%d",
             chip.delayTimer, chip.soundTimer);
    printf(TAG, "Modo: %s | Efecto: %s",
             chip.mode == MODE_8BIT ? "8-bit" : "16-bit",
             chip.currentEffect == EFFECT_NONE ? "Ninguno" : "Color Cycle");

    frameCount = 0;
    lastFPSReport = now;
  }
}

// ============================================================================
// INICIALIZACIÓN DEL SISTEMA
// ============================================================================

void initialize_system(void) {
  printf(TAG, "===========================================");
  printf(TAG, "     CHIP-16 EMULATOR para ESP32");
  printf(TAG, "===========================================");

  // === 1. Inicializar NVS (requerido por ESP-IDF) ===
  printf("1. Inicializando NVS...\n");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  printf(TAG, "✓ NVS inicializado");

  // === 2. Inicializar semilla aleatoria ===
  printf("2. Inicializando semilla aleatoria...\n");
  // Usar ruido del ADC para semilla verdaderamente aleatoria
  uint32_t seed = esp_timer_get_time() & 0xFFFFFFFF;
  srand(seed);

  printf(TAG, "✓ Semilla aleatoria: 0x%08X", seed);

  // === 3. Inicializar sistema de entrada ===
  printf("3. Inicializando sistema de entrada...\n");
  chip16_input_init();
  chip16_input_init_button_state(&buttonState);

  printf(TAG, "✓ Sistema de entrada listo");

  // === 4. Inicializar display ===
  printf("4. Inicializando display...\n");
  printf("   ANTES de llamar a chip16_display_init\n");  // NUEVO
  chip16_display_init(&display);
  printf("   DESPUÉS de llamar a chip16_display_init\n");  // NUEVO

  printf(TAG, "✓ Display ILI9341 listo");
  // Inicializar buzzer
  chip16_buzzer_init();
  // === 5. Inicializar emulador CHIP-16 ===
  printf("5. Inicializando emulador CHIP-16...\n");
  chip16_init(&chip);

  // Configurar nivel de debug (cambiar según necesidad)
  chip.config.debugLevel = DEBUG_NONE;  // DEBUG_OPCODES para ver cada instrucción
  chip.config.enableSound = true;
  chip.config.pixelColor = DEFAULT_PIXEL_COLOR;
  // chip16_set_effect(&chip, EFFECT_COLOR_CYCLE);
  // chip16_input_set_led_effect(true);
  printf(TAG, "✓ Emulador CHIP-16 inicializado");

  // === 6. Cargar ROM de prueba ===
  printf("6. Cargando ROM de prueba...\n");
  const TestROM* currentROM = &TEST_ROM_LIST[CURRENT_ROM_INDEX];
  printf(TAG, "ROM seleccionada: %s", currentROM->name);
  printf(TAG, "Descripción: %s", currentROM->description);

  chip16_load_rom(&chip, currentROM->data, currentROM->size);
  printf(TAG, "✓ ROM cargada: %d bytes", currentROM->size);

  // === 7. Inicializar timing ===
  printf("7. Inicializando timing...\n");
  lastCycleTime = get_time_us();
  lastTimerUpdate = get_time_us();
  lastRenderTime = get_time_us();
  lastFPSReport = get_time_us();
  frameCount = 0;

  printf("===========================================\n");
  printf("Sistema listo. Iniciando emulación...\n");
  printf("===========================================\n");

  // vTaskDelay(pdMS_TO_TICKS(3000));  // Esperar 3 segundos

  printf("Entrando en bucle principal...\n");
}


// ============================================================================
// MANEJO DE CONTROLES
// ============================================================================

void handle_controls(void) {
  uint32_t currentTime = get_time_ms();

  // === Botón RESET ===
  static bool resetPressed = false;
  bool resetCurrent = chip16_input_read_button_reset(&buttonState, currentTime);

  if (resetCurrent && !resetPressed) {
    printf(TAG, ">>> RESET presionado <<<");
    chip16_init(&chip);

    const TestROM* currentROM = &TEST_ROM_LIST[CURRENT_ROM_INDEX];
    chip16_load_rom(&chip, currentROM->data, currentROM->size);
    chip16_display_fill_screen(COLOR_BLACK);

    // Parpadear LED
    chip16_input_set_led_mode(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    chip16_input_set_led_mode(chip.mode == MODE_16BIT);
  }
  resetPressed = resetCurrent;

  // === Botón MODE (8bit/16bit) ===
  static bool modePressed = false;
  bool modeCurrent = chip16_input_read_button_mode(&buttonState, currentTime);

  if (modeCurrent && !modePressed) {
    chip.mode = (chip.mode == MODE_8BIT) ? MODE_16BIT : MODE_8BIT;
    chip16_input_set_led_mode(chip.mode == MODE_16BIT);

    printf(TAG, ">>> Modo cambiado: %s <<<",
             chip.mode == MODE_8BIT ? "8-bit (CHIP-8)" : "16-bit (CHIP-16)");
  }
  modePressed = modeCurrent;

  // === Botón EFFECT (ciclo de color) ===
  static bool effectPressed = false;
  bool effectCurrent = chip16_input_read_button_effect(&buttonState, currentTime);
  if (effectCurrent != effectPressed) {
    printf(TAG, ">>> Botón EFFECT detectado: %s <<<", 
             effectCurrent ? "PRESIONADO" : "SOLTADO");
}
  if (effectCurrent && !effectPressed) {
    GraphicsEffect newEffect = (chip.currentEffect == EFFECT_NONE)
                               ? EFFECT_COLOR_CYCLE
                               : EFFECT_NONE;
    chip16_set_effect(&chip, newEffect);
    chip16_input_set_led_effect(newEffect != EFFECT_NONE);

    printf(TAG, ">>> Efecto: %s <<<",
             newEffect == EFFECT_NONE ? "Desactivado" : "Ciclo de color");
  }
  effectPressed = effectCurrent;
}

// ============================================================================
// BUCLE PRINCIPAL DE EMULACIÓN
// ============================================================================

void emulation_loop(void) {
  printf(">>> LOOP PRINCIPAL INICIADO <<<\n");
  uint32_t loopCount = 0;

  while (true) {
    // Debug reducido - solo cada 60 iteraciones
    if (loopCount % 1000 == 0) {
      printf("Frames: %d | PC: 0x%04X | Efecto: %s\n",
             frameCount,
             chip.PC,
             chip.currentEffect == EFFECT_NONE ? "OFF" : "Ciclo Color");
    }

    uint64_t currentTime = get_time_us();
    uint32_t currentTimeMs = (uint32_t)(currentTime / 1000ULL);

    // === 1. PROCESAR ENTRADA ===
    chip16_input_scan_keypad(&chip);
    handle_controls();

    // === 2. ACTUALIZAR TIMERS Y BUZZER ===
    if (currentTime - lastTimerUpdate >= (TIMER_UPDATE_MS*1000)) {
    chip16_update_timers(&chip, currentTimeMs);
    lastTimerUpdate = currentTime;
    
    // Control del buzzer con PWM
    if (chip.soundTimer > 0) {
        chip16_buzzer_start();  // Activa PWM a 2000Hz
    } else {
        chip16_buzzer_stop();   // Silencia el buzzer
    }
}

    // === 3. EJECUTAR CICLOS DE CPU ===
    uint64_t elapsed = currentTime - lastCycleTime;
    uint32_t cyclesNeeded = (uint32_t)((elapsed * chip.config.intsPerSec) / 1000000ULL);

    if (cyclesNeeded > 0) {
      if (cyclesNeeded > 20) cyclesNeeded = 20;

      for (uint32_t i = 0; i < cyclesNeeded; i++) {
        chip16_cycle(&chip);
      }

      lastCycleTime = currentTime;
    }

    // === 4. RENDERIZAR ===
    if (currentTime - lastRenderTime >= 16667) {
      chip16_display_render(&display, &chip);
      lastRenderTime = currentTime;
      frameCount++;
    }

    // === 5. YIELD ===
    vTaskDelay(pdMS_TO_TICKS(1));
    loopCount++;
  }
}

// ============================================================================
// PUNTO DE ENTRADA PRINCIPAL
// ============================================================================

void chip16_main(void) {
  // Inicializar todo el sistema
  initialize_system();

  // Entrar en el bucle principal (nunca retorna)
  emulation_loop();
  // app_main();
}

