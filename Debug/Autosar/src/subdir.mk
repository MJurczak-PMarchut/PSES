################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Autosar/src/CanTP.c \
../Autosar/src/CanTp_Timers.c \
../Autosar/src/mandatory_interfaces_mock.c 

C_DEPS += \
./Autosar/src/CanTP.d \
./Autosar/src/CanTp_Timers.d \
./Autosar/src/mandatory_interfaces_mock.d 

OBJS += \
./Autosar/src/CanTP.o \
./Autosar/src/CanTp_Timers.o \
./Autosar/src/mandatory_interfaces_mock.o 


# Each subdirectory must supply rules for building sources it contributes
Autosar/src/%.o: ../Autosar/src/%.c Autosar/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I../UT_Tools -I../UT -I../Autosar/src -I../Autosar/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Autosar-2f-src

clean-Autosar-2f-src:
	-$(RM) ./Autosar/src/CanTP.d ./Autosar/src/CanTP.o ./Autosar/src/CanTp_Timers.d ./Autosar/src/CanTp_Timers.o ./Autosar/src/mandatory_interfaces_mock.d ./Autosar/src/mandatory_interfaces_mock.o

.PHONY: clean-Autosar-2f-src

