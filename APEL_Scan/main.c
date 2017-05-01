/*
 * APEL_Scan.c
 *
 * Created : 3/7/2017
 * Author : Tyler Huddleston
 * Microcontroller : ATmega328P (TQFP)
 * 
 * This program is for the microcontroller module of the A.P.E.L Scan project.  The
 * A.P.E.L. Scan project is the Spring 2017 Senior Design project created by Angsuman
 * Roy and designed and constructed by Raheel Sadiq, Sesomphone Lim, and Tyler 
 * Huddleston.  It is a device that makes use of a Silicon Photomultiplier (SiPM) to
 * detect photons emitted from a liquid.  The signal from the SiPM is a series of 
 * fast current pulses that are summed up and averaged using an integrator.  The 
 * output from the integrator is then a DC voltage level with a peak voltage related 
 * to the number of pulses output by the SiPM.
 *
 * The DC voltage level from the integrator is measured with an ADC channel of the
 * ATmega328P microcontroller.  The programmed microcontroller displays the 
 * resulting DC voltage level to an LCD screen.  It also measures the bias voltage
 * across the SiPM so that it can be adjusted without the need for extra equipment.
 * 
 * Push-buttons on the devices interface determine the mode the microcontroller is
 * operating in and which values to show on the LCD screen.
 */ 

/*  Program Update: 4/30/2017
	* Changed internal clock to 1MHz to avoid disabling fuses
	* Modified signal voltage detection to reference internal bandgap for improved
	  resolution.
*/

#define F_CPU 1000000UL		// 1.0 MHz internal clock

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

// LCD pins
#define rs PD3
#define rw PD4
#define en PD5
#define data PORTB

// push button pins
#define start PD0
#define stop PD1
#define readBias PD2

void lcd_init();						// clears LCD screen and sets positions
void lcd_cmd(char cmd_out);				// sends a command across the LCD command bits
void lcd_data(char data_out);			// sends data across the LCD data register
void lcd_str(char *str);				// prints a string to the LCD screen
void adc_init();						// sets the reference voltage and clock prescaler for the ADC
void adc_initBG();						// sets the reference voltage as internal 1.1V bandgap and prescaler for ADC
uint16_t read_adc(uint8_t channel);		// reads and returns the voltage input at an ADC channel
void print_volt(float voltage);			// prints a voltage to the LCD
void print_APEL();						// prints custom font
void welcome_screen();					// prints start up welcome message
void store_cust_chars();				// load custom characters into CGRAM on LCD

int main()
{
	int peakVoltage = 0;		// for holding peak voltage from test
	int Vread = 0;				// for reading voltages during test
	float bias;					// for displaying bias voltage as decimal
	float peak = 0.0;			// for storing peak voltage as decimal
	int state = 0;				// for storing the operating state based on button pushed
	
	DDRB = 0XFF;		// Port B: data register for LCD - set to output
	DDRD = 0XF8;		// Port D: PD0:2 push button input, PD3:5 LCD commands output
	
	lcd_init();
	store_cust_chars();
	welcome_screen();
	
	while(1)
	{
		/* 
			STATE 0: Initial mode - display welcome screen
			STATE 1: Start mode - run test to capture peak DC voltage level
			STATE 2: Stop mode - stop test and display peak voltage level
			STATE 3: Read bias mode - display the DC bias across the SiPM
		*/
		
		if((PIND & 0x01) == 0x01)			// START button pushed
		{
			adc_initBG();
			state = 1;
			peakVoltage = 0;
			lcd_init();
			print_APEL();
			lcd_str(". Scan");
			lcd_cmd(0xC3);					// set position [2,4]
			lcd_str("Running...");
		}
		else if ((PIND & 0x02) == 0x02)		// STOP button pushed
		{
			state = 2;
			lcd_init();
			lcd_cmd(0x82);					// set position [1,3]
			lcd_str("Peak Voltage");
			// convert 10-bit binary voltage value to decimal referenced to 1.1V with 1/6 divider at input
			peak = (float)(peakVoltage * 1.1 * ((495.2+101.65)/101.65) / 1023.0);  
			lcd_cmd(0xC0);					// set position [2,1];
			print_volt(peak);
		}
		else if ((PIND & 0x04) == 0x04)		// READ BIAS button pushed
		{
			adc_init();
			state = 3;
			lcd_init();
			lcd_cmd(0x83);					// set position [1,4]
			lcd_str("SiPM Bias");
			lcd_cmd(0xC2);					// set position [2,3]
		}
			
		switch(state)
		{
			case 1 :	// check for and capture peak voltage level
				Vread = read_adc(7);
				if(Vread > peakVoltage) peakVoltage = Vread;
			break;
			
			case 3 :	// display bias voltage
				Vread = read_adc(1);
				// read and convert ADC channel 6 with 6:1 voltage divider
				bias = (float)(Vread * 5.0 * ((498.1+101.8)/101.8) / 1023.0);	
				lcd_cmd(0xc0);				// set position [2,1]
				print_volt(bias);
			break;
		}
	}
}

