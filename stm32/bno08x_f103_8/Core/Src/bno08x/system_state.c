/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    system_state.c
  * @brief   Implementación del módulo de estado del sistema
  * 
  * Este módulo contiene el estado global del sistema que es actualizado
  * por el módulo IMU y leído por el módulo de telemetría.
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "system_state.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* USER CODE BEGIN Private typedef */

/* USER CODE END Private typedef */

/* USER CODE BEGIN Private macro */

/* USER CODE END Private macro */

/* USER CODE BEGIN Private variables */

/* USER CODE END Private variables */

/* USER CODE BEGIN Private function prototypes */

/* USER CODE END Private function prototypes */

/* USER CODE BEGIN Private functions */

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

/* USER CODE END Exported functions */

/* USER CODE BEGIN Exported variables */
SystemState_t g_systemState = {0};
/* USER CODE END Exported variables */
