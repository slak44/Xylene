CC = g++
DEBUG_FLAGS = -c -Wall -std=c++11 -O0 -g3
LDFLAGS = 
SOURCES = \
./src/global.cpp \
./src/operators.cpp \
./src/tokens.cpp \
./src/nodes.cpp \
./src/builtins.cpp \
./src/operator_maps.cpp \
./src/Lang.cpp \

OBJECTS=$(SOURCES:./src/%.cpp=./make/%.o)
	
./make/%.o: ./src/%.cpp
	$(CC) $(DEBUG_FLAGS) $< -o $@
	
EXECUTABLE = Lang

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

cleanph:
	@echo "Cleaning precompiled headers..."
	@rm src/*.gch

clean:
	@echo "Cleaning precompiled headers..."
	@rm -f src/*.gch
	@echo "Cleaning make folder..."
	@rm -f ./make/*
	@echo "Cleaning executable..."
	@rm -f $(EXECUTABLE)
