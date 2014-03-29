/*
 * spi.h 
 *
 * SPI communication header 1.0 for mcp3304
 *  Author: David
 */ 

#ifndef __avr/io_H_INCLUDED
	#include <avr/io.h>
#endif

#define SPI_PORT	PORTB
#define SPI_DDR		DDRB
#define SPI_SS		PINB2
#define SPI_MOSI	PINB3
#define SPI_MISO	PINB4
#define SPI_SCK		PINB5
#define VREF_TOGGLE_PIN	PINC0
#define VREF_TOGGLE_DDR DDRC
#define VREF_TOGGLE_PORT PORTC

char SPI_Data;
char SPI_Data2;
char dump;


void SPI_Init()
{
  // Initial the AVR ATMega168 SPI Peripheral

  // Set MOSI and SCK as output, others as input
  SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS); //
  //SPI_DDR &=~(1<<SPI_MISO);
  //SPI_DDR |= (1<<SPI_MISO);


  // Enable SPI, Master, set clock rate
  SPCR |= (1<<SPE)|(1<<MSTR);
  //SPCR |= (1<<SPR1)|(1<<SPR0);			// it seems that poll speed influences precision
  //SPSR |= (1<<SPI2X);
  SPCR |= (1<<SPR0);
  
  //SPCR |= (1<<CPOL);
  //SPCR |= (1<<DORD);
  
  VREF_TOGGLE_DDR |= 1<< VREF_TOGGLE_PIN;
    
  
}

unsigned char SPI_WriteRead(unsigned char dataout)
{
	unsigned char datain;
	
	// Start transmission (MOSI)
	SPDR = dataout;

	// Wait for transmission complete
	while(!(SPSR & (1<<SPIF)));

	// Get return Value;
	datain = SPDR;

	// Return Serial In Value (MISO)
	return datain;
}

// int16_t mcp3304_test()
// {
// 	SPI_PORT &= ~(1<<SPI_SS);
// 	SPI_Data  = SPI_WriteRead(0x08);
// 	SPI_Data  = SPI_WriteRead(0x00);
// 	SPI_Data2  = SPI_WriteRead(0x00);
// 	SPI_PORT |= (1<<SPI_SS);
// 	return ((SPI_Data&0b00011111)<<8) + SPI_Data2;
// }

int16_t mcp3304_get13(char bit1, char bit2)
{
	VREF_TOGGLE_PORT &=~(1<<VREF_TOGGLE_PIN);	// toggle lo vref
	// SS Low
	SPI_PORT &= ~(1<<SPI_SS);
	SPI_Data  = SPI_WriteRead(bit1);
	SPI_Data  = SPI_WriteRead(bit2);
	SPI_Data2 = SPI_WriteRead(0x00);
	//dump	  = SPI_WriteRead(0x00);
	// pulse to the SS Pin
	SPI_PORT |= (1<<SPI_SS);
    
	if ((((SPI_Data&0b00011111)<<8) + SPI_Data2)>4095)
	{
		return (((SPI_Data&0b00011111)<<8) + SPI_Data2 -8192-4095);
	}
	else
	{
		return (((SPI_Data&0b00011111)<<8) + SPI_Data2-4095);
	}
}

int16_t mcp3304_get12(char bit1, char bit2)
{
	VREF_TOGGLE_PORT |= 1<<VREF_TOGGLE_PIN;	// toggle hi vref
	// SS Low
	SPI_PORT &= ~(1<<SPI_SS);
	SPI_Data  = SPI_WriteRead(bit1);
	SPI_Data  = SPI_WriteRead(bit2);
	SPI_Data2 = SPI_WriteRead(0x00);
	//dump	  = SPI_WriteRead(0x00);
	// pulse to the SS Pin
	SPI_PORT |= (1<<SPI_SS);
	
	return (((SPI_Data&0b00011111)<<8) + SPI_Data2);

}

// Obsolete version of gather routines, that tried to encapsulate oversampling, but in effect halted mcu from recieving inputs
/*
int16_t mcp3304_get13(char bit1, char bit2, char a2d_samples)
{
		
		a2d_result_1 = 0;
		for (char i = 0; i < a2d_samples; i++)
		{
			
			// SS Low
			SPI_PORT &= ~(1<<SPI_SS);
			
			SPI_Data  = SPI_WriteRead(bit1);
			SPI_Data  = SPI_WriteRead(bit2);
			SPI_Data2 = SPI_WriteRead(0x00);
			dump	  = SPI_WriteRead(0x00);
			
			// pulse to the SS Pin
			SPI_PORT |= (1<<SPI_SS);
			
			if ((((SPI_Data&0b00011111)<<8) + SPI_Data2)>4095)
			{
				a2d_result_1 += ((SPI_Data&0b00011111)<<8) + SPI_Data2;
				a2d_result_1 -=8192;
			}
			else
			{
				a2d_result_1 += ((SPI_Data&0b00011111)<<8) + SPI_Data2;
			}
		}

		a2d_result_1 = (a2d_result_1/a2d_samples)+4095;
		return a2d_result_1;
}

int16_t mcp3304_get12(char bit1, char bit2, char a2d_samples)
{
	
	a2d_result_1 = 0;
	for (char i = 0; i < a2d_samples; i++)
	{
		
		// SS Low
		SPI_PORT &= ~(1<<SPI_SS);
		
		SPI_Data  = SPI_WriteRead(bit1);
		SPI_Data  = SPI_WriteRead(bit2);
		SPI_Data2 = SPI_WriteRead(0x00);
		dump	  = SPI_WriteRead(0x00);
		
		// pulse to the SS Pin
		SPI_PORT |= (1<<SPI_SS);
		

		a2d_result_1 += ((SPI_Data&0b00001111)<<8) + SPI_Data2;

	}

	a2d_result_1 = (a2d_result_1/a2d_samples);
	return a2d_result_1;
}*/