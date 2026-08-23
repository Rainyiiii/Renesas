################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dataset_storage.c \
../src/hal_entry.c \
../src/hal_warmstart.c \
../src/ili9488.c \
../src/machine_control.c \
../src/ov5640.c \
../src/platform_services.c \
../src/yolo_detector.c 

C_DEPS += \
./src/dataset_storage.d \
./src/hal_entry.d \
./src/hal_warmstart.d \
./src/ili9488.d \
./src/machine_control.d \
./src/ov5640.d \
./src/platform_services.d \
./src/yolo_detector.d 

CREF += \
rainy_dual_CPU0.cref 

OBJS += \
./src/dataset_storage.o \
./src/hal_entry.o \
./src/hal_warmstart.o \
./src/ili9488.o \
./src/machine_control.o \
./src/ov5640.o \
./src/platform_services.o \
./src/yolo_detector.o 

MAP += \
rainy_dual_CPU0.map 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -flax-vector-conversions -fshort-enums -fno-unroll-loops -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_gen" -I"." -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy_dual_CPU0\\src" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -DPLATFORM_SERVICES_MULTICORE=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

