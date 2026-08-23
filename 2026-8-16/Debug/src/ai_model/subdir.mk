################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ai_model/compute_sub_0000.c \
../src/ai_model/compute_sub_0002.c \
../src/ai_model/compute_sub_0004.c \
../src/ai_model/kernel_library_int.c \
../src/ai_model/kernel_library_utils.c \
../src/ai_model/model.c \
../src/ai_model/sub_0001_command_stream.c \
../src/ai_model/sub_0001_invoke.c \
../src/ai_model/sub_0001_model_data.c \
../src/ai_model/sub_0001_tensors.c \
../src/ai_model/sub_0003_command_stream.c \
../src/ai_model/sub_0003_invoke.c \
../src/ai_model/sub_0003_model_data.c \
../src/ai_model/sub_0003_tensors.c 

C_DEPS += \
./src/ai_model/compute_sub_0000.d \
./src/ai_model/compute_sub_0002.d \
./src/ai_model/compute_sub_0004.d \
./src/ai_model/kernel_library_int.d \
./src/ai_model/kernel_library_utils.d \
./src/ai_model/model.d \
./src/ai_model/sub_0001_command_stream.d \
./src/ai_model/sub_0001_invoke.d \
./src/ai_model/sub_0001_model_data.d \
./src/ai_model/sub_0001_tensors.d \
./src/ai_model/sub_0003_command_stream.d \
./src/ai_model/sub_0003_invoke.d \
./src/ai_model/sub_0003_model_data.d \
./src/ai_model/sub_0003_tensors.d 

CREF += \
rainy.cref 

OBJS += \
./src/ai_model/compute_sub_0000.o \
./src/ai_model/compute_sub_0002.o \
./src/ai_model/compute_sub_0004.o \
./src/ai_model/kernel_library_int.o \
./src/ai_model/kernel_library_utils.o \
./src/ai_model/model.o \
./src/ai_model/sub_0001_command_stream.o \
./src/ai_model/sub_0001_invoke.o \
./src/ai_model/sub_0001_model_data.o \
./src/ai_model/sub_0001_tensors.o \
./src/ai_model/sub_0003_command_stream.o \
./src/ai_model/sub_0003_invoke.o \
./src/ai_model/sub_0003_model_data.o \
./src/ai_model/sub_0003_tensors.o 

MAP += \
rainy.map 


# Each subdirectory must supply rules for building sources it contributes
src/ai_model/%.o: ../src/ai_model/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -fshort-enums -fno-unroll-loops -flax-vector-conversions -I"D:\\RA\\second\\rainy\\ra_gen" -I"." -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy\\src" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

