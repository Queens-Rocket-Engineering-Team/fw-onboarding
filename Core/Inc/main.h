/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#define FIRE_DROGUE_Pin GPIO_PIN_1
#define FIRE_DROGUE_GPIO_Port GPIOA
#define CONT_DROGUE_Pin GPIO_PIN_2
#define CONT_DROGUE_GPIO_Port GPIOA
#define FIRE_MAIN_Pin GPIO_PIN_3
#define FIRE_MAIN_GPIO_Port GPIOA
#define CONT_MAIN_Pin GPIO_PIN_4
#define CONT_MAIN_GPIO_Port GPIOA
#define CS_FL_Pin GPIO_PIN_12
#define CS_FL_GPIO_Port GPIOB
#define RESET_FL_Pin GPIO_PIN_11
#define RESET_FL_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_15
#define LED_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
