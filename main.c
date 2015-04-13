// testing the display library (draw the waveform from input ADC0)
//
// ATmega328P 16MHz, 128x64 monochrome display (SSD1306)
// AVR Toolchain, Eclipse IDE, USBasp
//
// 13-Apr-2015


#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "display.h"


uint16_t capture[128]; //capt. buffer

char strBuff[22]; //string buffer


int main(void)
{
ACSR=0b10000000; //analog comparator disable
ADMUX=0b01000000; //AVcc as voltage reference //select channel ADC0
ADCSRA=0b10000111; //prescaler 16M/128=125k

display_init();
screen_clear();
screen_update();

while(1)
	{
	for(uint8_t i=0; i<128; i++) //128 samples of the input voltage
		{
		SET_BIT(ADCSRA,ADSC); //start ADC
		while(bit_is_clear(ADCSRA,ADIF)); //wait the end of ADC
		capture[i]=ADC; //write the obtained value in buffer
		_delay_us(120); //sync
		}

	screen_clear();

	for(uint8_t i=0; i<=126; i++) draw_line(i, 63-capture[i]/16, i+1, 63-capture[i+1]/16); // draw waveform

	print_string(0,0,"ADC0");

	screen_update();
	}

}
