################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../UT\ Tools/catch_amalgamated.cpp 

CPP_DEPS += \
./UT\ Tools/catch_amalgamated.d 

OBJS += \
./UT\ Tools/catch_amalgamated.o 


# Each subdirectory must supply rules for building sources it contributes
UT\ Tools/catch_amalgamated.o: ../UT\ Tools/catch_amalgamated.cpp UT\ Tools/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"UT Tools/catch_amalgamated.d" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-UT-20-Tools

clean-UT-20-Tools:
	-$(RM) ./UT\ Tools/catch_amalgamated.d ./UT\ Tools/catch_amalgamated.o

.PHONY: clean-UT-20-Tools

