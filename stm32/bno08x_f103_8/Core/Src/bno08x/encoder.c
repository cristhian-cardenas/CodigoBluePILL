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

/* Cantidad de divisiones por vuelta del ángulo en ticks discretos (0..4095).
 * Coincide con la resolución típica del AS5048A al mapear ángulo ↔ relación duty/periodo del PWM. */
#define ENCODER_RESOLUTION 4096

/* Límite superior del |delta| permitido tras el unwrap entre dos muestras consecutivas.
 * Se fija según la velocidad angular máxima del eje y la frecuencia efectiva de actualización
 * del PWM capturado; un paso mayor indica lectura espuria (ruido, desincronización, glitch). */
#define MAX_STEP 100

/* USER CODE END Private defines */

/* USER CODE BEGIN Private typedef */

/* USER CODE END Private typedef */

/* USER CODE BEGIN Private macro */

/* USER CODE END Private macro */

/* USER CODE BEGIN Private variables */

/* Último periodo PWM completo y duty en cuentas de timer (modo PWM Input de HAL):
 * CH1 — periodo entre flancos activos del tren PWM; CH2 — tiempo en alto dentro de ese periodo.
 * Ambos valores provienen de capturas asíncronas y se validan antes de derivar ticks. */
static volatile uint32_t period_value = 0;
static volatile uint32_t duty_value = 0;

/* Estado de odometría en ticks (dominio circular 0..ENCODER_RESOLUTION-1):
 * last_ticks — posición discreta anterior para calcular delta y aplicar unwrap en el borde;
 * total_ticks — acumulador con signo del desplazamiento unwrapped (no se satura por vuelta);
 * first_capture — suprime el primer delta hasta fijar referencia inicial. */
static int32_t last_ticks = 0;
static int64_t total_ticks = 0;
static uint8_t first_capture = 1;

/* USER CODE END Private variables */

/* USER CODE BEGIN Private function prototypes */

/* USER CODE END Private function prototypes */

/* USER CODE BEGIN Private functions */

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
  * Flujo por ciclo PWM útil (canal activo CH2 tras disponer periodo en CH1):
  *  a) Lectura de registros de captura (periodo CH1, duty CH2).
  *  b) Coherencia física del PWM: periodo > 0 y duty ≤ period (descarta muestras incoherentes).
  *  c) Conversión a current_ticks en [0, ENCODER_RESOLUTION-1] (entero 64 bits en producto).
  *  d) Unwrap del dominio circular para obtener delta continuo en el cruce 4095 ↔ 0.
  *  e) Filtro de glitches: |delta| ≤ MAX_STEP según velocidad máxima esperada.
  *  f) Acumulación en total_ticks y publicación en g_systemState.encoder_total_ticks.
  *  g) Ángulo de presentación en grados sobre g_systemState.encoder_angle.
  *
  * @note   Mantener la rutina acotada en tiempo de ejecución (contexto de interrupción).
  *
  * @param  htim: Handle del timer
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    /* Procesar únicamente la instancia asignada al encoder (TIM2). */
    if (htim->Instance == TIM2)
    {
        /* (a) Captura CH1: periodo del PWM según modo PWM Input. */
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            period_value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        }
        /* (a) Captura CH2: duty dentro del periodo ya muestreado en CH1. */
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
            duty_value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

            /* (b) Coherencia PWM: división por cero y duty mayor que periodo (muestras inválidas). */
            if (period_value == 0 || duty_value > period_value)
            {
                return;
            }

            /* (c) Posición angular en ticks; producto en 64 bits evita saturación intermedia. */
            int32_t current_ticks = (int32_t)(((uint64_t)duty_value * ENCODER_RESOLUTION) / period_value);
            if (current_ticks >= ENCODER_RESOLUTION)
            {
                current_ticks = ENCODER_RESOLUTION - 1;
            }

            /* (d) Referencia inicial sin acumular delta. */
            if (first_capture)
            {
                first_capture = 0;
                last_ticks = current_ticks;
            }
            else
            {
                int32_t delta = current_ticks - last_ticks;
                /* (d) Unwrap: compensa discontinuidad circular del codificador en el salto 0 ↔ 4095. */
                if (delta > (int32_t)(ENCODER_RESOLUTION / 2))
                {
                    delta -= ENCODER_RESOLUTION;
                }
                else if (delta < -(int32_t)(ENCODER_RESOLUTION / 2))
                {
                    delta += ENCODER_RESOLUTION;
                }
                /* (e) Paso máximo físico entre muestras; exceso = error de medición o desincronización. */
                if (delta > MAX_STEP || delta < -MAX_STEP)
                {
                    return;
                }
                /* (f) Odometría unwrapped. */
                total_ticks += (int64_t)delta;
                last_ticks = current_ticks;
            }

            g_systemState.encoder_total_ticks = total_ticks;

            /* (g) Ángulo instantáneo [0°, 360°) para interfaz / depuración. */
            g_systemState.encoder_angle = ((float)current_ticks / (float)ENCODER_RESOLUTION) * 360.0f;
        }
    }
}

/* USER CODE END Callback functions */
