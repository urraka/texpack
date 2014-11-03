out := bin/texpack

ifeq ($(shell uname | grep 'MINGW32_NT' -c),1)
  out := bin/texpack.exe
endif

libs := -lpng -lz
inc := -Isrc

src += src/main.cpp
src += src/packer.cpp
src += src/bleeding.cpp
src += src/png/png.cpp
src += src/rbp/Rect.cpp
src += src/rbp/MaxRectsBinPack.cpp
src += src/json/json.cpp

hpp += src/packer.h
hpp += src/bleeding.h
hpp += src/png/png.h
hpp += src/rbp/Rect.h
hpp += src/rbp/MaxRectsBinPack.h
hpp += src/json/json.h

$(out): $(src) $(hpp)
	@mkdir -p bin
	$(CXX) $(src) $(inc) $(CFLAGS) $(LDFLAGS) $(libs) -o $(out)
