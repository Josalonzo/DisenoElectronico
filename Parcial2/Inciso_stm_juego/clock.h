/*
 * clock.h
 * Interface del modulo de reloj y alarma.
 * El tiempo se almacena en digitos separados (unidades y decenas)
 * para facilitar el display directo en los 7 segmentos.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

/*
 * struct time - almacena la hora en digitos separados
 * Cada campo representa un digito individual (0-9)
 * Ejemplo: 14:30:25 -> horas_decena=1, horas_unidad=4,
 *          minutos_decena=3, minutos_unidad=0,
 *          segundos_decena=2, segundos_unidad=5
 */
struct time {
    uint8_t segundos_unidad;  // 0-9
    uint8_t segundos_decena;  // 0-5
    uint8_t minutos_unidad;   // 0-9
    uint8_t minutos_decena;   // 0-5
    uint8_t horas_unidad;     // 0-9
    uint8_t horas_decena;     // 0-2
};

/* Hora actual del reloj */
extern struct time mihora;

/* Modo de display: 1=24h, 0=12h (toggle con boton PC13) */
extern volatile uint8_t mode_24h;

/* Configuracion de la alarma */
extern volatile uint8_t alarm_hh;       // hora de la alarma (0-23)
extern volatile uint8_t alarm_mm;       // minuto de la alarma (0-59)
extern volatile uint8_t alarm_ss;       // segundo de la alarma (0-59)
extern volatile uint8_t alarm_enabled;  // 1=alarma activa, 0=desactivada
extern volatile uint8_t alarm_fired;    // 1=ya sono este segundo

/* Funciones publicas */
void set_time(uint8_t hh, uint8_t mm, uint8_t ss);  // establece hora actual
uint8_t to_12h(uint8_t hh24);                        // convierte 24h a 12h
void clock_tick(void);                               // incrementa 1 segundo, llamar desde SysTick

#endif
