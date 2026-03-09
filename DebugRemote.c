#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware2.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

void rot1_pushed(void){

}

void rot2_pushed(void){

}
void rot3_pushed(void){

}
void rot4_pushed(void){

}
#define TIMEOUT_US 25000

const uint pins[] = {18, 19, 20, 21, 0, 1, 2, 3, 4, 5, 6, 7};
#define NUM_PINS 12

uint64_t last_trigger[NUM_PINS] = {0};

int get_pin_index(uint gpio) {
    for (int i = 0; i < NUM_PINS; i++) {
        if (pins[i] == gpio) return i;
    }
    return -1;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {

        int idx = get_pin_index(gpio);
        if (idx < 0) return;

        uint64_t now = time_us_64();

        if (now - last_trigger[idx] > TIMEOUT_US) {
            last_trigger[idx] = now;

            switch (gpio)
            {
            case 18:
                rot4_pushed();
                break;
            case 19:
                rot3_pushed();
                break;
            case 20:
                rot2_pushed();
                break;
            case 21:
                rot1_pushed();
                break;
            
            default:
                break;
            }
        }
    }
}

int main()
{
    stdio_init_all();
     for (int i = 0; i < NUM_PINS; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_down(pins[i]);
    }

    // eerste pin zet callback
    gpio_set_irq_enabled_with_callback(pins[0], GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    // overige pins alleen interrupt enable
    for (int i = 1; i < NUM_PINS; i++) {
        gpio_set_irq_enabled(pins[i], GPIO_IRQ_EDGE_RISE, true);
    }

    i2c_init(i2c0,400000);

    gpio_set_function(SDA_PIN,GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN,GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(i2c0);

    ssd1306_clear();

    ssd1306_printf(16, 32,"test");

    ssd1306_update();

    init_rot_status();
    init_status_leds();
    sleep_ms(1000);
    
    while(1){

        sleep_ms(1000);
        set_status(READY);

        sleep_ms(1000);
        set_status(CONECTING);

        sleep_ms(1000);
        set_status(ERROR);

        sleep_ms(1000);
        set_status(BUSSY);
        allrot_off();
        sleep_ms(1000);
    }
    
        
}