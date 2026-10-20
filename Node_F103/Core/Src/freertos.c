/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern uint8_t rx_flag;
extern uint8_t RxBuff[1];
extern volatile uint8_t can_rx_flag; 
extern CAN_RxHeaderTypeDef RxHeader; 
extern uint8_t RxData[8];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for TaskUART */
osThreadId_t TaskUARTHandle;
const osThreadAttr_t TaskUART_attributes = {
  .name = "TaskUART",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TaskLED */
osThreadId_t TaskLEDHandle;
const osThreadAttr_t TaskLED_attributes = {
  .name = "TaskLED",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTaskUART(void *argument);
void StartTaskLED(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TaskUART */
  TaskUARTHandle = osThreadNew(StartTaskUART, NULL, &TaskUART_attributes);

  /* creation of TaskLED */
  TaskLEDHandle = osThreadNew(StartTaskLED, NULL, &TaskLED_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartTaskUART */
/**
  * @brief  Function implementing the TaskUART thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskUART */
void StartTaskUART(void *argument)
{
  /* USER CODE BEGIN StartTaskUART */
  /* Infinite loop */
  for(;;)
  {
    if (rx_flag == 1)
    {
      rx_flag = 0;    // 先清零标志位
      HAL_UART_Transmit(&huart1, RxBuff, 1, 100);   // 回显收到的字节
      printf("Recv: %c\r\n", RxBuff[0]);           // 打印提示
    }
		if (can_rx_flag == 1)
    {
      can_rx_flag = 0; // 清零标志位
      printf("Recv CAN ID: 0x%X, Data: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
             RxHeader.StdId, RxData[0], RxData[1], RxData[2], RxData[3],
             RxData[4], RxData[5], RxData[6], RxData[7]);
    }
    osDelay(10);   // 短暂延时，主动让出CPU给其他任务（别写死循环空转）
  }
  /* USER CODE END StartTaskUART */
}

/* USER CODE BEGIN Header_StartTaskLED */
/**
* @brief Function implementing the TaskLED thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskLED */
void StartTaskLED(void *argument)
{
  /* USER CODE BEGIN StartTaskLED */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // 翻转LED
    osDelay(500);                            // FreeRTOS延时500ms（用osDelay，不用HAL_Delay！）
  }
  /* USER CODE END StartTaskLED */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

