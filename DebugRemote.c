#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h"
#include "hardware2.h"

#define SDA_PIN 8
#define SCL_PIN 9

// ============================================================
// TUNING PARAMETERS — adjust these for your encoders
// ============================================================

// How many A/B samples to keep per encoder
#define HISTORY_LEN         8

// Min/max number of agreeing samples required to commit a direction
// Recalibration moves threshold between these bounds
#define CONF_MIN            3
#define CONF_MAX            7

// Time window (us) to measure noise rate for recalibration
#define NOISE_WINDOW_US     50000   // 50ms

// Noise event count within NOISE_WINDOW_US that triggers raising confidence
#define NOISE_RAISE_THRESH  4

// How many clean windows before we lower confidence threshold again
#define CLEAN_LOWER_AFTER   5

// Velocity debounce: min/max debounce times in microseconds
// Fast spins use DEBOUNCE_MIN, slow/jittery use DEBOUNCE_MAX
#define DEBOUNCE_MIN_US     500
#define DEBOUNCE_MAX_US     8000

// Push button debounce (us)
#define PUSH_DEBOUNCE_US    30000

// ============================================================
// GPIO PINS
// ============================================================

#define NUM_ROT  4
#define NUM_PUSH 4

// Rotary encoder A/B pins: {A, B}
const uint rot_pins[NUM_ROT][2] = {
    {0, 1},  // rot1
    {2, 3},  // rot2
    {4, 5},  // rot3
    {6, 7},  // rot4
};

// Push button pins (rot1 → rot4)
const uint push_pins[NUM_PUSH] = {21, 20, 19, 18};

// ============================================================
// PER-ENCODER STATE
// ============================================================

typedef struct {
    // Raw A/B history ring buffer: each entry = (a<<1)|b, 0b00–0b11
    uint8_t  history[HISTORY_LEN];
    uint8_t  hist_idx;
    uint8_t  hist_count;        // how many valid entries

    // Last committed A/B state (to detect transitions)
    uint8_t  last_state;

    // Decoded direction history: +1=CW, -1=CCW, 0=invalid
    int8_t   dir_history[HISTORY_LEN];

    // Timing
    uint64_t last_event_us;     // last interrupt time
    uint64_t last_commit_us;    // last committed direction time

    // Dynamic confidence threshold (CONF_MIN..CONF_MAX)
    uint8_t  conf_threshold;

    // Noise tracking
    uint8_t  noise_count;       // noise events in current window
    uint64_t noise_window_start;
    uint8_t  clean_windows;     // consecutive clean windows

    // Current dynamic debounce (us)
    uint32_t debounce_us;
} RotaryState;

static RotaryState rot_state[NUM_ROT];
static uint64_t   last_push_time[NUM_PUSH];

// ============================================================
// APPLICATION CALLBACKS
// ============================================================

void rot1_left()   { ssd1306_clear(); ssd1306_printf(16, 32, "1 left");    ssd1306_update(); }
void rot1_right()  { ssd1306_clear(); ssd1306_printf(16, 32, "1 right");   ssd1306_update(); }
void rot2_left()   { ssd1306_clear(); ssd1306_printf(16, 32, "2 left");    ssd1306_update(); }
void rot2_right()  { ssd1306_clear(); ssd1306_printf(16, 32, "2 right");   ssd1306_update(); }
void rot3_left()   { ssd1306_clear(); ssd1306_printf(16, 32, "3 left");    ssd1306_update(); }
void rot3_right()  { ssd1306_clear(); ssd1306_printf(16, 32, "3 right");   ssd1306_update(); }
void rot4_left()   { ssd1306_clear(); ssd1306_printf(16, 32, "4 left");    ssd1306_update(); }
void rot4_right()  { ssd1306_clear(); ssd1306_printf(16, 32, "4 right");   ssd1306_update(); }

void rot1_pushed() { ssd1306_clear(); ssd1306_printf(16, 32, "1 pushed");  ssd1306_update(); }
void rot2_pushed() { ssd1306_clear(); ssd1306_printf(16, 32, "2 pushed");  ssd1306_update(); }
void rot3_pushed() { ssd1306_clear(); ssd1306_printf(16, 32, "3 pushed");  ssd1306_update(); }
void rot4_pushed() { ssd1306_clear(); ssd1306_printf(16, 32, "4 pushed");  ssd1306_update(); }

