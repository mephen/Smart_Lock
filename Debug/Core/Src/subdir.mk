################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/AUDIO.c \
../Core/Src/AUDIO_LINK.c \
../Core/Src/File_Handling.c \
../Core/Src/cs43l22.c \
../Core/Src/diskio.c \
../Core/Src/fatfs.c \
../Core/Src/ff.c \
../Core/Src/ff_gen_drv.c \
../Core/Src/fops.c \
../Core/Src/liquidcrystal_i2c.c \
../Core/Src/main.c \
../Core/Src/mmc_sd.c \
../Core/Src/rc522.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/user_diskio.c \
../Core/Src/waveplayer.c 

OBJS += \
./Core/Src/AUDIO.o \
./Core/Src/AUDIO_LINK.o \
./Core/Src/File_Handling.o \
./Core/Src/cs43l22.o \
./Core/Src/diskio.o \
./Core/Src/fatfs.o \
./Core/Src/ff.o \
./Core/Src/ff_gen_drv.o \
./Core/Src/fops.o \
./Core/Src/liquidcrystal_i2c.o \
./Core/Src/main.o \
./Core/Src/mmc_sd.o \
./Core/Src/rc522.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/user_diskio.o \
./Core/Src/waveplayer.o 

C_DEPS += \
./Core/Src/AUDIO.d \
./Core/Src/AUDIO_LINK.d \
./Core/Src/File_Handling.d \
./Core/Src/cs43l22.d \
./Core/Src/diskio.d \
./Core/Src/fatfs.d \
./Core/Src/ff.d \
./Core/Src/ff_gen_drv.d \
./Core/Src/fops.d \
./Core/Src/liquidcrystal_i2c.d \
./Core/Src/main.d \
./Core/Src/mmc_sd.d \
./Core/Src/rc522.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/user_diskio.d \
./Core/Src/waveplayer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Core/Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I"C:/Users/boy45/STM32CubeIDE/workspace_1.11.0/lab_test/FreeRTOS/include" -I"C:/Users/boy45/STM32CubeIDE/workspace_1.11.0/lab_test/FreeRTOS/portable/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/AUDIO.d ./Core/Src/AUDIO.o ./Core/Src/AUDIO.su ./Core/Src/AUDIO_LINK.d ./Core/Src/AUDIO_LINK.o ./Core/Src/AUDIO_LINK.su ./Core/Src/File_Handling.d ./Core/Src/File_Handling.o ./Core/Src/File_Handling.su ./Core/Src/cs43l22.d ./Core/Src/cs43l22.o ./Core/Src/cs43l22.su ./Core/Src/diskio.d ./Core/Src/diskio.o ./Core/Src/diskio.su ./Core/Src/fatfs.d ./Core/Src/fatfs.o ./Core/Src/fatfs.su ./Core/Src/ff.d ./Core/Src/ff.o ./Core/Src/ff.su ./Core/Src/ff_gen_drv.d ./Core/Src/ff_gen_drv.o ./Core/Src/ff_gen_drv.su ./Core/Src/fops.d ./Core/Src/fops.o ./Core/Src/fops.su ./Core/Src/liquidcrystal_i2c.d ./Core/Src/liquidcrystal_i2c.o ./Core/Src/liquidcrystal_i2c.su ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mmc_sd.d ./Core/Src/mmc_sd.o ./Core/Src/mmc_sd.su ./Core/Src/rc522.d ./Core/Src/rc522.o ./Core/Src/rc522.su ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/user_diskio.d ./Core/Src/user_diskio.o ./Core/Src/user_diskio.su ./Core/Src/waveplayer.d ./Core/Src/waveplayer.o ./Core/Src/waveplayer.su

.PHONY: clean-Core-2f-Src

