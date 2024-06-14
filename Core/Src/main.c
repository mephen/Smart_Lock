/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

#include "mmc_sd.h"
#include "fatfs.h"
#include "fops.h"
#include <string.h>
//#include "main.h"
#include "FreeRTOS.h"

#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "File_Handling.h"
#include "waveplayer.h"

#include "stdio.h" 
#include "stdlib.h" 
#include "rc522.h"
#include "string.h" 
#include "liquidcrystal_i2c.h"
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#define RXBUFFERSIZE  256
//char RxBuffer[RXBUFFERSIZE];
//#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
GPIO_TypeDef* R1_PORT = GPIOE;
GPIO_TypeDef* R2_PORT = GPIOE;
GPIO_TypeDef* R3_PORT = GPIOE;
GPIO_TypeDef* R4_PORT = GPIOE;
GPIO_TypeDef* C1_PORT = GPIOE;
GPIO_TypeDef* C2_PORT = GPIOE;
GPIO_TypeDef* C3_PORT = GPIOE;
GPIO_TypeDef* C4_PORT = GPIOE;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define R1_PIN GPIO_PIN_7
#define R2_PIN GPIO_PIN_8
#define R3_PIN GPIO_PIN_9
#define R4_PIN GPIO_PIN_10
#define C1_PIN GPIO_PIN_11
#define C2_PIN GPIO_PIN_12
#define C3_PIN GPIO_PIN_13
#define C4_PIN GPIO_PIN_14
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16
uint8_t sNum[5];
TaskHandle_t xHandle_1 = NULL;
TaskHandle_t xHandle_2 = NULL;
TaskHandle_t xHandle_lock_task = NULL;
TaskHandle_t xHandle_unlock_task = NULL;
  //registered card linked list
card *list_start = NULL;
card *list_end = NULL;

