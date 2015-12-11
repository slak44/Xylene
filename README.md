# test-lang

### Dependencies

- `make`
- `gcc`
  - Note, MinGW for Windows cannot currently be used, because it does not support `std::to_string` and other string operations
- boost libs

### Build Instructions

Just run `make all` to build with the default debug flags. Use `CONFIG=RELEASE` to enable optimizations.  
Other options:
- `EXECUTABLE`: name of linked executable
- `CC`: compiler command
- `DEBUG_FLAGS` and `RELEASE_FLAGS`: compiler flags
- `LDFLAGS`: linker flags

The script `make-releases` creates a 64bit Linux build and a 32bit Windows one.
