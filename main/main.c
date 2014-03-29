

/* Name: main.c
 * Project: USB Flight controller board with 3 axes, hardware trims and hardware calibration feature.
 * Author: David Janda
 * Creation Date: 2013-09
 * 
 * final build target for ATMega8
 * Fuse setup:
 *	0xC9	hi
 *	0xEF	lo
 * 
 * 
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>		/* for sei() */
#include <util/delay.h>			/* for _delay_ms() */
#include <avr/pgmspace.h>		/* required by usbdrv.h */
#include "vusb/usbdrv.h"		/* vusb driver */
#include "buttons/buttons.h"	/* buttons driver */
#include <avr/eeprom.h>			/* correction data storage */
#include "mcp3304/mcp3304.h"	/* communication with mcp3304 thru ISP



/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */
PROGMEM const char usbHidReportDescriptor[30] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x30,                    //   USAGE (X)
    0x09, 0x31,                    //   USAGE (Y)
    0x09, 0x32,                    //   USAGE (Z)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x1f,              //   LOGICAL_MAXIMUM (8191)
    0x75, 0x0d,                    //   REPORT_SIZE (13)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0xc0                           // END_COLLECTION
    };



	/* REPORT STRUCTURE
	** 3x13bit values
	**	-------0 -------1 -------2 -------3 -------4
	**  76543210 ---21098 -------- 543210-- -2109876 
	**           210      09876543       21          
	*/


int32_t a2dData[6] = {0, 0, 0, 0, 0, 0};		// array of data from inputs
uint16_t a2dDataPrev[3] = {0, 0, 0};				// array of previous data measured
uint8_t reportBuffer[5] = {0,0,0,0,0};			// array of data packets for host
uint16_t axisOversampling = 128;
uchar high;										// stores hi part of report byte
uchar low;										// stores lo part
uint16_t cycleCounter = 0;
uchar i = 0;



	
char device_configuration = 0b10000100;			// device config storage
// bit 0: calibration data gathering active
// bit 1: calibration set center point
// bit 2: calibration active
// bit 3:
// bit 4:
// bit 5:
// bit 6:
// bit 7: trims active
	
static uchar    idleRate;						// repeat rate for keyboards	

int16_t a2dDataMinimums[3];
int16_t a2dDataMaximums[3];
int16_t a2dDataCenters[3];

int16_t a2dDataMinimums_ee[3] EEMEM;
int16_t a2dDataMaximums_ee[3] EEMEM;
int16_t a2dDataCenters_ee[3] EEMEM;

void correction_data_reset()
{
	a2dDataMinimums[0] = 9000;
	a2dDataMinimums[1] = 9000;
	a2dDataMinimums[2] = 9000;
	
	a2dDataMaximums[0] = 100;
	a2dDataMaximums[1] = 100;
	a2dDataMaximums[2] = 100;
	
	a2dDataCenters[0]  = 4095;
	a2dDataCenters[1]  = 4095;
	a2dDataCenters[2]  = 4095;
	

}

void correction_data_load()
{
	eeprom_read_block(a2dDataMinimums, a2dDataMinimums_ee, 6);
	eeprom_read_block(a2dDataMaximums, a2dDataMaximums_ee, 6);
	eeprom_read_block(a2dDataCenters, a2dDataCenters_ee, 6);
}

void correction_data_store()
{
	eeprom_update_block(a2dDataMinimums, a2dDataMinimums_ee, 6);
	eeprom_update_block(a2dDataMaximums, a2dDataMaximums_ee, 6);
	eeprom_update_block(a2dDataCenters, a2dDataCenters_ee, 6);
}



int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
	//this works
	// must be unit32 all slots, or doesnt work
	return ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}


