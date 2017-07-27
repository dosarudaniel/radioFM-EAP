/*
 * TWI.c
 *
 * Created: 7/20/2017 6:55:57 PM
 *  Author: Daniel
 */ 
 #include "TWI.h"
 #include <inttypes.h>
 #include <avr/io.h>
 #define TIMEOUT 10000

 void TWIInit(void)
 {
	 //set SCL to 400kHz
	 TWSR = 0x00;
	 TWBR = 0x0F;
	 //enable TWI
	 TWCR = (1<<TWEN);
 }


 void TWIStart(void)
 {
	 TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	 uint16_t a = TIMEOUT;
	 while (((TWCR & (1<<TWINT)) == 0) && a--);
 }


 //send stop signal

 void TWIStop(void)
 {
	 TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
 }

void TWIWrite(uint8_t u8data)
{
	TWDR = u8data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	uint16_t a = TIMEOUT;
	while (((TWCR & (1<<TWINT)) == 0) && a--);
}



 void TWIWriteACK(uint8_t u8data)
 {
	 TWDR = u8data;
	 TWCR = (1<<TWINT)|(1<<TWEN) | (1<<TWEA);
	 uint16_t a = TIMEOUT;
	 while (((TWCR & (1<<TWINT)) == 0) && a--);
 }


 uint8_t TWIReadACK(void)
 {
	 TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	 uint16_t a = TIMEOUT;
	 while (((TWCR & (1<<TWINT)) == 0) && a--);
	 return TWDR;
 }
 //read byte with NACK
uint8_t TWIReadNACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	uint16_t a = TIMEOUT;
	while (((TWCR & (1<<TWINT)) == 0) && a--);
	return TWDR;
}


 uint8_t TWIGetStatus(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}