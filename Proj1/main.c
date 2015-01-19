/*
 * main.c
 *
 * Created: 2014-10-31 18:59:04
 *  Author: Mateusz Pêczek
 */
#define F_CPU 8000000UL

#define ITEM_CHANGE (1<<PD4)
#define ITEM_CHANGE_PRESSED !(PIND & ITEM_CHANGE)

#define VAL_PLUS (1<<PD0)
#define VAL_PLUS_PRESSED !(PIND & VAL_PLUS)

#define VAL_MINUS (1<<PD1)
#define VAL_MINUS_PRESSED !(PIND & VAL_MINUS)

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "lcd.h"
#include "dht11.h"
#include "TWI.h"
#include "BMP180.h"

void dataWriteDHT(void);
void dataWriteBMP(void);
void readData(void);
void dispMenu(void);
void read_eeprom(void);
void save_eeprom(void);
void hello(void);

int8_t tempDHT = 0;
int8_t humDHT = 0;
char text_buf[100];
long tempBMP = 0;
long pressBMP = 0;
uint8_t BMP180_state = 0;
extern uint8_t err_flag;

uint16_t EEMEM tempDHT_eeprom = 0;
uint16_t EEMEM humDHT_eeprom = 0;
uint16_t EEMEM tempBMP_eeprom = 0;
uint16_t EEMEM pressDHT_eeprom = 0;

int8_t tempDHT_reg = 0;
int8_t humDHT_reg = 0;
int8_t tempBMP_reg = 0;
int8_t pressBMP_reg = 0;

int main(void)
{
	LCD_Initalize();
	i2cSetBitrate(100);
	BMP180_state = BMP180_init();
	read_eeprom();
	DDRD &= ~(1<<0 | 1<<1 | 1<<2 | 1<<4);

	PORTD |= (1<<0) | (1<<1) | (1<<2) | (1<<4);

	MCUCR |= (1<<ISC01);	// Konfiguracja sposobu inicjalizacji przerwania
	GICR |= (1<<INT0);		// W³¹czenie przerwania INT0
	sei();					// Globalne w³¹czenie przerwañ

	hello();

    while(1)
    {
		readData();		//odczyt danych
		LCD_Clear();	//wyczyszczenie wyœwietlacza
		dataWriteDHT();	//wypisanie danych DHT11
		_delay_ms(4000);//oczekiwanie
		LCD_Clear();	//wyczyszczenie wyœwietlacza
		dataWriteBMP();	//wypisanie danych BMP180
		_delay_ms(4000);//oczekiwanie
    }
}

void readData(void)
{
	tempDHT = dht11_gettemperature();	//odczyt temperatury DHT11
	humDHT = dht11_gethumidity();		//odczyt wilgotnoœci DHT11
	if (BMP180_state == 0)				//sprawdzenie po³¹czenia z BMP180
	{
		tempBMP = BMP180_gett();		//odczyt temperatury BMP180
		pressBMP = BMP180_getp();		//odczyt ciœnienia BMP180
	}
}

void dataWriteDHT(void)
{
	if ((tempDHT == -1) && (humDHT == -1)) //sprawdzenie czy
	{									  //modu³ dzia³a poprawnie
		LCD_GoTo(0,0);
		LCD_WriteText("**** DHT11 *****");
		LCD_GoTo(0,1);
		LCD_WriteText("**** ERROR *****");
	}
	else
	{
		tempDHT = tempDHT + tempDHT_reg;
		humDHT = humDHT + humDHT_reg;
		LCD_GoTo(0,0);
		LCD_WriteText("Temp:");
		LCD_GoTo(6,0);
		sprintf(text_buf, "%d", tempDHT);
		LCD_WriteText(text_buf);
		LCD_GoTo(9,0);
		LCD_WriteText("C");
		LCD_GoTo(0,1);
		LCD_WriteText("Hum:");
		LCD_GoTo(6,1);
		sprintf(text_buf, "%d", humDHT);
		LCD_WriteText(text_buf);
		LCD_GoTo(9,1);
		LCD_WriteText("%");
	}
}