void dataToBuffer(void) 
{
	/* REPORT STRUCTURE
	** 3x13bit values
	**	-------0 -------1 -------2 -------3 -------4
	**  76543210 ---21098 --------                   
	**           210      09876543       21         
	**                             543210-- -2109876 
	*/	    
			
	// format the data into report bytes 
	// 0
	reportBuffer[0] = ((a2dData[0] & 0b0000000011111111));
	// 1
	low =  (a2dData[0] & 0b0001111100000000)>>8;
	high = (a2dData[1] & 0b0000000000000111)<<5;
	reportBuffer[1] = low+high;
	// 2
	reportBuffer[2] = (a2dData[1] & 0b0000011111111000)>>3;
	// 3
	low =  (a2dData[1] & 0b0001100000000000)>>11;
	high = (a2dData[2] & 0b0000000000111111)<<2;
	reportBuffer[3] = low+high;
	// 4
	low =  (a2dData[2] & 0b0001111111000000)>>6;
	reportBuffer[4] = 0x00+low;

}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        //DBG1(0x50, &rq->bRequest, 1);   /* debug output: print our request */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void)
{

//////////////////////////////////////////////////////////////////////////
//			INITIALIZE PROGRAM
//////////////////////////////////////////////////////////////////////////



	buttons_init();						// Initialize buttons //
	correction_data_load();
	
	//MCUCSR|= (1<<JTD);
	//MCUCSR|= (1<<JTD);	//disable JTAG

	SPI_Init();							// Init spi port comm //
    wdt_enable(WDTO_1S);
    usbInit();
    usbDeviceDisconnect();				// enforce re-enumeration, do this while interrupts are disabled! //
    uchar i = 0;
    while(--i){							// fake USB disconnect for > 250 ms //
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
	
	
	// 3-way switch setup
	DDRD &=~(1<<PIND5);
	PORTD |= 1<<PIND5;
	DDRD &=~(1<<PIND6);
	PORTD |= 1<<PIND6;										

//////////////////////////////////////////////////////////////////////////
//			MAIN LOOP
//////////////////////////////////////////////////////////////////////////

    for(;;){							 // main program loop //
	
		buttons_read();
		
        wdt_reset();
        usbPoll();

		//////////////////////////////////////////////////////////////////////////
		//			BUTTON HANDLING
		//////////////////////////////////////////////////////////////////////////
		// bit 0: calibration data gathering active
		// bit 1: calibration set center point
		// bit 2: calibration active
		// bit 3:
		// bit 4:
		// bit 5: 
		// bit 6: trims half
		// bit 7: trims active
		
		
		if (bit_is_set(PIND,PIND5))										// toggle trims active with 3-way switch
		{
			device_configuration |= 1<<7;
		} else {
			device_configuration &=~(1<<7);
		}
		
		if (bit_is_set(PIND,PIND6))										// toggle trims half with 3-way switch
		{
			device_configuration |= 1<<6;
			} else {
			device_configuration &=~(1<<6);
		}
				
		if (buttons_down(1))										// set center value flag
		{
			device_configuration |= 1<< 1;
		}
		
		
		//if (buttons_held(1))										// set correction gathering flag, when exiting set center value flag
		if (
				( (bit_is_clear(device_configuration, 0)) && (buttons_held(0)) )
				||
				( (bit_is_set(device_configuration, 0) && (buttons_down(0))) )
			)
		{
			if (bit_is_clear(device_configuration, 0))
			{
				correction_data_reset();
			} else {
				device_configuration |= 1<< 1;
			}
			device_configuration ^= 1 << 0;
		}
		
		
		if (buttons_down(2))										// reset correction data
		{
			correction_data_reset();
		}

		//////////////////////////////////////////////////////////////////////////
		//			LED HANDLING
		//////////////////////////////////////////////////////////////////////////
		// bit 0: calibration data gathering active
		// bit 1: calibration set center point
		// bit 2: calibration active
		// bit 3:
		// bit 4:
		// bit 5:
		// bit 6: trims half
		// bit 7: trims active
		
		if (bit_is_clear(device_configuration, 0))					// calibration data gathering active on LED1
		{
			DDRD |= 1 << PIND7;
			PORTD &= ~(1<<PIND7);
		} else {
			DDRD |= 1 << PIND7;
			PORTD |= (1<<PIND7);
		}
		
		if (bit_is_clear(device_configuration, 6))					// trims half on LED 2
		{
			DDRD |= 1 << PIND1;
			PORTD &= ~(1<<PIND1);
		} else {
			DDRD |= 1 << PIND1;
			PORTD |= (1<<PIND1);
		}
		
		if (bit_is_clear(device_configuration, 7))					// trims active on LED 3
		{
			DDRC |= 1 << PINC5;
			PORTC &= ~(1<<PINC5);
		} else {
			DDRC |= 1 << PINC5;
			PORTC |= (1<<PINC5);
		}
		
		

		//////////////////////////////////////////////////////////////////////////
		//			DATA GATHERING
		//////////////////////////////////////////////////////////////////////////

		// alternative approach, which uses main cycle as the gathering cycle

		if (cycleCounter < axisOversampling)
		{
			cycleCounter++;
			
			a2dData[0] += mcp3304_get13(0x0a,0x00);		// AILERON
			a2dData[1] += mcp3304_get13(0x0b,0x00);		// ELEVATOR
			a2dData[2] += mcp3304_get12(0x0c,0x80)*2;		// RUDDER
			

			a2dData[3] += mcp3304_get12(0x0d,0x00);		// AILE TRIM
			a2dData[4] += mcp3304_get12(0x0c,0x00);		// ELEV TRIM trim 3
			a2dData[5] += mcp3304_get12(0x0d,0x80);		// RUDD TRIM
		}
		
		//////////////////////////////////////////////////////////////////////////
		//			DATA MODIFY
		//////////////////////////////////////////////////////////////////////////
		else
		{
			cycleCounter=0;
			for (i=0; i<6; i++) {a2dData[i] = a2dData[i]/axisOversampling;}			// initial division

			for (i=0; i<3; i++)											// small change rejection to stop noise, seems that this is not necessary for 4095 trims
			{
				
				if (!(((a2dData[i+3]-a2dDataPrev[i])<-6) || ((a2dData[i+3]-a2dDataPrev[i])>6)))
				{
					a2dData[i+3] = a2dDataPrev[i];
				}
				
			}
			
			//////////////////////////////////////////////////////////////////////////

			
			if (bit_is_set(device_configuration, 0))				// gather correction data
			{
				for (i=0; i<3; i++)
				{
					if ((uint16_t)a2dDataMaximums[i]<(uint16_t)a2dData[i])
					{
						a2dDataMaximums[i]=(uint16_t)a2dData[i];
					}
				
					if ((int16_t)a2dData[i]<(int16_t)a2dDataMinimums[i])
					{
						a2dDataMinimums[i] = (uint16_t)a2dData[i];
					}
				}
			}
			

			//////////////////////////////////////////////////////////////////////////
			
			if (bit_is_set(device_configuration, 1))				// set center point and store correction data to eeprom
			{
				for (i=0; i<3; i++)
				{
					a2dDataCenters[i] = a2dData[i];
					device_configuration &= ~(1<<1);
				}
				correction_data_store();
			}
			
			//////////////////////////////////////////////////////////////////////////
			
			if (bit_is_set(device_configuration, 7))				// add trim information
			{
				for (uchar i=0; i<3; i++)
				{
// 					if (a2dData[i+3]<-a2dData[i]+0x0800)
// 					{
// 						a2dData[i+3] = -a2dData[i]+0x0800;
// 					}
// 					if (a2dData[i+3]>-a2dData[i]+0x0fff+0x0800)
// 					{
// 						a2dData[i+3] = -a2dData[i]+0x0fff+0x0800;
// 					}
					if (bit_is_set(device_configuration, 6))
					{
						a2dData[i] -= (2*(a2dData[i+3]-2048));
					} 
					else
					{
						a2dData[i] -= (1*(a2dData[i+3]-2048));
					}
					
				}
			}
			
			//////////////////////////////////////////////////////////////////////////
			
			// map data with our correction data
			//	map ( x, in_min, in_max, out_min , out_max )
			
			for (i=0; i<3; i++)
			{
				if (a2dData[i]<a2dDataMinimums[i])
				{
					a2dData[i] = a2dDataMinimums[i];
				}

				if (a2dData[i]>a2dDataMaximums[i])
				{
					a2dData[i] = a2dDataMaximums[i];
				}
				
				if (a2dData[i]<a2dDataCenters[i])
				{
				 	a2dData[i] = map(a2dData[i], a2dDataMinimums[i], a2dDataCenters[i], 0, 4095);
				}
				else
				{
				 	a2dData[i] = map(a2dData[i], a2dDataCenters[i], a2dDataMaximums[i], 4096, 8191);
				}
			}
			


			dataToBuffer();
			
			for (i=0; i<6; i++) 
			{
				if (i>2)
				{
					a2dDataPrev[i-3] = a2dData[i];
				}
				a2dData[i] = 0;
			}
		} // end data modify

		//_delay_ms(10);
 			
		
		//////////////////////////////////////////////////////////////////////////
		//			DEBUG
		//////////////////////////////////////////////////////////////////////////

		/*
		a2dData[0] = mcp3304_get13(0x08,0x00);
		a2dData[1] = mcp3304_get13(0x09,0x00);
		a2dData[2] = mcp3304_get12(0x0f,0x80);
		
		a2dData[0] = mcp3304_get13(0x08,0x00);
		a2dData[1] = mcp3304_get13(0x09,0x00);
		a2dData[2] = mcp3304_get12(0x0f,0x80);
		
		dataToBuffer();
		*/
		//////////////////////////////////////////////////////////////////////////
		//			DATA SEND
		//////////////////////////////////////////////////////////////////////////		
        
		if(usbInterruptIsReady()){									// called after every poll of the interrupt endpoint //
            usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
        }
		
		
		buttons_finalize();
    }
}

// ------------------------------------------------------------------------- //

