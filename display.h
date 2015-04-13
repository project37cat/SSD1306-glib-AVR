//SSD1306 128*64 display
//
//13-Apr-2015


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>


#include "font.h"


//software SPI pins

#define SCK_PORT PORTD //serial clock (D0)
#define SCK_PIN  2

#define DAT_PORT PORTD //data output (D1)
#define DAT_PIN  3

#define RST_PORT PORTD //display reset
#define RST_PIN  4

#define DC_PORT  PORTD //data/command
#define DC_PIN   5

#define CS_PORT  PORTD //chip select
#define CS_PIN   6


#define SET_BIT(reg, bit)  reg |= (1<<bit)
#define CLR_BIT(reg, bit)  reg &= (~(1<<bit))
#define INV_BIT(reg, bit)  reg ^= (1<<bit)


#define DDRX(x) (*(&x-1))

#define SCK_DDR  SET_BIT(DDRX(SCK_PORT), SCK_PIN)
#define DAT_DDR  SET_BIT(DDRX(DAT_PORT), DAT_PIN)
#define RST_DDR  SET_BIT(DDRX(RST_PORT), RST_PIN)
#define DC_DDR   SET_BIT(DDRX(DC_PORT), DC_PIN)
#define CS_DDR   SET_BIT(DDRX(CS_PORT), CS_PIN)


#define SCK_H  SET_BIT(SCK_PORT, SCK_PIN)
#define SCK_L  CLR_BIT(SCK_PORT, SCK_PIN)

#define DAT_H  SET_BIT(DAT_PORT, DAT_PIN)
#define DAT_L  CLR_BIT(DAT_PORT, DAT_PIN)

#define RST_H  SET_BIT(RST_PORT, RST_PIN)
#define RST_L  CLR_BIT(RST_PORT, RST_PIN)

#define DC_H   SET_BIT(DC_PORT, DC_PIN)
#define DC_L   CLR_BIT(DC_PORT, DC_PIN)

#define CS_H   SET_BIT(CS_PORT, CS_PIN)
#define CS_L   CLR_BIT(CS_PORT, CS_PIN)


uint8_t scrBuff[128*8]; //screen buffer 1024 byte


//commands for the initialization of SSD1306

const uint8_t init[] PROGMEM =
{
0xae, //display off sleep mode
0xd5, //display clock divide
0x80, //
0xa8, //set multiplex ratio
0x3f, //
0xd3, //display offset
0x00, //
0x40, //set display start line
0x8d, //charge pump setting
0x14, //
0x20, //memory addressing mode
0x00, //horizontal addressing mode
0xa1, //segment re-map
0xc8, //COM output scan direction
0xda, //COM pins hardware configuration
0x12, //
0x81, //set contrast (brightness)
0x8f, //0x8f, 0xcf  //0..255
0xd9, //pre-charge period
0xf1, //
0xdb, //VCOMH deselect level
0x40, //
0xa4, //entire display off
0xa6, //normal display, 0xa7 inverse display
0xaf  //display turned on
};


void display_init(void); //init port and display

void disp_write(uint8_t mode, uint8_t data); //mode: 1-write data, 0-write command

/*** operations with the screen buffer ***/

void screen_clear(void); //clear buffer
void screen_update(void); //write buffer to screen

void print_char(uint8_t column, uint8_t string, uint8_t sign); //print character
void print_string(uint8_t column, uint8_t string, char *str); //0..120 //0..7 //print string

void draw_pixel(uint8_t hPos, uint8_t vPos); //draw pixel //0..127 //0..63

void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2); //draw line //x - 0..127  //y - 0..63


///////////////////////////////////////////////////////////////////////////////////////////////////
void display_init(void) //init port and display
{
SCK_DDR; //pins is configured as an output pins
DAT_DDR;
RST_DDR;
DC_DDR;
CS_DDR;

SCK_L; //sck line low

CS_H; //chip select inactive

RST_L; //display chip reset
_delay_us(3); //delay for reset
RST_H; //display normal operation

for(uint8_t i=0; i<sizeof init; i++) disp_write(0,pgm_read_byte(&init[i])); //send the init commands
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void disp_write(uint8_t mode, uint8_t data) //mode: 1-data, 0-command  //data: data byte
{
uint8_t s=0x80;

if(mode) DC_H; //data mode
else DC_L; //command

CS_L; //chip select in the active state

for(uint8_t i=0; i<8; i++) //send byte
	{
	if(data & s) DAT_H;
	else DAT_L;
	s = s>>1;
	SCK_H; //on rising edge of SCLK
	SCK_L;
	}

CS_H;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void screen_clear(void) //clear buffer
{
for(uint16_t x=0; x<1024; x++) scrBuff[x]=0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void screen_update(void) //write buffer to screen
{
disp_write(0,0x21); //set column address
disp_write(0,0); //start address
disp_write(0,127); //end address

disp_write(0,0x22); //set page address
disp_write(0,0);
disp_write(0,7);

for(uint16_t x=0; x<1024; x++) disp_write(1,scrBuff[x]); //columns*strings(pages)=128*8=1024
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void print_char(uint8_t column, uint8_t string, uint8_t sign) //0..120 //0..7 //print a character
{
if(column<=120 && string<8)
	{
	if(sign<32 || sign>127) sign=128; //filling field for invalid characters
	for(uint8_t y=0; y<5; y++) scrBuff[128*string+column+y] = pgm_read_byte(&font[5*(sign-32)+y]);
	scrBuff[128*string+column+5]=0x00; //space between chars
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void print_string(uint8_t column, uint8_t string, char *str) //0..120 //0..7  //print a string
{
while(*str) //write
	{
	print_char(column, string, *str++);
	column+=6;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void draw_pixel(uint8_t hPos, uint8_t vPos) //0..127 //0..63 //ON pixel
{
if(hPos<=127 && vPos<=63) SET_BIT(scrBuff[hPos+128*(vPos/8)], vPos%8);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//draw any line, source:  http://www.edaboard.com/thread68526.html#post302856
///////////////////////////////////////////////////////////////////////////////////////////////////
void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) //x - 0..127  //y - 0..63
{
int16_t P;
int8_t x, y, addx, addy, dx, dy;

x = x1;
y = y1;

dx = abs((int8_t)(x2-x1));
dy = abs((int8_t)(y2-y1));

if(x1>x2) addx = -1;
else addx = 1;

if(y1>y2) addy = -1;
else addy = 1;

if(dx>=dy)
	{
	P = 2*dy-dx;

	for(uint8_t i=0; i<=dx; ++i)
		{
		draw_pixel(x, y);

		if(P<0)
			{
			P += 2*dy;
			x += addx;
			}
		else
			{
			P += 2*dy-2*dx;
			x += addx;
			y += addy;
			}
		}
	}
else
	{
	P = 2*dx-dy;

	for(uint8_t i=0; i<=dy; ++i)
		{
		draw_pixel(x, y);

		if(P<0)
			{
			P += 2*dx;
			y += addy;
			}
		else
			{
			P += 2*dx-2*dy;
			x += addx;
			y += addy;
			}
		}
	}
}
