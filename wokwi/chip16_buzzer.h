#ifndef CHIP16_BUZZER_H
#define CHIP16_BUZZER_H

#include <stdint.h>
#define BUZZER_FREQUENCY 700
// Inicializa el buzzer con PWM a 2000Hz
void chip16_buzzer_init(void);

// Activa el buzzer (empieza a sonar)
void chip16_buzzer_start(void);

// Desactiva el buzzer (silencio)
void chip16_buzzer_stop(void);

#endif