/*
 * main.c
 * Punto de entrada del sistema. Reloj digital con alarma y juego del granjero.
 * STM32L053R8 - Nucleo-64 - 16MHz HSI
 *
 * Modulos:
 * - clock.c:   reloj en tiempo real y alarma
 * - buzzer.c:  beeps y tono de alarma
 * - keypad.c:  teclado matricial 4x4 y entrada de datos
 * - seg7.c:    display de 7 segmentos multiplexado
 * - lcd.c:     display LCD 16x2 HD44780
 * - game.c:    juego del granjero (Cabbage, Wolf, Sheep)
 *
 * Mapa de interrupciones:
 * - SysTick      @1s:    incrementa el reloj
 * - TIM2         @6ms:   escaneo keypad + debounce botones
 * - TIM21        @1ms:   refresco 7 segmentos (multiplexado)
 * - TIM22        @0.5ms: tono buzzer + envio LCD no bloqueante
 * - EXTI4_15:    botones juego (PC4,PC5,PC6) y toggle 12/24h (PC13)
 *
 * Pines utilizados:
 * - PA0,1,4,5,6,11,12: segmentos 7seg (a,b,e,f,g,c,d)
 * - PA7,8,9,10:        digitos 7seg (0-3)
 * - PB0,PB1:           digitos 7seg (4-5)
 * - PB2,3,4,6:         filas keypad
 * - PB5:               buzzer
 * - PB7,8,9,10:        columnas keypad
 * - PB12,14,15:        LCD D5, D7, RS
 * - PC0,1,9:           LCD EN, D6, D4
 * - PC4,5,6:           botones juego (BTN_GAME, BTN_SEL, BTN_CROSS)
 * - PC13:              boton toggle 12h/24h
 */

#include <stdint.h>
#include "stm32l053xx.h"
#include "lcd.h"
#include "game.h"
#include "buzzer.h"
#include "clock.h"
#include "keypad.h"
#include "seg7.h"

void delayMs(uint16_t n);

/* Contador de debounce para BTN_SEL (PC5), decrementado en TIM2 */
volatile uint8_t debounce_sel = 0;
volatile uint8_t debounce_cross = 0;
volatile uint8_t debounce_game = 0;

