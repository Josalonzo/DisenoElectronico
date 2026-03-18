/*
 * clock.c
 * Manejo del reloj en tiempo real y alarma.
 * clock_tick() es llamado cada 1 segundo desde SysTick_Handler.
 * El tiempo se almacena en digitos separados (unidades y decenas)
 * para facilitar el display en los 7 segmentos.
 */

#include "clock.h"
#include "buzzer.h"

/* Estructura global que almacena la hora actual en digitos separados */
struct time mihora;

/* Modo de display: 1=formato 24h, 0=formato 12h */
volatile uint8_t mode_24h = 1;

/* Hora de la alarma en formato HH:MM:SS */
volatile uint8_t alarm_hh      = 0;
volatile uint8_t alarm_mm      = 0;
volatile uint8_t alarm_ss      = 0;

/* alarm_enabled: 1=alarma activa, 0=alarma desactivada */
volatile uint8_t alarm_enabled = 1;

/* alarm_fired: 1=alarma ya disparada en este segundo, evita retrigger */
volatile uint8_t alarm_fired   = 0;

/*
 * set_time - establece la hora actual
 * hh: horas (0-23), mm: minutos (0-59), ss: segundos (0-59)
 * Descompone cada valor en decenas y unidades para los 7 segmentos
 */
void set_time(uint8_t hh, uint8_t mm, uint8_t ss) {
    mihora.horas_decena    = hh / 10;
    mihora.horas_unidad    = hh % 10;
    mihora.minutos_decena  = mm / 10;
    mihora.minutos_unidad  = mm % 10;
    mihora.segundos_decena = ss / 10;
    mihora.segundos_unidad = ss % 10;
}

/*
 * to_12h - convierte hora de formato 24h a 12h
 * hh24: hora en formato 24h (0-23)
 * retorna: hora en formato 12h (1-12)
 * Ejemplo: 0->12, 13->1, 23->11
 */
uint8_t to_12h(uint8_t hh24) {
    uint8_t h = (uint8_t)(hh24 % 12);
    return (h == 0) ? 12 : h;
}

/*
 * clock_tick - incrementa el reloj en 1 segundo
 * Llamado desde SysTick_Handler cada 1 segundo.
 * Maneja el carry en cascada: segundos -> minutos -> horas
 * Al llegar a 23:59:59 el siguiente tick vuelve a 00:00:00
 * Tambien verifica si la hora actual coincide con la alarma
 */
void clock_tick(void) {
    /* Incremento en cascada de segundos, minutos y horas */
    mihora.segundos_unidad++;
    if (mihora.segundos_unidad > 9) {
        mihora.segundos_unidad = 0;
        mihora.segundos_decena++;
        if (mihora.segundos_decena > 5) {
            mihora.segundos_decena = 0;
            mihora.minutos_unidad++;
            if (mihora.minutos_unidad > 9) {
                mihora.minutos_unidad = 0;
                mihora.minutos_decena++;
                if (mihora.minutos_decena > 5) {
                    mihora.minutos_decena = 0;
                    mihora.horas_unidad++;
                    /* Reset al llegar a 24:00 */
                    if (mihora.horas_decena == 2 && mihora.horas_unidad > 3) {
                        mihora.horas_unidad = 0;
                        mihora.horas_decena = 0;
                    } else if (mihora.horas_unidad > 9) {
                        mihora.horas_unidad = 0;
                        mihora.horas_decena++;
                    }
                }
            }
        }
    }

    /* Reconstruir hora actual para comparar con alarma */
    uint8_t hh = (uint8_t)(mihora.horas_decena   * 10u + mihora.horas_unidad);
    uint8_t mm = (uint8_t)(mihora.minutos_decena  * 10u + mihora.minutos_unidad);
    uint8_t ss = (uint8_t)(mihora.segundos_decena * 10u + mihora.segundos_unidad);

    /* Disparar alarma si coincide con la hora programada */
    if (alarm_enabled && !alarm_fired &&
        hh == alarm_hh && mm == alarm_mm && ss == alarm_ss) {
        alarm_fired = 1;
        buzzer_ring_1s();
    }

    /* Reset alarm_fired cuando el segundo cambia */
    if (alarm_fired &&
        (hh != alarm_hh || mm != alarm_mm || ss != alarm_ss)) {
        alarm_fired = 0;
    }
}
