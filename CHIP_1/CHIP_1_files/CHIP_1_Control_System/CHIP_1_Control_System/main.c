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


unsigned char mode = 0; // 0 password to arm/disarm 1 password confirm for user pass 2 password confirm for random pass

//menu data
unsigned char keypad_val = 0x00;
unsigned short random_count = 0;
unsigned char in_main_menu = 0; //if 0 in main menu display temp
unsigned char pass_location = 0; //0-3 when entering 
unsigned char current_pass[4] = {1,2,3,4};
unsigned char user_input[4] = {' ', ' ', ' ', ' '};
unsigned char generated_pass[4] = {4, 3, 2, 1}; //{' ', ' ', ' ', ' '};


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
		current_pass[location] = 0;
	}
	else if(value == '1'){
		LCD_Cursor(17 + location);
		LCD_WriteData(1 + '0');
		current_pass[location] = 1;
	}
	else if(value == '2'){
		LCD_Cursor(17 + location);
		LCD_WriteData(2 + '0');
		current_pass[location] = 2;
	}
	else if(value == '3'){
		LCD_Cursor(17 + location);
		LCD_WriteData(3 + '0');
		current_pass[location] = 3;
	}
	else if(value == '4'){
		LCD_Cursor(17 + location);
		LCD_WriteData(4 + '0');
		current_pass[location] = 4;
	}
	else if(value == '5'){
		LCD_Cursor(17 + location);
		LCD_WriteData(5 + '0');
		current_pass[location] = 5;
	}
	else if(value == '6'){
		LCD_Cursor(17 + location);
		LCD_WriteData(6 + '0');
		current_pass[location] = 6;
	}
	else if(value == '7'){
		LCD_Cursor(17 + location);
		LCD_WriteData(7 + '0');
		current_pass[location] = 7;
	}
	else if(value == '8'){
		LCD_Cursor(17 + location);
		LCD_WriteData(8 + '0');
		current_pass[location] = 8;
	}
	else{
		LCD_Cursor(17 + location);
		LCD_WriteData(9 + '0');
		current_pass[location] = 9;
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
enum MENUState {init_menu, main_menu_armed, main_menu_disarmed, arm_or_disarm, pass_enter, reset_pass, choice, user_pass, user_pass_wait, random_pass} menu_state;

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
		PORTB = 0x00;
		if(keypad_val == '\0' ){ //null stay here
			menu_state = main_menu_disarmed;
			output_temp(received_value); //menu output with temp when disarmed
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
		PORTB = 0xFF;
		menu_state = main_menu_armed;
		if(keypad_val == '\0' || keypad_val == 'B' || keypad_val == 'C' || keypad_val == 'D' ){ //stay here
			menu_state = main_menu_armed;
			LCD_DisplayString(1, "system is armed!");
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
		else{						//some other invalid key is pressed
			menu_state = main_menu_armed;
			LCD_DisplayString(1, "system is armed!");
		}
		break;



		case arm_or_disarm:
		keypad_val = GetKeypadKey();
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
			menu_state = user_pass;
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
			menu_state = main_menu_disarmed;
		}
		else if(keypad_val == 'C'){ //user did not accept new passkey
			menu_state = main_menu_disarmed;
		}
		else{						//else some other invalid key is pressed do nothing
			menu_state = random_pass;
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
int main(void) 
{ 
   DDRA = 0xF0; PORTA = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s FOR KEYPAD
   DDRC = 0xFF; PORTC = 0x00; // set as output for lcd
   DDRD = 0xFF; PORTD = 0x00; // LCD control lines
   DDRB = 0xFF; PORTB = 0x00;

   //LCD init
   LCD_init();
   //LCD_DisplayString(1, Disarmed_LCD_data);
  
   //Start Tasks  
   //TransSecPulse(1);
   //RecSecPulse(1);

   MenuSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}