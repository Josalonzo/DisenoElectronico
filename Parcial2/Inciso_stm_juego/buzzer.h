/*
 * buzzer.h
 * Interface del modulo buzzer para STM32L053R8
 * Pin: PB5
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "stm32l053xx.h"

#define BUZZ_PIN 5u  // Pin PB5 del buzzer

/* Variables de estado - modificadas en TIM22_IRQHandler */
extern volatile uint8_t  buz_mode;       // 0=OFF, 1=BEEP_N, 2=RING_1S
extern volatile uint16_t buz_div;        // divisor de frecuencia del tono
extern volatile uint8_t  beep_remaining; // beeps pendientes
extern volatile uint8_t  beep_on;        // 1=fase ON, 0=fase OFF
extern volatile uint16_t beep_cnt;       // contador de ticks del beep
extern volatile uint16_t ring_cnt;       // contador de ticks del ring

/* Funciones publicas */
void buzzer_set(uint8_t on);      // encender/apagar directo
void buzzer_stop(void);           // detener inmediatamente
void buzzer_beep_n(uint8_t n);    // N beeps cortos
void buzzer_ring_1s(void);        // tono continuo 1 segundo

#endif
