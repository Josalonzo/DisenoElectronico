#include <stdint.h>

/* ======================= BASE ADDRESSES ======================= */
#define PERIPH_BASE           (0x40000000u)
#define AHB_BASE              (PERIPH_BASE + 0x20000u)
#define IOPORT_BASE           (PERIPH_BASE + 0x10000000u)

#define RCC_BASE              (AHB_BASE + 0x1000u)
#define GPIOA_BASE            (IOPORT_BASE + 0x0000u)
#define GPIOB_BASE            (IOPORT_BASE + 0x0400u)
#define GPIOC_BASE            (IOPORT_BASE + 0x0800u)

/* ============================== REGISTROS =============================== */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
    volatile uint32_t BRR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR, ICSCR, CRRCR, CFGR, CIER, CIFR, CICR, IOPRSTR,
                      AHBRSTR, APB2RSTR, APB1RSTR, IOPENR, AHBENR, APB2ENR,
                      APB1ENR, IOPSMENR, AHBSMENR, APB2SMENR, APB1SMENR,
                      CCIPR, CSR;
} RCC_TypeDef;

#define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
#define RCC   ((RCC_TypeDef  *) RCC_BASE)

/* ============================= DELAY SIMPLE ============================= */
static void delay_ticks(int32_t ticks)
{
    for (int32_t t = 0; t < ticks; t++) {
        for (int32_t i = 0; i < 15; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

/* ============================ DISPLAY SETUP ============================= */
#define INVERT_SEGMENTS 0
#define INVERT_DIGITS   0

static const uint8_t SEG_FONT[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

#define SEG_PINS_MASK   ( (1u<<0) | (1u<<1) | (1u<<4) | (1u<<5) | (1u<<6) | (1u<<11) | (1u<<12) )
#define DIG4_PINS_MASK  ( (1u<<7) | (1u<<8) | (1u<<9) | (1u<<10) )
#define DIG2_PINS_MASK  ( (1u<<0) | (1u<<1) )  /* PB0, PB1 */

/* ============================ BOTON 12/24 =============================== */
#define BTN_PIN         13u  /* PC13 */
static uint8_t mode_24h = 1;

/* Debounce botón */
static uint8_t  btn_last = 1;
static uint8_t  btn_stable = 1;
static uint16_t btn_cnt = 0;

static uint8_t to_12h(uint8_t hh24)
{
    uint8_t h = (uint8_t)(hh24 % 12);
    return (h == 0) ? 12 : h;
}

static uint8_t button_pressed_event(void)
{
    uint8_t now = (uint8_t)((GPIOC->IDR >> BTN_PIN) & 1u);

    if (now == btn_last) {
        if (btn_cnt < 500) btn_cnt++;
        if (btn_cnt == 50) {
            if (btn_stable != now) {
                btn_stable = now;
                if (btn_stable == 0) return 1;
            }
        }
    } else {
        btn_last = now;
        btn_cnt = 0;
    }
    return 0;
}

/* ================================ BUZZER ================================ */
#define BUZZ_PORT   GPIOB
#define BUZZ_PIN    5u

typedef enum {
    BUZ_OFF = 0,
    BUZ_BEEP_N,
    BUZ_RING_1S
} buz_mode_t;

static buz_mode_t buz_mode = BUZ_OFF;
static uint16_t buz_div = 0;

/* Para beeps N */
static uint8_t  beep_remaining = 0;
static uint8_t  beep_on = 0;
static uint16_t beep_cnt = 0;

/* Para ring 1s */
static uint16_t ring_cnt = 0;  /* cuenta hasta LOOPS_PER_SEC */

static inline void buzzer_set(uint8_t on)
{
    if (on) BUZZ_PORT->BSRR = (1u << BUZZ_PIN);
    else    BUZZ_PORT->BSRR = (1u << (BUZZ_PIN + 16u));
}

static void buzzer_stop(void)
{
    buz_mode = BUZ_OFF;
    beep_remaining = 0;
    beep_on = 0;
    beep_cnt = 0;
    ring_cnt = 0;
    buzzer_set(0);
}

static inline void buzzer_tone_tick(void)
{
    buz_div++;
    if (buz_div >= 3) {      /* cambia 3 para el tono */
        buz_div = 0;
        BUZZ_PORT->ODR ^= (1u << BUZZ_PIN);
    }
}

static void buzzer_beep_n(uint8_t n)
{
    if (n == 0) return;
    buz_mode = BUZ_BEEP_N;
    beep_remaining = n;
    beep_on = 1;
    beep_cnt = 0;
    buz_div = 0;
}

static void buzzer_ring_1s_start(void)
{
    buz_mode = BUZ_RING_1S;
    ring_cnt = 0;
    buz_div = 0;
}

/* Llamar siempre */
static void buzzer_tick(uint16_t loops_per_sec)
{
    if (buz_mode == BUZ_OFF) return;

    if (buz_mode == BUZ_RING_1S) {
        buzzer_tone_tick();
        ring_cnt++;
        if (ring_cnt >= loops_per_sec) {
            buzzer_stop(); /* <-- EXACTO 1 segundo (aprox) */
        }
        return;
    }

    /* Beeps cortos */
    const uint16_t ON_LEN  = 120;
    const uint16_t OFF_LEN = 90;

    if (beep_remaining == 0) {
        buzzer_stop();
        return;
    }

    beep_cnt++;

    if (beep_on) {
        buzzer_tone_tick();
        if (beep_cnt >= ON_LEN) {
            beep_on = 0;
            beep_cnt = 0;
            buzzer_set(0);
        }
    } else {
        if (beep_cnt >= OFF_LEN) {
            beep_on = 1;
            beep_cnt = 0;
            beep_remaining--;
        }
    }
}

/* ================================ KEYPAD 4x4 ================================ */
#define KP_ROW_PORT GPIOB
#define KP_COL_PORT GPIOB

#define KP_R0 2u
#define KP_R1 3u
#define KP_R2 4u
#define KP_R3 6u

#define KP_C0 7u
#define KP_C1 8u
#define KP_C2 9u
#define KP_C3 10u

#define KP_ROW_MASK ((1u<<KP_R0)|(1u<<KP_R1)|(1u<<KP_R2)|(1u<<KP_R3))

static const char KP_MAP[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

static char kp_last_raw = 0;
static char kp_stable   = 0;
static uint16_t kp_cnt  = 0;
static uint8_t kp_lock  = 0;

static inline void kp_rows_all_high(void)
{
    KP_ROW_PORT->BSRR = KP_ROW_MASK;
}

static inline void kp_select_row(uint8_t r)
{
    KP_ROW_PORT->BSRR = KP_ROW_MASK;

    uint32_t pin = KP_R0;
    if (r == 1) pin = KP_R1;
    else if (r == 2) pin = KP_R2;
    else if (r == 3) pin = KP_R3;

    KP_ROW_PORT->BSRR = (1u << (pin + 16u));
}

static inline uint8_t kp_read_cols_bits(void)
{
    uint32_t idr = KP_COL_PORT->IDR;
    uint8_t b0 = (uint8_t)((idr >> KP_C0) & 1u);
    uint8_t b1 = (uint8_t)((idr >> KP_C1) & 1u);
    uint8_t b2 = (uint8_t)((idr >> KP_C2) & 1u);
    uint8_t b3 = (uint8_t)((idr >> KP_C3) & 1u);
    return (uint8_t)((b0<<0)|(b1<<1)|(b2<<2)|(b3<<3));
}

static char keypad_scan_raw(void)
{
    for (uint8_t r = 0; r < 4; r++) {
        kp_select_row(r);
        __asm__ __volatile__("nop"); __asm__ __volatile__("nop"); __asm__ __volatile__("nop");

        uint8_t cols = kp_read_cols_bits();
        if (cols != 0x0F) {
            for (uint8_t c = 0; c < 4; c++) {
                if (((cols >> c) & 1u) == 0u) {
                    kp_rows_all_high();
                    return KP_MAP[r][c];
                }
            }
        }
    }
    kp_rows_all_high();
    return 0;
}

static char keypad_getkey_event(void)
{
    char raw = keypad_scan_raw();

    if (raw == kp_last_raw) {
        if (kp_cnt < 600) kp_cnt++;
        if (kp_cnt == 40) kp_stable = raw;
    } else {
        kp_last_raw = raw;
        kp_cnt = 0;
    }

    if (kp_stable != 0 && kp_lock == 0) {
        kp_lock = 1;
        return kp_stable;
    }

    if (kp_stable == 0) kp_lock = 0;

    return 0;
}

static void keypad_init(void)
{
    uint32_t rows[4] = {KP_R0, KP_R1, KP_R2, KP_R3};
    for (uint8_t i=0;i<4;i++){
        uint32_t p = rows[i];
        KP_ROW_PORT->MODER &= ~(3u << (p*2u));
        KP_ROW_PORT->MODER |=  (1u << (p*2u));
    }

    uint32_t cols[4] = {KP_C0, KP_C1, KP_C2, KP_C3};
    for (uint8_t i=0;i<4;i++){
        uint32_t p = cols[i];
        KP_COL_PORT->MODER &= ~(3u << (p*2u));
        KP_COL_PORT->PUPDR &= ~(3u << (p*2u));
        KP_COL_PORT->PUPDR |=  (1u << (p*2u));
    }

    kp_rows_all_high();
}

/* ============================ UI + ALARMA ============================ */
typedef enum {
    UI_NORMAL = 0,
    UI_SET_TIME,
    UI_SET_ALARM
} ui_state_t;

static ui_state_t ui_state = UI_NORMAL;

static uint8_t in_digits[6];
static uint8_t in_len = 0;

static inline void input_reset(void)
{
    in_len = 0;
    for (uint8_t i=0;i<6;i++) in_digits[i]=0;
}

static inline uint8_t is_digit(char k){ return (k >= '0' && k <= '9'); }

static uint8_t validate_hhmmss(uint8_t hh, uint8_t mm, uint8_t ss)
{
    if (hh > 23) return 0;
    if (mm > 59) return 0;
    if (ss > 59) return 0;
    return 1;
}

static void input_to_time(uint8_t *hh, uint8_t *mm, uint8_t *ss)
{
    *hh = (uint8_t)(in_digits[0]*10u + in_digits[1]);
    *mm = (uint8_t)(in_digits[2]*10u + in_digits[3]);
    *ss = (uint8_t)(in_digits[4]*10u + in_digits[5]);
}

/* Alarma */
static uint8_t alarm_hh = 6, alarm_mm = 0, alarm_ss = 0;
static uint8_t alarm_enabled = 1;
static uint8_t alarm_fired = 0;   /* evita retrigger en el mismo segundo */

static void ui_handle_key(char k, uint8_t *hh, uint8_t *mm, uint8_t *ss)
{
    if (k == 0) return;

    /* # en normal: toggle enable/disable */
    if (ui_state == UI_NORMAL && k == '#') {
        alarm_enabled ^= 1u;
        buzzer_beep_n(1);

        /* si la apagas mientras estaba sonando, corta */
        if (!alarm_enabled) buzzer_stop();
        return;
    }

    if (ui_state == UI_NORMAL) {
        if (k == 'A') { ui_state = UI_SET_TIME;  input_reset(); buzzer_beep_n(1); }
        else if (k == 'B') { ui_state = UI_SET_ALARM; input_reset(); buzzer_beep_n(1); }
        return;
    }

    /* SET TIME / SET ALARM */
    if (k == 'C') { ui_state = UI_NORMAL; input_reset(); buzzer_beep_n(1); return; }
    if (k == '*') { if (in_len > 0) in_len--; buzzer_beep_n(1); return; }

    if (is_digit(k)) {
        if (in_len < 6) {
            in_digits[in_len] = (uint8_t)(k - '0');
            in_len++;
        }
        return;
    }

    if (k == 'D') {
        if (in_len == 6) {
            uint8_t th, tm, ts;
            input_to_time(&th, &tm, &ts);

            if (validate_hhmmss(th, tm, ts)) {
                if (ui_state == UI_SET_TIME) {
                    *hh = th; *mm = tm; *ss = ts;
                } else {
                    alarm_hh = th; alarm_mm = tm; alarm_ss = ts;
                    alarm_enabled = 1;
                }
                ui_state = UI_NORMAL;
                input_reset();
                buzzer_beep_n(2);
            } else {
                input_reset();
                buzzer_beep_n(3);
            }
        } else {
            buzzer_beep_n(3);
        }
        return;
    }
}

/* ============================ DISPLAY FUNC ============================== */
static uint16_t seg_mask_from_digit(uint8_t digit)
{
    if (digit > 9) digit = 0;

    uint8_t pattern = SEG_FONT[digit];

#if INVERT_SEGMENTS
    pattern = (~pattern) & 0x7F;
#endif

    uint16_t mask = 0;
    if (pattern & (1u << 0)) mask |= (1u << 0);
    if (pattern & (1u << 1)) mask |= (1u << 1);
    if (pattern & (1u << 2)) mask |= (1u << 11);
    if (pattern & (1u << 3)) mask |= (1u << 12);
    if (pattern & (1u << 4)) mask |= (1u << 4);
    if (pattern & (1u << 5)) mask |= (1u << 5);
    if (pattern & (1u << 6)) mask |= (1u << 6);
    return mask;
}

static inline void display_clear_all(void)
{
#if INVERT_SEGMENTS
    GPIOA->BSRR = SEG_PINS_MASK;
#else
    GPIOA->BSRR = (SEG_PINS_MASK << 16);
#endif

#if INVERT_DIGITS
    GPIOA->BSRR = DIG4_PINS_MASK;
    GPIOB->BSRR = DIG2_PINS_MASK;
#else
    GPIOA->BSRR = (DIG4_PINS_MASK << 16);
    GPIOB->BSRR = (DIG2_PINS_MASK << 16);
#endif
}

static inline void enable_digit6(uint8_t idx)
{
    if (idx < 4) {
        uint32_t pin = 7u + idx;
#if INVERT_DIGITS
        GPIOA->BSRR = (1u << (pin + 16u));
#else
        GPIOA->BSRR = (1u << pin);
#endif
    } else {
        uint32_t pin = (idx - 4u);
#if INVERT_DIGITS
        GPIOB->BSRR = (1u << (pin + 16u));
#else
        GPIOB->BSRR = (1u << pin);
#endif
    }
}

static inline void set_segments(uint16_t segmask)
{
#if INVERT_SEGMENTS
    GPIOA->BSRR = SEG_PINS_MASK;
    GPIOA->BSRR = ((uint32_t)segmask << 16);
#else
    GPIOA->BSRR = segmask;
#endif
}

static void display_refresh_6digits(const uint8_t digits6[6])
{
    static uint8_t fsm = 0;

    display_clear_all();
    set_segments(seg_mask_from_digit(digits6[fsm]));
    enable_digit6(fsm);

    fsm++;
    if (fsm >= 6) fsm = 0;
}

static void build_display_digits(uint8_t hh, uint8_t mm, uint8_t ss, uint8_t out6[6])
{
    if (ui_state != UI_NORMAL) {
        for (uint8_t i=0;i<6;i++){
            out6[i] = (i < in_len) ? in_digits[i] : 0;
        }
        return;
    }

    out6[0] = (uint8_t)(hh / 10);
    out6[1] = (uint8_t)(hh % 10);
    out6[2] = (uint8_t)(mm / 10);
    out6[3] = (uint8_t)(mm % 10);
    out6[4] = (uint8_t)(ss / 10);
    out6[5] = (uint8_t)(ss % 10);
}

static void display_init(void)
{
    RCC->IOPENR |= (1u<<0) | (1u<<1) | (1u<<2);

    /* GPIOA outputs */
    GPIOA->MODER &= ~(0b1111u << 0);
    GPIOA->MODER |=  (0b0101u << 0);

    GPIOA->MODER &= ~(0b111111u << 8);
    GPIOA->MODER |=  (0b010101u << 8);

    GPIOA->MODER &= ~(0b11111111u << 14);
    GPIOA->MODER |=  (0b01010101u << 14);

    GPIOA->MODER &= ~(0b1111u << 22);
    GPIOA->MODER |=  (0b0101u << 22);

    /* GPIOB outputs PB0, PB1 */
    GPIOB->MODER &= ~(0b1111u << 0);
    GPIOB->MODER |=  (0b0101u << 0);

    /* PC13 input + pull-up */
    GPIOC->MODER &= ~(3u << (BTN_PIN * 2u));
    GPIOC->PUPDR &= ~(3u << (BTN_PIN * 2u));
    GPIOC->PUPDR |=  (1u << (BTN_PIN * 2u));

    /* BUZZER output PB5 */
    GPIOB->MODER &= ~(3u << (BUZZ_PIN * 2u));
    GPIOB->MODER |=  (1u << (BUZZ_PIN * 2u));
    buzzer_set(0);

    keypad_init();
    display_clear_all();
}

/* ============================ MAIN ============================== */
int main(void)
{
    display_init();

    uint8_t hh = 12, mm = 45, ss = 0;
    uint8_t digits6[6];

    const int32_t REFRESH_DELAY = 2;
    const uint16_t LOOPS_PER_SEC = 1100;
    int32_t sec_acc = 0;

    while (1)
    {
        if (button_pressed_event()) {
            mode_24h ^= 1u;
            buzzer_beep_n(1);
        }

        char k = keypad_getkey_event();
        ui_handle_key(k, &hh, &mm, &ss);

        uint8_t hh_show = mode_24h ? hh : to_12h(hh);
        build_display_digits(hh_show, mm, ss, digits6);
        display_refresh_6digits(digits6);

        buzzer_tick(LOOPS_PER_SEC);

        delay_ticks(REFRESH_DELAY);

        /* Timer aproximado */
        if (ui_state == UI_NORMAL) {
            sec_acc++;
            if (sec_acc >= LOOPS_PER_SEC) {
                sec_acc = 0;

                ss++;
                if (ss >= 60) { ss = 0; mm++; }
                if (mm >= 60) { mm = 0; hh++; }
                if (hh >= 24) { hh = 0; }

                /* dispara alarma 1 vez por coincidencia */
                if (alarm_enabled && !alarm_fired &&
                    hh == alarm_hh && mm == alarm_mm && ss == alarm_ss)
                {
                    alarm_fired = 1;
                    buzzer_ring_1s_start(); /* <-- 1 segundo */
                }

                /* rearme cuando ya se pasó el instante */
                if (alarm_fired &&
                    (hh != alarm_hh || mm != alarm_mm || ss != alarm_ss))
                {
                    alarm_fired = 0;
                }
            }
        }
    }
}
