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

unsigned char ARM_DISARM = 0; // when 0 system is disarmed, when 1 system is armed

//Variables for temp read and sensors FSM
unsigned char temp_reading = 0x00;
unsigned char temp_val = 0x00;
unsigned short temp_MV = 0x00;
unsigned char temp_trip = 0;
unsigned char IR_one_trip = 0;
unsigned char IR_two_trip = 0;
unsigned char button_one_trip = 0;
unsigned char button_two_trip = 0;

unsigned char triggered_sensors = 0x00;

//variables for MOTOR FSMS
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

//transmit FSM variables
unsigned char data_to_transmit = 0x00;

//receive FSM data
unsigned char received_data = 0x00;






//READ TEMP FSM WHEN SYSTEM DISARMED
enum TEMPState {T_Wait, Read_temp, Wait_Trans_temp, Trans_Temp } temp_state;

void TEMP_Init(){
	temp_state = T_Wait;
}

void TEMP_Tick(){
	//Actions
	switch(temp_state){
		case T_Wait:
		break;


		case Read_temp:
		temp_MV = ADC * (5000/1024); 
		temp_val = ((temp_MV - 500)/10); 
		//temp_val = 0x0F;
		//PORTD = temp_val; //for testing
		break;

		case Wait_Trans_temp:
		break;

		case Trans_Temp:
			USART_Send(temp_val, 0); //send temp via USART
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
				temp_state = Wait_Trans_temp;
			}
			else{
				temp_state = T_Wait;
			}
		break;
	case Wait_Trans_temp:
		if(USART_IsSendReady(0) && ARM_DISARM == 0){
			temp_state = Trans_Temp;
		}
		else if(ARM_DISARM == 1){
			temp_state = T_Wait;
		}
		else{
			temp_state = Wait_Trans_temp;
		}
		break;

		case Trans_Temp:
			temp_state = T_Wait;
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
		triggered_sensors = 0x00;
		break;

		case read_sensors:
			temp_MV = ADC * (5000/1024);
			temp_val = ((temp_MV - 500)/10);
			//PORTD = 0x00;
			if(temp_val >= 32){ //fire detected about 90 F
				temp_trip = 1; //fire detected trip
				triggered_sensors = triggered_sensors | 0x01;
			}
			if((GetBit(~PINC, 3) == 1)){ //IR 2
				IR_two_trip = 1;
				triggered_sensors = triggered_sensors | 0x02;
			}
			if((GetBit(~PINC, 2) == 1)){ //IR 1
				IR_one_trip = 1;
				triggered_sensors = triggered_sensors | 0x04;
			}
			if((GetBit(~PINC, 1) == 1)){ //button 2
				button_two_trip = 1;
				triggered_sensors = triggered_sensors | 0x08;
			}
			if((GetBit(~PINC, 0) == 1)){ //button 1
				button_one_trip = 1;
				triggered_sensors = triggered_sensors | 0x10;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FSM to transmit data over USART
enum TRANSState {Trans_Wait, Transmit_State } trans_state;

void TRANS_Init(){
	trans_state = Trans_Wait;
}

void Trans_Tick(){
	//Actions
	switch(trans_state){
		case Trans_Wait:
		break;


		case Transmit_State:
		USART_Send(triggered_sensors, 0);
		break;
		
		default:
		break;
	}
	//Transitions
	switch(trans_state){
			case Trans_Wait:
			if(USART_IsSendReady(0) && ARM_DISARM == 1){ //if usart is ready and ARMED  proceed to next state
				trans_state = Transmit_State;
			}
			else{
				trans_state = Trans_Wait;
			}
			break;


			case Transmit_State:
			if(USART_HasTransmitted(0)){ //if data transmitted go back to wait
				trans_state = Trans_Wait;
			}
			else{						//else stay till completion
				trans_state = Transmit_State;
			}
			break;
			
			default:
			trans_state = Trans_Wait;
			break;

		}

}
	

void TransSecTask()
{
	TRANS_Init();
	for(;;)
	{
		Trans_Tick();
		vTaskDelay(10);
	}
}

void TransSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TransSecTask, (signed portCHAR *)"TransSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RECEIVE FSM
enum RECState {Rec_Wait, Receive_State } rec_state;

void REC_Init(){
	rec_state = Rec_Wait;
}

void Rec_Tick(){
	//Actions
	switch(rec_state){
		case Rec_Wait:
		break;


		case Receive_State:
		//ARM_DISARM = USART_Receive(0); //receive data
		 received_data = USART_Receive(0);
		if( received_data == 0xFF){ //ARM SYSTEM
			ARM_DISARM = 1;
		}
		else{							//DISARM SYSTEM
			ARM_DISARM = 0;
		}
	
		USART_Flush(0); //flush so flag reset
		break;
		
		default:
		break;
	}
	//Transitions
	switch(rec_state){
		case Rec_Wait:
			if(USART_HasReceived(0)){
				rec_state = Receive_State; //if ready go to next state
			}
			else{
				rec_state = Rec_Wait; //if not ready to receive data stay
			}
		break;


		case Receive_State:
			rec_state = Rec_Wait; //go back 
		break;
		
		default:
		rec_state = Rec_Wait;
		break;
	}

}


void RecSecTask()
{
	REC_Init();
	for(;;)
	{
		Rec_Tick();
		vTaskDelay(10);
	}
}

void RecSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(RecSecTask, (signed portCHAR *)"RecSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}









//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 
int main(void) 
{ 
   ADC_init(); //initialize ADC
   initUSART(0);//Initialize USART 0
   DDRA = 0x00; PORTA=0xFF;
   DDRB = 0xFF; PORTB = 0x00;
   DDRC = 0x00; PORTC=0xFF;
   //DDRD = 0xFF; PORTD = 0x00; //used by USART 0

   //Start Tasks  
   TransSecPulse(1);
   MotorSecPulse(1);
   Motor2SecPulse(1);
   StartARMPulse(1);
   StartTempPulse(1);
   TransSecPulse(1);
   RecSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}