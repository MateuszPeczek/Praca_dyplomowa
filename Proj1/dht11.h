/*
DHT11 Library 0x01

copyright (c) Davide Gironi, 2011

Released under GPLv3.
Please refer to LICENSE file for licensing information.

References:
  - DHT-11 Library, by Charalampos Andrianakis on 18/12/11

*/


#ifndef DHT11_H_
#define DHT11_H_

#include <stdio.h>
#include <avr/io.h>

//setup parameters

#define DHT11_DDR DDRB
#define DHT11_PORT PORTB
#define DHT11_PIN PINB
#define DHT11_INPUTPIN PB1

extern int8_t dht11_gettemperature();	// odczyt temperatury
extern int8_t dht11_gethumidity();		// odczyt wilgotnoœci

#endif
