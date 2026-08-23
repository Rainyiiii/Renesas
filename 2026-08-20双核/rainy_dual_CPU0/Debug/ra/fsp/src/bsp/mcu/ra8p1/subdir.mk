################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/fsp/src/bsp/mcu/ra8p1/bsp_linker.c 

C_DEPS += \
./ra/fsp/src/bsp/mcu/ra8p1/bsp_linker.d 

CREF += \
rainy_dual_CPU0.cref 

OBJS += \
./ra/fsp/src/bsp/mcu/ra8p1/bsp_linker.o 

MAP += \
rainy_dual_CPU0.map 


# Each subdirectory must supply rules for building sources it contributes
ra/fsp/src/bsp/mcu/ra8p1/%.o: ../ra/fsp/src/bsp/mcu/ra8p1/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -flax-vector-conversions -fshort-enums -fno-unroll-loops -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_gen" -I"." -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy_dual_CPU0\\src" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy_dual_CPU0\\ra\\fsp\\src\\rm_ethosu" -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -DPLATFORM_SERVICES_MULTICORE=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

