/*------------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        18.12.2011
 Description:    DMX_3Kanal_Dimmer


 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#define F_CPU 12000000

//Relais or SSR SSR = 1 Relais = 0
#define SSR_RELAIS1		1
#define SSR_RELAIS2		1
#define SSR_RELAIS3		1

#define SSR1_PIN_OUT DDRD |= (1<<PD7);
#define SSR2_PIN_OUT DDRD |= (1<<PD4);
#define SSR3_PIN_OUT DDRD |= (1<<PD2);

#define SSR1_ON		PORTD |= (1<<PD7);
#define SSR1_OFF	PORTD &= ~(1<<PD7);
#define SSR2_ON		PORTD |= (1<<PD4);
#define SSR2_OFF	PORTD &= ~(1<<PD4);
#define SSR3_ON		PORTD |= (1<<PD2);
#define SSR3_OFF	PORTD &= ~(1<<PD2);


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

volatile unsigned char dmx_buffer[514];
volatile unsigned int dmx_lost = DMX_LOST_TIMEOUT;

volatile unsigned int dmx_adresse = 0;
volatile unsigned char phase_off_count = 0;

volatile unsigned char val1;
volatile unsigned char val2;
volatile unsigned char val3;

//############################################################################
//Empfangsroutine für DMX
ISR (USART_RX_vect)
//############################################################################
{
	static unsigned int dmx_channel_rx_count = 0;
	static unsigned char dmx_valid = 0;
	unsigned char tmp = 0;
	
	tmp =  UDR0;
	
	if(UCSR0A&(1<<FE0))
	{
		if(dmx_channel_rx_count > 1)
		{
			dmx_lost = 0;
		}
		dmx_channel_rx_count = 0;	
		dmx_buffer[0] = tmp;
		if(dmx_buffer[0] == 0)
		{
			dmx_valid = 1;
			dmx_channel_rx_count++;
		}
		else
		{
			dmx_valid = 0;
		}
		return;
	}
	
	if(dmx_valid)
	{
		dmx_buffer[dmx_channel_rx_count] = tmp;	
		if(dmx_channel_rx_count < 514)
		{
			dmx_channel_rx_count++;
		}
		return;
	}
}

//############################################################################
//Hier wird die Zeit gezählt (Tick 100µs)
ISR (TIMER2_COMPA_vect)
//############################################################################
{
	phase_off_count++;
	
	if(phase_off_count > 90) phase_off_count = 0;
	
	#if SSR_RELAIS1
		if(phase_off_count>val1)
		{
			SSR1_ON
		}
		else
		{
			SSR1_OFF
		}
	#endif
	
	#if SSR_RELAIS2
		if(phase_off_count>val2)
		{
			SSR2_ON
		}
		else
		{
			SSR2_OFF
		}
	#endif
	
	#if SSR_RELAIS3
		if(phase_off_count>val3)
		{
			SSR3_ON
		}
		else
		{
			SSR3_OFF
		}
	#endif

	if(dmx_lost<DMX_LOST_TIMEOUT)
	{
		dmx_lost++;
	}
}

//############################################################################
//ISR vom externen Interrupt (Zero cross detection)
ISR (INT1_vect)
//############################################################################
{
	TCNT2 = 0;
	phase_off_count = 0;
}

//############################################################################
//Hauptprogramm
int main (void) 
//############################################################################
{  
	unsigned int dmx_adresse_tmp;
	
	//INT1 ON PHASE SYNC
	EICRA |= (1<<ISC11)|(1<<ISC10); //The rising edge of INT1 generates an interrupt request
	EIMSK |= (1<<INT1); //External interrupt request 1 enable

	//Init usart DMX-BUS
	UBRR0   = (F_CPU / (DMX_BAUD * 16L) - 1);
	UCSR0B|=(1 << RXEN0 | 1<< RXCIE0);
	UCSR0C|=(1<<USBS0); //USBS0 2 Stop bits	
	sei();//Globale Interrupts Enable
	
	//Timer für die Phasenanschnittberechnung ( alle 100usec )
	TCCR2A |= (1<<WGM21);
	TCCR2B |= (1<<CS22|1<<CS21);
	TIMSK2 |= (1<<OCIE2A);
	OCR2A = F_CPU/256/2/50/92 - 1;
		
	SSR1_PIN_OUT;
	SSR2_PIN_OUT;
	SSR3_PIN_OUT;
	SSR1_OFF;
	SSR2_OFF;
	SSR3_OFF;
	
	DDRC = (1<<PC5);
	PORTB |= 0x3F;
	PORTC |= 0xFF;
	PORTC &=~(1<<PC5);
	
	//Endlosschleife
	while(1)
	{
		dmx_adresse_tmp = 0;
		
		if(!(PINB&(1<<PB0))) dmx_adresse_tmp |= 0x01;
 		if(!(PINB&(1<<PB1))) dmx_adresse_tmp |= 0x02;
		if(!(PINB&(1<<PB2))) dmx_adresse_tmp |= 0x04;
		if(!(PINB&(1<<PB3))) dmx_adresse_tmp |= 0x08;
		if(!(PINB&(1<<PB4))) dmx_adresse_tmp |= 0x10;
		if(!(PINB&(1<<PB5))) dmx_adresse_tmp |= 0x20;
		
		if(!(PINC&(1<<PC0))) dmx_adresse_tmp |= 0x40;
		if(!(PINC&(1<<PC1))) dmx_adresse_tmp |= 0x80;
		if(!(PINC&(1<<PC2))) dmx_adresse_tmp |= 0x0100;
		
		if(dmx_adresse != dmx_adresse_tmp) dmx_adresse =  dmx_adresse_tmp;	
		
		
		//new calculation of val1
		val1 = valMap[dmx_buffer[dmx_adresse]];
		val2 = valMap[dmx_buffer[dmx_adresse+1]];
		val3 = valMap[dmx_buffer[dmx_adresse+2]];
		
		/*val1 = (255 - dmx_buffer[dmx_adresse])/2;
		val2 = (255 - dmx_buffer[dmx_adresse+1])/2;
		val3 = (255 - dmx_buffer[dmx_adresse+2])/2; */
		
		#if (!SSR_RELAIS1)
			if(val1 > 64)
			{
				SSR1_ON;
			}
			else
			{
				SSR1_OFF;
			}	
		#endif
		
		#if (!SSR_RELAIS2)
			if(val2 > 64)
			{
				SSR2_ON;
			}
			else
			{
				SSR2_OFF;
			}	
		#endif 
		
		#if (!SSR_RELAIS3)
			if(val3 > 64)
			{
				SSR3_ON;
			}
			else
			{
				SSR3_OFF;
			}	
		#endif 
		
		if(dmx_lost==DMX_LOST_TIMEOUT)
		{
			dmx_buffer[dmx_adresse] = 0;
			dmx_buffer[dmx_adresse+1] = 0;
			dmx_buffer[dmx_adresse+2] = 0;
		}
	}
}