int main(void) {

    /* Hora inicial del reloj */
    set_time(14, 30, 20);

    __disable_irq();

    /* Habilitar HSI 16MHz como SYSCLK */
    RCC->CR   |= (1<<0);
    RCC->CFGR |= (1<<0);

    /* Habilitar clocks de GPIOA, GPIOB, GPIOC y SYSCFG */
    RCC->IOPENR  |= 1<<0;
    RCC->IOPENR  |= 1<<1;
    RCC->IOPENR  |= 1<<2;
    RCC->APB2ENR |= 1<<0;

    /* GPIOA: segmentos (PA0,1,4,5,6,11,12) y digitos (PA7,8,9,10) como salida */
    GPIOA->MODER &= ~((3u<<0)|(3u<<2)|(3u<<8)|(3u<<10)|(3u<<12)|(3u<<14)|(3u<<16)|(3u<<18)|(3u<<20)|(3u<<22)|(3u<<24));
    GPIOA->MODER |=  ((1u<<0)|(1u<<2)|(1u<<8)|(1u<<10)|(1u<<12)|(1u<<14)|(1u<<16)|(1u<<18)|(1u<<20)|(1u<<22)|(1u<<24));

    /* GPIOB: PB0,PB1 digitos 4 y 5 + PB5 buzzer como salida */
    GPIOB->MODER &= ~((3u<<0)|(3u<<2)|(3u<<(BUZZ_PIN*2u)));
    GPIOB->MODER |=  ((1u<<0)|(1u<<2)|(1u<<(BUZZ_PIN*2u)));
    buzzer_set(0);

    /* Keypad: filas PB2,PB3,PB4,PB6 como salida */
    GPIOB->MODER &= ~((3u<<(KP_ROW0_PIN*2u))|(3u<<(KP_ROW1_PIN*2u))|(3u<<(KP_ROW2_PIN*2u))|(3u<<(KP_ROW3_PIN*2u)));
    GPIOB->MODER |=  ((1u<<(KP_ROW0_PIN*2u))|(1u<<(KP_ROW1_PIN*2u))|(1u<<(KP_ROW2_PIN*2u))|(1u<<(KP_ROW3_PIN*2u)));

    /* Keypad: columnas PB7,PB8,PB9,PB10 como entrada */
    GPIOB->MODER &= ~((3u<<(KP_COL0_PIN*2u))|(3u<<(KP_COL1_PIN*2u))|(3u<<(KP_COL2_PIN*2u))|(3u<<(KP_COL3_PIN*2u)));

    /* Pull-up interno en columnas del keypad */
    GPIOB->PUPDR &= ~((3u<<(KP_COL0_PIN*2u))|(3u<<(KP_COL1_PIN*2u))|(3u<<(KP_COL2_PIN*2u))|(3u<<(KP_COL3_PIN*2u)));
    GPIOB->PUPDR |=  ((1u<<(KP_COL0_PIN*2u))|(1u<<(KP_COL1_PIN*2u))|(1u<<(KP_COL2_PIN*2u))|(1u<<(KP_COL3_PIN*2u)));

    /* Todas las filas en HIGH (reposo) */
    GPIOB->ODR |= KP_ROW_MASK;

    /* LCD: PB12,PB14,PB15 (D5,D7,RS) como salida */
    GPIOB->MODER &= ~((3u<<24)|(3u<<28)|(3u<<30));
    GPIOB->MODER |=  ((1u<<24)|(1u<<28)|(1u<<30));

    /* LCD: PC0,PC1,PC9 (EN,D6,D4) como salida */
    GPIOC->MODER &= ~((3u<<0)|(3u<<2)|(3u<<18));
    GPIOC->MODER |=  ((1u<<0)|(1u<<2)|(1u<<18));

    /* Botones juego: PC4,PC5,PC6 como entrada */
    GPIOC->MODER &= ~((3u<<8)|(3u<<10)|(3u<<12));

    /* Pull-up interno en botones del juego */
    GPIOC->PUPDR &= ~((3u<<8)|(3u<<10)|(3u<<12));
    GPIOC->PUPDR |=  ((1u<<8)|(1u<<10)|(1u<<12));

    /* PC13 como entrada (boton toggle 12h/24h) */
    GPIOC->MODER &= ~(3u << (13*2u));

    /* USART2: PA2=TX, PA3=RX con funcion alternativa AF4 */
    GPIOA->MODER &=~(1<<4);
    GPIOA->MODER &=~(1<<6);
    GPIOA->AFR[0] |=1<<10;
    GPIOA->AFR[0] |=1<<14;

    RCC->APB1ENR  |= 1<<17;
    USART2->BRR    = 0x682;        // 9600 baud a 16MHz
    USART2->CR1   |= (1<<2)|(1<<3);
    USART2->CR1   |= 1<<0;
    USART2->CR1   |= 1<<5;

    /* EXTI13: PC13 boton toggle 12h/24h, flanco de bajada */
    SYSCFG->EXTICR[3] |= 1<<5;
    EXTI->IMR  |= 1<<13;
    EXTI->FTSR |= 1<<13;

    /* EXTI4,5,6: PC4,PC5,PC6 botones del juego, flanco de bajada */
    SYSCFG->EXTICR[1] |= (2u<<0)|(2u<<4)|(2u<<8);
    EXTI->IMR  |= (1u<<4)|(1u<<5)|(1u<<6);
    EXTI->FTSR |= (1u<<4)|(1u<<5)|(1u<<6);

    /* SysTick: interrupcion cada 1 segundo a 16MHz */
    SysTick->LOAD = 16000000-1;
    SysTick->VAL  = 0;
    SysTick->CTRL = 7;

    /* TIM2: escaneo keypad cada ~6ms (PSC=16000, ARR=100, @16MHz) */
    RCC->APB1ENR |= (1<<0);
    TIM2->PSC     = 16000 - 1;
    TIM2->ARR     = 100-1;
    TIM2->CR1    |= (1<<0);
    TIM2->DIER   |= (1<<0);

    /* TIM21: refresco 7 segmentos cada ~1ms (PSC=16000, ARR=1, @16MHz) */
    RCC->APB2ENR |= (1<<2);
    TIM21->PSC    = 16000 - 1;
    TIM21->ARR    = 1;
    TIM21->CR1   |= (1<<0);
    TIM21->DIER  |= (1<<0);

    /* TIM22: tick buzzer y LCD cada ~0.5ms (PSC=16, ARR=500, @16MHz = 2000Hz) */
    RCC->APB2ENR |= (1<<5);
    TIM22->PSC    = 16 - 1;
    TIM22->ARR    = 500 - 1;
    TIM22->CR1   |= (1<<0);
    TIM22->DIER  |= (1<<0);

    /* Habilitar interrupciones en NVIC */
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM21_IRQn);
    NVIC_EnableIRQ(TIM22_IRQn);

    __enable_irq();

    /* Inicializar LCD despues de habilitar interrupciones */
    lcd_init();
    lcd_queue_update("Press BTN to    ", "start the game! ");

    /* Todo se maneja por interrupciones */
    while(1) {
    }
}

/*
 * TIM21_IRQHandler - refresco del display de 7 segmentos
 * Dispara cada ~1ms, llama a seg7_tick() que activa un digito por vez
 * (multiplexado de 6 digitos: HH MM SS)
 */
void TIM21_IRQHandler(void) {
    TIM21->SR = 0;
    seg7_tick();
}

