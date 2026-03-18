/*
 * game.h
 * Interface del juego del granjero (Cabbage, Wolf, Sheep).
 *
 * El juego consiste en cruzar al granjero, lobo, oveja y repollo
 * de SIDE_LEFT a SIDE_RIGHT sin dejar combinaciones peligrosas solas.
 *
 * Solucion clasica:
 * 1. Cruzar OVEJA
 * 2. Regresar SOLO
 * 3. Cruzar LOBO (o REPOLLO)
 * 4. Regresar con OVEJA
 * 5. Cruzar REPOLLO (o LOBO)
 * 6. Regresar SOLO
 * 7. Cruzar OVEJA
 */

#ifndef GAME_H
#define GAME_H

#include <stdint.h>

/* Estados del juego */
#define GAME_OFF     0  // juego inactivo, mostrando reloj
#define GAME_PLAYING 1  // juego en curso
#define GAME_WIN     2  // jugador gano, todos en SIDE_RIGHT
#define GAME_OVER    3  // jugador perdio, combinacion ilegal

/* Items del juego */
#define ITEM_NONE    0  // cruzar solo sin llevar nada
#define ITEM_SHEEP   1  // oveja
#define ITEM_WOLF    2  // lobo
#define ITEM_CABBAGE 3  // repollo

/* Orillas del rio */
#define SIDE_LEFT    0  // orilla izquierda (inicio)
#define SIDE_RIGHT   1  // orilla derecha (destino)

/* Estado actual del juego, visible desde main.c para las ISRs */
extern volatile uint8_t game_state;

/* Funciones publicas */
void game_init(void);        // inicializa/reinicia el juego
void game_btn_game(void);    // boton PC4: inicia/reinicia/sale
void game_btn_sel(void);     // boton PC5: cicla items disponibles
void game_btn_cross(void);   // boton PC6: cruza el rio
void game_update_lcd(void);  // actualiza el LCD con estado actual

#endif
