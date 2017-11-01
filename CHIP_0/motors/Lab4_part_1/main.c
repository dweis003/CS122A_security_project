/*part2.c - [10/24/17]
* Partner(s) Name & E-mail:Donald Weiss dweis003@ucr.edu
*			     Paul Rodriguez prodr010@ucr.edu
* Lab Section: 022
* Assignment: Lab #5 Exercise #2 STEPPER_MOTOR
* Exercise Description:

*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/


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

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "bit.h"

unsigned char ARM_DISARM = 1;

int numPhases = 2048; //number of phases needed to rotate 180 deg
//MOTOR 1 FSM VARIABLES
unsigned char phases[8] = {0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08,  0x09};
unsigned char orientation = 1; //1 = forward, 0 = backward
unsigned char p_index = 0;
int numCounter = 0;
unsigned char finished_reset_1 = 0;

//MOTOR 2 FSM VARIABLES
unsigned char phases_2[8] = {0x10, 0x30, 0x20, 0x60, 0x40, 0xC0, 0x80,  0x90};
unsigned char orientation_2 = 0; //0 = forward, 1 = backward opposite initial orientation so they rotate counter to eachother
unsigned char p_index_2 = 0;
int numCounter_2 = 0;
unsigned char finished_reset_2 = 0;


//////////////////////////////////////////////////////////////
//MOTOR 1 FSM
enum MotorState {m_wait, m_init, m_reset} motor_state;


void Motor_Init(){
	motor_state = m_init;
}

void Motor_Tick(){
	//Actions
	switch(motor_state){
		case m_wait:
		//reset to original values
		finished_reset_1 = 0;
		p_index = 0;
		numCounter = 0;
		orientation = 1;
		break;

		case m_init:
		if(orientation){ //forward orientation == 1
			if(numCounter < numPhases){
				++numCounter;
				PORTB = phases[p_index];
				
				if(p_index == 7){
					p_index = 0;
				}
				
				else{
					++p_index;
				}
			}
			
			else{
				numCounter = 0;
				orientation = 0;
				p_index = 0;
			}
			
		}
			
		else{ //orientation == 0 go backwards
			if(numCounter < numPhases){
				++numCounter;
				PORTB = phases[p_index];
				
				if(p_index == 0){
					p_index = 7;
				}
				
				else{
					--p_index;
				}
			}
			
			else{
				numCounter = 0;
				orientation = 1;
				p_index = 0;
			}

		}
		
		break;

		case m_reset:
		if(orientation){ //forward orientation == 1 go back 
			if(numCounter > 0){
				--numCounter;
				PORTB = phases[p_index];
				
				if(p_index == 0){
					p_index = 7;
				}
				
				else{
					--p_index;
				}

			}

			else{
				finished_reset_1 = 1; //done with reset
			}
		}

		else{ //motor already going back
			if(numCounter < numPhases){
				++ numCounter;
				PORTB = phases[p_index];
				
				if(p_index == 0){
					p_index = 7;
				}
				
				else{
					--p_index;
				}

			}

			else{
				finished_reset_1 = 1; //done with reset
			}
		}
		
			
		break;
		
		default:
		break;
	}
	//Transitions
	switch(motor_state){
		case m_wait:
		if(ARM_DISARM == 0){ //if system disarmed motor off
			motor_state = m_wait;
		}
		else{ //else turn on motors
			motor_state = m_init; 
		}
		break;

		case m_init:
		if(ARM_DISARM == 1){ //if system armed motor on
			motor_state = m_init;
		}
		else{
			motor_state = m_reset;
		}
		break;

		case m_reset:
		if(finished_reset_1 == 1){ //finished reset
			motor_state = m_wait;
		}
		else{
			motor_state = m_reset;
		}
		break;
		
		default:
		motor_state = m_wait;
		break;
	}
}

void MotorSecTask()
{
	Motor_Init();
	for(;;)
	{
		Motor_Tick();
		vTaskDelay(3);
	}
}

void MotorSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MotorSecTask, (signed portCHAR *)"MotorSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//////////////////////////////////////////////////////////////
//MOTOR 2 FSM
enum MotorState_2 {m_wait_2, m_init_2, m_reset_2} motor_state_2;


void Motor_2_Init(){
	motor_state_2 = m_init_2;
}

void Motor_Tick_2(){
	//Actions
	switch(motor_state_2){
		case m_wait_2:
		//reset to original values
		finished_reset_2 = 0;
		p_index_2 = 0;
		numCounter_2 = 0;
		orientation_2 = 0;
		break;

		case m_init_2:
		if(orientation_2){ //forward orientation == 1
			if(numCounter_2 < numPhases){
				++numCounter_2;
				PORTB = PORTB | phases_2[p_index_2];
				
				if(p_index_2 == 7){
					p_index_2 = 0;
				}
				
				else{
					++p_index_2;
				}
			}
			
			else{
				numCounter_2 = 0;
				orientation_2 = 0;
				p_index_2 = 0;
			}
			
		}
		
		else{ //orientation == 0 go forwards
			if(numCounter_2 < numPhases){
				++numCounter_2;
				PORTB = PORTB | phases_2[p_index_2];
				
				if(p_index_2 == 0){
					p_index_2 = 7;
				}
				
				else{
					--p_index_2;
				}
			}
			
			else{
				numCounter_2 = 0;
				orientation_2 = 1;
				p_index_2 = 0;
			}

		}
		
		break;

		case m_reset_2:
		if(orientation_2 == 0){ //going forward return to reset
			if(numCounter_2 > 0){
				--numCounter_2;
				PORTB = PORTB | phases_2[p_index_2];
				
				if(p_index_2 == 7){
					p_index_2 = 0;
				}
				
				else{
					++p_index_2;
				}

			}

			else{
				finished_reset_2 = 1; //done with reset
			}
		}

		else{ //motor going back continue to reset position
			if(numCounter_2 < numPhases){
				++ numCounter_2;
				PORTB = PORTB | phases_2[p_index_2];
				
				if(p_index_2 == 7){
					p_index_2 = 0;
				}
				
				else{
					++p_index_2;
				}

			}

			else{
				finished_reset_2 = 1; //done with reset
			}
		}
		
		
		break;
		
		default:
		break;
	}
	//Transitions
	switch(motor_state_2){
		case m_wait_2:
		if(ARM_DISARM == 0){ //if system disarmed motor off
			motor_state_2 = m_wait_2;
		}
		else{ //else turn on motors
			motor_state_2 = m_init_2;
		}
		break;

		case m_init_2:
		if(ARM_DISARM == 1){ //if system armed motor on
			motor_state_2 = m_init_2;
		}
		else{
			motor_state_2 = m_reset_2;
		}
		break;

		case m_reset_2:
		if(finished_reset_2 == 1){ //finished reset
			motor_state_2 = m_wait_2;
		}
		else{
			motor_state_2 = m_reset_2;
		}
		break;
		
		default:
		motor_state_2 = m_wait_2;
		break;
	}
}

void MotorSecTask_2()
{
	Motor_2_Init();
	for(;;)
	{
		Motor_Tick_2();
		vTaskDelay(3);
	}
}

void Motor2SecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MotorSecTask_2, (signed portCHAR *)"MotorSecTask_2", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ButtonState {b_release, b_push} button_state;

void Button_Init(){
	button_state = b_release;
}

void Button_Tick(){
	//Transitions
	switch(button_state){
		case b_release:
		if(GetBit(~PINC, 4) == 1){ // 
			button_state = b_push;
			if(ARM_DISARM == 1){
				ARM_DISARM = 0;
			}
			else{
				ARM_DISARM = 1;
			}
		}
		else{
			button_state = b_release;
		}
		break;

		case b_push:
		if(GetBit(~PINC, 4) == 1){ 
			button_state = b_push;
		}
		else{
			button_state = b_release;
		}
		break;

		default:
		button_state = b_release;
		break;
	}
	
	//Actions
	switch(button_state){
		case b_release:
		PORTD = 0x00;
		break;
		case b_push:
		PORTD =0xFF;
		break;
		default:
		break;
	}
	
	
}

void ButtonSecTask(){
	
	Button_Init();
	for(;;){
		Button_Tick();
		vTaskDelay(10);
	}
}

void ButtonSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ButtonSecTask, (signed portCHAR *)"ButtonSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}


int main(void)
{
	DDRC = 0x00; PORTC = 0xFF; 
	DDRB = 0xFF; PORTB = 0x00; //output


	
	
	//Start Tasks
	MotorSecPulse(1);
	Motor2SecPulse(1);
	ButtonSecPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}
