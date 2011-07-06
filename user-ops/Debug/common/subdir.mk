################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../common/Bgeo.cc 

OBJS += \
./common/Bgeo.o 

CC_DEPS += \
./common/Bgeo.d 


# Each subdirectory must supply rules for building sources it contributes
common/%.o: ../common/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/em -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Nb -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ng -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ni -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/system -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