char key;
char enteredPin[5];
uint8_t entered = 0;
const char setPin[4] = {'1', '2', '3', '4'};
uint8_t newPinSet = 0;
QueueHandle_t keypadQueue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
//extern ApplicationTypeDef Appli_state;
extern AUDIO_PLAYBACK_StateTypeDef AudioState;
SemaphoreHandle_t xSemaphore;
uint8_t flag = pdTRUE;
uint8_t isFinished = 0;
int k=0;
int recvt;
QueueHandle_t xQue1;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//UART_HandleTypeDef UartHandle;
/* Private function prototypes -----------------------------------------------*/
char read_keypad(void)
{
    char keys[4][4] = {{'1', '2', '3', 'A'},
                       {'4', '5', '6', 'B'},
                       {'7', '8', '9', 'C'},
                       {'*', '0', '#', 'D'}};

    for (int i = 0; i < 4; i++)
    {
        // Set one column to LOW and others to HIGH
        HAL_GPIO_WritePin(C1_PORT, C1_PIN, (i == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(C2_PORT, C2_PIN, (i == 1) ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(C3_PORT, C3_PIN, (i == 2) ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(C4_PORT, C4_PIN, (i == 3) ? GPIO_PIN_RESET : GPIO_PIN_SET);
//         printf("Row 1 Pin State: %d\n", HAL_GPIO_ReadPin(R1_PORT, R1_PIN));

        // Check each row
        if (HAL_GPIO_ReadPin(R1_PORT, R1_PIN) == GPIO_PIN_RESET) {
            HAL_Delay(20); // Debounce delay
            if (HAL_GPIO_ReadPin(R1_PORT, R1_PIN) == GPIO_PIN_RESET) { // Check again
                return keys[i][0];
            }
        }
        if (HAL_GPIO_ReadPin(R2_PORT, R2_PIN) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(R2_PORT, R2_PIN) == GPIO_PIN_RESET) {
                return keys[i][1];
            }
        }
        if (HAL_GPIO_ReadPin(R3_PORT, R3_PIN) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(R3_PORT, R3_PIN) == GPIO_PIN_RESET) {
                return keys[i][2];
            }
        }
        if (HAL_GPIO_ReadPin(R4_PORT, R4_PIN) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(R4_PORT, R4_PIN) == GPIO_PIN_RESET) {
                return keys[i][3];
            }
        }
    }

//    printf("Key read: %c\n", key);
    return '\0';
}

//search card in the linked list stored in the RAM of board
uint8_t search_card_list_Global(uint8_t *target_cardVal)
{
  uint8_t card_exist = 0;
  card *current = list_start;
  while(current != NULL){
    if(current->data[0] == target_cardVal[0] && current->data[1] == target_cardVal[1] && current->data[2] == target_cardVal[2] && current->data[3] == target_cardVal[3] && current->data[4] == target_cardVal[4]){
      card_exist = 1;
      break;
    }
    current = current->next;
  }
  return card_exist;
}

//search card in the linked list stored in the SD card
//not finished
uint8_t search_card_list_SD(uint8_t *target_cardVal){
  FIL file_be_read;

  if (f_open(&file_be_read, "test.txt", FA_READ) != 0) {
    printf("X:open file err in search_card_list_SD\r\n");
  } else {
    printf("O:open file ok in search_card_list_SD\r\n");
  }

  //get total_card number in the SD
  char total_card_str[11+2+1] = "None\n\r";//11 for "total_card:", 2 for total_card, 1 for'\0'
  UINT bytesread;
  f_read(&file_be_read, total_card_str, sizeof(total_card_str), &bytesread);
  printf (total_card_str); //debug

  uint8_t total_card = 0;
  sscanf(total_card_str, "total_card:%d", &total_card); //return 1 if success
  printf("%d\r\n", total_card); //debug

  f_close(&file_be_read);

  //todo: read card value from SD card

  return 0;
}

void update_card_to_SD(){
  //count total card in the linked list
  uint8_t total_card = 0;
  uint8_t value_num = 5;//each card has 5 values
  card *cur = list_start;
  while(cur != NULL){
    total_card++;
    cur = cur->next;
  }

  //transfer uint8_t card_value to string, and store in array:cardVal
  char cardVal[total_card * value_num][3+1+1];//store string transfered from uint8 card_value, and assume each card_value won't exceed 3 digits, and 1 for '\0', 1 for ","
  memset(cardVal, 0, sizeof(cardVal));
  cur = list_start;
  uint8_t card_index = 0;
  while(cur != NULL){
    for(int i = 0; i < value_num; i++){
      sprintf(cardVal[i+card_index], "%d", cur->data[i]);
      if(i != value_num - 1){
        strcat(cardVal[i+card_index], ",");
      }
    }
    card_index += value_num;
    cur = cur->next;
  }
  
  //concatenate all value string into a single string
  char temp[(2+1)];//assume total_card won't exceed 2 digits
  memset(temp, 0, sizeof(temp));
  sprintf(temp, "%d", total_card);
  char total_card_str[11+2+1+2];//11 for "total_card:", 2 for total_card, 1 for '\0', 2 for "\r\n"
  memset(total_card_str, 0, sizeof(total_card_str));
  strcat(total_card_str, "total_card:");
  strcat(total_card_str, temp);
  strcat(total_card_str, "\r\n");

  char to_write[sizeof(total_card_str) + total_card * value_num * (3+1+2)];//3 for value, 1 for ",", 2 for "\r\n"
  memset(to_write, 0, sizeof(to_write));
  strcat(to_write, total_card_str);
  for(int i = 0; i < total_card * value_num; i++){
    strcat(to_write, cardVal[i]);
    if((i+1)%5 == 0){//5-th value of a card
      strcat(to_write, "\r\n");
    }
  }
  printf(to_write); //debug

  //overwrite card_information to SD card
  FIL writed_file;
  if (f_open(&writed_file, "test.txt", FA_WRITE) != 0) {
    printf("X:open file err in update_card_to_SD\r\n");
  } else {
    printf("O:open file ok in update_card_to_SD\r\n");
  }
  UINT byteswrite;
  f_write(&writed_file, to_write, sizeof(to_write)+1, &byteswrite);
  f_close(&writed_file);
}

//add a new card into the linked list, and update SD card
void rc522_add_card(){
  HAL_GPIO_WritePin(GPIOD,LED_Blue_Pin,1);
  //reset str before reading
  for(int cardVal = 0; cardVal < 5; cardVal++){
    str[cardVal] = 0;
  }
  //if a card is read, str[0~4] will not be 0
  //repeat reading until a card is read
  while(!(str[0] != 0 && str[1] != 0 && str[2] != 0 && str[3] != 0 && str[4] != 0)){
    status = MFRC522_Request(PICC_REQIDL, str);
    status = MFRC522_Anticoll(str);
  }
  //check if the readed card already exists in the linked list
  //if it exists, keep reading until a new card is read
  uint8_t card_exist = search_card_list_Global(str);
  while(card_exist){
    //reset str before reading
    for(int cardVal = 0; cardVal < 5; cardVal++){
      str[cardVal] = 0;
    }
    while(!(str[0] != 0 && str[1] != 0 && str[2] != 0 && str[3] != 0 && str[4] != 0)){
      status = MFRC522_Request(PICC_REQIDL, str);
      status = MFRC522_Anticoll(str);
    }
    card_exist = search_card_list_Global(str);
  }
  //add the new card to the list, don't forget to free this memory in delete_card fn.
  card *new_card = (card *)malloc(sizeof(card));
  new_card->next = NULL;
  for(int cardVal = 0; cardVal < 5; cardVal++){
    new_card->data[cardVal] = str[cardVal];
  }
  if(list_start == NULL){//the list is empty
    list_start = new_card;
    list_end = new_card;
  }else if (list_start -> next == NULL){//there is only one card in the list
    list_start->next = new_card;
    list_end = new_card;
  }else{//there are >= 2 cards in the list
    list_end->next = new_card;
    list_end = new_card;
  }
  HAL_GPIO_WritePin(GPIOD,LED_Blue_Pin,0);

  //update registered_card_linked_list to SD card.(overwrite)
  // update_card_to_SD();
}

//assume the target card is in the list
//delete the target card from the linked list, and update SD card
void rc522_delete_card(uint8_t *target_card_value){
  card *cur = list_start;
  card *prev = NULL;
  while(cur != NULL){
    if(cur->data[0] == target_card_value[0] && cur->data[1] == target_card_value[1] && cur->data[2] == target_card_value[2] && cur->data[3] == target_card_value[3] && cur->data[4] == target_card_value[4]){
      break; //found
    }
    prev = cur;
    cur = cur->next;
  }
  if(cur == list_start){//target is the first card
    list_start = list_start->next;
    free(cur);
    cur == NULL;
  }else if(cur == list_end){//target is the last card
    list_end = prev;
    prev->next = NULL;
    free(cur);
    cur == NULL;
  }else{//target is in the middle
    prev->next = cur->next;
    free(cur);
    cur == NULL;
  }
  //update registered_card_linked_list to SD card.(overwrite)
  // update_card_to_SD();
}

void rc522_test(void *pvParameters)
{
  //test
  // rc522_add_card();
  // uint8_t del[5] = {211,113,208,2,112};
  // rc522_delete_card(del);
  while(1){
    //it's possible that no card is read
    status = MFRC522_Request(PICC_REQIDL, str);
    status = MFRC522_Anticoll(str);
    memcpy(sNum, str, 5); 
    uint8_t led_state = 0;

    // test
    // if((str[0]==211) && (str[1]==113) && (str[2]==208) && (str[3]==2) && (str[4]==112) )
    // {
    //   // HAL_GPIO_WritePin(GPIOD,LED_Orange_Pin,0);
    //   led_state = 0;
    //   HAL_Delay(100);
    //   }
    // else if((str[0]==106) && (str[1]==73) && (str[2]==5) && (str[3]==128) && (str[4]==166) )
    //   {
    //   // HAL_GPIO_WritePin(GPIOD,LED_Orange_Pin,0);
    //   led_state = 0;
    //   HAL_Delay(200);
    // }
    // else
    // {
    //   // HAL_GPIO_WritePin(GPIOD,LED_Orange_Pin,1);
    //   led_state = 1;
    // }

    //test: os will crash when readiing SD card first.(i.e. Not write to SD card first)
    //search_card_list_SD(str);

    uint8_t card_exist = search_card_list_Global(str);
    if(card_exist){
      led_state = 0;
    }else{
      led_state = 1;
    }
    HAL_GPIO_WritePin(GPIOD,LED_Orange_Pin,led_state);
    HAL_Delay(300);
  }
}

void Task1(void *pvParameters);
void Task2(void *pvParameters);
void Task3(void *pvParameters);

void Task1(void *pvParameters) {
	uint8_t count=0;
	for (;;)
	{
		FIL test;
    //task1 should wait task 2 finish reading
		if (xSemaphoreTake(xSemaphore,( TickType_t ) 10) == pdTRUE)
		{
			if (f_open(&test, "TEST.txt", FA_WRITE) != 0) {
				printf("open file err in task1\r\n");
			} else {
				printf("file open ok in task1\r\n");
			}

			char buff[13];
      char append[24];
			memset(buff, 0, 13);
      memset(append, 0, 24);
			if(count%2){
				sprintf(buff, "%s", "DataWriteT");//11 char + '\0' = 12 bytes to write
			}else{
				sprintf(buff, "%s", "DataWriteF");
			}
			UINT byteswrite;
      strcat(append, buff);
      strcat(append, "\n");
      strcat(append, "test_append");
			f_write(&test, append, sizeof(append)+1, &byteswrite);
      f_close(&test);
			
      count++;
			xSemaphoreGive(xSemaphore);
		}

		vTaskDelay(1000);
	}

}

void Task2(void *pvParameters) {
	while (1)
	{
		FIL test_1;
    //task2 should wait task 1 finish writing
		if (xSemaphoreTake(xSemaphore,( TickType_t ) 10) == pdTRUE)
		{
			if (f_open(&test_1, "TEST.txt", FA_READ) != 0) {
				printf("open file err in task2\r\n");
			} else {
				printf("file open ok in task2\r\n");
			}

			char buff[25];
			memset(buff, 0, 25);
			UINT bytesread;
			f_read(&test_1, buff, 25, &bytesread);//'\0' is not necessary to read

			printf("data read is %s in task 2\r\n", buff);

			f_close(&test_1);
			xSemaphoreGive(xSemaphore);
		}
		vTaskDelay(1000);
	}
}

void Task3(void *pvParameters) {
	for (;;) {
		int isFinished = 0;
		TickType_t xDelay = 10 / portTICK_PERIOD_MS;
		AUDIO_PLAYER_Start(0);
		while(!isFinished){
			AUDIO_PLAYER_Process(pdTRUE);
			if(AudioState == AUDIO_STATE_STOP){
				isFinished = 1;
			}
      int opt = 0;
			if(xQueueReceive(xQue1,&opt,(TickType_t)3) == pdTRUE){
				AudioState=AUDIO_STATE_NEXT;
				vTaskDelay(xDelay);
			}
		}
	}
}

int debounce(int state){
  //pressed
  if(state){
    //wait for some time to debounce
    vTaskDelay(25);
    //check if still pressed
    if(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0)){
      //wait for button to be released
      while(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0)){;}
      return 1;//pressed
    }
  }
  return 0;//not pressed
}

