EXECUTABLE = Lang
CC = g++
CONFIG = DEBUG
DEBUG_FLAGS = -c -Wall -std=c++11 -O0 -g3
RELEASE_FLAGS = -c -std=c++11 -O3
LDFLAGS = 
SOURCES = $(wildcard ./src/*.cpp)
OBJECTS = $(SOURCES:./src/%.cpp=./make/%.o)

.PHONY: all clean cleanph

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@rm -f .fuse_hidden*

# -include $(OBJECTS:.o=.d)

./make/%.o: ./src/%.cpp
	[[ $(CONFIG) == DEBUG ]] && $(CC) $(DEBUG_FLAGS) $< -o $@ || $(CC) $(RELEASE_FLAGS) $< -o $@
	# $(CC) -std=c++11 -MM -MF make/$*.d $<

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
