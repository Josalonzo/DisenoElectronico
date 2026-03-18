/*
 * lcd.h
 * Interface del driver para LCD 16x2 HD44780 en modo 4 bits.
 *
 * Conexion de pines:
 * - RS  -> PB15  (Register Select)
 * - EN  -> PC0   (Enable)
 * - D4  -> PC9   (dato bit 4)
 * - D5  -> PB12  (dato bit 5)
 * - D6  -> PC1   (dato bit 6)
 * - D7  -> PB14  (dato bit 7)
 * - RW  -> GND   (siempre escritura)
 * - D0-D3 -> NC  (no conectados en modo 4 bits)
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "stm32l053xx.h"

/* Pines de control */
#define LCD_RS_PORT  GPIOB
#define LCD_RS_PIN   15u   // Register Select: 0=cmd, 1=dato

#define LCD_EN_PORT  GPIOC
#define LCD_EN_PIN   0u    // Enable: pulso para latch de datos

/* Pines de datos (modo 4 bits, solo D4-D7) */
#define LCD_D4_PORT  GPIOC
#define LCD_D4_PIN   9u    // bit 4

#define LCD_D5_PORT  GPIOB
#define LCD_D5_PIN   12u   // bit 5

#define LCD_D6_PORT  GPIOC
#define LCD_D6_PIN   1u    // bit 6

#define LCD_D7_PORT  GPIOB
#define LCD_D7_PIN   14u   // bit 7

/* Buffer no bloqueante - 32 chars (16 por linea) */
extern volatile char    lcd_buf[32];     // contenido pendiente de enviar
extern volatile uint8_t lcd_pending;     // 1=hay datos pendientes

/* Funciones publicas */
void lcd_init(void);                                    // inicializar LCD, llamar una vez al inicio
void lcd_cmd(uint8_t cmd);                              // enviar comando al LCD
void lcd_data(uint8_t ch);                              // enviar caracter al LCD
void lcd_set_cursor(uint8_t col, uint8_t row);          // posicionar cursor (col 0-15, row 0-1)
void lcd_print(const char *str);                        // imprimir cadena en posicion actual
void lcd_clear(void);                                   // limpiar display
void lcd_queue_update(const char *line1, const char *line2); // cargar buffer no bloqueante
void lcd_tick(void);                                    // enviar 1 char del buffer, llamar desde TIM22

#endif
