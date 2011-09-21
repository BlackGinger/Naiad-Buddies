################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include user-ops/Arnold-Render/subdir.mk
-include user-ops/Arnold-Particle/subdir.mk
-include user-ops/Arnold-Mesh/subdir.mk
-include user-ops/Arnold-Implicit/subdir.mk
-include user-ops/Arnold-ASS-Write/subdir.mk
-include user-ops/Arnold-ASS-ASCII-Write/subdir.mk
-include plug-ins/subdir.mk
-include build_release/user-ops/Arnold-Render/CMakeFiles/Arnold-Render.dir/subdir.mk
-include build_release/user-ops/Arnold-Particle/CMakeFiles/Arnold-Particle.dir/subdir.mk
-include build_release/user-ops/Arnold-Mesh/CMakeFiles/Arnold-Mesh.dir/subdir.mk
-include build_release/user-ops/Arnold-Implicit/CMakeFiles/Arnold-Implicit.dir/subdir.mk
-include build_release/user-ops/Arnold-ASS-Write/CMakeFiles/Arnold-ASS-Write.dir/subdir.mk
-include build_release/user-ops/Arnold-ASS-ASCII-Write/CMakeFiles/Arnold-ASS-ASCII-Write.dir/subdir.mk
-include build_release/plug-ins/CMakeFiles/naiad_geo.dir/subdir.mk
-include build_release/plug-ins/CMakeFiles/naiad_distance_field.dir/subdir.mk
-include build_release/CMakeFiles/CompilerIdCXX/subdir.mk
-include build_release/CMakeFiles/CompilerIdC/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: naiad-buddy-arnold

# Tool invocations
naiad-buddy-arnold: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o"naiad-buddy-arnold" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) naiad-buddy-arnold
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