void PushButton(){
	for(;;){
		int opt=1;
    
		if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){
      xQueueSend(xQue1,&opt,(TickType_t)3);
      vTaskDelay(25);
    }
	}
}

//not relative to lab5
char str_c[30][100] = {
		"Thinking about y                        ou",
		"As I'm lying nex                        t to someone else",
		"Drinking without                        you",
		"Doesn't fix me b                        ut it kinda helps",
		"Oh I still wish                         we could go back to where we started",
		"OH, OH, OH, OH,                         OH",
		"OH, OH, OH, OH,                         OH",
		"I'M GAZELLE. WEL                        COME TO ZOOTOPIA.",
		"OH, OH, OH, OH,                         OH",
		"OH, OH, OH, OH,                         OH",
		"I MESSED UP TONI                        GHT",
		};

char str_lcd[16] = "Hello World!";//one line of lcd contains 16 char
char str_test[16] = "world hello!";
//not relative to lab5
void lcd_test(void *pvParameters){
  int count = 0;
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
	HAL_Delay(100);
	while(1) {
    if(count%2 ==0){
      HD44780_PrintStr(str_lcd);
      HD44780_SetCursor(0,1);//move cursor to the first word of the second line
      HD44780_PrintStr(str_test);
    }else{
      HD44780_PrintStr(str_test);
    }
    HAL_Delay(1000);
    HD44780_Clear();
    count++;
		// size_t numElements_c = sizeof(str_c) / sizeof(str_c[0]);
		// for(int x=0;x<numElements_c;x++){
		// 	HD44780_PrintStr(str_c[x]);
		// 	if(x<5){
		// 		HAL_Delay(1000);
		// 		HD44780_Clear();
		// 	}else{
		// 		HAL_Delay(2000);
		// 		HD44780_Clear();
		// 	}
		// }
	}

//	HD44780_Clear();
//	HAL_Delay(1300);
//	size_t numElements_m = sizeof(str_m) / sizeof(str_m[0]);
//	for(int x=0;x<numElements_m;x++){
//		HD44780_PrintStr(str_c[x]);
//		HAL_Delay(3000);
//		HD44780_Clear();
//	}

//	HD44780_Clear();
//	HAL_Delay(1000);
//	size_t numElements_t = sizeof(str_t) / sizeof(str_t[0]);
//	for(int x=0;x<numElements_t;x++){
//		HD44780_PrintStr(str_c[x]);
//		HAL_Delay(3000);
//		HD44780_Clear();
//	}


//	  HD44780_SetCursor(0,0);
//	  size_t len = strlen(str_c[0]);
//	  size_t numElements = sizeof(str_c) / sizeof(str_c[0]);
//
//	  for(int j=0;j<numElements;j++){
//		  HD44780_PrintStr(str_c[j]);
//		  HAL_Delay(500);
//		  for(int x=0;x<len;x++){
			  // HD44780_ScrollDisplayLeft();
//			  HAL_Delay(500);
//		  }
//		  HD44780_Clear();
//	  }



//
//	  HD44780_Clear();
//	  HD44780_SetCursor(0,0);
//	  HD44780_PrintStr("HELLO");
//	  HAL_Delay(2000);
//	  HD44780_NoBacklight();
//	  HAL_Delay(2000);
//	  HD44780_Backlight();
//
//	  HAL_Delay(2000);
//	  HD44780_Cursor();
//	  HAL_Delay(2000);
//	  HD44780_Blink();
//	  HAL_Delay(5000);
//	  HD44780_NoBlink();
//	  HAL_Delay(2000);
//	  HD44780_NoCursor();
//	  HAL_Delay(2000);
//
//	  HD44780_NoDisplay();
//	  HAL_Delay(2000);
//	  HD44780_Display();
//
//	  HD44780_Clear();
//	  HD44780_SetCursor(0,0);
//	  HD44780_PrintStr("Learning STM32 with LCD is fun :-)");
//	  for(int x=0; x<40; x=x+1)
//	  {
// HD44780_ScrollDisplayLeft();  //HD44780_ScrollDisplayRight();
//	    HAL_Delay(500);
//	  }
//
//	  char snum[5];
//	  for ( int x = 1; x <= 200 ; x++ )
//	  {
//	    itoa(x, snum, 10);
//	    HD44780_Clear();
//	    HD44780_SetCursor(0,0);
//	    HD44780_PrintStr(snum);
//	    HAL_Delay (1000);
//	  }
}

