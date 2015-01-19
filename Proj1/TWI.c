/*
 * TWI1.c
 *
 * Created: 2014-11-06 23:40:41
 *  Author: Mateusz
 */ 
#define F_CPU 8000000UL
#define ACK 1
#define NACK 0

#include "TWI.h"
#include <util/delay.h>


char text_buf[10];
uint8_t iter = 0;
uint8_t err_flag = 0;

void wait(void)
{
	for (int i=0;i<200;i++);
}

void i2cSetBitrate(uint16_t bitrateKHz) {
	uint8_t bitrate_div;

	bitrate_div = ((F_CPU/1000l)/bitrateKHz);
	if(bitrate_div >= 16)
	bitrate_div = (bitrate_div-16)/2;

	TWBR = bitrate_div;
}

uint8_t TWI_start(void) 
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA);	// Konfiguracja rejestrów steruj¹cych
	for (iter=0;iter<200;iter++)
	{
		err_flag = (!(TWCR & (1 << TWINT)));// Oczekiwanie na odpowiedŸ urz¹dzenia 
		if (err_flag == 0)					// Slave. W przypadku braku odpowiedzi
		{									// flaga b³êdu zostaje ustawiona
			return 0;						// a nastêpnie funckja koñczy dzia³anie
		}									// i zwraca wartoœæ 1. W przeciwnym razie 
	}										// funkcja zwraca 0 a flaga b³êdu nie jest 
											// ustawiana.
		err_flag = !(TWCR & (1 << TWINT));		
		return 1;
}

void TWI_stop(void) 
{
	if (err_flag == 0)	// Je¿eli nie wyst¹pi³ b³¹d
	{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);	// Sekwencja STOP
	while ( (TWCR&(1<<TWSTO)) );
	}
}

void TWI_write(uint8_t bajt)
{
	if (err_flag == 0)	// Je¿eli nie wyst¹pi³ b³¹d
	{
		TWDR = bajt;
		TWCR = (1<<TWINT)|(1<<TWEN);
		while ( !(TWCR&(1<<TWINT)));
	}
}

uint8_t TWI_read(uint8_t ack) 
{
	if (err_flag==0)	// Je¿eli nie wyst¹pi³ b³¹d
	{
	TWCR = (1<<TWINT)|(ack<<TWEA)|(1<<TWEN);
	while ( !(TWCR & (1<<TWINT)));
	return TWDR;
	}
	else
	{
		return 0;
	}
}

void TWI_write_buf( uint8_t SLA, uint8_t adr, uint8_t len, uint8_t *buf ) {

	TWI_start();
	TWI_write(SLA);
	TWI_write(adr);
	while (len--) TWI_write(*buf++);
	TWI_stop();

}

void TWI_read_buf(uint8_t SLA, uint8_t adr, uint8_t len, uint8_t *buf) 
{
	TWI_start();
	TWI_write(SLA);
	TWI_write(adr);
	TWI_start();
	TWI_write(SLA + 1);
	while (len--) *buf++ = TWI_read( len ? ACK : NACK );
	TWI_stop();
}
