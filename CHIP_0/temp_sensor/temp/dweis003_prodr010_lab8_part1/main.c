/* dweis003_prodr010_lab8_part3.c - [4/25/16]
* Partner(s) Name & E-mail:Donald Weiss dweis003@ucr.edu
*			     Paul Rodriguez prodr010@ucr.edu
* Lab Section: 001
* Assignment: Lab #8 Exercise #3
* Exercise Description:

*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/
#include <avr/io.h>
unsigned char temp_val = 0x00;
unsigned short temp_MV = 0x00;
#include "io.c"

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	// in Free Running Mode, a new conversion will trigger whenever
	// the previous conversion completes.
}


void read(){
	temp_MV = ADC * (5000/1024);
	temp_val = ((temp_MV - 500)/10);
	if(temp_val < 10){
		LCD_DisplayString(1,"temp less than 10");
	}
	else if((temp_val > 10) && (temp_val < 20)){
		LCD_DisplayString(1,"temp between 10 and 20");
	}
	else if((temp_val > 20) && (temp_val < 25)){
		LCD_DisplayString(1,"temp between 20 and 25");
	}
	else if((temp_val > 25) && (temp_val < 30)){
		LCD_DisplayString(1,"temp between 25 and 30");
	}
	else{
		LCD_DisplayString(1,"temp > 30");
	}



}

int main(void)
{
	DDRA = 0x00; // Set port A to input
	PORTA = 0xFF; // Init port A to 0s
	DDRC = 0xFF; PORTC = 0x00; // set as output for lcd
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	LCD_init();

	ADC_init();
	while (1)
	{
		read();
		
	}
}

