/*
 * TWI.h
 *
 * Created: 2014-11-06 23:40:48
 *  Author: Mateusz
 */ 

#include <avr/io.h>

void i2cSetBitrate(uint16_t bitrateKHz);	// Ustalenie prêdkoœci transmisji
uint8_t TWI_start(void);	// Wys³anie sekwencji START
void TWI_stop(void);	// Wys³anie sekwencji STOP
void TWI_write(uint8_t bajt);	// Zapis bajtu danych
uint8_t TWI_read(uint8_t ack);	// Odczyt bajtu danych
void TWI_write_buf( uint8_t SLA,	// Wys³anie ramki danych  
	uint8_t adr, uint8_t len, uint8_t *buf );
void TWI_read_buf(uint8_t SLA,	// Odczyt ramki danych
	 uint8_t adr, uint8_t len, uint8_t *buf);



