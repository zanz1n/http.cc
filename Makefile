CXX := g++

TARGET := httpc

SRC := src
BIN := bin
EXLIBS := include

CFLAGS := -std=c++20 -Wall -fpermissive -I$(SRC) -I$(EXLIBS)

C_SOURCES := $(wildcard $(EXLIBS)/*.c)
CXX_SOURCES := $(wildcard $(SRC)/*.cc)

SOURCES := $(C_SOURCES) $(CXX_SOURCES)

C_OBJS := $(patsubst $(EXLIBS)/%.c, $(BIN)/%.o, $(C_SOURCES))
CXX_OBJS := $(patsubst $(SRC)/%.cc, $(BIN)/%.oo, $(CXX_SOURCES))

OBJECTS := $(C_OBJS) $(CXX_OBJS)

TARGET := $(BIN)/httpcc

all: $(BIN) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $^ -o $@

$(BIN)/%.oo: $(SRC)/%.cc
	$(CXX) $(CFLAGS) -c $< -o $@

$(BIN)/%.o: $(EXLIBS)/%.c
	$(CXX) $(CFLAGS) -c $< -o $@

$(BIN):
	mkdir $(BIN)

clear: $(BIN)
	rm -f $(OBJECTS) $(TARGET)
