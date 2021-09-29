/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_UP_Pin GPIO_PIN_0
#define KEY_UP_GPIO_Port GPIOC
#define KEY_DOWN_Pin GPIO_PIN_1
#define KEY_DOWN_GPIO_Port GPIOC
#define KEY_BACK_Pin GPIO_PIN_2
#define KEY_BACK_GPIO_Port GPIOC
#define KEY_BACK_EXTI_IRQn EXTI2_IRQn
#define KEY_OK_Pin GPIO_PIN_3
#define KEY_OK_GPIO_Port GPIOC
#define ST25_IRQ_PIN_Pin GPIO_PIN_0
#define ST25_IRQ_PIN_GPIO_Port GPIOA
#define ST25_IRQ_PIN_EXTI_IRQn EXTI0_IRQn
#define ST25_CS_PIN_Pin GPIO_PIN_4
#define ST25_CS_PIN_GPIO_Port GPIOA
#define GREEN_LED_Pin GPIO_PIN_10
#define GREEN_LED_GPIO_Port GPIOB
#define RED_LED_Pin GPIO_PIN_11
#define RED_LED_GPIO_Port GPIOB
#define SD_CS_Pin GPIO_PIN_12
#define SD_CS_GPIO_Port GPIOB
#define SPIRAM_CS_Pin GPIO_PIN_6
#define SPIRAM_CS_GPIO_Port GPIOC
#define OLED_CS_Pin GPIO_PIN_7
#define OLED_CS_GPIO_Port GPIOC
#define OLED_RES_Pin GPIO_PIN_8
#define OLED_RES_GPIO_Port GPIOC
#define OLED_DC_Pin GPIO_PIN_9
#define OLED_DC_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
