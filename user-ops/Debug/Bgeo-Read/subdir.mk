################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Bgeo-Read/Bgeo-Read.cc 

OBJS += \
./Bgeo-Read/Bgeo-Read.o 

CC_DEPS += \
./Bgeo-Read/Bgeo-Read.d 


# Each subdirectory must supply rules for building sources it contributes
Bgeo-Read/%.o: ../Bgeo-Read/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/em -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Nb -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ng -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ni -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/system -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


