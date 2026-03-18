/*
 * game.c
 * Implementacion del juego del granjero (Cabbage, Wolf, Sheep).
 *
 * Logica del juego:
 * - Hay dos orillas (SIDE_LEFT y SIDE_RIGHT) separadas por un rio
 * - El granjero tiene un bote que transporta maximo 1 item a la vez
 * - Restricciones: lobo y oveja no pueden quedarse solos,
 *   oveja y repollo no pueden quedarse solos
 * - Objetivo: pasar todos los items a SIDE_RIGHT
 *
 * Controles:
 * - BTN_GAME (PC4): inicia/reinicia/sale del juego
 * - BTN_SEL  (PC5): cicla entre items disponibles en la orilla actual
 * - BTN_CROSS(PC6): cruza el rio con el item seleccionado
 *
 * Display en LCD 16x2:
 * - Linea 1: estado de las orillas  ej: FWCS|~~|
 * - Linea 2: item seleccionado      ej: Sel: OVEJA
 */

#include "game.h"
#include "lcd.h"

/* Estado actual del juego */
volatile uint8_t game_state = GAME_OFF;

/* Orilla actual del granjero: SIDE_LEFT o SIDE_RIGHT */
static uint8_t farmer_side;

/* Orilla actual de cada item */
static uint8_t sheep_side;
static uint8_t wolf_side;
static uint8_t cabbage_side;

/* Item actualmente seleccionado para cruzar */
static uint8_t selected;

/* Lista de items disponibles en la orilla del granjero */
static uint8_t available[4];

/* Cantidad de items disponibles en la orilla actual */
static uint8_t avail_count;

/* Indice del item seleccionado dentro de available[] */
static uint8_t avail_idx;

/*
 * build_available - reconstruye la lista de items disponibles
 * Siempre incluye ITEM_NONE (cruzar solo) como primera opcion.
 * Luego agrega los items que esten en la misma orilla que el granjero.
 * Resetea la seleccion al primer item disponible (ITEM_NONE).
 */
static void build_available(void) {
    avail_count = 0;
    available[avail_count++] = ITEM_NONE;  // siempre puede cruzar solo
    if (wolf_side    == farmer_side) available[avail_count++] = ITEM_WOLF;
    if (sheep_side   == farmer_side) available[avail_count++] = ITEM_SHEEP;
    if (cabbage_side == farmer_side) available[avail_count++] = ITEM_CABBAGE;
    avail_idx = 0;
    selected  = available[0];
}

/*
 * is_illegal - verifica si el estado actual es invalido
 * Retorna 1 si hay una combinacion peligrosa sin el granjero:
 *   - Lobo y oveja solos en la misma orilla
 *   - Oveja y repollo solos en la misma orilla
 */
static uint8_t is_illegal(void) {
    if (wolf_side  == sheep_side   && wolf_side  != farmer_side) return 1;
    if (sheep_side == cabbage_side && sheep_side != farmer_side) return 1;
    return 0;
}

/*
 * is_win - verifica si el jugador gano
 * Retorna 1 si todos los items y el granjero estan en SIDE_RIGHT
 */
static uint8_t is_win(void) {
    return (sheep_side   == SIDE_RIGHT &&
            wolf_side    == SIDE_RIGHT &&
            cabbage_side == SIDE_RIGHT &&
            farmer_side  == SIDE_RIGHT);
}

/*
 * game_init - inicializa el juego
 * Coloca al granjero y todos los items en SIDE_LEFT.
 * Construye la lista de items disponibles y actualiza el LCD.
 */
void game_init(void) {
    farmer_side  = SIDE_LEFT;
    sheep_side   = SIDE_LEFT;
    wolf_side    = SIDE_LEFT;
    cabbage_side = SIDE_LEFT;
    game_state   = GAME_PLAYING;
    build_available();
    game_update_lcd();
}

/*
 * game_btn_game - maneja el boton BTN_GAME (PC4)
 * - Si el juego esta OFF: inicia el juego
 * - Si el juego termino (WIN o OVER): reinicia el juego
 * - Si esta jugando: sale al reloj y limpia el LCD
 */
void game_btn_game(void) {
    if (game_state == GAME_OFF) {
        game_init();
    } else if (game_state == GAME_WIN || game_state == GAME_OVER) {
        game_init();
    } else {
        game_state = GAME_OFF;
        lcd_queue_update("Press BTN to    ", "start the game! ");
    }
}

/*
 * game_btn_sel - maneja el boton BTN_SEL (PC5)
 * Cicla entre los items disponibles en la orilla actual del granjero.
 * Solo funciona si el juego esta en estado GAME_PLAYING.
 */
