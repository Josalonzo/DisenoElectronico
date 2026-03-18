/*
 * lcd.c
 * Driver para LCD 16x2 HD44780 en modo 4 bits.
 *
 * Hardware - pines del LCD:
 * - RS  -> PB15  (Register Select: 0=comando, 1=dato)
 * - EN  -> PC0   (Enable: pulso para latch de datos)
 * - D4  -> PC9   (bit de datos 4)
 * - D5  -> PB12  (bit de datos 5)
 * - D6  -> PC1   (bit de datos 6)
 * - D7  -> PB14  (bit de datos 7)
 * - RW  -> GND   (siempre escritura)
 *
 * Modo no bloqueante:
 * lcd_queue_update() llena un buffer de 32 caracteres (2 lineas x 16).
 * lcd_tick() enviа un caracter por llamada desde TIM22_IRQHandler,
 * evitando bloquear el CPU y afectar el multiplexado de los 7 segmentos.
 */

#include "lcd.h"

extern void delayMs(uint16_t n);

/* Buffer de 32 caracteres pendientes de enviar al LCD (16 por linea) */
volatile char lcd_buf[32];

/* 1=hay datos pendientes de enviar, 0=LCD actualizado */
volatile uint8_t lcd_pending = 0;

/* Indice del siguiente caracter a enviar del buffer */
static uint8_t lcd_idx = 0;

/*
 * lcd_set_rs - controla el pin RS del LCD
 * v=0: modo comando (instruccion al controlador)
 * v=1: modo dato (caracter a mostrar)
 * Usa BSRR para escritura atomica sin afectar otros pines
 */
static void lcd_set_rs(uint8_t v) {
    if (v) LCD_RS_PORT->BSRR = (1u << LCD_RS_PIN);
    else   LCD_RS_PORT->BSRR = (1u << (LCD_RS_PIN + 16u));
}

/*
 * lcd_set_en - controla el pin EN del LCD
 * v=1: activa el latch (HD44780 lee los datos en flanco de bajada)
 * v=0: desactiva el latch
 */
static void lcd_set_en(uint8_t v) {
    if (v) LCD_EN_PORT->BSRR = (1u << LCD_EN_PIN);
    else   LCD_EN_PORT->BSRR = (1u << (LCD_EN_PIN + 16u));
}

/*
 * lcd_pulse_en - genera el pulso de Enable requerido por el HD44780
 * El HD44780 requiere minimo ~450ns de pulso en EN.
 * El loop de 50 iteraciones a 16MHz da ~3us, suficiente margen.
 */
static void lcd_pulse_en(void) {
    lcd_set_en(1);
    for (volatile uint16_t i = 0; i < 20; i++);
    lcd_set_en(0);
    for (volatile uint16_t i = 0; i < 20; i++);
}

/*
 * lcd_write_nibble - escribe 4 bits en los pines D4-D7 y pulsa EN
 * nibble: valor de 4 bits a escribir (bits 0-3 = D4-D7)
 * Usa BSRR para escritura atomica de cada pin individualmente
 */
static void lcd_write_nibble(uint8_t nibble) {
    if (nibble & 0x01) LCD_D4_PORT->BSRR = (1u << LCD_D4_PIN);
    else               LCD_D4_PORT->BSRR = (1u << (LCD_D4_PIN + 16u));
    if (nibble & 0x02) LCD_D5_PORT->BSRR = (1u << LCD_D5_PIN);
    else               LCD_D5_PORT->BSRR = (1u << (LCD_D5_PIN + 16u));
    if (nibble & 0x04) LCD_D6_PORT->BSRR = (1u << LCD_D6_PIN);
    else               LCD_D6_PORT->BSRR = (1u << (LCD_D6_PIN + 16u));
    if (nibble & 0x08) LCD_D7_PORT->BSRR = (1u << LCD_D7_PIN);
    else               LCD_D7_PORT->BSRR = (1u << (LCD_D7_PIN + 16u));

    lcd_pulse_en();
}

/*
 * lcd_send - envia un byte completo al LCD en modo 4 bits
 * Envia primero el nibble alto y luego el nibble bajo.
 * rs=0: comando, rs=1: dato
 * El loop entre nibbles da tiempo al HD44780 para procesar
 */
