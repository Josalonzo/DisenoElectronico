/*
 * keypad.c
 * Manejo del teclado matricial 4x4 y entrada de datos del reloj.
 *
 * Hardware:
 * - Filas (salidas): PB2, PB3, PB4, PB6
 * - Columnas (entradas con pull-up): PB7, PB8, PB9, PB10
 *
 * Teclas del reloj:
 * - A: entrar a modo set time
 * - B: entrar a modo set alarm
 * - C: cancelar
 * - D: confirmar
 * - 0-9: ingresar digitos
 * - *: borrar ultimo digito
 * - #: activar/desactivar alarma
 *
 * El escaneo se hace por TIM2 barriendo una fila por tick (~6ms).
 * Se usa kp_lock para evitar deteccion multiple de la misma tecla.
 */

#include "keypad.h"
#include "clock.h"
#include "buzzer.h"
#include "game.h"

/* Mapa de teclas del teclado 4x4 */
static const char KP_MAP[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* Fila actual siendo escaneada (0-3, cicla en keypad_tick) */
volatile uint8_t kp_row_seq    = 0;

/* Lock: 1=tecla detectada, espera a que se suelte antes de aceptar otra */
volatile uint8_t kp_lock       = 0;

/* Fila que estaba activa cuando se detecto la tecla */
volatile uint8_t kp_row_active = 0;

/* Ultima tecla decodificada como caracter ASCII */
volatile uint8_t kp_key        = 0xFF;

/* Flag: 1=hay tecla nueva disponible para leer */
volatile uint8_t kp_pressed    = 0;

/* Estado de la UI: 0=normal, 1=set time, 2=set alarm */
volatile uint8_t ui_state   = 0;

/* Buffer de digitos ingresados por teclado (HHMMSS) */
volatile uint8_t st_digits[6];

/* Cantidad de digitos ingresados actualmente (0-6) */
volatile uint8_t st_len     = 0;

/*
 * kp_cols_all_high - verifica si todas las columnas estan en HIGH
 * Retorna 1 si ninguna tecla esta presionada (todas las columnas HIGH)
 * Usado para detectar cuando se suelta una tecla y liberar kp_lock
 */
uint8_t kp_cols_all_high(void) {
    uint32_t idr = GPIOB->IDR;
    uint8_t c0 = (uint8_t)((idr >> KP_COL0_PIN) & 1u);
    uint8_t c1 = (uint8_t)((idr >> KP_COL1_PIN) & 1u);
    uint8_t c2 = (uint8_t)((idr >> KP_COL2_PIN) & 1u);
    uint8_t c3 = (uint8_t)((idr >> KP_COL3_PIN) & 1u);
    return (uint8_t)(c0 & c1 & c2 & c3);
}

/*
 * keypad_getkey - retorna la ultima tecla presionada
 * Retorna 0 si no hay tecla nueva disponible
 * Limpia el flag kp_pressed al leer
 */
uint8_t keypad_getkey(void) {
    if (!kp_pressed) return 0;
    kp_pressed = 0;
    return kp_key;
}

/*
 * st_reset - limpia el buffer de digitos ingresados
 * Resetea st_len a 0 y borra todos los digitos del buffer
 */
void st_reset(void) {
    st_len = 0;
    for (uint8_t i = 0; i < 6; i++) st_digits[i] = 0;
}

/*
 * st_valid_time - valida si los valores ingresados forman una hora valida
 * hh: horas (debe ser 0-23)
 * mm: minutos (debe ser 0-59)
 * ss: segundos (debe ser 0-59)
 * Retorna 1 si es valido, 0 si no
 */
uint8_t st_valid_time(uint8_t hh, uint8_t mm, uint8_t ss) {
    if (hh > 23) return 0;
    if (mm > 59) return 0;
    if (ss > 59) return 0;
    return 1;
}

/*
 * handle_key - procesa la tecla presionada segun el estado actual
 * Modo normal (ui_state=0):
 *   A -> entrar a set time
 *   B -> entrar a set alarm
 *   # -> toggle alarma on/off
 * Modo set time/alarm (ui_state=1,2):
 *   0-9 -> ingresar digito
 *   *   -> borrar ultimo digito
 *   C   -> cancelar
 *   D   -> confirmar (requiere 6 digitos validos)
 */
void handle_key(char k) {
    if (k == 0) return;

    /* Modo normal */
    if (ui_state == 0) {
        if (k == 'A') {
            ui_state = 1;          // entrar a set time
            st_reset();
            buzzer_beep_n(1);
        } else if (k == 'B') {
            ui_state = 2;          // entrar a set alarm
            st_reset();
            buzzer_beep_n(1);
        } else if (k == '#') {
            /* Si la alarma esta sonando, detenerla primero */
            if (!alarm_enabled && buz_mode == 2) {
                buzzer_stop();
            }
            alarm_enabled ^= 1u;   // toggle alarma
            buzzer_beep_n(1);
        }
        return;
    }

    /* Cancelar - vuelve al modo normal */
    if (k == 'C') {
        ui_state = 0;
        st_reset();
        buzzer_beep_n(1);
        return;
    }

    /* Backspace - borra el ultimo digito ingresado */
    if (k == '*') {
        if (st_len > 0) {
            st_len--;
            st_digits[st_len] = 0;
        }
        buzzer_beep_n(1);
        return;
    }

    /* Digitos 0-9 - agregar al buffer si hay espacio */
    if (k >= '0' && k <= '9') {
        if (st_len < 6) {
            st_digits[st_len] = (uint8_t)(k - '0');
            st_len++;
        }
        return;
    }

    /* Confirmar - requiere exactamente 6 digitos */
    if (k == 'D') {
        if (st_len == 6) {
            uint8_t hh = (uint8_t)(st_digits[0]*10u + st_digits[1]);
            uint8_t mm = (uint8_t)(st_digits[2]*10u + st_digits[3]);
            uint8_t ss = (uint8_t)(st_digits[4]*10u + st_digits[5]);

            if (st_valid_time(hh, mm, ss)) {
                if (ui_state == 1) {
                    set_time(hh, mm, ss);   // guardar nueva hora
                } else {
                    alarm_hh      = hh;     // guardar nueva alarma
                    alarm_mm      = mm;
                    alarm_ss      = ss;
                    alarm_enabled = 1;
                }
                ui_state = 0;
                st_reset();
                buzzer_beep_n(2);   // 2 beeps = exito
            } else {
                st_reset();
                buzzer_beep_n(3);   // 3 beeps = error hora invalida
            }
        }
        return;
    }
}

/*
 * keypad_tick - escanea una fila del teclado por llamada
 * Llamado desde TIM2_IRQHandler cada ~6ms.
 * Lee las columnas de la fila activa del tick anterior,
 * luego prepara la siguiente fila para el proximo tick.
 * Usa kp_lock para ignorar detecciones multiples de la misma tecla.
 */
void keypad_tick(void) {
    /* Leer columnas de la fila activa en el tick anterior */
    if (!kp_lock) {
        uint8_t col = 0xFF;
        if (!(GPIOB->IDR & (1u << KP_COL0_PIN)))      col = 0;
        else if (!(GPIOB->IDR & (1u << KP_COL1_PIN))) col = 1;
        else if (!(GPIOB->IDR & (1u << KP_COL2_PIN))) col = 2;
        else if (!(GPIOB->IDR & (1u << KP_COL3_PIN))) col = 3;

        if (col != 0xFF) {
            /* Tecla detectada: decodificar y procesar */
            kp_key     = (uint8_t)KP_MAP[kp_row_active][col];
            kp_pressed = 1;
            kp_lock    = 1;        // bloquear hasta que se suelte
            handle_key((char)kp_key);
        }
    }

    /* Desbloquear cuando todas las columnas vuelven a HIGH (tecla suelta) */
    if (kp_lock) {
        if (kp_cols_all_high()) {
            kp_lock = 0;
        }
    }

    /* Preparar siguiente fila: poner HIGH todas y bajar solo la activa */
    kp_row_active = kp_row_seq;
    GPIOB->ODR |= KP_ROW_MASK;
    switch (kp_row_seq) {
        case 0: GPIOB->ODR &= ~(1u << KP_ROW0_PIN); break;
        case 1: GPIOB->ODR &= ~(1u << KP_ROW1_PIN); break;
        case 2: GPIOB->ODR &= ~(1u << KP_ROW2_PIN); break;
        default: GPIOB->ODR &= ~(1u << KP_ROW3_PIN); break;
    }
    kp_row_seq++;
    if (kp_row_seq >= 4) kp_row_seq = 0;
}
