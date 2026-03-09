#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware2.h"

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


    ssd1306_update();

    init_rot_status();
    init_status_leds();
    sleep_ms(1000);
    
    while(1){
        rot1_on();
        sleep_ms(1000);
        set_status(READY);
        rot2_on();
        sleep_ms(1000);
        set_status(CONECTING);
        rot3_on();
        sleep_ms(1000);
        set_status(ERROR);
        rot4_on();
        sleep_ms(1000);
        set_status(BUSSY);
        allrot_off();
        sleep_ms(1000);
    }
    
        
}