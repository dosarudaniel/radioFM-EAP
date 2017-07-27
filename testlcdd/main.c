#define F_CPU 16000000UL
#include "lcd.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "TWI.h"
#include <stdio.h>
#include <string.h>

// char text[16];

void decodeRdsMessage(uint16_t RDSA, uint16_t RDSB, uint16_t RDSC, uint16_t RDSD, char stationName[]);
char rdsStationName[9];
uint16_t adc_result0 = 0; // reglaj frecventa (87 -> 108)
uint16_t adc_result1 = 0; // reglaj frecventa fin (87.0 - > 87.9)
uint16_t adc_result2 = 0; // reglaj volum
uint8_t volume = 8, old_volume = 8;
volatile uint8_t data[26];

void radio_write_reg(uint8_t reg, uint8_t high, uint8_t low){
	uint8_t i;
	TWIStart();
	TWIWrite(0x22);
	TWIWrite(reg);
	TWIWrite(high);
	TWIWrite(low);
	TWIStop();
}
void radio_read(){
	uint8_t i;
	TWIStart();
	TWIWrite(0x22);
	TWIWrite(0xC);
	TWIStop();

	TWIStart();
	TWIWrite(0x23);
	for (i=0; i<7; i++) data [i] = TWIReadACK();
	data[7] = TWIReadNACK();
	TWIStop();
}

void InitADC()
{
	ADMUX=(1<<REFS0);             // For Aref=AVcc;
	ADCSRA=(1<<ADEN)|(1<<ADPS2); //Rrescalar div factor = 16
}

uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel ch must be 0-7
	ch=ch&0b00000111;
	ADMUX = (ADMUX & 0b11110000) | ch;
	
	//Start Single conversion
	ADCSRA|=(1<<ADSC);
	
	//Wait for conversion to complete
	while(!(ADCSRA & (1<<ADIF)));

	//Clear ADIF by writing one to it
	//Note you may be wondering why we have write one to clear it
	//This is standard way of clearing bits in io as said in datasheets.
	//The code writes '1' but it result in setting bit to '0' !!!
	ADCSRA|=(1<<ADIF);
	return(ADC);
}

void Wait()
{
	uint8_t i;
	for(i=0;i<10;i++)
		_delay_ms(5); // It needs time to read ADC (frequency, volume)
}

int main(){
	 DDRB = 0xFF;	// Enable output for the LED

	 lcd_init(LCD_DISP_ON);
	 InitADC();

	 TWIInit();
	 radio_write_reg(2, 0xc0, 0x03);
	 radio_write_reg(2, 0xc0, 0x0d);
	 
	 uint16_t old_freq = 0, freq = 921;
	 uint16_t freq_copy_for_show = freq;
	 
	 freq = freq - 870;
	 uint8_t freqH = freq >> 2;
	 uint8_t freqL = (freq & 3) << 6;

	 radio_write_reg(3, freqH, freqL + 0x10);
	 char output[16];
	 sprintf(output, "%d.%d Mhz", freq_copy_for_show/10,
							freq_copy_for_show % 10 * 10);
	 lcd_clrscr();
	 lcd_home();
	 lcd_puts(output);
	 
	 radio_write_reg(5, 0x88, 0xdF);
	 char precision_freq = 0;
	 char stationName[9] = "";
	 char stableStationName[9] = "";
	 
	 while(1) {
		 radio_read();
		 if ((data[2] & (15 << 4)) == 0) {
			 int offset = (data[3] & 3)*2;
			 if(stationName[offset] == data[6]) {
				 stableStationName[offset] = data[6];
			 } else {
				 stationName[offset] = data[6];
			 }
			 
			 if(stationName[offset+1] == data[7]) {
				 stableStationName[offset+1] = data[7];
			 } else {
				 stationName[offset+1] = data[7];
			 }
			 
			 int areLetters = 1;
			 for (int i = 0; i < 8; i++) {
				 if((stableStationName[i] < 'A' || stableStationName[i] > 'Z') && stableStationName[i] != ' ') {
					areLetters = 0;
				 }
			 }
			 
			 if (areLetters == 1) {
				 lcd_gotoxy(0, 1);
				 lcd_puts(stableStationName);
			 }
		}
		adc_result0=ReadADC(0);
		Wait();
		
		adc_result0 = ReadADC(0);
		freq = 870;
		freq += adc_result0  * 21 / 1023*10;
		
		adc_result1=ReadADC(1);
		Wait();
		adc_result1=ReadADC(1);
		
		freq += adc_result1  * 10 / 1023;
		
		if (old_freq != freq) {
			TWCR=0;
			TWIInit();
			lcd_gotoxy(0,1);
			lcd_puts("        ");					// put 8 spaces, clear station name
			freq_copy_for_show = freq;
			old_freq = freq;
			freq = freq - 870;
			uint8_t freqH = freq >> 2;
			uint8_t freqL = (freq & 3) << 6;
			radio_write_reg(3, freqH, freqL + 0x10);
		} 
		
		adc_result2=ReadADC(2);
		Wait();
		adc_result2=ReadADC(2);
		volume = adc_result2 * 15 / 1023;			// volume has 16 steps
		
		if (old_volume != volume) {
			TWCR=0;
			TWIInit();
			old_volume = volume;
			volume = volume | 0xd0; 
			radio_write_reg(5, 0x88, volume);
		}
		
		char volume_text[3]={0};
		sprintf(output, "%d.%d Mhz", freq_copy_for_show/10, freq_copy_for_show%10*10);
		lcd_gotoxy(0, 0);
		lcd_puts(output);
		lcd_gotoxy(10,1);
		if(old_volume>=0 && old_volume<=9){
			sprintf(volume_text, "Vol: %d", old_volume);
			lcd_puts(volume_text);
			Wait();
		} else {
			sprintf(volume_text, "Vol:%d", old_volume);
			lcd_puts(volume_text);
			Wait();
		}
	 }
	 
	 return 0;
 }

