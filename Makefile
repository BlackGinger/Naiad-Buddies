# Makefile helper for GNU make utility.
#
# To use, create your own file named 'Makefile' with something like this:
#       DSONAME = SOP_MySOP.so (or SOP_MySOP.dll for Windows)
#       SOURCES = SOP_MySOP.C
#       include $(HFS)/toolkit/makefiles/Makefile.gnu
# Then you just need to invoke make from the same directory.
#
# Complete list of variables used by this file:
#   OPTIMIZER   Override the optimization level (optional, defaults to -O2)
#   INCDIRS     Specify any additional include directories.
#   LIBDIRS     Specify any addition library include directories
#   SOURCES     List all .C files required to build the DSO or App
#   DSONAME     Name of the desired output DSO file (if applicable)
#   APPNAME     Name of the desires output application (if applicable)
#   INSTDIR     Directory to be installed. If not specified, this will
#               default to the the HOME houdini directory.
#   ICONS       Name of the icon files to install (optionial)

#Make sure this points to your naiad installation

#You shouldn't need to touch anything below this line.

NAIAD_INC=-I $(NAIAD_PATH)/include/Ni -I $(NAIAD_PATH)/include/Ng -I $(NAIAD_PATH)/include/em

NAIAD_LIBS=-lNi -liomp5 -limf -lsvml -l:libintlc.so.5 -lpthread

#LIBDIRS=-L$(HOME)/local/lib
LIBDIRS= -L$(HFS)/python/lib/ -L $(NAIAD_PATH)/lib
INCDIRS=-I$(HFS)/toolkit/include $(NAIAD_INC)
LIBS= $(NAIAD_LIBS)
#CXXFLAGS=-frounding-math
APPNAME=bin/geo2emp
SOURCES=src/geo2emp.C src/empload.C src/empsave.C

#Include a locally modified makefile
include $(HFS)/toolkit/makefiles/Makefile.gnu
