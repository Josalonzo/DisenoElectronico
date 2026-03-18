/*
 * seg7.h
 * Interface del driver de display de 7 segmentos multiplexado.
 *
 * Hardware - 6 digitos catodo comun:
 * Segmentos en GPIOA:
 *   PA0=a, PA1=b, PA4=e, PA5=f, PA6=g, PA11=c, PA12=d
 *
 * Digitos en GPIOA y GPIOB:
 *   PA7=dig0, PA8=dig1, PA9=dig2, PA10=dig3 (horas y minutos)
 *   PB0=dig4, PB1=dig5                      (segundos)
 *
 * Distribucion de digitos:
 *   dig0 dig1 : dig2 dig3 : dig4 dig5
 *    H    H  :  M    M  :  S    S
 */

#ifndef SEG7_H
#define SEG7_H

#include <stdint.h>
#include "stm32l053xx.h"

/* Pines de control de digitos en GPIOA (activo HIGH) */
#define DIG0_PIN_A  (1u<<7)   // PA7  - decena horas
#define DIG1_PIN_A  (1u<<8)   // PA8  - unidad horas
#define DIG2_PIN_A  (1u<<9)   // PA9  - decena minutos
#define DIG3_PIN_A  (1u<<10)  // PA10 - unidad minutos
#define ALL_DIG_A   (DIG0_PIN_A|DIG1_PIN_A|DIG2_PIN_A|DIG3_PIN_A)

/* Pines de control de digitos en GPIOB (activo HIGH) */
#define DIG4_PIN_B  (1u<<0)   // PB0 - decena segundos
#define DIG5_PIN_B  (1u<<1)   // PB1 - unidad segundos
#define ALL_DIG_B   (DIG4_PIN_B|DIG5_PIN_B)

/* Mascara de todos los pines de segmentos en GPIOA */
#define SEG_PINS_MASK  ((1u<<0)|(1u<<1)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<11)|(1u<<12))

/* Indice del digito activo en el multiplexado (0-5) */
extern uint8_t seven_segs_sequence;

/* Funciones publicas */
void set_segments(uint8_t digit);  // activa segmentos para digito 0-9
void seg7_tick(void);              // actualiza un digito, llamar desde TIM21 cada 1ms

#endif
