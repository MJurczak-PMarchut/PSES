################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Autosar/src/CanTP.c 

C_DEPS += \
./Autosar/src/CanTP.d 

OBJS += \
./Autosar/src/CanTP.o 


# Each subdirectory must supply rules for building sources it contributes
Autosar/src/%.o: ../Autosar/src/%.c Autosar/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"C:\Users\Paulina\eclipse-workspace\PSES\UT_Tools" -I"C:\Users\Paulina\eclipse-workspace\PSES\UT" -I"C:\Users\Paulina\eclipse-workspace\PSES\Autosar\include" -I"C:\Users\Paulina\eclipse-workspace\PSES\Autosar\src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Autosar-2f-src

clean-Autosar-2f-src:
	-$(RM) ./Autosar/src/CanTP.d ./Autosar/src/CanTP.o

.PHONY: clean-Autosar-2f-src