// Dispatch tables indexed by encoder number
void (*left_cb[NUM_ROT])()   = { rot1_left,   rot2_left,   rot3_left,   rot4_left   };
void (*right_cb[NUM_ROT])()  = { rot1_right,  rot2_right,  rot3_right,  rot4_right  };
void (*pushed_cb[NUM_PUSH])() = { rot1_pushed, rot2_pushed, rot3_pushed, rot4_pushed };

// ============================================================
// LOOKUP HELPERS
// ============================================================

int get_rot_index(uint gpio) {
    for (int i = 0; i < NUM_ROT; i++)
        if (gpio == rot_pins[i][0] || gpio == rot_pins[i][1]) return i;
    return -1;
}

int get_push_index(uint gpio) {
    for (int i = 0; i < NUM_PUSH; i++)
        if (gpio == push_pins[i]) return i;
    return -1;
}

// ============================================================
// DIRECTION DECODE
// Gray-code transition table.
// Index = (prev_state << 2) | new_state
// Value: +1 = CW, -1 = CCW, 0 = invalid/bounce
// ============================================================

static const int8_t gray_table[16] = {
//  new: 00  01  10  11
         0,  -1,  1,  0,   // prev 00
         1,   0,  0, -1,   // prev 01
        -1,   0,  0,  1,   // prev 10
         0,   1, -1,  0,   // prev 11
};

// ============================================================
// RECALIBRATION LOGIC
// ============================================================

static void recalibrate(RotaryState *s, uint64_t now, bool is_noise) {
    // Start a new noise window if needed
    if ((now - s->noise_window_start) > NOISE_WINDOW_US) {
        if (s->noise_count < NOISE_RAISE_THRESH) {
            // Clean window
            s->clean_windows++;
            if (s->clean_windows >= CLEAN_LOWER_AFTER && s->conf_threshold > CONF_MIN) {
                s->conf_threshold--;
                s->clean_windows = 0;
            }
        } else {
            // Noisy window — raise threshold
            s->clean_windows = 0;
            if (s->conf_threshold < CONF_MAX)
                s->conf_threshold++;
        }
        s->noise_count = 0;
        s->noise_window_start = now;
    }

    if (is_noise) s->noise_count++;
}

// ============================================================
// ROTARY ENCODER HANDLER
// Called on every edge on an encoder pin
// ============================================================

static void handle_rotary_event(int idx, uint64_t now) {
    RotaryState *s = &rot_state[idx];

    // --- Dynamic debounce based on recent spin velocity ---
    uint64_t dt = now - s->last_event_us;
    s->last_event_us = now;

    // Fast events → shorter debounce, slow → longer
    if (dt < 2000) {
        s->debounce_us = DEBOUNCE_MIN_US;
    } else if (dt > 20000) {
        s->debounce_us = DEBOUNCE_MAX_US;
    } else {
        // Linear interpolation between min and max
        uint32_t range = DEBOUNCE_MAX_US - DEBOUNCE_MIN_US;
        s->debounce_us = DEBOUNCE_MIN_US + (uint32_t)(range * (dt - 2000) / 18000);
    }

    // --- Read current A/B state ---
    uint8_t a = gpio_get(rot_pins[idx][0]);
    uint8_t b = gpio_get(rot_pins[idx][1]);
    uint8_t new_state = (a << 1) | b;

    // No change — pure noise, skip
    if (new_state == s->last_state) {
        recalibrate(s, now, true);
        return;
    }

    // --- Decode direction from gray-code table ---
    int8_t dir = gray_table[(s->last_state << 2) | new_state];
    s->last_state = new_state;

    if (dir == 0) {
        // Invalid transition (skipped state) — noise
        recalibrate(s, now, true);
        return;
    }

    // --- Push into direction history ring buffer ---
    s->dir_history[s->hist_idx] = dir;
    s->hist_idx = (s->hist_idx + 1) % HISTORY_LEN;
    if (s->hist_count < HISTORY_LEN) s->hist_count++;

    recalibrate(s, now, false);

    // --- Count agreement in recent history ---
    int agree = 0;
    int window = (s->hist_count < HISTORY_LEN) ? s->hist_count : HISTORY_LEN;
    for (int i = 0; i < window; i++) {
        int8_t d = s->dir_history[(s->hist_idx - 1 - i + HISTORY_LEN) % HISTORY_LEN];
        if (d == dir) agree++;
    }

    // --- Commit if confidence threshold met and debounce elapsed ---
    if (agree >= s->conf_threshold && (now - s->last_commit_us) >= s->debounce_us) {
        s->last_commit_us = now;

        // Clear history after commit so next event starts fresh
        s->hist_count = 0;

        if (dir > 0)
            right_cb[idx]();
        else
            left_cb[idx]();
    }
}