static void lcd_send(uint8_t value, uint8_t rs) {
    lcd_set_rs(rs);
    lcd_write_nibble(value >> 4);          // nibble alto primero
    for (volatile uint16_t i = 0; i < 20; i++);
    lcd_write_nibble(value & 0x0F);        // nibble bajo
    for (volatile uint16_t i = 0; i < 20; i++);
}

/* lcd_cmd - envia un comando al LCD (RS=0) */
void lcd_cmd(uint8_t cmd)  { lcd_send(cmd, 0); }

/* lcd_data - envia un caracter a mostrar en el LCD (RS=1) */
void lcd_data(uint8_t ch)  { lcd_send(ch,  1); }

/*
 * lcd_set_cursor - posiciona el cursor en col, row
 * col: columna (0-15)
 * row: fila (0=primera linea, 1=segunda linea)
 * Direcciones DDRAM: linea 1 = 0x80, linea 2 = 0xC0
 */
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_cmd(addr);
}

/*
 * lcd_print - imprime una cadena de caracteres en la posicion actual
 * Envia cada caracter hasta encontrar el terminador nulo
 */
void lcd_print(const char *str) {
    while (*str) lcd_data((uint8_t)*str++);
}

/*
 * lcd_clear - limpia el display y vuelve el cursor al inicio
 * El comando 0x01 requiere ~2ms de espera segun datasheet HD44780
 */
void lcd_clear(void) {
    lcd_cmd(0x01);
    delayMs(2);
}

/*
 * lcd_init - inicializa el LCD en modo 4 bits segun datasheet HD44780
 * Secuencia de inicializacion requerida:
 * 1. Esperar >40ms tras encendido
 * 2. Tres pulsos 0x03 con delays (reset por software)
 * 3. Pulso 0x02 para entrar a modo 4 bits
 * 4. Configurar: 2 lineas, fuente 5x8, display ON, cursor OFF
 * Solo se llama una vez al inicio del programa
 */
void lcd_init(void) {
    delayMs(100);          // espera estabilizacion tras encendido
    lcd_set_rs(0);
    lcd_set_en(0);

    /* Secuencia de reset por software (3 veces 0x03) */
    lcd_write_nibble(0x03); delayMs(10);
    lcd_write_nibble(0x03); delayMs(5);
    lcd_write_nibble(0x03); delayMs(1);
    lcd_write_nibble(0x02); delayMs(1);   // entrar a modo 4 bits

    lcd_cmd(0x28); delayMs(1);   // 4 bits, 2 lineas, fuente 5x8
    lcd_cmd(0x0C); delayMs(1);   // display ON, cursor OFF, blink OFF
    lcd_cmd(0x06); delayMs(1);   // incremento automatico, sin scroll
    lcd_cmd(0x01);               // limpiar display
    delayMs(5);
}

/*
 * lcd_queue_update - carga el buffer con nuevo contenido a mostrar
 * line1: cadena de 16 caracteres para la primera linea
 * line2: cadena de 16 caracteres para la segunda linea
 * Activa lcd_pending para que lcd_tick() empiece el envio
 */
void lcd_queue_update(const char *line1, const char *line2) {
    uint8_t i;
    for (i = 0; i < 16; i++) lcd_buf[i]    = line1[i];
    for (i = 0; i < 16; i++) lcd_buf[16+i] = line2[i];
    lcd_idx     = 0;
    lcd_pending = 1;
}

/*
 * lcd_tick - envia un caracter del buffer al LCD por llamada
 * Llamado desde TIM22_IRQHandler cada ~0.5ms.
 * Envia un caracter por tick para no bloquear el CPU.
 * Al llegar al caracter 0 y 16 posiciona el cursor en la linea correcta.
 * Cuando termina los 32 caracteres limpia lcd_pending.
 */
void lcd_tick(void) {
    if (!lcd_pending) return;

    /* Posicionar cursor al inicio de cada linea */
    if (lcd_idx == 0) {
        lcd_cmd(0x80);   // inicio linea 1
    } else if (lcd_idx == 16) {
        lcd_cmd(0xC0);   // inicio linea 2
    }

    if (lcd_idx < 32) {
        lcd_data((uint8_t)lcd_buf[lcd_idx]);
        lcd_idx++;
    } else {
        /* Buffer completo, LCD actualizado */
        lcd_pending = 0;
        lcd_idx     = 0;
    }
}
