# Makefile helper for GNU make utility.
#
# To use, create your own file named 'Makefile' with something like this:
#	DSONAME = SOP_MySOP.so (or SOP_MySOP.dll for Windows)
#	SOURCES = SOP_MySOP.C
#	include $(HFS)/toolkit/makefiles/Makefile.gnu
# Then you just need to invoke make from the same directory.
#
# Complete list of variables used by this file:
#   OPTIMIZER	Override the optimization level (optional, defaults to -O2)
#   INCDIRS	Specify any additional include directories.
#   LIBDIRS	Specify any addition library include directories
#   SOURCES	List all .C files required to build the DSO or App
#   DSONAME	Name of the desired output DSO file (if applicable)
#   APPNAME	Name of the desires output application (if applicable)
#   INSTDIR	Directory to be installed. If not specified, this will
#		default to the the HOME houdini directory.
#   ICONS	Name of the icon files to install (optionial)

ifeq ($(OS),Windows_NT)
    ifndef OPTIMIZER
	OPTIMIZER = -Ox
    endif

    AMD64 = 0 
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	AMD64 = 1
    endif
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
	AMD64 = 1
    endif
    ifeq ($(AMD64),1)
	AMD64_SUFFIX = /amd64
    endif

    include $(HFS)/toolkit/makefiles/Makefile.win
else
    include $(HFS)/toolkit/makefiles/Makefile.linux
endif

HVERSION = 10.5
ifndef INSTDIR
    ifeq ($(OSTYPE),Darwin)
	INSTDIR = $(HOME)/Library/Preferences/houdini/$(HVERSION)
    else
	INSTDIR = $(HOME)/houdini$(HVERSION)
    endif
endif

OBJECTS = $(SOURCES:.C=.o)
OBJECTS := $(OBJECTS:.cpp=.o)

ifdef DSONAME
TAGINFO = $(shell (echo -n "Compiled on:" `date`"\n         by:" `whoami`@`hostname`"\n$(SESI_TAGINFO)") | sesitag -m)

%.o:		%.C 
	$(CC) $(CXXFLAGS) $(OBJFLAGS) -DMAKING_DSO $(TAGINFO) $< $(OBJOUTPUT)$@

$(DSONAME):	$(OBJECTS)
	$(LINK) $(LDFLAGS) $(SHAREDFLAG) $(OBJECTS) $(DSOFLAGS) $(DSOOUTPUT)$@
else
%.o:		%.C 
	$(CC) $(CXXFLAGS) $(OBJFLAGS) $< $(OBJOUTPUT)$@

$(APPNAME):	$(OBJECTS)
	$(LINK) $(LDFLAGS) $(OBJECTS) $(SAFLAGS) $(SAOUTPUT)$@
endif

default:	$(DSONAME) $(APPNAME)

ifdef ICONS
icons:		$(ICONS)
	@mkdir -p $(INSTDIR)/config/Icons
	@cp $(ICONS) $(INSTDIR)/config/Icons
else
icons:
endif

ifdef DSONAME
install:	default	icons
	@mkdir -p $(INSTDIR)/dso
	@cp $(DSONAME) $(INSTDIR)/dso
else
install:	default icons
endif


clean:
	rm -f $(OBJECTS) $(APPNAME) $(DSONAME)
