/*
 * seg7.c
 * Driver para display de 7 segmentos multiplexado (6 digitos).
 * Muestra HH MM SS del reloj en modo normal,
 * o los digitos ingresados por teclado en modo set time/alarm.
 *
 * Hardware:
 * - Segmentos (catodo comun) en GPIOA:
 *   PA0=a, PA1=b, PA4=e, PA5=f, PA6=g, PA11=c, PA12=d
 * - Digitos 0-3 en GPIOA: PA7, PA8, PA9, PA10
 * - Digitos 4-5 en GPIOB: PB0, PB1
 *
 * Multiplexado:
 * seg7_tick() es llamado desde TIM21 cada ~1ms.
 * Activa un digito por tick en secuencia 0-5, dando
 * una frecuencia de refresco de ~167Hz por digito.
 */

#include "seg7.h"
#include "clock.h"

/* Variables externas del keypad necesarias para modo set time/alarm */
extern volatile uint8_t ui_state;    // 0=normal, 1=set time, 2=set alarm
extern volatile uint8_t st_digits[6]; // digitos ingresados por teclado
extern volatile uint8_t mode_24h;    // 1=formato 24h, 0=formato 12h

/* Indice del digito actualmente activo en el multiplexado (0-5) */
uint8_t seven_segs_sequence = 0x00;

/*
 * Tabla de patrones de segmentos para digitos 0-9
 * Bit 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g (catodo comun, 1=encendido)
 */
static const uint8_t SEG_FONT[10] = {
    0b00111111,  // 0: a,b,c,d,e,f
    0b00000110,  // 1: b,c
    0b01011011,  // 2: a,b,d,e,g
    0b01001111,  // 3: a,b,c,d,g
    0b01100110,  // 4: b,c,f,g
    0b01101101,  // 5: a,c,d,f,g
    0b01111101,  // 6: a,c,d,e,f,g
    0b00000111,  // 7: a,b,c
    0b01111111,  // 8: a,b,c,d,e,f,g
    0b01101111   // 9: a,b,c,d,f,g
};

/*
 * set_segments - activa los segmentos correspondientes al digito
 * digit: valor 0-9 a mostrar
 * Mapea los bits del patron a los pines fisicos de GPIOA:
 *   bit0(a)->PA0, bit1(b)->PA1, bit2(c)->PA11, bit3(d)->PA12
 *   bit4(e)->PA4, bit5(f)->PA5, bit6(g)->PA6
 */
void set_segments(uint8_t digit) {
    uint8_t pattern = SEG_FONT[digit];
    uint16_t mask = 0;

    if (pattern & (1u<<0)) mask |= (1u<<0);   // seg a -> PA0
    if (pattern & (1u<<1)) mask |= (1u<<1);   // seg b -> PA1
    if (pattern & (1u<<2)) mask |= (1u<<11);  // seg c -> PA11
    if (pattern & (1u<<3)) mask |= (1u<<12);  // seg d -> PA12
    if (pattern & (1u<<4)) mask |= (1u<<4);   // seg e -> PA4
    if (pattern & (1u<<5)) mask |= (1u<<5);   // seg f -> PA5
    if (pattern & (1u<<6)) mask |= (1u<<6);   // seg g -> PA6

    GPIOA->ODR &= ~SEG_PINS_MASK;  // apagar todos los segmentos
    GPIOA->ODR |= mask;             // encender los del digito actual
}

/*
 * seg7_tick - actualiza un digito del display por llamada
 * Llamado desde TIM21_IRQHandler cada ~1ms.
 * Apaga todos los digitos, configura los segmentos del digito actual
 * y enciende solo ese digito (multiplexado).
 *
 * En modo normal (ui_state=0): muestra HH MM SS del reloj
 * En modo set time/alarm (ui_state!=0): muestra st_digits ingresados
 *
 * Digitos: 0,1=horas  2,3=minutos  4,5=segundos
 */
void seg7_tick(void) {
    /* Calcular horas segun modo 12h o 24h */
    uint8_t hh_full = (uint8_t)(mihora.horas_decena * 10u + mihora.horas_unidad);
    if (!mode_24h) hh_full = to_12h(hh_full);
    uint8_t hd = (uint8_t)(hh_full / 10u);  // decena de horas
    uint8_t hu = (uint8_t)(hh_full % 10u);  // unidad de horas

    /* Apagar todos los digitos antes de activar el siguiente */
    GPIOA->ODR &= ~ALL_DIG_A;
    GPIOB->ODR &= ~ALL_DIG_B;

    switch (seven_segs_sequence) {
        case 0: /* Decena de horas */
            set_segments(ui_state ? st_digits[0] : hd);
            GPIOA->ODR |= DIG0_PIN_A;
            seven_segs_sequence++;
            break;
        case 1: /* Unidad de horas */
            set_segments(ui_state ? st_digits[1] : hu);
            GPIOA->ODR |= DIG1_PIN_A;
            seven_segs_sequence++;
            break;
        case 2: /* Decena de minutos */
            set_segments(ui_state ? st_digits[2] : mihora.minutos_decena);
            GPIOA->ODR |= DIG2_PIN_A;
            seven_segs_sequence++;
            break;
        case 3: /* Unidad de minutos */
            set_segments(ui_state ? st_digits[3] : mihora.minutos_unidad);
            GPIOA->ODR |= DIG3_PIN_A;
            seven_segs_sequence++;
            break;
        case 4: /* Decena de segundos */
            set_segments(ui_state ? st_digits[4] : mihora.segundos_decena);
            GPIOB->ODR |= DIG4_PIN_B;
            seven_segs_sequence++;
            break;
        case 5: /* Unidad de segundos */
            set_segments(ui_state ? st_digits[5] : mihora.segundos_unidad);
            GPIOB->ODR |= DIG5_PIN_B;
            seven_segs_sequence = 0;  // reiniciar ciclo
            break;
        default:
            seven_segs_sequence = 0;
            break;
    }
}
