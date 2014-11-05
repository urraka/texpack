libs := -lpng -lz
inc := -Isrc
flags := -g -Wall -std=c++11

out := bin/texpack

ifeq ($(shell uname | grep 'MINGW32_NT' -c),1)
  out := bin/texpack.exe
  flags += -Wall -static-libgcc -static-libstdc++
endif

src += src/main.cpp
src += src/packer.cpp
src += src/bleeding.cpp
src += src/png/png.cpp
src += src/rbp/MaxRects.cpp
src += src/json/json.cpp

hpp += src/help.h
hpp += src/packer.h
hpp += src/bleeding.h
hpp += src/png/png.h
hpp += src/rbp/MaxRects.h
hpp += src/json/json.h

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

clean:
	rm -rf bin

.PHONY: clean