uint8_t rc522_check(){
  status = MFRC522_Request(PICC_REQIDL, str);
  status = MFRC522_Anticoll(str);
  memcpy(sNum, str, 5); 
  uint8_t card_exist = search_card_list_Global(str);
  return card_exist;
}

uint8_t keypad_check() {
    printf("Entered keypad_check()\n");  
    char enteredPin[5] = {0};
    uint8_t entered = 0;
    char key;
    bool isButtonPressed = false;
    bool startInput = false;  // Flag to indicate whether to start PIN input

    HD44780_Clear();
    HD44780_SetCursor(0, 0);
    HD44780_PrintStr("Ready");
    HAL_Delay(20);

    while (entered < 4) {
        key = read_keypad();  // Read key input

        if (key != '\0' && !isButtonPressed) {
            printf("Entered key: %c\n", key);
            if (!startInput) {
                HD44780_Clear();  
                HD44780_SetCursor(0, 0);
                HD44780_PrintStr("Enter PIN:");
                startInput = true;  // Start showing PIN input screen and accepting PIN input
            }

            if (key >= '0' && key <= '9' && entered < 4) {
                strncat(enteredPin, &key, 1);
                entered++;

                HD44780_SetCursor(11 + entered - 1, 0);
                HD44780_PrintStr("*");  // Print each '*' as it is entered
                printf(key);
            } else if (key == 'C' && entered > 0) {
                enteredPin[--entered] = '\0';
                HD44780_SetCursor(11 + entered, 0);
                HD44780_PrintStr(" ");  // Clear the '*' from the display
            }

            isButtonPressed = true;  // Mark the button as pressed
        } else if (key == '\0') {
            isButtonPressed = false;  // Reset button press state
        }
    }

    if (entered == 4) {
        if (strncmp(enteredPin, setPin, 4) == 0) {
            HD44780_Clear();
            HD44780_SetCursor(0, 0);
            HD44780_PrintStr("UNLOCKED");
            return 1;  // PIN is correct
        } else {
            HD44780_SetCursor(0, 1);
            HD44780_PrintStr("Wrong PIN. Try again.");
            return 0;  // PIN is incorrect
        }
    }
    return 0;  // Default return if less than 4 digits were entered
}

