out := bin/texpack.exe

lib := -Isrc -lpng -lz
opt := -O3 -Wall -fno-exceptions -fno-rtti -static-libgcc -static-libstdc++
cxx := g++

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
	g++ $(src) $(lib) $(opt) -o $(out)

install:
	cp $(out) /local/bin/