void dataWriteBMP(void)
{
	if (BMP180_state == 0)  //sprawdzenie czy
	{					   //modu³ dzia³a poprawnie
		LCD_GoTo(0,0);
		LCD_WriteText("Temp:");
		LCD_GoTo(6,0);
		sprintf(text_buf, "%ld", tempBMP/10 + tempBMP_reg);
		LCD_WriteText(text_buf);
		LCD_GoTo(8,0);
		LCD_WriteText(".");
		LCD_GoTo(9,0);
		sprintf(text_buf, "%ld", tempBMP%10);
		LCD_WriteText(text_buf);
		LCD_GoTo(11,0);
		LCD_WriteText("C");
		LCD_GoTo(0,1);
		LCD_WriteText("Pres:");
		if ((pressBMP/100 + pressBMP_reg) > 999)
		{
			LCD_GoTo(5,1);
		}
		else
		{
			LCD_GoTo(6,1);
		}
		sprintf(text_buf, "%ld", pressBMP/100 + pressBMP_reg);
		LCD_WriteText(text_buf);
		LCD_GoTo(9,1);
		LCD_WriteText(".");
		if (pressBMP%100<10) LCD_WriteText("0");
		LCD_GoTo(10,1);
		sprintf(text_buf, "%ld", tempBMP%100);
		LCD_WriteText(text_buf);
		LCD_GoTo(13,1);
		LCD_WriteText("hPa");
	}
	else
	{
		LCD_GoTo(0,0);
		LCD_WriteText("**** BMP180 *****");
		LCD_GoTo(0,1);
		LCD_WriteText("**** ERROR *****");
	}
}

