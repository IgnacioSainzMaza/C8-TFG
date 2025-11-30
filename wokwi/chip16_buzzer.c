#include "chip16_buzzer.h"
#include "chip16_config.h"
#include <driver/ledc.h>
#include <esp_log.h>

static const char* TAG = "BUZZER";

void chip16_buzzer_init(void) {
    // Configurar timer LEDC para generar 2000Hz
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = BUZZER_FREQUENCY,  // 2kHz - tono audible
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);
    
    // Configurar canal LEDC vinculado al GPIO del buzzer
    ledc_channel_config_t channel = {
        .gpio_num = BUZZER_PIN,  // GPIO 21
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,  // Apagado inicialmente
        .hpoint = 0
    };
    ledc_channel_config(&channel);
    
    ESP_LOGI(TAG, "Buzzer inicializado en GPIO %d a 2000Hz", BUZZER_PIN);
}

void chip16_buzzer_start(void) {
    // Duty cycle 50% = 128/255
    // Esto genera una onda cuadrada que hace vibrar el buzzer
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 128);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void chip16_buzzer_stop(void) {
    // Duty cycle 0% = pin siempre en LOW
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}