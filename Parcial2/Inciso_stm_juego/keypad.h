/*
 * keypad.h
 * Interface del modulo de teclado matricial 4x4 y entrada de datos.
 *
 * Hardware:
 * - Filas  (salidas):          PB2, PB3, PB4, PB6
 * - Columnas (entradas pull-up): PB7, PB8, PB9, PB10
 *
 * Distribucion del teclado:
 *   1  2  3  A
 *   4  5  6  B
 *   7  8  9  C
 *   *  0  #  D
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>
#include "stm32l053xx.h"

/* Pines de columnas (entradas) en GPIOB */
#define KP_COL0_PIN   7u   // PB7
#define KP_COL1_PIN   8u   // PB8
#define KP_COL2_PIN   9u   // PB9
#define KP_COL3_PIN   10u  // PB10
#define KP_COL_MASK  ((1u<<KP_COL0_PIN)|(1u<<KP_COL1_PIN)|(1u<<KP_COL2_PIN)|(1u<<KP_COL3_PIN))

/* Pines de filas (salidas) en GPIOB */
#define KP_ROW0_PIN   2u   // PB2
#define KP_ROW1_PIN   3u   // PB3
#define KP_ROW2_PIN   4u   // PB4
#define KP_ROW3_PIN   6u   // PB6
#define KP_ROW_MASK  ((1u<<KP_ROW0_PIN)|(1u<<KP_ROW1_PIN)|(1u<<KP_ROW2_PIN)|(1u<<KP_ROW3_PIN))

/* Variables de estado del escaneo */
extern volatile uint8_t kp_row_seq;    // fila actual en escaneo (0-3)
extern volatile uint8_t kp_lock;       // 1=esperando que se suelte la tecla
extern volatile uint8_t kp_row_active; // fila activa cuando se detecto tecla
extern volatile uint8_t kp_key;        // ultima tecla detectada en ASCII
extern volatile uint8_t kp_pressed;    // 1=hay tecla nueva disponible

/* Variables de entrada de datos del reloj */
extern volatile uint8_t ui_state;      // 0=normal, 1=set time, 2=set alarm
extern volatile uint8_t st_digits[6];  // buffer HHMMSS ingresado por teclado
extern volatile uint8_t st_len;        // digitos ingresados actualmente (0-6)

/* Funciones publicas */
uint8_t kp_cols_all_high(void);                      // verifica si ninguna tecla esta presionada
uint8_t keypad_getkey(void);                         // retorna ultima tecla y limpia flag
void handle_key(char k);                             // procesa tecla segun ui_state
void st_reset(void);                                 // limpia buffer de digitos
uint8_t st_valid_time(uint8_t hh, uint8_t mm, uint8_t ss); // valida hora ingresada
void keypad_tick(void);                              // escanea una fila, llamar desde TIM2

#endif