// ============================================================
// GPIO INTERRUPT CALLBACK
// ============================================================

void gpio_callback(uint gpio, uint32_t events) {
    uint64_t now = time_us_64();

    // --- Push buttons ---
    int p_idx = get_push_index(gpio);
    if (p_idx >= 0) {
        if ((now - last_push_time[p_idx]) > PUSH_DEBOUNCE_US) {
            last_push_time[p_idx] = now;
            pushed_cb[p_idx]();
        }
        return;
    }

    // --- Rotary encoders ---
    int r_idx = get_rot_index(gpio);
    if (r_idx >= 0) {
        handle_rotary_event(r_idx, now);
        return;
    }
}

// ============================================================
// INITIALISATION
// ============================================================

static void init_rotary_states(void) {
    for (int i = 0; i < NUM_ROT; i++) {
        RotaryState *s = &rot_state[i];
        uint8_t a = gpio_get(rot_pins[i][0]);
        uint8_t b = gpio_get(rot_pins[i][1]);
        s->last_state       = (a << 1) | b;
        s->hist_idx         = 0;
        s->hist_count       = 0;
        s->conf_threshold   = CONF_MIN + 1;  // start mid-range
        s->debounce_us      = DEBOUNCE_MAX_US;
        s->noise_count      = 0;
        s->noise_window_start = time_us_64();
        s->clean_windows    = 0;
        s->last_event_us    = 0;
        s->last_commit_us   = 0;
    }
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
    stdio_init_all();

    // Init I2C / SSD1306 first so we can show boot message
    i2c_init(i2c0, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(i2c0);
    ssd1306_clear();
    ssd1306_printf(16, 32, "booting...");
    ssd1306_update();

    // Init rotary encoder GPIO pins
    for (int i = 0; i < NUM_ROT; i++) {
        for (int j = 0; j < 2; j++) {
            gpio_init(rot_pins[i][j]);
            gpio_set_dir(rot_pins[i][j], GPIO_IN);
            gpio_pull_up(rot_pins[i][j]);
            gpio_set_irq_enabled_with_callback(
                rot_pins[i][j],
                GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                true,
                &gpio_callback
            );
        }
    }

    // Init push button GPIO pins
    for (int i = 0; i < NUM_PUSH; i++) {
        gpio_init(push_pins[i]);
        gpio_set_dir(push_pins[i], GPIO_IN);
        gpio_pull_up(push_pins[i]);
        gpio_set_irq_enabled_with_callback(
            push_pins[i],
            GPIO_IRQ_EDGE_FALL,  // active-low push buttons
            true,
            &gpio_callback
        );
        last_push_time[i] = 0;
    }

    // Init rotary state machines (reads initial A/B levels)
    init_rotary_states();

    // Hardware2 init
    init_rot_status();
    init_status_leds();

    ssd1306_clear();
    ssd1306_printf(16, 32, "ready");
    ssd1306_update();

    // Main loop — demo cycling status LEDs
    while (1) {
        sleep_ms(1000); set_status(READY);
        sleep_ms(1000); set_status(CONECTING);
        sleep_ms(1000); set_status(ERROR);
        sleep_ms(1000); set_status(BUSSY);
        allrot_off();
    }
}