################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s16.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s16.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_per_channel_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s16.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s4.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_wrapper_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8.c \
../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8_s64.c 

C_DEPS += \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s16.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s16.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_per_channel_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s16.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s4.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_wrapper_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8.d \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8_s64.d 

CREF += \
rainy.cref 

OBJS += \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s16.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_batch_matmul_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s16.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_per_channel_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s16.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s4.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_fully_connected_wrapper_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8.o \
./ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/arm_vector_sum_s8_s64.o 

MAP += \
rainy.map 


# Each subdirectory must supply rules for building sources it contributes
ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/%.o: ../ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -flax-vector-conversions -fshort-enums -fno-unroll-loops -w -I"D:\\RA\\second\\rainy\\ra_gen" -I"." -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy\\src" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

