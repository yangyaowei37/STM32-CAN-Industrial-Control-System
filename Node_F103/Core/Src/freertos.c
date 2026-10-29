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
#include "tim.h"
#include "can.h"
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
extern volatile int32_t target_speed;
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
  
  // 声明 F407 接收反馈的 CAN 帧头
  CAN_TxHeaderTypeDef TxHeader;
  uint32_t TxMailbox;
  uint8_t TxData[2]; 
  
  TxHeader.StdId = 0x201;  // 0x201 是回传数据
  TxHeader.ExtId = 0;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 2;
  TxHeader.TransmitGlobalTime = DISABLE;

  // --- PID 参数变量 ---
  int32_t last_encoder = 0;
  float err = 0, last_err = 0, prev_err = 0;
  float pwm_output = 0;
  
  // ⚠️ PID 三个参数 (Kp, Ki, Kd)，你需要根据实际情况微调
  float Kp = 0.8; 
  float Ki = 0.2; 
  float Kd = 0.5;

  for(;;)
  {
    // 1. 读取当前编码器数值 (强转成 16 位有符号整数)
    int16_t current_encoder = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    
    // 2. 计算实际速度 (强转成 int16_t 相减，自动处理 65535->0 的溢出)
    int16_t actual_speed = current_encoder - (int16_t)last_encoder;
		if (actual_speed > 5000) actual_speed = 5000;
    if (actual_speed < -5000) actual_speed = -5000;
    last_encoder = current_encoder;

    // 3. 计算误差
    err = target_speed - actual_speed;
    
    // 4. 增量式 PID 计算
    float pwm_increment = (Kp * (err - last_err)) + (Ki * err) + (Kd * (err - 2*last_err + prev_err));
    pwm_output += pwm_increment;
    prev_err = last_err;
    last_err = err;
    
    // 5. 限幅输出 (保证 PWM 在 0-999 之间)
    if(pwm_output > 999) pwm_output = 999;
    if(pwm_output < 0) pwm_output = 0;
    
    // 6. 把 PID 算出来的值送给电机
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)pwm_output);
    
    // 7. 通过 CAN 把实际速度发给 F407
    TxData[0] = (actual_speed >> 8) & 0xFF; // 高 8 位
    TxData[1] = actual_speed & 0xFF;        // 低 8 位
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0) 
    {
        HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
    }
    
    // 8. 打印到串口方便调试
    printf("Target: %d, Actual: %d, PWM: %.1f\r\n", target_speed, actual_speed, pwm_output);
    
    osDelay(50); // ⚠️ 周期变成 50ms，PID 响应更快
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
    osDelay(2000);                            // FreeRTOS延时500ms（用osDelay，不用HAL_Delay！）
  }
  /* USER CODE END StartTaskLED */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

