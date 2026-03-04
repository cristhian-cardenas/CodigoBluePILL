################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/bno08x/encoder.c \
../Core/Src/bno08x/imu.c \
../Core/Src/bno08x/sh2.c \
../Core/Src/bno08x/sh2_SensorValue.c \
../Core/Src/bno08x/sh2_hal.c \
../Core/Src/bno08x/sh2_util.c \
../Core/Src/bno08x/shtp.c \
../Core/Src/bno08x/system_state.c \
../Core/Src/bno08x/telemetry.c 

OBJS += \
./Core/Src/bno08x/encoder.o \
./Core/Src/bno08x/imu.o \
./Core/Src/bno08x/sh2.o \
./Core/Src/bno08x/sh2_SensorValue.o \
./Core/Src/bno08x/sh2_hal.o \
./Core/Src/bno08x/sh2_util.o \
./Core/Src/bno08x/shtp.o \
./Core/Src/bno08x/system_state.o \
./Core/Src/bno08x/telemetry.o 

C_DEPS += \
./Core/Src/bno08x/encoder.d \
./Core/Src/bno08x/imu.d \
./Core/Src/bno08x/sh2.d \
./Core/Src/bno08x/sh2_SensorValue.d \
./Core/Src/bno08x/sh2_hal.d \
./Core/Src/bno08x/sh2_util.d \
./Core/Src/bno08x/shtp.d \
./Core/Src/bno08x/system_state.d \
./Core/Src/bno08x/telemetry.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/bno08x/%.o Core/Src/bno08x/%.su Core/Src/bno08x/%.cyclo: ../Core/Src/bno08x/%.c Core/Src/bno08x/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Core/Inc/bno08x -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-bno08x

clean-Core-2f-Src-2f-bno08x:
	-$(RM) ./Core/Src/bno08x/encoder.cyclo ./Core/Src/bno08x/encoder.d ./Core/Src/bno08x/encoder.o ./Core/Src/bno08x/encoder.su ./Core/Src/bno08x/imu.cyclo ./Core/Src/bno08x/imu.d ./Core/Src/bno08x/imu.o ./Core/Src/bno08x/imu.su ./Core/Src/bno08x/sh2.cyclo ./Core/Src/bno08x/sh2.d ./Core/Src/bno08x/sh2.o ./Core/Src/bno08x/sh2.su ./Core/Src/bno08x/sh2_SensorValue.cyclo ./Core/Src/bno08x/sh2_SensorValue.d ./Core/Src/bno08x/sh2_SensorValue.o ./Core/Src/bno08x/sh2_SensorValue.su ./Core/Src/bno08x/sh2_hal.cyclo ./Core/Src/bno08x/sh2_hal.d ./Core/Src/bno08x/sh2_hal.o ./Core/Src/bno08x/sh2_hal.su ./Core/Src/bno08x/sh2_util.cyclo ./Core/Src/bno08x/sh2_util.d ./Core/Src/bno08x/sh2_util.o ./Core/Src/bno08x/sh2_util.su ./Core/Src/bno08x/shtp.cyclo ./Core/Src/bno08x/shtp.d ./Core/Src/bno08x/shtp.o ./Core/Src/bno08x/shtp.su ./Core/Src/bno08x/system_state.cyclo ./Core/Src/bno08x/system_state.d ./Core/Src/bno08x/system_state.o ./Core/Src/bno08x/system_state.su ./Core/Src/bno08x/telemetry.cyclo ./Core/Src/bno08x/telemetry.d ./Core/Src/bno08x/telemetry.o ./Core/Src/bno08x/telemetry.su

.PHONY: clean-Core-2f-Src-2f-bno08x