void game_btn_sel(void) {
    if (game_state != GAME_PLAYING) return;
    avail_idx++;
    if (avail_idx >= avail_count) avail_idx = 0;  // cicla de vuelta al inicio
    selected = available[avail_idx];
    game_update_lcd();
}

/*
 * game_btn_cross - maneja el boton BTN_CROSS (PC6)
 * Cruza el rio con el item seleccionado.
 * Mueve el item y al granjero al lado opuesto.
 * Luego verifica si el estado es ilegal, ganador o continua jugando.
 */
void game_btn_cross(void) {
    if (game_state != GAME_PLAYING) return;

    /* Destino = lado opuesto al granjero */
    uint8_t dest = (farmer_side == SIDE_LEFT) ? SIDE_RIGHT : SIDE_LEFT;

    /* Mover el item seleccionado al destino */
    if (selected == ITEM_SHEEP)   sheep_side   = dest;
    if (selected == ITEM_WOLF)    wolf_side    = dest;
    if (selected == ITEM_CABBAGE) cabbage_side = dest;

    /* El granjero siempre cruza */
    farmer_side = dest;

    /* Verificar resultado del cruce */
    if (is_illegal()) {
        game_state = GAME_OVER;         // movimiento invalido
    } else if (is_win()) {
        game_state = GAME_WIN;          // todos al otro lado
    } else {
        build_available();              // actualizar items disponibles
    }

    game_update_lcd();
}

/*
 * game_update_lcd - actualiza el display LCD con el estado actual
 * Linea 1: posicion de F(granjero) W(lobo) C(repollo) S(oveja)
 *          separados por |~~| que representa el rio
 *          Ejemplo: FWCS|~~|     (todos en la izquierda)
 *                    WC |~~|FS   (granjero y oveja cruzaron)
 * Linea 2: item seleccionado o mensaje de fin de juego
 *          Ejemplo: Sel: OVEJA
 *                   GANASTE!
 *                   GAME OVER
 * Usa lcd_queue_update() para envio no bloqueante via TIM22
 */
void game_update_lcd(void) {
    char line1[17];
    char line2[17];
    uint8_t i;

    /* Construir representacion de cada orilla */
    char left[5]  = "    ";
    char right[5] = "    ";

    /* Colocar cada personaje en su orilla correspondiente */
    if (farmer_side  == SIDE_LEFT)  left[0]  = 'F'; else right[0] = 'F';
    if (wolf_side    == SIDE_LEFT)  left[1]  = 'W'; else right[1] = 'W';
    if (cabbage_side == SIDE_LEFT)  left[2]  = 'C'; else right[2] = 'C';
    if (sheep_side   == SIDE_LEFT)  left[3]  = 'S'; else right[3] = 'S';

    /* Construir linea 1: orilla_izq|~~|orilla_der */
    i = 0;
    line1[i++] = left[0];
    line1[i++] = left[1];
    line1[i++] = left[2];
    line1[i++] = left[3];
    line1[i++] = '|';
    line1[i++] = '~';
    line1[i++] = '~';
    line1[i++] = '|';
    line1[i++] = right[0];
    line1[i++] = right[1];
    line1[i++] = right[2];
    line1[i++] = right[3];
    line1[i++] = ' ';
    line1[i++] = ' ';
    line1[i++] = ' ';
    line1[i++] = ' ';
    line1[i]   = '\0';

    /* Construir linea 2 segun estado del juego */
    if (game_state == GAME_PLAYING) {
        const char *name;
        switch (selected) {
            case ITEM_SHEEP:   name = "SHEEP  "; break;
            case ITEM_WOLF:    name = "WOLF   "; break;
            case ITEM_CABBAGE: name = "CABBAGE"; break;
            default:           name = "ALONE  "; break;
        }
        line2[0]='S'; line2[1]='e'; line2[2]='l';
        line2[3]=':'; line2[4]=' ';
        for (i = 0; i < 7; i++) line2[5+i] = name[i];
        line2[12]=' '; line2[13]=' ';
        line2[14]=' '; line2[15]=' ';
        line2[16]='\0';
    } else if (game_state == GAME_WIN) {
        const char *msg = "YOU WIN!        ";
        for (i = 0; i < 16; i++) line2[i] = msg[i];
        line2[16] = '\0';
    } else if (game_state == GAME_OVER) {
        const char *msg = "GAME OVER       ";
        for (i = 0; i < 16; i++) line2[i] = msg[i];
        line2[16] = '\0';
    }

    /* Enviar al LCD de forma no bloqueante */
    lcd_queue_update(line1, line2);
}
