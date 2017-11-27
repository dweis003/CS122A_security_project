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
#include "matrix_print_function.h" //links other header files needed for matrix "print_matrix()"
//#include "eeprom.h"  //code taken from * Author: ExploreEmbedded* Website: http://www.exploreembedded.com/wiki
//#include "eeprom.c" 
 
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
//menu and tempout fsm data
unsigned char PassKey_LCD_data[32] = {'E', 'N', 'T', 'E', 'R', ' ', 'P', 'A', 'S', 'S', 'K', 'E', 'Y', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};



unsigned char Disarmed_LCD_data[32] = {'D', 'i', 's', 'a', 'r', 'm', 'e', 'd', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'T', 'e', 'm', 'p', ':',
'0', '0', ' ', 'C', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

unsigned char arm_LCD_data[32] = {'A', 'R', 'M', 'E', 'D', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

unsigned char pass_enter_LCD_data[32] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

unsigned char new_pass_enter_LCD_data[32] = {'E', 'N', 'T', 'E', 'R', ' ', '4', ' ', 'N', 'U', 'M', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

unsigned char random_pass_LCD_data[32] = {'R', 'A', 'N', 'D', 'O', 'M', ' ', 'P', 'A', 'S', 'S', 'K', 'E', 'Y', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};


unsigned char mode = 0; // 0 password to arm/disarm 1 password confirm for user pass 2 password confirm for random pass/ 3 for alarm tripped

//tripped sensor FSM
unsigned char sensors_tripped[5] = {0,0,0,0,0};
unsigned char trip_location = 0;
unsigned char sensor_delay_count = 0;
unsigned char sensor_trip = 0; //0 no trip 1 = trip;

//menu data
unsigned char keypad_val = 0x00;
unsigned short random_count = 0;
unsigned char in_main_menu = 0; //if 0 in main menu display temp
unsigned char pass_location = 0; //0-3 when entering 
unsigned char current_pass[4] = {1,2,3,4};
unsigned char user_pass_temp[4] = {0,0,0,0};
unsigned char user_input[4] = {' ', ' ', ' ', ' '};
unsigned char generated_pass[4] = {4, 3, 2, 1}; //{' ', ' ', ' ', ' '};

//data for matrix function
int matrix_array_lock[8][8] = {
	{0,0,0,1,1,0,0,0},
	{0,0,1,0,0,1,0,0},
	{0,1,0,0,0,0,1,0},
	{0,1,0,0,0,0,1,0},
	{0,1,1,1,1,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,1,1,1,1,0},
};

int matrix_array_unlock[8][8] = {
	{0,0,0,1,1,0,0,0},
	{0,0,1,0,0,1,0,0},
	{0,0,0,0,0,0,1,0},
	{0,0,0,0,0,0,1,0},
	{0,1,1,1,1,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,1,1,1,1,0},
};
//buzzer pwm data
unsigned char buzzer_counter = 0;
unsigned char buzzer_hign_period = 50;
unsigned char buzzer_low_period = 50;
unsigned char buzz_off_period = 250; 

//bluetooth USART FSM
unsigned char bluetooth_arm_disarm = 2; // 0 = disarm, 1 = arm, 2 = no valid input
unsigned char bluetooth_temp = 0x00; 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//functions
unsigned char tens_place = 0x00;
unsigned char ones_place = 0x00;
void alter_c_string(unsigned char number_val, int location){
	if(number_val == 0){
		Disarmed_LCD_data[location] = '0';
	}
	else if(number_val == 1){
		Disarmed_LCD_data[location] = '1';
	}
	else if(number_val == 2){
		Disarmed_LCD_data[location] = '2';
	}
	else if(number_val == 3){
		Disarmed_LCD_data[location] = '3';
	}
	else if(number_val == 4){
		Disarmed_LCD_data[location] = '4';
	}
	else if(number_val == 5){
		Disarmed_LCD_data[location] = '5';
	}
	else if(number_val == 6){
		Disarmed_LCD_data[location] = '6';
	}
	else if(number_val == 7){
		Disarmed_LCD_data[location] = '7';
	}
	else if(number_val == 8){
		Disarmed_LCD_data[location] = '8';
	}
	else{
		Disarmed_LCD_data[location] = '9';
	}

}

void output_temp(unsigned char temp_to_output){
	tens_place = temp_to_output/10;
	ones_place = temp_to_output - (tens_place*10); 
	alter_c_string(tens_place,21);
	alter_c_string(ones_place,22);
	LCD_DisplayString(1, Disarmed_LCD_data);
}

void enter_c_string(unsigned char number_val, int location){
	if(number_val == '0'){
		if(location == 0){
			user_input[location] = 0;
			PassKey_LCD_data[location + 16] = '0';
		}
		else{
			user_input[location] = 0;
			PassKey_LCD_data[location + 16] = '0';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '1'){
		if(location == 0){
			user_input[location] = 1;
			PassKey_LCD_data[location + 16] = '1';
		}
		else{
			user_input[location] = 1;
			PassKey_LCD_data[location + 16] = '1';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '2'){
		if(location == 0){
			user_input[location] = 2;
			PassKey_LCD_data[location + 16] = '2';
		}
		else{
			user_input[location] = 2;
			PassKey_LCD_data[location + 16] = '2';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '3'){
		if(location == 0){
			user_input[location] = 3;
			PassKey_LCD_data[location + 16] = '3';
		}
		else{
			user_input[location] = 3;
			PassKey_LCD_data[location + 16] = '3';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '4'){
		if(location == 0){
			user_input[location] = 4;
			PassKey_LCD_data[location + 16] = '4';
		}
		else{
			user_input[location] = 4;
			PassKey_LCD_data[location + 16] = '4';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '5'){
		if(location == 0){
			user_input[location] = 5;
			PassKey_LCD_data[location + 16] = '5';
		}
		else{
			user_input[location] = 5;
			PassKey_LCD_data[location + 16] = '5';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '6'){
		if(location == 0){
			user_input[location] = 6;
			PassKey_LCD_data[location + 16] = '6';
		}
		else{
			user_input[location] = 6;
			PassKey_LCD_data[location + 16] = '6';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '7'){
		if(location == 0){
			user_input[location] = 7;
			PassKey_LCD_data[location + 16] = '7';
		}
		else{
			user_input[location] = 7;
			PassKey_LCD_data[location + 16] = '7';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else if(number_val == '8'){
		if(location == 0){
			user_input[location] = 8;
			PassKey_LCD_data[location + 16] = '8';
		}
		else{
			user_input[location] = 8;
			PassKey_LCD_data[location + 16] = '8';
			PassKey_LCD_data[location + 15] = '*';
		}
	}
	else{
		if(location == 0){
			user_input[location] = 9;
			PassKey_LCD_data[location + 16] = '9';
		}
		else{
			user_input[location] = 9;
			PassKey_LCD_data[location + 16] = '9';
			PassKey_LCD_data[location + 15] = '*';
		}
	}

}

int check_password(){
	if(current_pass[0] == user_input[0] && current_pass[1] == user_input[1] && current_pass[2] == user_input[2]  && current_pass[3] == user_input[3]){
		return 1;
	}
	else{
		return 0;
	}
}

void output_for_user_pass_reset(unsigned char value, unsigned char location){
	if(value == '0'){
		LCD_Cursor(17 + location);
		LCD_WriteData(0 + '0');
		user_pass_temp[location] = 0; 
	}
	else if(value == '1'){
		LCD_Cursor(17 + location);
		LCD_WriteData(1 + '0');
		user_pass_temp[location] = 1;;  
	}
	else if(value == '2'){
		LCD_Cursor(17 + location);
		LCD_WriteData(2 + '0');
		user_pass_temp[location] = 2;
	}
	else if(value == '3'){
		LCD_Cursor(17 + location);
		LCD_WriteData(3 + '0');
		user_pass_temp[location] = 3;
	}
	else if(value == '4'){
		LCD_Cursor(17 + location);
		LCD_WriteData(4 + '0');
		user_pass_temp[location] = 4;
	}
	else if(value == '5'){
		LCD_Cursor(17 + location);
		LCD_WriteData(5 + '0');
		user_pass_temp[location] = 5;
	}
	else if(value == '6'){
		LCD_Cursor(17 + location);
		LCD_WriteData(6 + '0');
		user_pass_temp[location] = 6;
	}
	else if(value == '7'){
		LCD_Cursor(17 + location);
		LCD_WriteData(7 + '0');
		user_pass_temp[location] = 7;
	
	}
	else if(value == '8'){
		LCD_Cursor(17 + location);
		LCD_WriteData(8 + '0');
		user_pass_temp[location] = 8;
	}
	else{
		LCD_Cursor(17 + location);
		LCD_WriteData(9 + '0');
		user_pass_temp[location] = 9;
	}
}

void generate_random_pass(unsigned short seed){
	srand(seed);
	generated_pass[0] = rand() % 10; //gives value between 0 - 9
	generated_pass[1] = rand() % 10;
	generated_pass[2] = rand() % 10;
	generated_pass[3] = rand() % 10;
}
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
//output temp when disarmed
enum TEMPState {temp_wait, Display_temp} temp_state;

void Temp_Init(){
	temp_state = temp_wait;
}

void Temp_Tick(){
	//Actions
	switch(temp_state){
		case temp_wait:
		break;


		case Display_temp:
		output_temp(received_value);
		break;
		
		default:
		break;
	}
	//Transitions
	switch(temp_state){
		case temp_wait:
		if(ARM_DISARM == 1){ //system is armed do not output temp
			temp_state = temp_wait;
		}
		else if (ARM_DISARM ==  0 && in_main_menu == 0){ //only display temp if in main menu and disarmed
			temp_state = Display_temp;
		}
		else{
			temp_state = temp_wait;
		}
		break;


		case Display_temp:
		if(ARM_DISARM == 1){ //system is armed do not output temp
			temp_state = temp_wait;
		}
		else if(ARM_DISARM == 0 && in_main_menu == 0){
			temp_state = Display_temp;
		}
		else{
			temp_state = temp_wait;
		}
		break;
		
		default:
		temp_state = temp_wait;
		break;
	}

}


void TempSecTask()
{
	Temp_Init();
	for(;;)
	{
		Temp_Tick();
		vTaskDelay(250);
	}
}

void TempSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TempSecTask, (signed portCHAR *)"TempSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//menu FSM
enum MENUState {init_menu, main_menu_armed, main_menu_disarmed, arm_or_disarm, pass_enter, reset_pass, choice, user_pass, user_pass_wait, random_pass, sensor_is_tripped} menu_state;

void Menu_Init(){
	menu_state = init_menu;
}

void Menu_Tick(){
	//Actions
	switch(menu_state){
		case init_menu:
		break;

		case main_menu_armed:
		break;
		
		case main_menu_disarmed:
		break;

		case arm_or_disarm:
		break;

		case pass_enter:
		break;

		case reset_pass:
		break;

		case choice:
		break;

		case user_pass:
		break;

		case user_pass_wait:
		break;

		case random_pass:
		break;
		
		case sensor_is_tripped:
		break;
		
		default:
		break;
	}
	//Transitions
	switch(menu_state){
		case init_menu:
		//LCD_DisplayString(1, Disarmed_LCD_data);
		output_temp(received_value); //menu output with temp when disarmed
		menu_state = main_menu_disarmed;
		break;

		case main_menu_disarmed:
		keypad_val = GetKeypadKey();
		//PORTB = 0x00;
		//print_matrix(matrix_array_lock);

		if(keypad_val == '\0' ){ //null stay here
			if(bluetooth_arm_disarm == 2){   //bluetooth either never received data yet or received invalid do nothing
				menu_state = main_menu_disarmed;
				output_temp(received_value); //menu output with temp when disarmed
			}
			else if(bluetooth_arm_disarm == 0){ //bluetooth received disarm code, already disarmed so do nothing
				menu_state = main_menu_disarmed;
				bluetooth_arm_disarm = 2; //set back to null aka 2
				output_temp(received_value); //menu output with temp when disarmed
			}
			else{ //bluetooth received arm code go to armed mode
				menu_state = main_menu_armed;
				bluetooth_arm_disarm = 2; //set back to null aka 2
				ARM_DISARM = 1; //set to armed mode
				LCD_DisplayString(1, "system is armed!");
			}
		}
		else if(keypad_val == 'A'){
			menu_state = arm_or_disarm;
			LCD_DisplayString(1, PassKey_LCD_data);
			mode = 0;
			pass_location = 0;
			user_input[0] = NULL;
			user_input[1] = NULL;
			user_input[2] = NULL;
			user_input[3] = NULL;
			PassKey_LCD_data[16] = ' ';
			PassKey_LCD_data[17] = ' ';
			PassKey_LCD_data[18] = ' ';
			PassKey_LCD_data[19] = ' ';
			
		}
		else if(keypad_val == 'B'){
			menu_state = reset_pass;
			LCD_DisplayString(1, "* User Password or # Random Password");
			random_count = 0; //initialize to zero this count is for if a user decides to use a random passkey
		}
		else{					//any other button stay 
			menu_state = main_menu_disarmed;
		}
		break;

		case main_menu_armed:
		keypad_val = GetKeypadKey();
		//PORTB = 0xFF;
		menu_state = main_menu_armed;
		if(sensors_tripped[0] == 1 || sensors_tripped[1] == 1 ||sensors_tripped[2] == 1 ||sensors_tripped[3] == 1 ||sensors_tripped[4] == 1 ){
			menu_state = sensor_is_tripped;
			LCD_DisplayString(1, "Tripped Sensor(s)");
		}
		else if(keypad_val == '\0' || keypad_val == 'B' || keypad_val == 'C' || keypad_val == 'D' ){ //stay here and check bluetooth
			if(bluetooth_arm_disarm == 2){   //bluetooth either never received data yet or received invalid do nothing
				menu_state = main_menu_armed;
				LCD_DisplayString(1, "system is armed!");
			}
			else if(bluetooth_arm_disarm == 1){ //bluetooth received arm code, already armed so do nothing
				bluetooth_arm_disarm = 2; //set back to null aka 2
				menu_state = main_menu_armed;
				LCD_DisplayString(1, "system is armed!");
			}
			else{ //bluetooth received disarm code go to disarmed mode
				menu_state = main_menu_disarmed;
				bluetooth_arm_disarm = 2; //set back to null aka 2
				ARM_DISARM = 0; //set to disarmed mode
			}
		}
		else if(keypad_val == 'A'){ //enter passkey
			menu_state = arm_or_disarm;
			LCD_DisplayString(1, PassKey_LCD_data);
			mode = 0;
			pass_location = 0;
			user_input[0] = NULL;
			user_input[1] = NULL;
			user_input[2] = NULL;
			user_input[3] = NULL;
			PassKey_LCD_data[16] = ' ';
			PassKey_LCD_data[17] = ' ';
			PassKey_LCD_data[18] = ' ';
			PassKey_LCD_data[19] = ' ';
		}
		///////////////////////////////////////////////////////////////
		//added so armed state can now detect tripped sensors
		
		else{						//some other invalid key is pressed
			menu_state = main_menu_armed;
			LCD_DisplayString(1, "system is armed!");
			trip_location = 0; //reset location
		}
		break;



		case arm_or_disarm:
		keypad_val = GetKeypadKey();
		bluetooth_arm_disarm = 2; //invalidate any bluetooth input user has chosen to use keypad
		if(keypad_val == '\0' || keypad_val == 'A' || keypad_val == 'B' || keypad_val == '*'|| keypad_val == '#'){ //null stay here
			menu_state = arm_or_disarm;
			LCD_DisplayString(1, PassKey_LCD_data);
		}
		else if(keypad_val == 'C'){  //go back to main menu
			if(ARM_DISARM == 0){
				menu_state = main_menu_disarmed;
			}
			else{
				menu_state = main_menu_armed;
			}
			
		}
		else if(keypad_val == 'D'){ //enter button check password
			if(check_password() == 1 && mode == 0){ //password correct
				if(ARM_DISARM == 0){
					ARM_DISARM = 1;
					menu_state = main_menu_armed;

				}
				else{				//system was armed now unarmed
					ARM_DISARM = 0;
					menu_state = main_menu_disarmed;
				}

			}
			else if((check_password() == 0 && mode == 0)){
				if(ARM_DISARM == 0){
					menu_state = main_menu_disarmed;
				}
				else{
					menu_state = main_menu_armed;
				}
			}
			else if((check_password() == 1 && mode == 1)){
				LCD_DisplayString(1, "Enter 4 Numbers");
				LCD_Cursor(33); //move cursor off screen
				//LCD_Cursor(17);
				//LCD_WriteData(1 + '0');
				menu_state = user_pass;
				pass_location = 0;
				user_input[0] = NULL;
				user_input[1] = NULL;
				user_input[2] = NULL;
				user_input[3] = NULL;

			}
			else if((check_password() == 0 && mode == 1)){
				menu_state = reset_pass;
				LCD_DisplayString(1, "* User Password or # Random Password");
			}
			else if((check_password() == 1 && mode == 2)){
				menu_state = random_pass;
				//call function to generate pass key
				generate_random_pass(random_count);
				LCD_DisplayString(1, "Random passkey");
				LCD_Cursor(17);
				LCD_WriteData(generated_pass[0] + '0');
				LCD_Cursor(18);
				LCD_WriteData(generated_pass[1] + '0');
				LCD_Cursor(19);
				LCD_WriteData(generated_pass[2] + '0');
				LCD_Cursor(20);
				LCD_WriteData(generated_pass[3] + '0');
				LCD_Cursor(33); //position off screen 
			}
			else if((check_password() == 0 && mode == 2)){
				menu_state = reset_pass;
				LCD_DisplayString(1, "* User Password or # Random Password");
			}
			else if((check_password() == 1 && mode == 3)){
				menu_state = main_menu_disarmed; //user entered correct passkey disarm and go back to main menu
				ARM_DISARM = 0;
			}
			else if((check_password() == 0 && mode == 3)){
				menu_state = arm_or_disarm; //incorrect passkey try again
				//reset values for passkey input
				pass_location = 0;
				user_input[0] = NULL;
				user_input[1] = NULL;
				user_input[2] = NULL;
				user_input[3] = NULL;
				PassKey_LCD_data[16] = ' ';
				PassKey_LCD_data[17] = ' ';
				PassKey_LCD_data[18] = ' ';
				PassKey_LCD_data[19] = ' ';
			}

		}
		else{					//0-9 pressed 
			menu_state = pass_enter;
			if(pass_location < 4){
				enter_c_string(keypad_val, pass_location);
				++pass_location;
			}
		}

		break;

		case pass_enter:
		keypad_val = GetKeypadKey();
		if(keypad_val == '\0'){
			menu_state = arm_or_disarm;
		}
		else{
			menu_state = pass_enter;
		}
		break;

		case reset_pass:
		keypad_val = GetKeypadKey();
		++random_count; 
		if(keypad_val == '\0' || keypad_val == 'A'|| keypad_val == 'B' || keypad_val == 'D'){
			menu_state = reset_pass;
		}
		else if(keypad_val == 'C'){
			if(ARM_DISARM == 0){
				menu_state = main_menu_disarmed;
			}
			else{
				menu_state = main_menu_armed;
			}
		}
		else if(keypad_val == '*'){
			menu_state = arm_or_disarm;
			pass_location = 0;
			user_input[0] = NULL;
			user_input[1] = NULL;
			user_input[2] = NULL;
			user_input[3] = NULL;
			PassKey_LCD_data[16] = ' ';
			PassKey_LCD_data[17] = ' ';
			PassKey_LCD_data[18] = ' ';
			PassKey_LCD_data[19] = ' ';
			mode = 1;
		}
		else if(keypad_val == '#'){
			menu_state = arm_or_disarm;
			pass_location = 0;
			user_input[0] = NULL;
			user_input[1] = NULL;
			user_input[2] = NULL;
			user_input[3] = NULL;
			PassKey_LCD_data[16] = ' ';
			PassKey_LCD_data[17] = ' ';
			PassKey_LCD_data[18] = ' ';
			PassKey_LCD_data[19] = ' ';
			mode = 2;
		}
		else{ //invalid input stay
			menu_state = reset_pass;
		}
		break;

		case choice:
		break;

		case user_pass:
		keypad_val = GetKeypadKey();
		if(keypad_val == '\0' || keypad_val == 'A'|| keypad_val == 'B' || keypad_val == 'D' || keypad_val == '*' || keypad_val == '#'){
			if(pass_location == 4){
				menu_state = main_menu_disarmed;
				pass_location = 0;
				//update password
				current_pass[0] = user_pass_temp[0];
				eeprom_update_byte(0,current_pass[0]);
				current_pass[1] = user_pass_temp[1];
				eeprom_update_byte(1,current_pass[1]);
				current_pass[2] = user_pass_temp[2];
				eeprom_update_byte(2,current_pass[2]);
				current_pass[3] = user_pass_temp[3];
				eeprom_update_byte(3,current_pass[3]);
				//reset temp
				user_pass_temp[0] = 0;
				user_pass_temp[1] = 0;
				user_pass_temp[2] = 0;
				user_pass_temp[3] = 0;
			}
			else{
				menu_state = user_pass;
			}
		}
		else if(keypad_val == 'C'){
			menu_state = main_menu_disarmed;
		} 
		else{
		
			if(pass_location == 4){
				menu_state = main_menu_disarmed;
				pass_location = 0;
			}

			else{
				menu_state = user_pass_wait; //valid key pressed
				output_for_user_pass_reset(keypad_val, pass_location);
				++pass_location;
				

			}
			
		}

		break;

		case user_pass_wait:
		keypad_val = GetKeypadKey();
		if(keypad_val == '\0'){
			menu_state = user_pass;
		}
		else{
			menu_state = user_pass_wait;
		}
		break;

		case random_pass:
		keypad_val = GetKeypadKey();
		if(keypad_val == '\0'){
			menu_state = random_pass;
		}
		else if(keypad_val == '#'){ //user accepted new passkey
			current_pass[0] = generated_pass[0];
			current_pass[1] = generated_pass[1];
			current_pass[2] = generated_pass[2];
			current_pass[3] = generated_pass[3];
			//update eeprom
			eeprom_update_byte(0,current_pass[0]);  //address 0
			eeprom_update_byte(1,current_pass[1]);  //address 1
			eeprom_update_byte(2,current_pass[2]);  //address 2
			eeprom_update_byte(3,current_pass[3]);  //address 3
			menu_state = main_menu_disarmed;
		}
		else if(keypad_val == 'C'){ //user did not accept new passkey
			menu_state = main_menu_disarmed;
		}
		else{						//else some other invalid key is pressed do nothing
			menu_state = random_pass;
		}
		break;

		case sensor_is_tripped:
		menu_state = sensor_is_tripped;
		if(sensors_tripped[0] == 1){
			LCD_Cursor(18);
			LCD_WriteData(1 + '0');
			LCD_Cursor(33); //move off screen
		}
		if(sensors_tripped[1] == 1){
			LCD_Cursor(20);
			LCD_WriteData(2 + '0');
			LCD_Cursor(33); //move off screen
		}
		if(sensors_tripped[2] == 1){
			LCD_Cursor(22);
			LCD_WriteData(3 + '0');
			LCD_Cursor(33); //move off screen
		}
		if(sensors_tripped[3] == 1){
			LCD_Cursor(24);
			LCD_WriteData(4 + '0');
			LCD_Cursor(33); //move off screen
		}
		if(sensors_tripped[4] == 1){
			LCD_Cursor(26);
			LCD_WriteData(5 + '0');
			LCD_Cursor(33); //move off screen
		}
		keypad_val = GetKeypadKey();
		//add ability to reset via password input
		if(keypad_val == 'A'){ //user wants to input password to turn off alarm
			menu_state = arm_or_disarm;
			pass_location = 0;
			user_input[0] = NULL;
			user_input[1] = NULL;
			user_input[2] = NULL;
			user_input[3] = NULL;
			PassKey_LCD_data[16] = ' ';
			PassKey_LCD_data[17] = ' ';
			PassKey_LCD_data[18] = ' ';
			PassKey_LCD_data[19] = ' '; 
			mode = 3; 
		}
		else{                    //either no or invalid input user stays in this mode or check bluetooth for input
			if(bluetooth_arm_disarm == 2){   //bluetooth either never received data yet or received invalid do nothing
				menu_state = sensor_is_tripped;
			}
			else if(bluetooth_arm_disarm == 1){ //bluetooth received arm code, already armed so do nothing
				bluetooth_arm_disarm = 2; //set back to null aka 2
				menu_state = sensor_is_tripped;
			}
			else{ //bluetooth received disarm code go to disarmed mode
				menu_state = main_menu_disarmed;
				bluetooth_arm_disarm = 2; //set back to null aka 2
				ARM_DISARM = 0; //set to disarmed mode
			}

		}
		
		
		break;

		
		default:
		break;
	}

}


void MenuSecTask()
{
	Menu_Init();
	for(;;)
	{
		Menu_Tick();
		vTaskDelay(10);
	}
}

void MenuSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MenuSecTask, (signed portCHAR *)"MenuSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//detect triggered sensors when armed
enum SENSEState {sense_init, sense_wait, sense_detect} sense_state;

void sense_Init(){
	sense_state = sense_wait;
}

void Sense_Tick(){
	//Actions
	switch(sense_state){
		case sense_init:
		//do nothing
		sensor_trip = 0;
		break;

		case sense_wait:
		//do nothing
		break;


		case sense_detect:
		//check received value from CHIP0
		if(GetBit(received_value,0) == 1){
			sensors_tripped[0] = 1;
			sensor_trip= 1;
		}
		if(GetBit(received_value,1) == 1){
			sensors_tripped[1] = 1;
			sensor_trip = 1;
		}
		if(GetBit(received_value,2) == 1){
			sensors_tripped[2] = 1;
			sensor_trip = 1;
		}
		if(GetBit(received_value,3) == 1){
			sensors_tripped[3] = 1;
			sensor_trip = 1;
		}
		if(GetBit(received_value,4) == 1){
			sensors_tripped[4] = 1;
			sensor_trip = 1;
		}
		break;
		
		default:
		break;
	}
	//Transitions
	switch(sense_state){
		case sense_init:
		if(ARM_DISARM == 1){
			sense_state = sense_wait;
		}
		else{
			sense_state = sense_init;
			sensors_tripped[0] = 0;
			sensors_tripped[1] = 0;
			sensors_tripped[2] = 0;
			sensors_tripped[3] = 0;
			sensors_tripped[4] = 0;
			sensor_delay_count = 0;
		}
		break;

		case sense_wait:
			++sensor_delay_count;
			if(ARM_DISARM == 0){			//if disarmed go back to init
				sense_state = sense_init;
				sensor_delay_count = 0; //initialize
			}
			else if(sensor_delay_count < 10){ //let chip 0 have time to send correct data
				sense_state = sense_wait;
			}
			else{ //ready
				sense_state = sense_detect;
				//reset values
				sensors_tripped[0] = 0;
				sensors_tripped[1] = 0;
				sensors_tripped[2] = 0;
				sensors_tripped[3] = 0;
				sensors_tripped[4] = 0;
				sensor_delay_count = 0;
			}
		break;


		case sense_detect:
			if (ARM_DISARM == 1){ //system is armed detect tripped sensors
				sense_state = sense_detect;
			}
			else{
				sense_state = sense_init;
			}
		break;
		
		default:
			sense_state = sense_init;
		break;
	}

}


void SenseSecTask()
{
	sense_Init();
	for(;;)
	{
		Sense_Tick();
		vTaskDelay(200);
	}
}

void SenseSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(SenseSecTask, (signed portCHAR *)"SenseSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//detect triggered sensors when armed
enum MATRIXState {output_matrix} matrix_state;

void matrix_Init(){
	matrix_state = output_matrix;
}

void Matrix_Tick(){
	//Actions
	switch(matrix_state){
		case output_matrix:
		if(ARM_DISARM == 1){ //2 images selected depending on whether armed or disarmed
			print_matrix(matrix_array_lock);
		}
		else{
			print_matrix(matrix_array_unlock);
		}
		break;

		default:
		break;
	}
	//Transitions
	switch(matrix_state){
		case output_matrix:
		matrix_state = output_matrix;

		default:
		matrix_state = output_matrix;
		break;
	}

}
void MatrixSecTask()
{
	matrix_Init();
	for(;;)
	{
		Matrix_Tick();
		vTaskDelay(1);
	}
}

void MatrixSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MatrixSecTask, (signed portCHAR *)"MatrixSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bluetooth RECEIVE FSM
enum BLUERECState {Blue_Rec_Wait, Blue_Receive_State } blue_rec_state;

void BLUE_REC_Init(){
	blue_rec_state = Blue_Rec_Wait;
}

void Blue_Rec_Tick(){
	//Actions
	switch(blue_rec_state){
		case Blue_Rec_Wait:
		break;


		case Blue_Receive_State:
		//get data from bluetooth
		//bluetooth_arm_disarm = USART_Receive(1);
		bluetooth_temp = USART_Receive(1);
		if(bluetooth_temp == 0x0F){	//received arm command
			bluetooth_arm_disarm = 1;
		}
		else if(bluetooth_temp == 0xF0){ //received disarm command
			bluetooth_arm_disarm = 0; 
		}
		else{
			bluetooth_arm_disarm = 2;  //random or invalid data set to 2
		}
		
		USART_Flush(1); //flush so flag reset
		break;
		
		default:
		break;
	}
	//Transitions
	switch(blue_rec_state){
		case Blue_Rec_Wait:
		if(USART_HasReceived(1)){
			blue_rec_state = Blue_Receive_State; //if ready go to next state
		}
		else{
			blue_rec_state = Blue_Rec_Wait; //if not ready to receive data stay
		}
		break;


		case Blue_Receive_State:
		blue_rec_state = Blue_Rec_Wait; //go back
		break;
		
		default:
		blue_rec_state = Blue_Rec_Wait;
		break;
	}

}


void BlueRecSecTask()
{
	BLUE_REC_Init();
	for(;;)
	{
		Blue_Rec_Tick();
		vTaskDelay(10);
	}
}

void BlueRecSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BlueRecSecTask, (signed portCHAR *)"BlueRecSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//buzz when sensor tripped
enum BUZZERState {buzz_wait, no_buzz, buzz_high, buzz_low} buzzer_state;

void buzz_Init(){
	buzzer_state = buzz_wait;
}

void Buzz_Tick(){
	//Actions
	switch(buzzer_state){
		case buzz_wait:
		break;

		case no_buzz:
			++buzzer_counter;
		break;

		case buzz_high:
			++buzzer_counter;
			PORTB = PORTB|0x10;  //set PIN B4 to 1
		break;

		case buzz_low:
			++buzzer_counter;    //set PIN B4 to 0
			PORTB = PORTB & 0xEF; 
		break;

		default:
		break;
	}
	//Transitions
	switch(buzzer_state){
		case buzz_wait:
		if(ARM_DISARM == 0){
			buzzer_state = buzz_wait;
		}
		else if(sensor_trip == 1 && ARM_DISARM == 1){
			buzzer_state = no_buzz;
			buzzer_counter = 0; //initialize counter
		}
		else if(sensor_trip == 0 && ARM_DISARM == 1){
			buzzer_state = buzz_wait;
			buzzer_counter = 0; //initialize counter
		}
		break;

		case no_buzz:            //if disarmed no buzz go back
		if(ARM_DISARM == 0){
			buzzer_state = buzz_wait;
		}						//if counter less than off period stay
		else if(buzzer_counter < buzz_off_period){
			buzzer_state = no_buzz;
		}
		else{					//else go to buzzer high
			buzzer_state = buzz_high;
			buzzer_counter = 0; //initialize counter
		}
		break;

		case buzz_high:
		if(buzzer_counter < buzzer_hign_period){
			buzzer_state = buzz_high;
		}
		else{					//else go to buzzer high
			buzzer_state = buzz_low;
			buzzer_counter = 0; //initialize counter
		}
		break;

		case buzz_low:
		if(ARM_DISARM == 0){    //if system is disarmed turn off alarm
			buzzer_state = buzz_wait;
		}
		else if(buzzer_counter < buzzer_low_period){
			buzzer_state = buzz_low;
		}
		else{					//else go to buzzer high
			buzzer_state = buzz_high;
			buzzer_counter = 0; //initialize counter
		}
		break;

		default:
		buzzer_state = buzz_wait;
		break;
	}

}


void BuzzSecTask()
{
	buzz_Init();
	for(;;)
	{
		Buzz_Tick();
		vTaskDelay(1);
	}
}

void BuzzSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BuzzSecTask, (signed portCHAR *)"BuzzSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void) 
{ 
   DDRA = 0xF0; PORTA = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s FOR KEYPAD
   DDRC = 0xFF; PORTC = 0x00; // set as output for lcd
   DDRD = 0xFF; PORTD = 0x00; // LCD control lines
   DDRB = 0xFF; PORTB = 0x00;
   //EEPROM
  //eeprom_write_byte(0,1);  //address 0
  //eeprom_write_byte(1,2);  //address 1
  //eeprom_write_byte(2,3);  //address 2
  //eeprom_write_byte(3,4);  //address 3

  current_pass[0] = eeprom_read_byte(0);
  current_pass[1] = eeprom_read_byte(1);
  current_pass[2] = eeprom_read_byte(2);
  current_pass[3] = eeprom_read_byte(3);

  unsigned char temp = eeprom_read_byte(0);

   //LCD init
   LCD_init();
   //init USART
   initUSART(0); //used to communicate to CHIP 0
   initUSART(1); //used to communicate via bluetooth  
  
  
   //Start Tasks  
   TransSecPulse(1);
   RecSecPulse(1);
   MatrixSecPulse(1);
   MenuSecPulse(1);
   SenseSecPulse(1);
   BuzzSecPulse(1);
   BlueRecSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}