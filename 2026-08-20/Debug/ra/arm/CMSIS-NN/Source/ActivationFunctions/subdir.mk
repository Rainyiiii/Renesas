################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_nn_activation_s16.c \
../ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu6_s8.c \
../ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q15.c \
../ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q7.c 

C_DEPS += \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_nn_activation_s16.d \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu6_s8.d \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q15.d \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q7.d 

CREF += \
rainy.cref 

OBJS += \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_nn_activation_s16.o \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu6_s8.o \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q15.o \
./ra/arm/CMSIS-NN/Source/ActivationFunctions/arm_relu_q7.o 

MAP += \
rainy.map 


# Each subdirectory must supply rules for building sources it contributes
ra/arm/CMSIS-NN/Source/ActivationFunctions/%.o: ../ra/arm/CMSIS-NN/Source/ActivationFunctions/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -flax-vector-conversions -fshort-enums -fno-unroll-loops -w -I"D:\\RA\\second\\rainy\\ra_gen" -I"." -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy\\src" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

