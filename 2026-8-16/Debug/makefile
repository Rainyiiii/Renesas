################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/fatfs/subdir.mk
-include src/ai_model/subdir.mk
-include src/subdir.mk
-include ra_gen/subdir.mk
-include ra/npu/ethos-u-core-driver/src/subdir.mk
-include ra/fsp/src/rm_ethosu/subdir.mk
-include ra/fsp/src/r_spi_b/subdir.mk
-include ra/fsp/src/r_sdhi/subdir.mk
-include ra/fsp/src/r_sci_b_uart/subdir.mk
-include ra/fsp/src/r_ioport/subdir.mk
-include ra/fsp/src/r_iic_master/subdir.mk
-include ra/fsp/src/r_gpt/subdir.mk
-include ra/fsp/src/r_dmac/subdir.mk
-include ra/fsp/src/r_ceu/subdir.mk
-include ra/fsp/src/bsp/mcu/ra8p1/subdir.mk
-include ra/fsp/src/bsp/mcu/all/subdir.mk
-include ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/subdir.mk
-include ra/arm/CMSIS_6/subdir.mk
-include ra/arm/CMSIS-NN/Source/TransposeFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/SoftmaxFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/SVDFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/ReshapeFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/PoolingFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/PadFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/NNSupportFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/LSTMFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/FullyConnectedFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/ConvolutionFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/ConcatenationFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/BasicMathFunctions/subdir.mk
-include ra/arm/CMSIS-NN/Source/ActivationFunctions/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(JMP_DEPS)),)
-include $(JMP_DEPS)
endif
ifneq ($(strip $(FSY_DEPS)),)
-include $(FSY_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_UPPER_DEPS)),)
-include $(SRC_UPPER_DEPS)
endif
ifneq ($(strip $(P_UPPER_DEPS)),)
-include $(P_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(PP_UPPER_DEPS)),)
-include $(PP_UPPER_DEPS)
endif
ifneq ($(strip $(P_DEPS)),)
-include $(P_DEPS)
endif
ifneq ($(strip $(FSY_UPPER_DEPS)),)
-include $(FSY_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
$(C_DEPS):$(SMS_ASM)
endif
ifneq ($(strip $(SRC_DEPS)),)
-include $(SRC_DEPS)
endif
ifneq ($(strip $(JMP_UPPER_DEPS)),)
-include $(JMP_UPPER_DEPS)
endif
ifneq ($(strip $(PP_DEPS)),)
-include $(PP_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(SX_UPPER_DEPS)),)
-include $(SX_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
OBJCOPY += \
rainy.srec \

SECONDARY_SIZE += \
rainy.siz \


# All Target
all: rainy.elf secondary-outputs

# Tool invocations
rainy.elf: $(OBJS) $(USER_OBJS) $(LIBRARY_GENERATOR_OUTPUTTYPE_OUTPUTS) $(LINKER_SCRIPT)
	$(file > rainy.elf.in,-mcpu=cortex-m85 -mthumb -mlittle-endian -mfloat-abi=hard -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -o "rainy.elf" $(OBJS) $(USER_OBJS) -T "fsp.lld" -L"D:\\RA\\second\\rainy/script" -L"D:\\RA\\second\\rainy\\Debug" -Wl,-Map=rainy.map -Wl,--start-group -lcrt0 -lc++abi  -Wl,--end-group -Wl,--gc-sections -Wl,--cref -Wl,-z,norelro -Wl,--icf=none -I"D:\\RA\\second\\rainy\\ra_gen" -I"." -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\RA\\second\\rainy\\ra_cfg\\fsp_cfg" -I"D:\\RA\\second\\rainy\\src" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\api" -I"D:\\RA\\second\\rainy\\ra\\fsp\\inc\\instances" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN\\Include" -I"D:\\RA\\second\\rainy\\ra\\arm\\CMSIS-NN" -I"D:\\RA\\second\\rainy\\ra\\npu\\ethos-u-core-driver\\include" -I"D:\\RA\\second\\rainy\\ra\\fsp\\src\\rm_ethosu")
	@echo Building target: $@ && clang  --target=arm-none-eabi @"rainy.elf.in"> rainy.cref
rainy.srec: rainy.elf
	llvm-objcopy "rainy.elf" -O srec  "rainy.srec"
rainy.siz: rainy.elf
	llvm-size --format=berkeley "rainy.elf"
# Other Targets
clean:
	-$(file > clean_file.tmp,  $(JMP_DEPS)  $(FSY_DEPS)  $(C_UPPER_DEPS)  $(SECONDARY_SIZE)  $(SRC_UPPER_DEPS)  $(P_UPPER_DEPS)  $(S_DEPS)  $(PP_UPPER_DEPS)  $(P_DEPS)  $(FSY_UPPER_DEPS)  $(C_DEPS)  $(CREF)  $(SRC_DEPS)  $(OBJCOPY)  $(JMP_UPPER_DEPS)  $(PP_DEPS)  $(SX_DEPS)  $(ASM_UPPER_DEPS)  $(OBJS)  $(SX_UPPER_DEPS)  $(ASM_DEPS)  $(MAP)  $(S_UPPER_DEPS) )
	-@ xargs -P 2 -s 32000 -t -a clean_file.tmp $(RM)
	-@$(RM) clean_file.tmp
	-$(RM)  rainy.elf
	-@echo ' '

secondary-outputs: $(OBJCOPY) $(SECONDARY_SIZE)

.PHONY: all clean dependents 

-include ../makefile.targets
