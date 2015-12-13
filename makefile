EXECUTABLE = Lang
CC = g++
CONFIG = DEBUG
DEBUG_FLAGS = -Wall -O0 -g3
RELEASE_FLAGS = -O3
CFLAGS = $(shell [[ $(CONFIG) == DEBUG ]] && echo $(DEBUG_FLAGS) || echo $(RELEASE_FLAGS))
LDFLAGS = -lboost_program_options
SOURCES = $(wildcard ./src/*.cpp)
OBJECTS = $(SOURCES:./src/%.cpp=./make/%.o)

.PHONY: all clean cleanph

all: $(SOURCES) $(EXECUTABLE)
	@echo "Built executable" $(EXECUTABLE) "with" $(CC)
	
$(EXECUTABLE): $(OBJECTS)
	@echo "Linking executable..."
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@rm -f .fuse_hidden*

# -include $(OBJECTS:.o=.d)

./make/%.o: ./src/%.cpp
	@echo "Compiling file: $@"
	@$(CC) -c -std=c++11 $(CFLAGS) $< -o $@
	@# $(CC) -std=c++11 -MM -MF make/$*.d $<

cleanph:
	@echo "Cleaning precompiled headers..."
	@rm -f src/*.gch

clean:
	@echo "Cleaning precompiled headers..."
	@rm -f src/*.gch
	@echo "Cleaning make folder..."
	@rm -f ./make/*
	@echo "Cleaning executable..."
	@rm -f $(EXECUTABLE)
	@rm -f .fuse_hidden*

