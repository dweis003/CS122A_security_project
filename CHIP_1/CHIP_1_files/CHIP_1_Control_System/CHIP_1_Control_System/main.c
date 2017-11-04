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
#include "keypad.h"
#include "io.c" //contains lcd.h file
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 
//GLOBAL VARIABLES
unsigned char ARM_DISARM = 0; // 0 = disarmed, 1 = armed
//RECIEVE FSM VARIABLES
unsigned char received_value = 0x00;
//TRANSMIT FSM VARIABLES 
unsigned char data_to_send = 0x00; //0x00 = tell chip 0 disarm, 0xFF tell chip 1 arm

//FSM
/////////////////////////////////////////////////////////////////////////////////////
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
		USART_Send(data_to_send, 0);
		break;
		
		default:
		break;
	}
	//Transitions
	switch(trans_state){
		case Trans_Wait:
		if(USART_IsSendReady(0)){ //if usart is ready proceed to next state
			if(ARM_DISARM == 1){
				data_to_send = 0xFF; //if armed send 0xFF to CHIP 0
			}
			else{
				data_to_send = 0x00; //if disarmed send 0x00 to CHIP 0
			}
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
		//get data from CHIP 0
		received_value = USART_Receive(0);
		
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



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;
  
   //Start Tasks  
   TransSecPulse(1);
   RecSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}