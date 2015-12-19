# test-lang

### Dependencies

- `make`
- `gcc`
  - Note, MinGW for Windows cannot currently be used, because it does not support `std::to_string` and other string operations
- boost libs
  - Specifically, `boost/any.hpp`, `boost/program_options.hpp` and the `libboost_program_options` shared library
- zip (for bundling the windows release)

### Build Instructions

Just run `make all` to build with the default debug flags. Use `CONFIG=RELEASE` to enable optimizations.  
Other options:
- `EXECUTABLE`: name of linked executable
- `CC`: compiler command
- `DEBUG_FLAGS` and `RELEASE_FLAGS`: compiler flags
- `LDFLAGS`: linker flags

Running `make releases` creates a 64bit Linux build and a 32bit Windows one.  
The Linux executable is not statically liked with anything.  
The Windows executable only requires the bundled `libwinpthread` and `libboost_program_options` to be dynamically linked.
