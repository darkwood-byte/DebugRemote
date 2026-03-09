#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "ssd1306.h"
#include "hardware2.h"

#define SDA_PIN 8
#define SCL_PIN 9

// ============================================================
// CALLBACKS — vul deze functies zelf in
// ============================================================
int r12 = 56;
int r34 = 32;


bool p1 = false;
bool p2 = false;
bool p3 = false;
bool p4 = false;

void update(void){
    if(r12 < 0)r12 = 0;
    if(r12 > 116)r12 = 116;
    if(r34 < 0)r34 = 0;
    if(r34 > 56)r34 = 56;
    ssd1306_clear();
    if(p1){
        ssd1306_printf(r12,r34, "X-");
        p1 = false;
    }
    else ssd1306_printf(r12,r34, "<-");
    ssd1306_update();
}

void rot1_rotated(void) {
    r12+= 6;
    update();
}
void rot2_rotated(void) {
    r12-= 6;
    update();
}
void rot3_rotated(void) {
    r34+= 4;
    update();
}
void rot4_rotated(void) {
    r34-= 4;
    update();
}

void rot1_pushed(void) {
    p1 = true;
    update();
}
void rot2_pushed(void) {
    p2 = true;
    update();
}
void rot3_pushed(void) {
    p3 = true;
    update();
}
void rot4_pushed(void) {
    p4 = true;
    update();
}

// ============================================================
// PINS & DEBOUNCE
// ============================================================

#define TIMEOUT_US 25000

// Alle pins die een interrupt krijgen
const uint pins[] = {18, 19, 20, 21,   // push knoppen
                      0,  1,  2,  3,   // rot 1 & 2 (A+B)
                      4,  5,  6,  7};  // rot 3 & 4 (A+B)
#define NUM_PINS 12

static uint64_t last_trigger[NUM_PINS];

static int get_pin_index(uint gpio) {
    for (int i = 0; i < NUM_PINS; i++)
        if (pins[i] == gpio) return i;
    return -1;
}

// ============================================================
// GPIO INTERRUPT CALLBACK
// ============================================================

void gpio_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;

    int idx = get_pin_index(gpio);
    if (idx < 0) return;

    uint64_t now = time_us_64();
    if (now - last_trigger[idx] <= TIMEOUT_US) return;
    last_trigger[idx] = now;

    switch (gpio) {
        // Push knoppen
        case 18: rot4_pushed();   break;
        case 19: rot3_pushed();   break;
        case 20: rot2_pushed();   break;
        case 21: rot1_pushed();   break;

        // Rotary encoder 1 (pins 0 & 1)
        case 0: case 1: rot1_rotated(); break;

        // Rotary encoder 2 (pins 2 & 3)
        case 2: case 3: rot2_rotated(); break;

        // Rotary encoder 3 (pins 4 & 5)
        case 4: case 5: rot3_rotated(); break;

        // Rotary encoder 4 (pins 6 & 7)
        case 6: case 7: rot4_rotated(); break;
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();

    // GPIO init — alle pins als input met pull-down
    for (int i = 0; i < NUM_PINS; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_down(pins[i]);
    }

    // Eerste pin registreert de callback, rest alleen interrupt enable
    gpio_set_irq_enabled_with_callback(pins[0], GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    for (int i = 1; i < NUM_PINS; i++)
        gpio_set_irq_enabled(pins[i], GPIO_IRQ_EDGE_RISE, true);

    // I2C + OLED
    i2c_init(i2c0, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(i2c0);
    for(int i = 0; i < 66; i++){
        ssd1306_clear();
        ssd1306_printf(20, i, "starting all");
        ssd1306_update();
        sleep_ms(20);
    }
    update();

    // LED test
    init_rot_status();
    init_status_leds();
    sleep_ms(1000);
    set_status(BUSSY);  
    rot1_on();
    rot2_on();
    rot3_on();
    rot4_on();

    while (1) {
        sleep_ms(1000);
    }
}