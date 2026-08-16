################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dataset_storage.c \
../src/ft6336.c \
../src/hal_entry.c \
../src/hal_warmstart.c \
../src/ili9488.c \
../src/machine_control.c \
../src/ov5640.c \
../src/weight_sensor.c \
../src/yolo_detector.c 

C_DEPS += \
./src/dataset_storage.d \
./src/ft6336.d \
./src/hal_entry.d \
./src/hal_warmstart.d \
./src/ili9488.d \
./src/machine_control.d \
./src/ov5640.d \
./src/weight_sensor.d \
./src/yolo_detector.d 

CREF += \
rainy.cref 

OBJS += \
./src/dataset_storage.o \
./src/ft6336.o \
./src/hal_entry.o \
./src/hal_warmstart.o \
./src/ili9488.o \
./src/machine_control.o \
./src/ov5640.o \
./src/weight_sensor.o \
./src/yolo_detector.o 

MAP += \
rainy.map 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -fshort-enums -fno-unroll-loops -flax-vector-conversions -I"D:\\RA\\second\\rainy\\ra_gen" -I"." -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy\\src" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