uint8_t bluetooth_check(){
	uint8_t rx_data = 0;
	if(HAL_UART_Receive(&huart2,&rx_data,1, 300) == HAL_OK){
		if(rx_data == '1'){
			return 1;
		}
		else{
			return 0;
		}
	}
	return 0;
}

// uint8_t unlock_fn_AddCard(){
//   char add_Card_str[16] = "Add Card";
//   HD44780_SetCursor(0,1);//move cursor to the first word of the second line
//   HD44780_PrintStr(add_Card_str);
//   uint32_t From_begin_time = HAL_GetTick();
//   while(HAL_GetTick() - From_begin_time < 1000/portTICK_RATE_MS){ //busy waiting 1000 ms, can't use vTaskDelay
//     if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){//push button
//       rc522_add_card();
//       return 1;//do rc522_add_card
//     }
//   }
//   return 0;//do nothing
// }

// uint8_t unlock_fn_DelCard(){
//   char del_Card_str[16] = "Del Card";
//   HD44780_SetCursor(0,1);
//   HD44780_PrintStr(del_Card_str);
//   uint32_t From_begin_time = HAL_GetTick();
//   while(HAL_GetTick() - From_begin_time < 1000/portTICK_RATE_MS){ 
//     if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){
//       uint8_t del[5] = {211,113,208,2,112};
//       rc522_delete_card(del);
//       return 1;//do rc522_delete_card
//     }
//   }
//   return 0;//do nothing
// }

