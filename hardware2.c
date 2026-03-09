#include "hardware2.h"

void init_status_leds(void){
    gpio_init(28);
    gpio_init(27);
    gpio_init(26);
    gpio_init(22);

    gpio_set_dir(28, true);
    gpio_set_dir(27, true);
    gpio_set_dir(26, true);
    gpio_set_dir(22, true);

    gpio_put(28, false);
    gpio_put(27, true);
    gpio_put(26, false);
    gpio_put(22, false);
}

void set_status(int code){
    switch (code)
    {
    case READY:
        gpio_put(28, true);
        gpio_put(27, false);
        gpio_put(26, false);
        gpio_put(22, false);
        break;
    case BUSSY:
        gpio_put(28, false);
        gpio_put(27, true);
        gpio_put(26, false);
        gpio_put(22, false);
        break;
    case ERROR:
        gpio_put(28, false);
        gpio_put(27, false);
        gpio_put(26, true);
        gpio_put(22, false);
        break;
    case CONECTING:
        gpio_put(28, false);
        gpio_put(27, false);
        gpio_put(26, false);
        gpio_put(22, true);
        break;
    
    default:
        break;
    }
}
void init_rot_status(void){
    for(int pin = ROTSTATSTART; pin < ROTSTATEND + 1; pin++){
        gpio_init(pin);
        gpio_set_dir(pin, true);
        gpio_put(pin, false);
    }
    
    gpio_put(ROTSTATSTART, true);
    gpio_put(ROTSTATSTART+ 2, true);
    gpio_put(ROTSTATSTART + 5, true);
    gpio_put(ROTSTATSTART+ 6, true);
}

void allrot_off(void){
    for(int pin = ROTSTATSTART; pin < ROTSTATEND + 1; pin++){
        gpio_put(pin, false);
    }
    gpio_put(ROTSTATSTART, true);
    gpio_put(ROTSTATSTART+ 2, true);
    gpio_put(ROTSTATSTART + 5, true);
    gpio_put(ROTSTATSTART+ 6, true);
}

void rot1_on(void){
    gpio_put(10, false);
    gpio_put(11, true);
}

void rot1_off(void){
    gpio_put(11, false);
    gpio_put(10, true);
}

void rot2_on(void){
    gpio_put(12, false);
    gpio_put(13, true);
}

void rot2_off(void){
    gpio_put(13, false);
    gpio_put(12, true);
}
void rot3_on(void){
    gpio_put(15, false);
    gpio_put(14, true);
}

void rot3_off(void){
    gpio_put(14, false);
    gpio_put(15, true);
}
void rot4_on(void){
    gpio_put(16, false);
    gpio_put(17, true);
}

void rot4_off(void){
    gpio_put(17, false);
    gpio_put(16, true);
}