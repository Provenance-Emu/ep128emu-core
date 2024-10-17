TARGET_NAME := ep128emu_core

EXCLUDE_SOUND_LIBS ?= 1
STATIC_LINKING := 0
DEBUG   = 0
LIBS    :=

ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -a),)
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		platform = osx
	else ifneq ($(findstring win,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring armv,$(shell uname -a)),)
		platform = armv
	endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	ifeq ($(shell uname -p),arm)
		arch = arm
	endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

# Unix
ifeq ($(platform), unix)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,-version-script=link.T -Wl,-no-undefined
	CC ?= gcc
	CXX ?= g++
  LDFLAGS += -Wl,--as-needed
else ifneq (,$(findstring linux-portable,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC -nostdlib
	LIBM :=
	SHARED := -shared -Wl,-version-script=link.T
  LDFLAGS += -Wl,--as-needed
# ARM
else ifneq (,$(findstring armv,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,-version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++
	ifneq (,$(findstring cortexa8,$(platform)))
		PLATFORM_DEFINES += -marm -mcpu=cortex-a8
	else ifneq (,$(findstring cortexa9,$(platform)))
		PLATFORM_DEFINES += -marm -mcpu=cortex-a9
	endif
	PLATFORM_DEFINES += -marm
	PLATFORM_DEFINES += -mtune=generic-armv7-a -mhard-float
  LDFLAGS += -Wl,--as-needed

# OS X
else ifeq ($(platform), osx)
	TARGET := $(TARGET_NAME)_libretro.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
        MINVERSION :=
	OSXVER = `sw_vers -productVersion | cut -d. -f 2`
	OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
	ifeq ($(OSX_LT_MAVERICKS),"YES")
		MINVERSION = -mmacosx-version-min=10.5
	endif
	fpic += $(MINVERSION)

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif

	CFLAGS  += $(ARCHFLAGS)
	CXXFLAGS  += $(ARCHFLAGS)
	LDFLAGS += $(ARCHFLAGS)
# Windows cross-compilation
# If variables are set up externally by buildbot, do not override.
else ifeq ($(platform), win64)
	TARGET := $(TARGET_NAME)_libretro.dll
    ifeq ($(findstring mingw,$(AR)),)
		AR = x86_64-w64-mingw32-ar
	endif
    ifeq ($(findstring mingw,$(CC)),)
		CC = x86_64-w64-mingw32-gcc
	endif
    ifeq ($(findstring mingw,$(CXX)),)
		CXX = x86_64-w64-mingw32-g++
	endif
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++
	LDFLAGS += -lwinmm -Wl,--export-all-symbols
  LDFLAGS += -Wl,--as-needed
else ifeq ($(platform), win32)
	TARGET := $(TARGET_NAME)_libretro.dll
    ifeq ($(findstring mingw,$(AR)),)
		AR = i686-w64-mingw32-ar
	endif
    ifeq ($(findstring mingw,$(CC)),)
		CC = i686-w64-mingw32-gcc
	endif
    ifeq ($(findstring mingw,$(CXX)),)
		CXX = i686-w64-mingw32-g++
	endif
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++ -L. -m32
	LDFLAGS += -lwinmm -Wl,--export-all-symbols
  LDFLAGS += -Wl,--as-needed
	PLATFORM_DEFINES += -march=pentium2
	CFLAGS += -m32
	CXXFLAGS += -m32
# emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	SHARED := -shared -s TOTAL_MEMORY=184842208
	STATIC_LINKING=1
endif

ifeq ($(STATIC_LINKING), 1)
    EXT := a
endif

LIBM		= -lpthread
ifeq ($(EXCLUDE_SOUND_LIBS), 0)
  LIBM += -lsndfile
endif


LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g
	CXXFLAGS += -O0 -g
else
    CFLAGS += -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
	CXXFLAGS += -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
    LDFLAGS += -s
endif

DEFINES := $(PLATFORM_DEFINES) -DEP128EMU_LIBRETRO_CORE
ifeq ($(EXCLUDE_SOUND_LIBS), 1)
  DEFINES += -DEXCLUDE_SOUND_LIBS
endif
# DEFINES += -DEP128EMU_USE_XRGB8888

CFLAGS += $(DEFINES)
CXXFLAGS += $(DEFINES) -std=c++0x

include Makefile.common

INCLUDES := $(INCFLAGS)
OBJECTS := $(SOURCES_CPP:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)
CFLAGS += -Wall $(fpic)
CXXFLAGS += -Wall $(fpic)

all Release: $(TARGET)

$(TARGET): $(OBJECTS)

ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
	#@$(CC) -o $@ $(SHARED) $(OBJS) $(LDFLAGS) $(LIBS)
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(fpic) -c -o $@ $<
	#$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(fpic) -c -o $@ $<
	#@echo $@
	#@$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

clean cleanRelease:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