// uint8_t unlock_fn_Lock(){
//   char del_Card_str[16] = "LOCK!";
//   HD44780_SetCursor(0,1);
//   HD44780_PrintStr(del_Card_str);
//   uint32_t From_begin_time = HAL_GetTick();
//   while(HAL_GetTick() - From_begin_time < 1000/portTICK_RATE_MS){ 
//     if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){
//       vTaskResume(xHandle_lock_task);
//       unlock_bee();
//       return 1;//switch to lock_task
//     }
//   }
//   return 0;//do nothing
// }

void unlock_fn_AddCard(){
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
  char unlock[16] = "UNLOCK!";
  HD44780_PrintStr(unlock);
  char add_Card_str[16] = "Add Card";
  HD44780_SetCursor(0,1);//move cursor to the first word of the second line
  HD44780_PrintStr(add_Card_str);
  uint32_t From_begin_time = HAL_GetTick();
  while(HAL_GetTick() - From_begin_time < 3000/portTICK_RATE_MS){//wait 3s for push button 
    if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){//push button
      rc522_add_card();
    }
  }
}

void unlock_fn_DelCard(){
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
  char unlock[16] = "UNLOCK!";
  HD44780_PrintStr(unlock);
  char del_Card_str[16] = "Del Card";
  HD44780_SetCursor(0,1);
  HD44780_PrintStr(del_Card_str);
  uint32_t From_begin_time = HAL_GetTick();
  while(HAL_GetTick() - From_begin_time < 3000/portTICK_RATE_MS){//wait 3s for push button
    if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){//make sure doing delete card
      while(1){
        int count = 0;
        card *current = list_start;
        while(current != NULL){
          HD44780_Clear();
          char CARD[16] = "card ";
          char card_number[1];
          sprintf(card_number, "%d", count);
          strcat(CARD, card_number);
          HD44780_PrintStr(CARD);
          uint32_t card_showTime = HAL_GetTick();
          while(HAL_GetTick() - card_showTime < 1000/portTICK_RATE_MS){//delete selected card
            if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){
              uint8_t del[5] = {current->data[0], current->data[1], current->data[2], current->data[3], current->data[4]};
              rc522_delete_card(del);
              HD44780_Clear();
              char unlock_str[16] = "UNLOCK!";
              HD44780_PrintStr(unlock_str);
              return;
            }
          }
          current = current->next;
          count++;
        }
      }
    }
  }
}

void unlock_fn_Lock(){
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
  char unlock[16] = "UNLOCK!";
  HD44780_PrintStr(unlock);
  char del_Card_str[16] = "LOCK function";
  HD44780_SetCursor(0,1);
  HD44780_PrintStr(del_Card_str);
  uint32_t From_begin_time = HAL_GetTick();
  while(HAL_GetTick() - From_begin_time < 3000/portTICK_RATE_MS){//wait 3s for push button
    if(debounce(HAL_GPIO_ReadPin(btn_blue_GPIO_Port, GPIO_PIN_0))){
      HD44780_Clear();
      vTaskResume(xHandle_lock_task);
      unlock_bee();
    }
  }
}

void lock_bee(){
  for (size_t i = 0; i < 2; i++)
  {
    uint32_t bee_time = HAL_GetTick();
    HAL_GPIO_WritePin(GPIOD, Bee_Pin, GPIO_PIN_SET);
    while(HAL_GetTick() - bee_time < 300/portTICK_RATE_MS){
      ;
    }
    HAL_GPIO_WritePin(GPIOD, Bee_Pin, GPIO_PIN_RESET);
    while(HAL_GetTick() - bee_time < 600/portTICK_RATE_MS){
      ;
    }
  }
}

