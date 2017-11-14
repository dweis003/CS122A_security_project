/* dweis003_prodr010_lab2_part1.c - [10/5/17]
* Partner(s) Name & E-mail:Donald Weiss dweis003@ucr.edu
*			     Paul Rodriguez prodr010@ucr.edu
* Lab Section: 022
* Assignment: Lab #2 Exercise #1
* Exercise Description:

*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "bit.h"
#include "timer.h"
#include "io.c"
//#include "keypad.h"
#include "usart_ATmega1284.h"
#include <stdio.h>
//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------
//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
	a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;
//--------End Task scheduler data structure-----------------------------------


//--------Shared Variables----------------------------------------------------
unsigned char LED_val = 0; //default output value
//--------End Shared Variables------------------------------------------------


//--------User defined FSMs---------------------------------------------------
//Enumeration of states.
enum SM1_States { off, on};
//timer FSM

int SMTick1(int state) {
	// Local Variables

	//State machine transitions
	switch (state) {
		case off: 
		state = on;
		break;
		
	

		case on:
		state = off;
		break;


		default: state = on; // default: Initial state
		break;
	}
	//State machine actions
	switch(state) {
		case off: 
			LED_val = 0;
			PORTA = 0x00;
			LCD_DisplayString(1, "system is disarmed!");
		break;
		

		case on:
			LED_val = 1;
			PORTA = 0x01;
			LCD_DisplayString(1, "system is armed!");
		break;
		
		default: break;
	}
	return state;
}
//Enumeration of states.


enum SM2_States { wait, transmitting };
//transmit FSM
int SMTick2(int state) {
	//State machine transitions
	switch (state) {
		case wait:
			if(USART_IsSendReady(0)){
				state = transmitting;
			}
			else{
				state = wait;
			}

		break;


		case transmitting:
			if(USART_HasTransmitted(0)){
				state = wait;
			}
			else{
				state = transmitting;
			}
		break;

		default: state = wait;
		break;
	}
	//State machine actions
	switch(state) {
		case wait:
		break;
		
		case transmitting:
			if(LED_val == 1){
				USART_Send(0x20, 0);
			}
			else{
				USART_Send(0x21, 0);
			}
		break;
		
		default: break;
	}
	return state;
}

// --------END User defined FSMs-----------------------------------------------
// Implement scheduler code from PES.
int main()
{

	DDRA = 0xFF; PORTA = 0x00; // PORTA set to output, outputs init 0s
	//DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s
	DDRC = 0xFF; PORTC = 0x00; // set as output for lcd
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	LCD_init();

	// . . . etc
	// Period for the tasks
	unsigned long int SMTick1_calc = 10000;
	unsigned long int SMTick2_calc = 10;
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc); //can only calculate 2 at a time. If more than 2 FSM run more that once
	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;

	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &SMTick1;//Function pointer for the tick.
	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &SMTick2;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	

	//code to initialize USART


	initUSART(0);     //init USART
	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		// Scheduler code ---------------------------------------------
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	} //---------------------------------------------------------------
	// Error: Program should not exit!
	return 0;
}