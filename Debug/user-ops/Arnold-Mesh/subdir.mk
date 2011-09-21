################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../user-ops/Arnold-Mesh/Arnold-Mesh.cc 

OBJS += \
./user-ops/Arnold-Mesh/Arnold-Mesh.o 

CC_DEPS += \
./user-ops/Arnold-Mesh/Arnold-Mesh.d 


# Each subdirectory must supply rules for building sources it contributes
user-ops/Arnold-Mesh/%.o: ../user-ops/Arnold-Mesh/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