void unlock_bee(){
  uint32_t bee_time = HAL_GetTick();
  HAL_GPIO_WritePin(GPIOD, Bee_Pin, GPIO_PIN_SET);
  while(HAL_GetTick() - bee_time < 300/portTICK_RATE_MS){
    ;
  }
  HAL_GPIO_WritePin(GPIOD, Bee_Pin, GPIO_PIN_RESET);
}

// void lock_task(void *pvParameters){
//   HD44780_Init(2);//lcd init, should be called in "task"
//   HD44780_Clear();//clean screen
//   while(1){
//     char lock_str[16] = "LOCK!";
//     HAL_GPIO_TogglePin(GPIOD, LED_Green_Pin);
//     HD44780_PrintStr(lock_str);
//     uint8_t unlock = 0;
//     unlock = rc522_check();//str_1
//     if(unlock){
//       vTaskSuspend(xHandle_lock_task);
//       lock_bee();
//     }

//     uint32_t From_begin_time = HAL_GetTick();
//     while(HAL_GetTick() - From_begin_time < 300/portTICK_RATE_MS){ //busy waiting 300 ms, can't use vTaskDelay
//       //todo: password check
//       // unlock = password_check();//str_2
//       // if(unlock){
//       //   vTaskSuspend(xHandle_lock_task);
//       //   lock_bee();
//       // }
//       ;
//     }

//     // unlock = bluetooth_check();//wait 300ms for bluetooth signal
//     // if(unlock){
//     //   vTaskSuspend(xHandle_lock_task);
//     //   lock_bee();
//     // }

//     HD44780_Clear();
//   }
// }

// void unlock_task(void *pvParameters){
//   HD44780_Init(2);//lcd init, should be called in "task"
//   HD44780_Clear();//clean screen
//   uint32_t timeout_count = HAL_GetTick();
//   unlock_bee();
//   while(1){
//     char unlock_str[16] = "UNLOCK!";
//     HD44780_PrintStr(unlock_str);
//     uint8_t fn_execute = 0;
//     //test
//     //add card: 1000ms
//     fn_execute = unlock_fn_AddCard();
//     if(fn_execute){
//       timeout_count = HAL_GetTick();//reset timeout_count
//     }
//     //del card: 1000ms
//     fn_execute = unlock_fn_DelCard();
//     if(fn_execute){
//       timeout_count = HAL_GetTick();//reset timeout_count
//     }
//     //switch to lock_task
//     fn_execute = unlock_fn_Lock();
//     if(fn_execute){
//       timeout_count = HAL_GetTick();//reset timeout_count
//     }

//     HD44780_Clear();
//     if(HAL_GetTick() - timeout_count < 5000/portTICK_RATE_MS){//if 5s passed, switch to lock_task
//       vTaskResume(xHandle_lock_task);//switch to lock_task
//       timeout_count = HAL_GetTick();
//     }
//   }
// }


void lock_task(void *pvParameters){
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
  uint8_t keyPressed = 0;
  while(1){
    char lock_str[16] = "LOCK!";
    HAL_GPIO_TogglePin(GPIOD, LED_Green_Pin);
    HD44780_PrintStr(lock_str);
    uint8_t unlock = 0;
    unlock = rc522_check();//str_1
    if(unlock){
      vTaskSuspend(xHandle_lock_task);
      lock_bee();
    }

    uint32_t From_begin_time = HAL_GetTick();
    while(HAL_GetTick() - From_begin_time < 300/portTICK_RATE_MS){ //busy waiting 300 ms, can't use vTaskDelay
        char key = read_keypad();
        if (key != '\0' && !keyPressed) {
            keyPressed = 1;
            if (keypad_check()) {
                vTaskSuspend(xHandle_lock_task);
            }
        } else if (key == '\0' && keyPressed) {
            keyPressed = 0; // Reset the flag when the key is released
        }
    }

    // unlock = bluetooth_check();//wait 300ms for bluetooth signal
    // if(unlock){
    //   vTaskSuspend(xHandle_lock_task);
    //   lock_bee();
    // }

    HD44780_Clear();
  }
}

void unlock_task(void *pvParameters){
  HD44780_Init(2);//lcd init, should be called in "task"
  HD44780_Clear();//clean screen
  unlock_bee();
  while(1){
    char unlock_str[16] = "UNLOCK!";
    HD44780_PrintStr(unlock_str);
    uint8_t fn_execute = 0;
    char key = read_keypad();
    if (key == 'A'){
      unlock_fn_AddCard();
    }
    if (key == 'B'){
      unlock_fn_DelCard();
    }
    if (key == 'C'){
      unlock_fn_Lock();
    }

    HD44780_Clear();
  }
}


