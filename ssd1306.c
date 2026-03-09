#include "ssd1306.h"
#include "pico/stdlib.h"

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static i2c_inst_t *i2c_port;

static void cmd(uint8_t c)
{
    uint8_t data[2]={0x80,c};
    i2c_write_blocking(i2c_port,SSD1306_ADDR,data,2,false);
}

void ssd1306_init(i2c_inst_t *i2c)
{
    i2c_port=i2c;

    sleep_ms(100);

    cmd(0xAE);
    cmd(0x20); cmd(0x00);
    cmd(0xB0);
    cmd(0xC8);
    cmd(0x00);
    cmd(0x10);
    cmd(0x40);
    cmd(0x81); cmd(0x7F);
    cmd(0xA1);
    cmd(0xA6);
    cmd(0xA8); cmd(0x3F);
    cmd(0xD3); cmd(0x00);
    cmd(0xD5); cmd(0x80);
    cmd(0xD9); cmd(0xF1);
    cmd(0xDA); cmd(0x12);
    cmd(0xDB); cmd(0x40);
    cmd(0x8D); cmd(0x14);
    cmd(0xAF);
}

void ssd1306_clear()
{
    for(int i=0;i<sizeof(buffer);i++)
        buffer[i]=0;
}

void ssd1306_pixel(int x,int y,bool color)
{
    if(x<0||x>=SSD1306_WIDTH||y<0||y>=SSD1306_HEIGHT)
        return;

    int index=x+(y/8)*SSD1306_WIDTH;

    if(color)
        buffer[index]|=(1<<(y%8));
    else
        buffer[index]&=~(1<<(y%8));
}

void ssd1306_update()
{
    cmd(0x21);
    cmd(0);
    cmd(SSD1306_WIDTH-1);

    cmd(0x22);
    cmd(0);
    cmd(SSD1306_HEIGHT/8-1);

    uint8_t data[SSD1306_WIDTH+1];
    data[0]=0x40;

    for(int page=0;page<8;page++)
    {
        for(int i=0;i<SSD1306_WIDTH;i++)
            data[i+1]=buffer[page*SSD1306_WIDTH+i];

        i2c_write_blocking(i2c_port,SSD1306_ADDR,data,SSD1306_WIDTH+1,false);
    }
}