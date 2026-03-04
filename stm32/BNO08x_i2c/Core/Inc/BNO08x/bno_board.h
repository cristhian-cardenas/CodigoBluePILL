#ifndef BNO_BOARD_H
#define BNO_BOARD_H

#include "main.h"
#include "stm32f4xx_hal_gpio.h"

/* ===================  Pines BNO08X =================== */

#define BNO_BOOT_Pin        GPIO_PIN_5
#define BNO_BOOT_GPIO_Port GPIOB

#define BNO_RST_Pin        GPIO_PIN_8
#define BNO_RST_GPIO_Port GPIOA

#define BNO_INT_Pin        GPIO_PIN_10
#define BNO_INT_GPIO_Port GPIOA
#define BNO_INT_EXTI_IRQn  EXTI15_10_IRQn

#define BNO_PS1_Pin        GPIO_PIN_10
#define BNO_PS1_GPIO_Port GPIOB

#define BNO_PS0_Pin        GPIO_PIN_4
#define BNO_PS0_GPIO_Port GPIOB

/* (Opcional) otro sensor */
#define BMP_INT_Pin        GPIO_PIN_2
#define BMP_INT_GPIO_Port GPIOA

/* Dirección I2C del BNO08x según PS1/PS0 */
#define BNO08X_I2C_ADDR 0x4A

/* ===================  Macros de control =================== */
// GPIO control using HAL functions

#define BNO_RST_On()   HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_SET)
#define BNO_RST_Off()  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET)

#define BNO_BOOT_On()  HAL_GPIO_WritePin(BNO_BOOT_GPIO_Port, BNO_BOOT_Pin, GPIO_PIN_SET)
#define BNO_BOOT_Off() HAL_GPIO_WritePin(BNO_BOOT_GPIO_Port, BNO_BOOT_Pin, GPIO_PIN_RESET)

// PS1/PS0 are expected to be hardwired per board (ADD=0 -> 0x4A).
// Disable runtime control to avoid changing hardware-strapped pins.
#define BNO_PS1_On()   ((void)0)
#define BNO_PS1_Off()  ((void)0)

#define BNO_PS0_On()   ((void)0)
#define BNO_PS0_Off()  ((void)0)

#endif /* BNO_BOARD_H */
