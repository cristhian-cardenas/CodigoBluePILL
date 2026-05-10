/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    encoder.c
  * @brief   Implementación del módulo de lectura del encoder AS5048A vía PWM
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

/* Includes ------------------------------------------------------------------*/
#include "encoder.h"
#include "tim.h"

/* USER CODE BEGIN Includes */
#include "system_state.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

#define ENCODER_RESOLUTION 4096

/* USER CODE END Private defines */

/* USER CODE BEGIN Private typedef */

/* USER CODE END Private typedef */

/* USER CODE BEGIN Private macro */

/* USER CODE END Private macro */

/* USER CODE BEGIN Private variables */

// Variables para almacenar las capturas del timer
// En modo PWM Input:
// - period_value: capturado en CH1 (flanco de subida) - periodo completo
// - duty_value: capturado en CH2 (flanco de bajada) - duty cycle
static volatile uint32_t period_value = 0;
static volatile uint32_t duty_value = 0;

// Odometría: ticks por vuelta (0..4095), unwrap y acumulación
static int32_t last_ticks = 0;
static int64_t total_ticks = 0;
static uint8_t first_capture = 1;

/* USER CODE END Private variables */

/* USER CODE BEGIN Private function prototypes */

/**
  * @brief  Calcula el ángulo en grados a partir del periodo y duty cycle
  * @param  period: Valor del periodo capturado
  * @param  duty: Valor del duty cycle capturado
  * @retval Ángulo en grados (0-360°)
  */
static float encoder_calculate_angle(uint32_t period, uint32_t duty);

/* USER CODE END Private function prototypes */

/* USER CODE BEGIN Private functions */

/**
  * @brief  Calcula el ángulo en grados a partir del period y duty cycle
  * 
  * El AS5048A genera un PWM donde:
  * - Periodo: tiempo total del ciclo PWM
  * - Duty cycle: tiempo en alto del ciclo PWM
  * - Ángulo = (duty_cycle / period) * 360.0°
  * 
  * @param  period: Valor del periodo capturado (IC1Value)
  * @param  duty: Valor del duty cycle capturado (IC2Value)
  * @retval Ángulo en grados (0-360°)
  */
 static float encoder_calculate_angle(uint32_t period, uint32_t duty)
{
    float angle = 0.0f;

    // Evitar división por cero
    if (period > 0)
    {
        // Calcular ángulo: (duty / period) * 360°
        angle = ((float)duty / (float)period) * 360.0f;

        // Normalizar al rango 0-360°
        if (angle < 0.0f)
        {
            angle = 0.0f;
        }
        else if (angle > 360.0f)
        {
            angle = 360.0f;
        }
    }

    return angle;
}

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

/**
  * @brief  Inicializa el módulo encoder
  * 
  * Inicializa el timer TIM2 en modo PWM Input para leer el PWM del encoder AS5048A.
  * Usa HAL_TIM_IC_Start_IT para habilitar las interrupciones de captura en ambos canales.
  * 
  * @retval 0 si tiene éxito, código de error en caso contrario
  */
int encoder_init(void)
{
    HAL_StatusTypeDef status;
    
    // Inicializar variables
    period_value = 0;
    duty_value = 0;
    last_ticks = 0;
    total_ticks = 0;
    first_capture = 1;
    g_systemState.encoder_angle = 0.0f;
    g_systemState.encoder_total_ticks = 0;
    
    // Iniciar captura en modo Input Capture con interrupciones
    // CH1 captura el periodo (flanco de subida)
    status = HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    if (status != HAL_OK)
    {
        return -1;
    }
    
    // CH2 captura el duty cycle (flanco de bajada)
    status = HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    if (status != HAL_OK)
    {
        HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
        return -1;
    }
    
    return 0;
}

/* USER CODE END Exported functions */

/* USER CODE BEGIN Callback functions */

/**
  * @brief  Callback de captura del timer (llamado automáticamente por HAL)
  * 
  * Este callback es llamado por HAL cuando se captura un flanco en el timer.
  * En modo PWM Input:
  * - CH1 captura el periodo (flanco de subida) - resetea el contador
  * - CH2 captura el duty cycle (flanco de bajada)
  * 
  * El callback es liviano: solo lee los valores capturados y calcula el ángulo.
  * El cálculo se hace en el callback para mantener la ISR lo más liviana posible.
  * 
  * @param  htim: Handle del timer
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    // Verificar que es TIM2
    if (htim->Instance == TIM2)
    {
        // CH1 captura el periodo (flanco de subida)
        // En modo PWM Input, CH1 captura el periodo completo del ciclo anterior
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            // Leer el valor capturado del periodo
            period_value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        }
        // CH2 captura el duty cycle (flanco de bajada)
        // Cuando CH2 captura, ya tenemos el periodo del ciclo anterior en period_value
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
            // Leer el valor capturado del duty cycle
            duty_value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
            
            if (period_value > 0)
            {
                // Trabajar en ticks (0..4095), sin float para acumulación
                int32_t current_ticks = (int32_t)((duty_value * (uint32_t)ENCODER_RESOLUTION) / period_value);
                if (current_ticks >= ENCODER_RESOLUTION)
                {
                    current_ticks = ENCODER_RESOLUTION - 1;
                }

                // Unwrap: detectar cruce por 0 para odometría
                if (first_capture)
                {
                    first_capture = 0;
                    last_ticks = current_ticks;
                }
                else
                {
                    int32_t delta = current_ticks - last_ticks;
                    if (delta > (int32_t)(ENCODER_RESOLUTION / 2))
                    {
                        delta -= ENCODER_RESOLUTION;
                    }
                    else if (delta < -(int32_t)(ENCODER_RESOLUTION / 2))
                    {
                        delta += ENCODER_RESOLUTION;
                    }
                    total_ticks += (int64_t)delta;
                    last_ticks = current_ticks;
                }

                g_systemState.encoder_total_ticks = total_ticks;

                // Ángulo 0-360° solo para mostrar (float)
                g_systemState.encoder_angle = ((float)current_ticks / 4096.0f) * 360.0f;
            }
        }
    }
}

/* USER CODE END Callback functions */
