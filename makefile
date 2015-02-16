libs := -lpng -lz
flags := -g -O2 -Wall -std=c++11
out := bin/texpack
PREFIX ?= /usr/local

ifeq ($(shell uname | grep 'MINGW32_NT' -c),1)
  out := bin/texpack.exe
  flags += -static-libgcc -static-libstdc++
  win32 := yes
endif

src += src/main.cpp
src += src/packer.cpp
src += src/bleeding.cpp
src += src/png/png.cpp
src += src/rbp/MaxRects.cpp

hpp += src/help.h
hpp += src/packer.h
hpp += src/bleeding.h
hpp += src/png/png.h
hpp += src/rbp/MaxRects.h

$(out): $(src) $(hpp)
	@mkdir -p bin
	$(CXX) $(src) $(inc) $(flags) $(CFLAGS) $(LDFLAGS) $(libs) -o $(out)

src/help.h: help.txt
	echo "#pragma once" > src/help.h
	echo "const char *help_text = " >> src/help.h
	cat help.txt             | \
		sed -e 's/\\/\\\\/g' | \
		sed -e 's/"/\\"/g'   | \
		sed -e 's/^/\t"/g'   | \
		sed -e 's/$$/\\n"/g' >> src/help.h
	echo '	"";' >> src/help.h
ifeq ($(win32),yes)
	sed -i 's/$$/\r/' src/help.h
endif

clean:
	rm -rf bin

install: $(out)
	cp $(out) "$(PREFIX)/bin/"

.PHONY: clean install
