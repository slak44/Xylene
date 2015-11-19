EXECUTABLE = Lang
CC = g++
CONFIG = DEBUG
DEBUG_FLAGS = -c -Wall -std=c++11 -O0 -g3
RELEASE_FLAGS = -c -std=c++11 -O3
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
	[[ $(CONFIG) == DEBUG ]] && $(CC) $(DEBUG_FLAGS) $< -o $@ || $(CC) $(RELEASE_FLAGS) $< -o $@

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@rm -f .fuse_hidden*

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
	@rm -f .fuse_hidden*
