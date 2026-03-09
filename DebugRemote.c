#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define SDA_PIN 8
#define SCL_PIN 9

#define ROTSTATSTART 10
#define ROTSTATEND 17

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

void rot1_on(void){
    gpio_put(16, false);
    gpio_put(17, true);
}

void rot1_off(void){
    gpio_put(17, false);
    gpio_put(16, true);
}

void rot2_on(void){
    gpio_put(16, false);
    gpio_put(17, true);
}

void rot2_off(void){
    gpio_put(17, false);
    gpio_put(16, true);
}
void rot3_on(void){
    gpio_put(16, false);
    gpio_put(17, true);
}

void rot3_off(void){
    gpio_put(17, false);
    gpio_put(16, true);
}
void rot4_on(void){
    gpio_put(16, false);
    gpio_put(17, true);
}

void rot4_off(void){
    gpio_put(17, false);
    gpio_put(16, true);
}
int main()
{
    stdio_init_all();

    i2c_init(i2c0,400000);

    gpio_set_function(SDA_PIN,GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN,GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(i2c0);

    ssd1306_clear();

    for(int x=0;x<128;x++)
        ssd1306_pixel(x,32,1);

    ssd1306_update();

    init_rot_status();
    sleep_ms(1000);
    rot1_on();
    sleep_ms(1000);
    rot2_on();
    sleep_ms(1000);
    rot3_on();
    sleep_ms(1000);
    rot4_on();
    while(1)
        sleep_ms(1000);
        
}