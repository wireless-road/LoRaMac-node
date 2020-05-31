################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include system/subdir.mk
-include startup/subdir.mk
-include src/subdir.mk
-include lora-protobuf/subdir.mk
-include boards/mcu/stm32/STM32L0xx_HAL_Driver/Src/subdir.mk
-include boards/mcu/stm32/subdir.mk
-include boards/mcu/subdir.mk
-include boards/m2m-76-impulse-counter/cmsis/subdir.mk
-include boards/m2m-76-impulse-counter/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: m2m-76-impulse-counter-bootloader.elf

# Tool invocations
m2m-76-impulse-counter-bootloader.elf: $(OBJS) $(USER_OBJS) D:\Work\Dmitriy\LoRaMac-node\loramac-node\src\apps\LoRaMac\classA\m2m-76-impulse-counter-bootloader/LinkerScript.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -specs=nosys.specs -T"D:/Work/Dmitriy/LoRaMac-node/loramac-node/src/apps/LoRaMac/classA/m2m-76-impulse-counter-bootloader/LinkerScript.ld" -Wl,-Map=output.map -Wl,--gc-sections -o "m2m-76-impulse-counter-bootloader.elf" @"objects.list" $(USER_OBJS) $(LIBS) -lm
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

post-build:
	-@echo 'Generating binary and Printing size information:'
	arm-none-eabi-objcopy -O binary "m2m-76-impulse-counter-bootloader.elf" "m2m-76-impulse-counter-bootloader.bin"
	arm-none-eabi-size "m2m-76-impulse-counter-bootloader.elf"
	srec_cat m2m-76-impulse-counter-bootloader.bin -binary -crop 0x0 0x5FFC -fill 0x00 0x0 0x5FFC -crc32-l-e 0x5FFC -o m2m-76-impulse-counter-bootloader_crc.bin -binary
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
