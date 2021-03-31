.DEFAULT_GOAL=all
.PHONY: all clean

Z3_INCLUDE ?= .
Z3_LIB ?= .

SRC_DIR = Source
INC_DIR = Header
OBJ_DIR = Object_Files

SRCS := $(shell find $(SRC_DIR) -name "*.cpp")
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS += -g -Ofast
LDFLAGS += -lz3

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -I . -I $(Z3_INCLUDE) $(CPPFLAGS) -c -o $@ $<

all: flie

flie: $(OBJS)
	$(CXX) -I . -I $(Z3_INCLUDE) -L $(Z3_LIB) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf flie
	rm -rf flie.exe
