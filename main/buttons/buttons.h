/*
 * BUTTON DEBOUNCE LIBRARY
 *
 * Created: 26.9.2013 18:55:09
 *  Author: David Janda
 *
 *	Easily scalable button management library, for up to 8 buttons with debouncing and event-like behavior. 
 *  This can be easily extended to much more by adding more variables to it.
 */ 

#ifndef BUTTONS_H
#define BUTTONS_H

#include <avr/io.h>

// user settings
int confidence_level	= 200;	//200
int hold_level			= 5000;
int number_of_buttons	= 3;

// system settings

char button_state			= 0x00;
char button_previous_state	= 0x00;
char button_rising_edge		= 0x00;
char button_held			= 0x00;
int button_pressed_confidence[8];
int button_released_confidence[8];
int button_held_confidence[8];

//////////////////////////////////////////////////////////////////////////

// button definitions
// beware dragons
#define SFRPTR(port)   (volatile uint8_t*)& port
#define PINARR(x)      *(pin_address[x].PortAddr)
#define PINDDR(x)      *(pin_address[x].DDRAddr)

#define PINDDRIN(x)    PINDDR(x) &= ~pin_address[x].Mask				// set pin DDR to INPUT
//#define PINARRVAL(x)   PINARR(x) & pin_address[x].Mask				// ??
#define PINARRVAL(x)   *(pin_address[x].PinAddr) & pin_address[x].Mask	// get pin value
#define SETPINARR(x)   PINARR(x) |= pin_address[x].Mask					// set pin value to 1
#define CLEARPINARR(x) PINARR(x) &= ~pin_address[x].Mask				// set pin value to 0


struct PortType_s
{
	volatile uint8_t* PortAddr;
	volatile uint8_t* PinAddr;
	volatile uint8_t* DDRAddr;
	uint8_t  Mask;
};

typedef struct PortType_s PortType_t;

// user settings
static const PortType_t pin_address[4] =
{
	{ SFRPTR(PORTB),SFRPTR(PINB),SFRPTR(DDRB), (1 << 0) },      // pin channel 1
	{ SFRPTR(PORTD),SFRPTR(PIND),SFRPTR(DDRD), (1 << 3) },
	{ SFRPTR(PORTD),SFRPTR(PIND),SFRPTR(DDRD), (1 << 0) }
};


/*
#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))
*/

//////////////////////////////////////////////////////////////////////////

void buttons_init(void)
{		
	for (char i = 0;i < number_of_buttons; i++)
	{
		PINDDRIN(i);
		SETPINARR(i);
		button_state &= ~(1<<i);
	}
}

void buttons_read(void)
{		
	
	
	for (char i = 0;i < number_of_buttons; i++)
	{
		if (~PINARRVAL(i))										// get the physical state debounced and write it to button_state byte
		{
			button_pressed_confidence[i]++;
			button_released_confidence[i]=0;
			if ((button_pressed_confidence[i] > confidence_level) && (bit_is_clear(button_state, i)))
			{
				button_state |= 1 << i;
				button_pressed_confidence[i]=0;
			}
		} 
		else
		{
			button_released_confidence[i]++;
			button_pressed_confidence[i]=0;
			if ((button_released_confidence[i] > confidence_level) && (bit_is_set(button_state, i)))
			{
				button_state &= ~(1 << i);
				button_released_confidence[i]=0;
			}
		}
		
		if (bit_is_set(button_state, i))
		{
			button_held_confidence[i]++;
			if (button_held_confidence[i]>hold_level)
			{
				button_held |=1 << i;
				button_held_confidence[i] = 0;
			}
		} else {
			button_held_confidence[i] = 0;	
		}
	}
}

/* next is series of functions that will return true on certain events, and only once. */

char buttons_down(char button_number)							// process the button_state and button_previous_state to detect rising edge
{
	if ((bit_is_set(button_state, button_number)) && (bit_is_clear(button_previous_state, button_number)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
char buttons_up(char button_number)
{
	if ((bit_is_clear(button_state, button_number)) && (bit_is_set(button_previous_state, button_number)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

char buttons_held(char button_number)
{
	if (bit_is_set(button_held, button_number)) 
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////

void buttons_finalize(void) 
{
	button_previous_state = button_state;
	button_held = 0x00;
}


#endif