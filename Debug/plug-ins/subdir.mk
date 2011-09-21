################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../plug-ins/naiad_distance_field.cc \
../plug-ins/naiad_geo.cc 

OBJS += \
./plug-ins/naiad_distance_field.o \
./plug-ins/naiad_geo.o 

CC_DEPS += \
./plug-ins/naiad_distance_field.d \
./plug-ins/naiad_geo.d 


# Each subdirectory must supply rules for building sources it contributes
plug-ins/%.o: ../plug-ins/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


