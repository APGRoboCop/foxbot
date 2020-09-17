##
## compiling under ubuntu:
##  for compiling linux: make 
##  for compiling win32: make OSTYPE=win32
##
## borrowed from: https://github.com/Bots-United/jk_botti
##

ifeq ($(OSTYPE),win32)
	CPP = i686-w64-mingw32-gcc -m32
	LINKFLAGS = -mdll -lm -Xlinker -add-stdcall-alias -static-libgcc -shared -s -Wl,--no-undefined
	DLLEND = .dll
else
	CPP = gcc -m32
	ARCHFLAG = -fPIC
	LINKFLAGS = -fPIC -shared -ldl -lm -s 
	DLLEND = .so
endif

TARGET = foxbot
BASEFLAGS = -Wall -Wno-write-strings
ARCHFLAG += -march=i686 -mtune=generic -msse -msse2 -msse3

ifeq ($(DBG_FLGS),1)
	OPTFLAGS = -O0 -g
else
	OPTFLAGS = -O2 -fomit-frame-pointer -g
	OPTFLAGS += -funsafe-math-optimizations
	LINKFLAGS += ${OPTFLAGS}
endif

INCLUDES = -I"./metamod" \
	-I"./hlsdk/common" \
	-I"./hlsdk/dlls" \
	-I"./hlsdk/engine" \
	-I"./hlsdk/pm_shared"

CFLAGS = ${BASEFLAGS} ${OPTFLAGS} ${ARCHFLAG} ${INCLUDES}
CPPFLAGS += -fno-rtti -fno-exceptions -fno-threadsafe-statics ${CFLAGS} 
LINKFLAGS += -Wl,--no-undefined

SRC = 	bot.cpp \
   bot_client.cpp \
   bot_combat.cpp \
   bot_job_assessors.cpp \
   bot_job_functions.cpp \
   bot_job_think.cpp \
   bot_navigate.cpp \
   bot_start.cpp \
   botcam.cpp \
   dll.cpp \
   engine.cpp \
   h_export.cpp \
   linkfunc.cpp \
   meta_api.cpp \
   sdk_util.cpp \
   util.cpp \
   version.cpp \
   waypoint.cpp

OBJ = $(SRC:%.cpp=%.o)

${TARGET}${DLLEND}: ${OBJ} 
	${CPP} -o $@ ${OBJ} ${LINKFLAGS}
	cp $@ addons/foxbot/bin/

clean:
	rm -f *.o ${TARGET}${DLLEND} Rules.depend

distclean:
	rm -f Rules.depend ${TARGET}.dll ${TARGET}.so addons/foxbot/bin/*

%.o: %.cpp
	${CPP} ${CPPFLAGS} ${LTOFLAGS} -c $< -o $@

depend: Rules.depend

Rules.depend: Makefile $(SRC)
	$(CPP) -MM ${INCLUDES} $(SRC) > $@

include Rules.depend
