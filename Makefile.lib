#****************************************************************************
#
# Makefile for record code
#
# This is a GNU make (gmake) makefile
#****************************************************************************
# DEBUG can be set to YES to include debugging info, or NO otherwise
DEBUG          := YES 

#输出头文件
EXT_HEADER_FILE :=
#输出库及头文件目的地址
EXT_HDR_DST		:= ./inc/
EXT_LIB_DST		:= ./lib

#****************************************************************************
export CROSS:=
export CC:=$(CROSS)gcc
export AR:=$(CROSS)ar
export TP:=$(CROSS)strip

DEBUG_CFLAGS     := -Wall -Wno-format -g -D_GNU_SOURCE 
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 -D_GNU_SOURCE 

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS} 
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}

LDFLAGS          := 

ifeq (YES, ${DEBUG})
	export GSF_MAKE_DBG:=GSF_DEBUG
	export GSF_SUPPORT_GDB:=YES
	CFLAGS       := ${DEBUG_CFLAGS}
	CXXFLAGS     := ${DEBUG_CXXFLAGS}
	LDFLAGS      := ${LDFLAGS}
else
	export GSF_MAKE_DBG:=GSF_RELEASE
	CFLAGS       := ${RELEASE_CFLAGS}
	CXXFLAGS     := ${RELEASE_CXXFLAGS}
	LDFLAGS      := ${LDFLAGS}
endif

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************
CFLAGS   := ${CFLAGS}   ${DEFS}
CXXFLAGS := ${CXXFLAGS} ${DEFS}

#****************************************************************************
# Targets of the build
#****************************************************************************

OUTPUT := ./lib/libtika_parser.so

all: ${OUTPUT}


#****************************************************************************
# Source files
#****************************************************************************
SUBDIRS=$(shell find ./src -maxdepth 1 -type d)

SRCS:=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.cpp))\
			$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))

OBJS=$(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRCS))) 

#****************************************************************************
# Include paths
#****************************************************************************
INCDIRS=$(shell find ./inc -maxdepth 2 -type d)
INCPRJ=$(foreach dir,$(INCDIRS),$(join -I,$(dir)))

INCS := ${INCPRJ}
INCS += -I ./inc/ -I /usr/java/jdk1.8.0_05/include -I /usr/java/jdk1.8.0_05/include/linux -I /root/libchar/libchardet-0.0.4/src -I /usr/local/lib/glib-2.0/include
INCS += -I /usr/local/include/gmime-2.6  -I /usr/local/include/glib-2.0 -I /usr/local/include/glib-2.0/include
INCS += -I /usr/local/c-icap-0.4.x/include/c_icap
INCS += -I /root/c_icap-0.4.3
#****************************************************************************
# Output
#****************************************************************************

${OUTPUT}: ${OBJS}
	${AR} rs ${OUTPUT} ${OBJS}  ./src/extractmain.o

#****************************************************************************
# common rules
#****************************************************************************

# Rules for compiling source files to object files
%.o : %.cpp
	${CXX} -c ${CXXFLAGS} ${INCS} $< -o $@

%.o : %.c
	${CC} -c ${CFLAGS} ${INCS} $< -o $@	

clean:
	rm -f ${OUTPUT} ${OBJS}		
install:
	cp ./lib/libtika_parser.so  /usr/local/c-icap-0.4.x/bin
	cp ./lib/libtika_parser.so  /root/c_icap-0.4.3