void lcd_init()
{
	lcd_cmd(0X38);				// initialize 2 lines, 5x7 font for 16x2 LCD
	lcd_cmd(0X0C);				// display on, cursor off
	lcd_cmd(0X01);				// clear screen
	lcd_cmd(0X80);				// set position [1,1]
}

void lcd_cmd(char cmd_out)
{
	data=cmd_out;					// set the command across LCD data register
	PORTD=(0<<rs)|(0<<rw)|(1<<en);	// enable LCD to receive command
	_delay_ms(2);
	PORTD=(0<<rs)|(0<<rw)|(0<<en);	// latch last command
	_delay_ms(2);
}

void lcd_data(char data_out)
{
	data=data_out;					// set the data across the LCD data register	
	PORTD=(1<<rs)|(0<<rw)|(1<<en);	// enable LCD to receive data
	_delay_ms(2);
	PORTD=(1<<rs)|(0<<rw)|(0<<en);	// latch last data
	_delay_ms(2);
}

void lcd_str(char *str)
{
	unsigned int i=0;
	while(str[i]!='\0')		// print each character in string to LCD
	{
		lcd_data(str[i]);
		i++;
	}
}

void adc_init()
{
	ADMUX = (0<<REFS1|1<<REFS0);	// Aref = Vcc
	
	/* the ADC needs a clock signal between 50kHz and 200kHz, with internal 1MHz clock,
	   a prescaler of 8 gives the ADC a clock of 125kHz */
	
	ADCSRA = (0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);	// enable ADC, set prescaler to 8
}

void adc_initBG()
{
	ADMUX = (1<<REFS1|1<<REFS1);	// Aref = 1.1V inernal bandgap reference
	
	ADCSRA = (0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);	// enable ADC, set prescaler to 8
}

uint16_t read_adc(uint8_t channel)
{
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);	// select ADC channel
	ADCSRA |= (1<<ADSC);						// select single conversion mode
	while( ADCSRA & (1<<ADSC) );				// wait until ADC conversion is complete
	return ADC;
}

void print_volt(float voltage)
{
	int intV;
	float diffV;
	int decV;
	char number[16];	// character array for converting doubles to string
	
	intV = (int) voltage;
	diffV = voltage - (float)intV;
	decV = (int) (diffV*1000);
	
	if(decV < 10){	// decimal values of 0.00
		sprintf(number, "%u.00%u V", intV, decV);		// convert voltage to string
	}
	else if(decV < 100){  // decimal values of 0.0x
		sprintf(number, "%u.0%u V", intV, decV);
	}	// decimal values of 0.xx
	else sprintf(number, "%u.%u V", intV, decV);
	
	lcd_str(number);									// print voltage
}

void welcome_screen()
{
	print_APEL();
	lcd_cmd(0xC3);					// set position [2,4]
	lcd_str("UNLV  2017");
	_delay_ms(10);
}

void store_cust_chars()
{
	// A CGRAM address: 0x00
	lcd_cmd(0x40);
	lcd_data(0x03);
	lcd_data(0x07);
	lcd_data(0x0B);
	lcd_data(0x13);
	lcd_data(0x1F);
	lcd_data(0x13);
	lcd_data(0x13);
	lcd_data(0x00);
	
	// P 0x08
	lcd_cmd(0x48);
	lcd_data(0x1E);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1E);
	lcd_data(0x18);
	lcd_data(0x18);
	lcd_data(0x18);
	lcd_data(0x00);
	
	// E 0x10
	lcd_cmd(0x50);
	lcd_data(0x1E);
	lcd_data(0x18);
	lcd_data(0x1E);
	lcd_data(0x18);
	lcd_data(0x18);
	lcd_data(0x1E);
	lcd_data(0x1E);
	lcd_data(0x00);
	
	// L 0x18
	lcd_cmd(0x58);
	lcd_data(0x10);
	lcd_data(0x10);
	lcd_data(0x10);
	lcd_data(0x10);
	lcd_data(0x10);
	lcd_data(0x1E);
	lcd_data(0x1E);
	lcd_data(0x00);
}

void print_APEL()
{
	lcd_cmd(0X81);		// set position [1,2]
	lcd_data(0x00);		// A
	lcd_data(0x2E);		// .
	lcd_data(0x01);		// P
	lcd_data(0x2E);		// .
	lcd_data(0x02);		// E
	lcd_data(0x2E);		// .
	lcd_data(0x03);		// L
	lcd_str(". Scan");	
}