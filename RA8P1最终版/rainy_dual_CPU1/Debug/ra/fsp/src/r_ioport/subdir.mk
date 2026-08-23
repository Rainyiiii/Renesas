################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/fsp/src/r_ioport/r_ioport.c 

C_DEPS += \
./ra/fsp/src/r_ioport/r_ioport.d 

CREF += \
rainy_dual_CPU1.cref 

OBJS += \
./ra/fsp/src/r_ioport/r_ioport.o 

MAP += \
rainy_dual_CPU1.map 


# Each subdirectory must supply rules for building sources it contributes
ra/fsp/src/r_ioport/%.o: ../ra/fsp/src/r_ioport/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mlittle-endian -mfloat-abi=hard -mfpu=fpv5-sp-d16 -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -fshort-enums -fno-unroll-loops -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_gen" -I"." -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy_dual_CPU1\\src" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy_dual_CPU1\\ra\\fsp\\inc\\instances" -D_RENESAS_RA_ -D_RA_CORE=CPU1 -D_RA_ORDINAL=2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

