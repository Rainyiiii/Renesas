################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ft6336.c \
../src/hal_entry.c \
../src/hal_warmstart.c \
../src/m33_services.c \
../src/weight_sensor.c 

C_DEPS += \
./src/ft6336.d \
./src/hal_entry.d \
./src/hal_warmstart.d \
./src/m33_services.d \
./src/weight_sensor.d 

CREF += \
rainy_dual_CPU1.cref 

OBJS += \
./src/ft6336.o \
./src/hal_entry.o \
./src/hal_warmstart.o \
./src/m33_services.o \
./src/weight_sensor.o 

MAP += \
rainy_dual_CPU1.map 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mlittle-endian -mfloat-abi=hard -mfpu=fpv5-sp-d16 -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -fshort-enums -fno-unroll-loops -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_gen" -I"." -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy_dual_CPU1\\src" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc\\instances" -D_RENESAS_RA_ -D_RA_CORE=CPU1 -D_RA_ORDINAL=2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

