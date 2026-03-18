/*
 * buzzer.c
 * Manejo del buzzer pasivo conectado en PB5.
 * Soporta dos modos:
 *   - Modo 1 (BEEP_N): N beeps cortos de 150ms ON / 100ms OFF
 *   - Modo 2 (RING_1S): tono continuo de 1 segundo
 * La generacion del tono se hace en TIM22_IRQHandler (main.c)
 * toggleando el pin cada tick a ~2000Hz.
 */

#include "buzzer.h"

/* Modo actual del buzzer: 0=OFF, 1=BEEP_N, 2=RING_1S */
volatile uint8_t  buz_mode = 0;

/* Divisor para controlar la frecuencia del tono */
volatile uint16_t buz_div = 0;

/* Cantidad de beeps que faltan por ejecutar */
volatile uint8_t  beep_remaining = 0;

/* Flag: 1=buzzer encendido en este ciclo, 0=apagado */
volatile uint8_t  beep_on = 0;

/* Contador de ticks del beep actual */
volatile uint16_t beep_cnt = 0;

/* Contador de ticks del ring (maximo 2000 = 1 segundo) */
volatile uint16_t ring_cnt = 0;

/*
 * buzzer_set - enciende o apaga el buzzer directamente
 * on: 1 = encender, 0 = apagar
 * Usa BSRR para escritura atomica sin afectar otros pines de GPIOB
 */
void buzzer_set(uint8_t on) {
    if (on) GPIOB->BSRR = (1u << BUZZ_PIN);
    else    GPIOB->BSRR = (1u << (BUZZ_PIN + 16u));
}

/*
 * buzzer_stop - detiene el buzzer inmediatamente
 * Resetea todas las variables de estado y apaga el pin
 */
void buzzer_stop(void) {
    buz_mode       = 0;
    beep_remaining = 0;
    beep_on        = 0;
    beep_cnt       = 0;
    ring_cnt       = 0;
    buzzer_set(0);
}

/*
 * buzzer_beep_n - inicia una secuencia de N beeps cortos
 * n: cantidad de beeps a ejecutar
 * Cada beep dura 150ms ON y 100ms OFF (controlado por TIM22)
 */
void buzzer_beep_n(uint8_t n) {
    if (n == 0) return;
    buz_mode       = 1;   // modo BEEP_N
    beep_remaining = n;   // beeps pendientes
    beep_on        = 1;   // empieza encendido
    beep_cnt       = 0;   // reset contador
    buz_div        = 0;   // reset divisor de frecuencia
}

/*
 * buzzer_ring_1s - inicia un tono continuo de 1 segundo
 * Usado para la alarma del reloj
 * TIM22 lo detiene automaticamente cuando ring_cnt llega a 2000
 */
void buzzer_ring_1s(void) {
    buz_mode = 2;   // modo RING_1S
    ring_cnt = 0;   // reset contador de duracion
    buz_div  = 0;   // reset divisor de frecuencia
}
