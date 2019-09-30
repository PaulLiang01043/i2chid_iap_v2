#
# Makefile for i2chid_iap_v2 (I2C-HID Interface)
# Date: 2019/03/28
#
program := i2chid_iap_v2
objects := BaseLog.o \
		   I2CHIDLinuxGet.o \
		   ElanTsFuncUtility.o \
		   main.o
libraries := stdc++ rt pthread
executable_path := ./bin
source_path := ./src
include_path := ./include 

CXX ?= g++ # Compiler: GCC C++ Compiler
#CXX ?= arm-none-linux-gnueabi-g++ # Compiler: arm cross compiler
CXXFLAGS = -Wall -ansi -O3 -g
CXXFLAGS += -D__ENABLE_DEBUG__
CXXFLAGS += -D__ENABLE_OUTBUF_DEBUG__
CXXFLAGS += -D__ENABLE_INBUF_DEBUG__
CXXFLAGS += -D__ENABLE_LOG_FILE_DEBUG__
#CXXFLAGS += -D__ENABLE_SYSLOG_DEBUG__
CXXFLAGS += -static
INC_FLAGS += $(addprefix -I, $(include_path))
LIB_FLAGS += $(addprefix -l, $(libraries))
VPATH = $(include_path)
vpath %.h $(include_path)
vpath %.c $(source_path)
vpath %.cpp $(source_path)
.SUFFIXS: .c .cpp .h

.PHONY: all
all: $(objects)
	$(CXX) $^ $(CXXFLAGS) $(INC_FLAGS) $(LIB_FLAGS) -o $(program)
	@chmod 777 $(program)
	@mv $(program) $(executable_path)
	@rm -rf $^
	
%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS) $(INC_FLAGS) $(LIB_FLAGS)
	
.PHONY: clean
clean: 
	@rm -rf $(executable_path)/$(program) $(objects)