/*
 * TIM22_IRQHandler - manejo del buzzer y envio no bloqueante al LCD
 * Dispara cada ~0.5ms (2000Hz).
 * Modo 2 (RING_1S): toggle del pin del buzzer cada 3 ticks (~667Hz)
 *                   durante 2000 ticks (1 segundo)
 * Modo 1 (BEEP_N):  toggle durante 300 ticks (150ms ON)
 *                   silencio durante 200 ticks (100ms OFF)
 *                   repite N veces
 * Al final llama lcd_tick() para enviar 1 caracter del buffer al LCD
 */
void TIM22_IRQHandler(void) {
    TIM22->SR = 0;

    if (buz_mode == 2) {
        /* Modo ring: tono continuo de 1 segundo */
        buz_div++;
        if (buz_div >= 3) {
            buz_div = 0;
            GPIOB->ODR ^= (1u << BUZZ_PIN);  // toggle para generar tono
        }
        ring_cnt++;
        if (ring_cnt >= 2000) {
            buzzer_stop();   // detener tras 1 segundo
        }
    } else if (buz_mode == 1) {
        /* Modo beep: N beeps cortos */
        beep_cnt++;
        if (beep_on) {
            buz_div++;
            if (buz_div >= 3) {
                buz_div = 0;
                GPIOB->ODR ^= (1u << BUZZ_PIN);
            }
            if (beep_cnt >= 300) {       // 150ms encendido
                beep_on = 0;
                beep_cnt = 0;
                buzzer_set(0);
                beep_remaining--;
                if (beep_remaining == 0) {
                    buzzer_stop();       // todos los beeps completados
                }
            }
        } else {
            if (beep_cnt >= 200) {       // 100ms apagado
                beep_on = 1;
                beep_cnt = 0;
            }
        }
    }

    /* Enviar un caracter del buffer al LCD */
    lcd_tick();
}

/*
 * TIM2_IRQHandler - escaneo del keypad y debounce
 * Dispara cada ~6ms.
 * Decrementa debounce_sel para el boton BTN_SEL.
 * Llama keypad_tick() que escanea una fila del teclado.
 * Llama lcd_tick() como respaldo adicional para el LCD.
 */
void TIM2_IRQHandler(void) {
    TIM2->SR = 0;
    if (debounce_sel > 0) debounce_sel--;  // decrementar contador debounce
    if (debounce_cross > 0) debounce_cross--;
    if (debounce_game > 0) debounce_game--;
    keypad_tick();
}

/*
 * SysTick_Handler - incremento del reloj cada 1 segundo
 * No actualiza el reloj si el usuario esta en modo set time/alarm
 * (ui_state != 0) para evitar cambios mientras se ingresan datos
 */
void SysTick_Handler(void) {
    if (ui_state) return;
    clock_tick();
}

/*
 * USART2_IRQHandler - recepcion de datos por UART
 * Actualmente solo lee el dato recibido sin procesarlo
 */
void USART2_IRQHandler(void) {
    uint8_t c;
    if (USART2->ISR & 1<<5) {
        c = USART2->RDR;
    }
}

/*
 * EXTI4_15_IRQHandler - manejo de botones externos
 * PC4 (EXTI4) BTN_GAME:  inicia/reinicia/sale del juego
 * PC5 (EXTI5) BTN_SEL:   cicla items del juego (con debounce de ~30ms)
 * PC6 (EXTI6) BTN_CROSS: cruza el rio en el juego
 * PC13(EXTI13) BTN_MODE: toggle entre modo 12h y 24h en el reloj
 */
void EXTI4_15_IRQHandler(void) {
	if (EXTI->PR & (1u << 4)) {
	    EXTI->PR = (1u << 4);
	    if (debounce_game == 0) {
	        debounce_game = 5;  // ~30ms
	        game_btn_game();
	    }
	}
    if (EXTI->PR & (1u << 5)) {
        EXTI->PR = (1u << 5);
        if (debounce_sel == 0) {
            debounce_sel = 5;   // bloquear ~30ms (5 ticks x 6ms)
            game_btn_sel();
        }
    }
    if (EXTI->PR & (1u << 6)) {
        EXTI->PR = (1u << 6);
        if (debounce_cross == 0) {
            debounce_cross = 5;  // ~30ms
            game_btn_cross();
        }
    }
    if (EXTI->PR & (1u << 13)) {
        EXTI->PR = (1u << 13);
        mode_24h ^= 1u;        // toggle 12h/24h
        buzzer_beep_n(1);      // feedback de 1 beep
    }
}

/*
 * delayMs - delay bloqueante en milisegundos
 * Usado SOLO durante lcd_init() antes de que el sistema este corriendo.
 * No usar en ISRs ni durante operacion normal del sistema.
 * Calibrado para 16MHz: ~3195 iteraciones = 1ms
 */
void delayMs(uint16_t n) {
    uint16_t i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++);
}
