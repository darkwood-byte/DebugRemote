#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"
#include <stdbool.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_ADDR 0x3C

void ssd1306_init(i2c_inst_t *i2c);
void ssd1306_clear(void);
void ssd1306_pixel(int x,int y,bool color);
void ssd1306_update(void);
void ssd1306_printf(int x, int y, const char *fmt, ...);
#endif