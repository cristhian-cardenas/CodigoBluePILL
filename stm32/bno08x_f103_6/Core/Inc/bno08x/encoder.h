/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    encoder.h
  * @brief   Módulo de lectura del encoder AS5048A vía PWM
  * 
  * Este módulo encapsula toda la lógica relacionada con la lectura del encoder:
  * - Inicialización del timer en modo PWM Input
  * - Procesamiento del callback de captura
  * - Cálculo del ángulo en grados (0-360°)
  * - Actualización de g_systemState.encoder_angle
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef ENCODER_H
#define ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Exported types */

/* USER CODE END Exported types */

/* USER CODE BEGIN Exported constants */

/* USER CODE END Exported constants */

/* USER CODE BEGIN Exported macro */

/* USER CODE END Exported macro */

/* USER CODE BEGIN Exported functions prototypes */

/**
  * @brief  Inicializa el módulo encoder
  * 
  * Inicializa el timer TIM2 en modo PWM Input para leer el PWM del encoder AS5048A.
  * Usa HAL_TIM_IC_Start_IT para habilitar las interrupciones de captura.
  * 
  * @retval 0 si tiene éxito, código de error en caso contrario
  */
int encoder_init(void);

/* USER CODE END Exported functions prototypes */

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H */
