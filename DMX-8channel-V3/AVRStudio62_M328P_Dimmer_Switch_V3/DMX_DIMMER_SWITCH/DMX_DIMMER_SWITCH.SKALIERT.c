/*------------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        19.04.2017
 Description:    DMX_8 Kanal_Dimmer
------------------------------------------------------------------------------*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#define F_CPU 12000000

#define LED_OUT		DDRD |= (1<<PD5);
#define LED_TOGGLE	PORTD ^= (1<<PD5);

#define DMX_BAUD 250000
#define DMX_LOST_TIMEOUT 8000

// map table with 255 values. Three times the same value scaled down at the end
//uint8_t valMap[] = {90,89,89,88,88,87,87,...7,7,6,6,5,5,4,4,3,3,2,2,1,0,0};


// see https://www.ulrichradig.de/forum/viewtopic.php?f=53&t=2656
unsigned char valMap[256] = {
90,90,89,89,89,88,88,88,87,87,86,86,86,85,85,85,84,
84,84,83,83,83,82,82,82,81,81,80,80,80,79,79,79,78,
78,78,77,77,77,76,76,76,75,75,74,74,74,73,73,73,72,
72,72,71,71,71,70,70,70,69,69,68,68,68,67,67,67,66,
66,66,65,65,65,64,64,64,63,63,62,62,62,61,61,61,60,
60,60,59,59,59,58,58,58,57,57,56,56,56,55,55,55,54,
54,54,53,53,53,52,52,52,51,51,50,50,50,49,49,49,48,
48,48,47,47,47,46,46,46,45,45,44,44,44,43,43,43,42,
42,42,41,41,41,40,40,40,39,39,38,38,38,37,37,37,36,
36,36,35,35,35,34,34,34,33,33,32,32,32,31,31,31,30,
30,30,29,29,29,28,28,28,27,27,26,26,26,25,25,25,24,
24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,
18,18,17,17,17,16,16,16,15,15,14,14,14,13,13,13,12,
12,12,11,11,11,10,10,10,9,9,8,8,8,7,7,7,6,6,6,5,5,5,4,
4,4,3,3,2,2,2,1,1,1,0,0};


volatile unsigned char dmx_buffer[513];
volatile unsigned int dmx_lost = DMX_LOST_TIMEOUT;

volatile unsigned int dmx_adresse = 0;
volatile unsigned char phase_on_count = 0;

volatile unsigned char val[8];

#define SPI_DDR                 DDRB
#define SPI_PORT                PORTB
#define SPI_SCK                 5
#define SPI_MOSI                3
#define CS_595                  2

#define CS_595_HI()				PORTB &= ~(1<<CS_595)
#define CS_595_LO()				PORTB |= (1<<CS_595)

//############################################################################
//Empfangsroutine für DMX
ISR (USART_RX_vect){
	static unsigned int dmx_channel_rx_count = 0;
	static unsigned char dmx_valid = 0;
	unsigned char tmp = 0;
	
	tmp =  UDR0;
	
	if(UCSR0A&(1<<FE0)){
		if(dmx_channel_rx_count > 1){
			dmx_lost = 0;
		}
		dmx_channel_rx_count = 0;	
		dmx_buffer[0] = tmp;
		if(dmx_buffer[0] == 0){
			dmx_valid = 1;
			dmx_channel_rx_count++;
		}else{
			dmx_valid = 0;
		}
		return;
	}
	
	if(dmx_valid){
		dmx_buffer[dmx_channel_rx_count] = tmp;	
		if(dmx_channel_rx_count < 513)
		{
			dmx_channel_rx_count++;
		}
		return;
	}
}

//############################################################################
//Porterweiterung OUTPUT 1 - 8 via SPI-BUS
void out_ext (unsigned char value){
	CS_595_LO();
	SPDR = value;
	while( !(SPSR & (1<<SPIF)) ) ;
	CS_595_HI();
}

//############################################################################
//
static inline void spi_init(void){
	// configure pins MOSI, SCK and CS595 as output
	SPI_DDR |= (1<<SPI_MOSI) | (1<<SPI_SCK) | (1<<CS_595);
	// pull SCK high
	SPI_PORT |= (1<<SPI_SCK);
	//SPI: enable, master, positive clock phase, msb first, SPI speed fosc/2
	SPCR = (1<<SPE) | (1<<MSTR);
	SPSR = (1<<SPI2X);
	
	out_ext(0x00);
}

//############################################################################
//Hier wird die Zeit gezählt (Tick 100µs)
ISR (TIMER2_COMPA_vect)
{
	unsigned char out = 0;
	unsigned char tmp = 0;
	phase_on_count++;
	
	if(phase_on_count > 240){
		phase_on_count = 0;
	}
	
	for(tmp = 0;tmp <8;tmp++){
		if(phase_on_count>val[tmp]) out |= (1<<tmp);
	}
	out_ext(out);
	
	if(dmx_lost<DMX_LOST_TIMEOUT){
		dmx_lost++;
	}
}

//############################################################################
//ISR vom externen Interrupt (ICP)
ISR (PCINT0_vect){
	TCNT2 = 0;
	phase_on_count = 0;
}

//############################################################################
//Hauptprogramm
int main (void) 
{  
	unsigned int dmx_adresse_tmp = 0;
	unsigned char tmp = 0;
	unsigned int live_counter = 0;

	LED_OUT;
	spi_init();
		
	//PIN CHANGE INTERRUPT ON PHASE SYNC
	PCICR |= (1<<PCIE0);
	PCMSK0 |= (1<<PCINT0);

	//Init usart DMX-BUS
	UBRR0   = (F_CPU / (DMX_BAUD * 16L) - 1);
	UCSR0B|=(1 << RXEN0 | 1<< RXCIE0);
	UCSR0C|=(1<<USBS0); //USBS0 2 Stop bits	
	sei();//Globale Interrupts Enable
	
	//Switch 75176 to INPUT RXD
	DDRD |= (1<<PD2);
	PORTD &= ~(1<<PD2);
	
	//Timer für die Phasenanschnittberechnung
	TCCR2A |= (1<<WGM21);
	TCCR2B |= (1<<CS21|1<<CS20); // :32
	TIMSK2 |= (1<<OCIE2A);

	// :32 Teiler
	// :2 Clock Source 
	// :50 Hz
	// :92 - 1 Stepping
	
	OCR2A = (F_CPU/32/2/50/240) - 1;
	
	//Pullup for DIP Switch
	PORTD |= (1<<PD3)|(1<<PD4)|(1<<PD7);
	PORTC |= (1<<PC5)|(1<<PC4)|(1<<PC3)|(1<<PC2)|(1<<PC1)|(1<<PC0);
	PORTB |= (1<<PB1);
	
	//Endlosschleife
	while(1){
		//Read DMX Address from Switch
		dmx_adresse_tmp = 0;
		if(!(PIND&(1<<PD4))) dmx_adresse_tmp |= 0x01;
 		if(!(PIND&(1<<PD3))) dmx_adresse_tmp |= 0x02;
		if(!(PINC&(1<<PC5))) dmx_adresse_tmp |= 0x04;
		if(!(PINC&(1<<PC4))) dmx_adresse_tmp |= 0x08;
		if(!(PINC&(1<<PC3))) dmx_adresse_tmp |= 0x10;
		if(!(PINC&(1<<PC2))) dmx_adresse_tmp |= 0x20;
		if(!(PINC&(1<<PC1))) dmx_adresse_tmp |= 0x40;
		if(!(PINC&(1<<PC0))) dmx_adresse_tmp |= 0x80;
		if(!(PINB&(1<<PB1))) dmx_adresse_tmp |= 0x0100;
		
		if(dmx_adresse_tmp == 0) dmx_adresse_tmp = 1;  
		if(dmx_adresse_tmp > 505) dmx_adresse_tmp = 505;
		if(dmx_adresse != dmx_adresse_tmp) dmx_adresse =  dmx_adresse_tmp;	
		
		//new calculation of val
		if(PIND&(1<<PD7)){
			for(tmp = 0;tmp <8;tmp++){
				val[tmp] = valMap[dmx_buffer[dmx_adresse+tmp]];
				//val[tmp] = (255 - dmx_buffer[dmx_adresse+tmp]);
			}
		}else{
			for(tmp = 0;tmp <8;tmp++){
				if(dmx_buffer[dmx_adresse+tmp]>128){
					val[tmp] = 0;
				}
				else{
					val[tmp] = 255;
				}
			}
		}
		
		if(dmx_lost==DMX_LOST_TIMEOUT){
			for(tmp = 0;tmp <8;tmp++){
				dmx_buffer[dmx_adresse + tmp] = 0;
			}
		}
		
		if((live_counter++) > 10000) {
			LED_TOGGLE;
			live_counter = 0;
		}
		
	}
}
