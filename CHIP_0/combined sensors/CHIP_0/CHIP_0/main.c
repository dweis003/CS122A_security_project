#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
#include "usart_ATmega1284.h"
#include "bit.h"
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 

// Pins on PORTA are used as input for A2D conversion
//    The default channel is 0 (PA0)
// The value of pinNum determines the pin on PORTA
//    used for A2D conversion
// Valid values range between 0 and 7, where the value
//    represents the desired pin for A2D conversion

//FUNCTION TO INITIALIZE ADC
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	// in Free Running Mode, a new conversion will trigger whenever
	// the previous conversion completes.
}


//GLOBAL VARIABLES 
unsigned char ARM_DISARM = 1; // when 0 system is disarmed, when 1 system is armed
unsigned char temp_reading = 0x00;
unsigned char temp_val = 0x00;
unsigned short temp_MV = 0x00;
unsigned char temp_trip = 0;
unsigned char IR_one_trip = 0;
unsigned char IR_two_trip = 0;
unsigned char button_one_trip = 0;
unsigned char button_two_trip = 0;




//READ TEMP FSM WHEN SYSTEM DISARMED
enum TEMPState {T_Wait, Read_temp } temp_state;

void TEMP_Init(){
	temp_state = T_Wait;
}

void TEMP_Tick(){
	//Actions
	switch(temp_state){
		case T_Wait:
		break;


		case Read_temp:
		temp_MV = ADC * (5000/1024); //
		temp_val = ((temp_MV - 500)/10); 
		PORTD = temp_val; //for testing
		break;
		
		default:
		break;
	}
	//Transitions
	switch(temp_state){
		case T_Wait:
			if(ARM_DISARM == 0){ //if system is disarmed read temp 
				temp_state = Read_temp;
			}
			else{
				temp_state = T_Wait;
			}
		break;

		case Read_temp:
			if(ARM_DISARM == 0){ //if system is disarmed read temp
				temp_state = Read_temp;
			}
			else{
				temp_state = T_Wait;
			}
		break;

		default:
			temp_state = T_Wait;
		break;
	}
}

void TempSecTask()
{
   TEMP_Init();
   for(;;) 
   { 	
	TEMP_Tick();
	vTaskDelay(10); 
   } 
}

void StartTempPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TempSecTask, (signed portCHAR *)"TempSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ARMED MODE FSM
//READ TEMP FSM WHEN SYSTEM DISARMED
enum ARMState {ARM_wait, read_sensors } arm_state;

void ARM_Init(){
	arm_state = T_Wait;
}

void ARM_Tick(){
	//Actions
	switch(arm_state){
		case ARM_wait:
		//initialize values, these values signal which sensor has been tripped
		temp_trip = 0;
		IR_one_trip = 0;
		IR_two_trip = 0;
		button_one_trip = 0;
		button_two_trip = 0;
		break;

		case read_sensors:
			temp_MV = ADC * (5000/1024);
			temp_val = ((temp_MV - 500)/10);
			PORTD = 0x00;
			PORTB = 0x00;
			if(temp_val >= 32){ //fire detected about 90 F
				temp_trip = 1; //fire detected trip
				PORTD = 0x01;
			}
			if((GetBit(~PINC, 3) == 1)){ //IR 2
				IR_two_trip = 1;
				PORTD = 0x02;
			}
			if((GetBit(~PINC, 2) == 1)){ //IR 1
				IR_one_trip = 1;
				PORTD = 0x04;
			}
			if((GetBit(~PINC, 1) == 1)){ //button 2
				button_two_trip = 1;
				PORTD = 0x08;
			}
			if((GetBit(~PINC, 0) == 1)){ //button 1
				button_one_trip = 1;
				PORTD = 0x10;
			}
		break;
		
		default:
		break;
	}
	//Transitions
	switch(arm_state){
		case ARM_wait:
		if(ARM_DISARM == 1){ //enter armed mode
			arm_state = read_sensors;
		}
		else{
			arm_state = ARM_wait;
		}
		break;

		case read_sensors:
		if(ARM_DISARM == 1){ //enter armed mode
			arm_state = read_sensors;
		}
		else{
			arm_state = ARM_wait;
		}
		break;
		
		default:
		arm_state = ARM_wait;
		break;
	}
}

void ARMSecTask()
{
	ARM_Init();
	for(;;)
	{
		ARM_Tick();
		vTaskDelay(10);
	}
}

void StartARMPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(ARMSecTask, (signed portCHAR *)"ARMSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

 
int main(void) 
{ 
   ADC_init();
   DDRA = 0x00; PORTA=0xFF;
   DDRC = 0x00; PORTC=0xFF;
   DDRD = 0xFF; PORTD = 0x00;
   DDRB = 0xFF; PORTB = 0x00;
   //Start Tasks  
   StartTempPulse(1);
   StartARMPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}