ISR(INT0_vect)
{
	uint8_t item = 0;	// Aktualna pozycja menu

	LCD_Clear();
	LCD_GoTo(6,0);
	LCD_WriteText("*");	// Oznaczenie aktualnej pozycji do edycji
	for (int i=0;i<1000;i++)
	{
		dispMenu();		// Wypisanie danych na ekranie
		if (ITEM_CHANGE_PRESSED)	// Wciœniêcie przycisku zmiany pozycji w menu
		{
			_delay_ms(80);	// Filtrowanie drgania styków
			if (ITEM_CHANGE_PRESSED) // Ponowne sprawdzenie wciœniêcia przycisku
			{
				i=0;	// Wyzerowanie czasu spêdzonego w menu
				item = item + 1;	// Przejœcie do nastêpnej zmiennej do edycji
				if (item > 4)		// Powrót je¿eli aktualna 
				item = 0;			// pozycja to pozycja czwarta
				switch(item)
				{
					case 0:
					{
						LCD_GoTo(6,0);
						LCD_WriteText("*");
						LCD_GoTo(15,0);
						LCD_WriteText(" ");		// Oznaczenie aktualnie
						LCD_GoTo(6,1);			// edytowanej wartoœci
						LCD_WriteText(" ");
						LCD_GoTo(15,1);
						LCD_WriteText(" ");
						break;
					}
					case 1:
					{
						LCD_GoTo(6,0);
						LCD_WriteText(" ");
						LCD_GoTo(15,0);
						LCD_WriteText("*");		// Oznaczenie aktualnie
						LCD_GoTo(6,1);			// edytowanej wartoœci
						LCD_WriteText(" ");
						LCD_GoTo(15,1);
						LCD_WriteText(" ");
						break;
					}
					case 2:
					{
						LCD_GoTo(6,0);
						LCD_WriteText(" ");
						LCD_GoTo(15,0);
						LCD_WriteText(" ");		// Oznaczenie aktualnie
						LCD_GoTo(6,1);			// edytowanej wartoœci
						LCD_WriteText("*");
						LCD_GoTo(15,1);
						LCD_WriteText(" ");
						break;
					}
					case 3:
					{
						LCD_GoTo(6,0);
						LCD_WriteText(" ");
						LCD_GoTo(15,0);
						LCD_WriteText(" ");		// Oznaczenie aktualnie
						LCD_GoTo(6,1);			// edytowanej wartoœci
						LCD_WriteText(" ");
						LCD_GoTo(15,1);
						LCD_WriteText("*");
						break;
					}
				}
				_delay_ms(100);
			}
		}
		if (VAL_MINUS_PRESSED)	// Sprawdzenie wciœniêcia przycisku
		{						// zmniejszenia wartoœci 
			_delay_ms(80);		// Filtrowawnie drgania styków
			if (VAL_MINUS_PRESSED)	// Ponowne sprawdzenie przycisku
			{
				switch(item)
				{
					case 0:
					{
						tempDHT_reg = tempDHT_reg - 1;	// Zmniejszenie bufora edycji
						dispMenu();						// Wyœwietlenie zaktualizowanego menu
						LCD_GoTo(6,0);
						LCD_WriteText("*");
						_delay_ms(200);					// Oczekiwanie na mo¿liwoœæ kolejnej zmiany
						break;
					}
					case 1:
					{
						humDHT_reg = humDHT_reg - 1;
						dispMenu();
						LCD_GoTo(15,0);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
					case 2:
					{
						tempBMP_reg = tempBMP_reg - 1;
						dispMenu();
						LCD_GoTo(6,1);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
					case 3:
					{
						pressBMP_reg = pressBMP_reg - 1;
						LCD_GoTo(15,1);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
				}
				i=0;	// Wyzerowanie czasu spêdzonego w menu
			}
		}
		if (VAL_PLUS_PRESSED)
		{
			_delay_ms(80);
			if (VAL_PLUS_PRESSED)
			{

				switch(item)
				{
					case 0:
					{
						tempDHT_reg = tempDHT_reg + 1;
						dispMenu();
						LCD_GoTo(6,0);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
					case 1:
					{
						humDHT_reg = humDHT_reg + 1;
						dispMenu();
						LCD_GoTo(15,0);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
					case 2:
					{
						tempBMP_reg = tempBMP_reg + 1;
						dispMenu();
						LCD_GoTo(6,1);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
					case 3:
					{
						pressBMP_reg = pressBMP_reg + 1;
						LCD_GoTo(15,1);
						LCD_WriteText("*");
						_delay_ms(200);
						break;
					}
				}
				i=0;
			}
		}

		if (ITEM_CHANGE_PRESSED | VAL_MINUS_PRESSED | VAL_PLUS_PRESSED) // Je¿eli naciœniêto przycisk
		i=0;															// wyzeruj czas spêdzony w menu
	}
	save_eeprom();	// Wywo³anie zapisu zmian 
}

void dispMenu(void)
{
	LCD_GoTo(0,0);			// Ustawienie pozycji kursora
	LCD_WriteText("TD:");	// Wypisanie tekstu TD:
	LCD_GoTo(3,0);			// Ustawienie pozycji kursora
	sprintf(text_buf, "%02d", tempDHT_reg);	// Konwersja zmiennej tempDHT_reg na typ string 
	LCD_WriteText(text_buf);				// i wypisanie jej na wyœwietlacz
	LCD_GoTo(9,0);
	LCD_WriteText("HD:");
	LCD_GoTo(12,0);
	sprintf(text_buf, "%02d", humDHT_reg);
	LCD_WriteText(text_buf);
	LCD_GoTo(0,1);
	LCD_WriteText("TB:");
	LCD_GoTo(3,1);
	sprintf(text_buf, "%02d", tempBMP_reg);
	LCD_WriteText(text_buf);
	LCD_GoTo(9,1);
	LCD_WriteText("PB:");
	LCD_GoTo(12,1);
	sprintf(text_buf, "%02d", pressBMP_reg);
	LCD_WriteText(text_buf);
}

inline void read_eeprom()
{
	tempDHT_reg = eeprom_read_word(&tempDHT_eeprom);
	eeprom_busy_wait();

	humDHT_reg = eeprom_read_word(&humDHT_eeprom);
	eeprom_busy_wait();

	eeprom_read_block((void*)&tempBMP_eeprom, (const void*)&tempBMP_reg, sizeof(long));
	eeprom_busy_wait();

	eeprom_read_block((void*)&pressBMP_reg, (const void*)&pressDHT_eeprom, sizeof(long));
	eeprom_busy_wait();
}

void save_eeprom()
{
	LCD_Clear();	
	LCD_GoTo(1,0);
	LCD_WriteText("Saving changes");
	eeprom_write_word(&tempDHT_eeprom, (uint16_t)tempDHT_reg);	// Zapis zmiennej bufora edycji do zmiennej w EEPROM
	eeprom_busy_wait();		// Oczekiwanie na koniec zapisu

	eeprom_write_word(&humDHT_eeprom, (uint16_t)humDHT_reg);
	eeprom_busy_wait();

	eeprom_write_block((const void*)&tempBMP_reg, (void*)&tempBMP_eeprom, sizeof(long));
	eeprom_busy_wait();

	eeprom_write_block((const void*)&pressBMP_reg, (void*)&pressDHT_eeprom, sizeof(long));
	eeprom_busy_wait();
}

void hello()
{
	LCD_GoTo(0,0);
	LCD_WriteText("STACJA POGODOWA");
	_delay_ms(2500);
}
