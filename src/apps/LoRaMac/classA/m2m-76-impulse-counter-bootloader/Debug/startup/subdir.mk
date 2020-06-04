################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

C_SRCS += \
../startup/sysmem.c 

OBJS += \
./startup/startup_stm32.o \
./startup/sysmem.o 

C_DEPS += \
./startup/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

startup/%.o: ../startup/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -DSTM32 -DSTM32L0 -DSTM32L071CBTx -DDEBUG -DSX1276MB1LAS -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DREGION_EU868 -DBOOTLOADER -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/m2m-fuota" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../common" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../common/LmHandler" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../common/LmHandler/packages" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards/m2m-76-impulse-counter" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards/m2m-76-impulse-counter/cmsis" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards/mcu/stm32" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards/mcu/stm32/cmsis" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../boards/mcu/stm32/STM32L0xx_HAL_Driver/Inc" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../mac" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../mac/region" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../peripherals" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../radio" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../proto" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../libraries/nanopb" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../lora-protobuf" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../lora-protobuf/generator" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../system" -I"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/../../../../system/crypto" -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


