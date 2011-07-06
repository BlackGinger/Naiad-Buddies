################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Bgeo-Write/Bgeo-Write.cc 

OBJS += \
./Bgeo-Write/Bgeo-Write.o 

CC_DEPS += \
./Bgeo-Write/Bgeo-Write.d 


# Each subdirectory must supply rules for building sources it contributes
Bgeo-Write/%.o: ../Bgeo-Write/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/em -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Nb -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ng -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/Ni -I/usr/exoticmatter/naiad-0.5.5.x86_64/server/include/system -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