void SD_RdWrTest(void) {
	int i = 0;
	static uint8_t bufr[SD_SECTOR_SIZE];
	static uint8_t bufw[SD_SECTOR_SIZE];

	for (i = 0; i < sizeof(bufw); i++) {
		bufr[i] = 0;
		bufw[i] = i % 0xFF;
	}

	SD_WriteDisk(bufw, 1, 1);
	SD_ReadDisk(bufr, 1, 1);
	printf("# SD Card Read & Write Test %s!\r\n",
			memcmp(bufr, bufw, sizeof(bufr)) == 0 ? "Successfully" : "Failed");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint64_t CardSize = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  xQue1=xQueueCreate(5,sizeof(int));
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  MFRC522_Init();//card reader init
  card *default_card = (card *)malloc(sizeof(card));
  default_card->next = NULL;
  default_card->data[0] = 211;
  default_card->data[1] = 113;
  default_card->data[2] = 208;
  default_card->data[3] = 2;
  default_card->data[4] = 112;
  list_start = default_card;
  list_end = default_card;

	CardSize = SD_GetSectorCount();
	CardSize = CardSize * SD_SECTOR_SIZE / 1024 / 1024;

	// printf("# SD Card Type:0x%02X\r\n", SD_Type);
	// printf("# SD Card Size:%luMB\r\n", (uint32_t) CardSize);
	// // HAL_Delay(1000);



	// printf(
	// 		"\r\n\r\n####################### HAL Libary SD Card SPI FATFS Demo ################################\r\n");
	// MX_FATFS_Init();
	// exf_mount();
	// exf_getfree();
  // FATFS_RdWrTest();

  /*lab5-3*/
  // xTaskCreate(Task3, "task3", 500, NULL, 1, NULL);
  // xTaskCreate(PushButton, "PushButton", 128, NULL, 1, NULL);
  /*not relative to lab5*/
  // xTaskCreate(joystick,"joystick",2048,NULL,1,NULL);
	// xTaskCreate(lcd_test,"lcd_test", 500, NULL, 2, &xHandle_2);
  
  /*lab5-1, 5-2*/
	/* How to use semaphore_binary in Lab2... */
	// xSemaphore = xSemaphoreCreateBinary(); //create a binary semaphore
	// xSemaphoreGive(xSemaphore);      // it must give a xSemaphorefirst to let a task run first
	// xTaskCreate(Task1, "task1", 500, NULL, 1, NULL);
	// xTaskCreate(Task2, "task2", 500, NULL, 1, NULL);

  //rc522-test
  // xTaskCreate(rc522_test, "rc522_test_task", 128, NULL, 1, &xHandle_1);

  //because configUSE_TIME_SLICING = 1
  //if priorities of lock_task and unlock_task are the same, they will be executed in turn. 
  xTaskCreate(lock_task, "lock_task", 128, NULL, 2, &xHandle_lock_task);
  xTaskCreate(unlock_task, "unlock_task", 128, NULL, 1, &xHandle_unlock_task);
	
  vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 90;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 89;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4|C1_Pin|C2_Pin|C3_Pin
                          |C4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BOOT1_Pin|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|Bee_Pin|LED_Green_Pin|LED_Orange_Pin
                          |LED_Red_Pin|LED_Blue_Pin|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE4 C1_Pin C2_Pin C3_Pin
                           C4_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_4|C1_Pin|C2_Pin|C3_Pin
                          |C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : btn_blue_Pin */
  GPIO_InitStruct.Pin = btn_blue_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(btn_blue_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BOOT1_Pin PB11 PB12 */
  GPIO_InitStruct.Pin = BOOT1_Pin|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : R1_Pin R2_Pin R3_Pin R4_Pin */
  GPIO_InitStruct.Pin = R1_Pin|R2_Pin|R3_Pin|R4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 Bee_Pin LED_Green_Pin LED_Orange_Pin
                           LED_Red_Pin LED_Blue_Pin PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|Bee_Pin|LED_Green_Pin|LED_Orange_Pin
                          |LED_Red_Pin|LED_Blue_Pin|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PE0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	BaseType_t xHigherPriorityTaskWoken;
	if (GPIO_Pin == GPIO_PIN_0) {
		xHigherPriorityTaskWoken = pdFALSE;
		if (flag == pdTRUE) {
			xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
		} else {
			//do nothing
		}

		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
		uint8_t data_1 = 0x5f | 0x80;
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
		HAL_SPI_Transmit(&hspi1, &data_1, 1, 10);
		////	HAL_Delay(10);
		HAL_SPI_Receive(&hspi1, &data_1, 1, 10);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
