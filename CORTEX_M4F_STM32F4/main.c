/**
  ******************************************************************************
  * @file    Template/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    20-September-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "draw_graph.h"
#include "move_car.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
xQueueHandle t_queue; /* Traffic light queue. */
xQueueHandle t_mutex; /* Traffic light mutex. */

static int traffic_index = 0; 
static int button_change_traffic = 0;
static int states[] = {TRAFFIC_RED, TRAFFIC_YELLOW, TRAFFIC_GREEN, 
							TRAFFIC_YELLOW};

void
prvInit()
{
	//LCD init
	LCD_Init();
	IOE_Config();
	LTDC_Cmd( ENABLE );

	LCD_LayerInit();
	LCD_SetLayer( LCD_FOREGROUND_LAYER );
	LCD_Clear( LCD_COLOR_BLACK );
	LCD_SetTextColor( LCD_COLOR_WHITE );

	//Button
	STM_EVAL_PBInit( BUTTON_USER, BUTTON_MODE_GPIO );
}

static void GetTrafficState(int change_state, int *v_state, int *h_state)
{

	switch (change_state) {
	case TRAFFIC_RED:
		*v_state = TRAFFIC_RED;
		*h_state = TRAFFIC_GREEN;
		break;
	case TRAFFIC_YELLOW:
		if (*v_state == TRAFFIC_GREEN)
			*v_state = TRAFFIC_YELLOW;
		else
			*h_state = TRAFFIC_YELLOW;
		break;
	case TRAFFIC_GREEN:
		*v_state = TRAFFIC_GREEN;
		*h_state = TRAFFIC_RED;
		break;
	default:
		ReportError("out of range");
		break;
	}
}

static void DrawGraphTask( void *pvParameters)
{
	const portTickType ticks = 100 / portTICK_RATE_MS;
	int value;
	int traffic_v_state = TRAFFIC_GREEN;
	int traffic_h_state = TRAFFIC_RED;

	portBASE_TYPE status;

	DrawBackground();

	while ( 1 ) {
		/*
		 * Check if the traffic changed event is sent to
		 * the queue. If so, we need to change the traffic
		 * light.
		 */
		status = xQueueReceive(t_queue, &value, ticks);

		if (status == pdPASS) {
			GetTrafficState(value, &traffic_v_state, 
						&traffic_h_state);
		}

		MoveCar(traffic_v_state, traffic_h_state);
	}
}

static void ChgTrafficLightTask(void *pvParameters)
{
	int num_ticks;
	int states_num = sizeof(states) / sizeof(states[0]);

	portBASE_TYPE status;
	portTickType ticks = TRAFFIC_GREEN_TICK;

	while ( 1 ) {
		ticks = (states[traffic_index] == TRAFFIC_YELLOW ? 
			TRAFFIC_YELLOW_TICK : TRAFFIC_GREEN_TICK);

		num_ticks = ticks / TRAFFIC_TICK_SLICE;

		status = xQueueSendToBack(t_queue, &states[traffic_index++], 0);
	
		if (status != pdPASS)
			ReportError("Cannot send to the queue!");

		if (traffic_index >= states_num)
			traffic_index = 0;

		while (num_ticks--) { 
			xSemaphoreTake(t_mutex, portMAX_DELAY);
			
			if (button_change_traffic) {
				button_change_traffic = 0;
				xSemaphoreGive(t_mutex);
				break;
			}

			xSemaphoreGive(t_mutex);

			vTaskDelay(TRAFFIC_TICK_SLICE);
		}
	}
}

static void ButtonEventTask(void *pvParameters)
{
	while (1) {
		if( STM_EVAL_PBGetState( BUTTON_USER ) ){

			while( STM_EVAL_PBGetState( BUTTON_USER ) );

			xSemaphoreTake(t_mutex, portMAX_DELAY);
			button_change_traffic = 1;
			xSemaphoreGive(t_mutex);
		}
	}
}


/** @addtogroup Template

  * @{

  */ 



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/



void RCC_Configuration(void)

{

      /* --------------------------- System Clocks Configuration -----------------*/

      /* USART1 clock enable */

      RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

      /* GPIOA clock enable */

      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

}

 

/**************************************************************************************/

 

void GPIO_Configuration(void)

{

    GPIO_InitTypeDef GPIO_InitStructure;



    /*-------------------------- GPIO Configuration ----------------------------*/

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;

    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);



    /* Connect USART pins to AF */

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);   // USART1_TX

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // USART1_RX

}

 

/**************************************************************************************/

 

void USART1_Configuration(void)

{

    USART_InitTypeDef USART_InitStructure;



    /* USARTx configuration ------------------------------------------------------*/

    /* USARTx configured as follow:

     *  - BaudRate = 9600 baud

     *  - Word Length = 8 Bits

     *  - One Stop Bit

     *  - No parity

     *  - Hardware flow control disabled (RTS and CTS signals)

     *  - Receive and transmit enabled

     */

    USART_InitStructure.USART_BaudRate = 115200;

    USART_InitStructure.USART_WordLength = USART_WordLength_8b;

    USART_InitStructure.USART_StopBits = USART_StopBits_1;

    USART_InitStructure.USART_Parity = USART_Parity_No;

    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);

}



void USART1_puts(char* s)

{

    while(*s) {

        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

        USART_SendData(USART1, *s);

        s++;

    }

}

static void Test_USART(void *pvParameters)

{
    RCC_Configuration();

    GPIO_Configuration();

    USART1_Configuration();



    USART1_puts("Hello World!\r\n");

    USART1_puts("Just for STM32F429I Discovery verify USART1 with USB TTL Cable\r\n");

     int a[3]={1,1,1};
     DrawFunction(a,3);
    // int x=0;
    // int y = 0;
    while(1)

    {

        while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

        char t = USART_ReceiveData(USART1);
      //  DrawUsart(t);

        if ((t == '\r')) {        

            while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	    
                                       

            USART_SendData(USART1, t);

            t = '\n';

        }

        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

        USART_SendData(USART1, t);

    }



    while(1); // Don't want to exit

}


//Main Function
int main(void)
{
       

	t_queue = xQueueCreate(1, sizeof(int));
	if (!t_queue) {
		ReportError("Failed to create t_queue");
		while(1);
	}

	t_mutex = xSemaphoreCreateMutex();
	if (!t_mutex) {
		ReportError("Failed to create t_mutex");
		while(1);
	}

	prvInit();

	xTaskCreate(ChgTrafficLightTask, "Traffic Light Task", 256, 
			( void * ) NULL, tskIDLE_PRIORITY + 1, NULL);

	xTaskCreate(ButtonEventTask, (char *) "Button Event Task", 256,
		   	NULL, tskIDLE_PRIORITY + 1, NULL);

	xTaskCreate(DrawGraphTask, (char *) "Draw Graph Task", 256,
		   	NULL, tskIDLE_PRIORITY + 1, NULL);
	
	 xTaskCreate(Test_USART, (char *) "Test USART", 256,
                        NULL, tskIDLE_PRIORITY + 2, NULL);


	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
        RNG_Cmd(ENABLE);

	//Call Scheduler
	vTaskStartScheduler();
}
#ifdef  USE_FULL_ASSERT



/**

  * @brief  Reports the name of the source file and the source line number

  *         where the assert_param error has occurred.

  * @param  file: pointer to the source file name

  * @param  line: assert_param error line source number

  * @retval None

  */

void assert_failed(uint8_t* file, uint32_t line)

{ 

  /* User can add his own implementation to report the file name and line number,

     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */



  /* Infinite loop */

  while (1)

  {

  }

}

#endif
