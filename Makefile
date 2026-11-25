TARGET = pspblox
OBJS = src/main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl \
       -lpspkernel -lpspiofilemgr -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSPBlox

include $(PSPSDK)/lib/build.mak
