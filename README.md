### **texpack**

Simple cross-platform command line texture packer based on the MaxRects packing algorithm by Jukka Jylänki (https://github.com/juj/RectangleBinPack).

There's a Photoshop script port of this tool written in JavaScript. Credits go to Rhody Lugo for that. See here: https://github.com/rraallvv/Layer2SpriteSheet

**Usage:**

```
Usage: texpack -o <output-files-prefix> [options...] [<input-file>]

The <input-file> should contain a list of image files (png) separated by new
lines. If no <input-file> is given, it will read from stdin.

Options:

-h, --help            Show this help.
-o, --output          Prefix for the generated files (atlas and json).
-p, --padding         Padding between sprites.
-s, --size            Fixed size for the atlas image (i.e. 512x512).
-S, --max-size        Treat size parameter as maximum size.
-P, --POT             Keep atlas size a power of two (ignored if size is used).
-r, --allow-rotate    Allows sprites to be rotated for better packing.
-m, --metadata        Input metadata file in json format. (*)
-e, --pretty          Generated json file will be human readable.
-t, --trim            Trim input images.
-i, --indentation     Number of spaces for indentation, 0 to use tabs (default).
-u, --premultiplied   Atlas images will have premultiplied alpha.
-b, --alpha-bleeding  Post-process atlas image with an alpha bleeding algorithm.
-M, --mode            Specifies the packing heuristic. Allowed values are:
                        * auto (default; tries all modes and selects one)
                        * bottom-left
                        * short-side
                        * long-side
                        * best-area
                        * contact-point
-f, --format          Specifies the output format of the JSON file. Values are:
                        * legacy (default; uses the original JSON format created by urraka)
                        * jsonhash (Texture Atlas JSON Hash format)
                        * jsonarray (Texture Atlas JSON Array format)
                        * xml (Texture Atlas XML)

(*) The format of the metadata file should be as follows:

    {
      ".global": {"param": "value", ...},
      "someimage.png":    {"param1": "some-value", "param2": 0, ...},
      "anotherimage.png": "not necessarily an object",
      ...
    }

    Each file name should match one in the <input-file> list. The ".global" file
    name is used for global metadata.
```

**Input:**

The input file should be a plain text file with each image on a new line. For example, if your file tree looked like this:

```
assets
│   input.txt
│   ship.png
│
└───ambient
│   │   stars.png
│   │   supernova.png
│   │
│   └───light
│       │   lightspeed.png
│       │   glare.png
│       │   ...
│
└───planets
    │   jupiter.png
```

Then in your *input.txt* file you would put this:

```
ship.png
ambient/stars.png
ambient/supernova.png
ambient/light/lightspeed.png
ambient/light/glare.png
planets/jupiter.png
```

For ease of use, you can include several numbered files in a single line within the input file. For example, if you had a 12 frame fire animation with the files `fire1.png`, `fire2.png` ... `fire12.png`, instead of writing each file on their own individual line in the input file, you can write the range of numbers in brackets where the numbers would usually go. For our fire animation example, the line to import them in the input file would look like this:

```
fire[1-12].png
```

Some animation creation programs output the files with numbers like `fire0001.png`, `fire0002.png` etc. If there are leading zeros to the file numbers, you must specify them on the input file as such:

```
fire[0001-0012].png
```

**Output:**

At least one image (the texture atlas) and its corresponding json file will be generated. If the sprites don't fit in the atlas (when using --size or --max-size), a set of images with its json file will be generated.

The generated json file will have one of the following formats:

*Legacy*
```javascript
{
    "width": 512,                // texture atlas width
    "height": 512,               // texture atlas height

    "sprites": {
        "image1.png": {
            "x": 0,              // coords of sprite rect in atlas
            "y": 0,

            "width": 60,         // size of sprite rect in atlas
            "height": 100,

            "rotated": true,     // whether sprite is rotated or not (clockwise)
                                 // available when using --allow-rotate

            "real_width": 100,   // original dimensions of the image (when using --trim)
            "real_height": 60,

            "xoffset": 0,        // top-left offset from which the image was trimmed
            "yoffset": 0,

            "meta": /*...*/      // sprite metadata; available if --metadata
                                 // is given and it has data for the sprite
        },
        //...
    }
}
```

*JSON Hash*
```javascript
{"frames": {

"image1":
{
    "frame": {"x":249,"y":205,"w":213,"h":159},
    "rotated": false,
    "trimmed": true,
    "spriteSourceSize": {"x":0,"y":0,"w":213,"h":159},
    "sourceSize": {"w":231,"h":175}
},
"image2":
{
    "frame": {"x":20,"y":472,"w":22,"h":21},
    "rotated": false,
    "trimmed": false,
    "spriteSourceSize": {"x":0,"y":0,"w":22,"h":21},
    "sourceSize": {"w":22,"h":21}
}},
"meta": {
    "app": "https://github.com/urraka/texpack",
    "image": "atlas.png",
    "size": {"w":650,"h":497}
    }
}
```

*JSON Array*
```javascript
{"frames": [

{
    "filename": "image1",
    "frame": {"x":249,"y":205,"w":213,"h":159},
    "rotated": false,
    "trimmed": true,
    "spriteSourceSize": {"x":0,"y":0,"w":213,"h":159},
    "sourceSize": {"w":231,"h":175}
},
{
    "filename": "image2",
    "frame": {"x":29,"y":472,"w":22,"h":21},
    "rotated": false,
    "trimmed": false,
    "spriteSourceSize": {"x":0,"y":0,"w":22,"h":21},
    "sourceSize": {"w":22,"h":21}
}],
"meta": {
    "app": "https://github.com/urraka/texpack",
    "image": "atlas.png",
    "size": {"w":650,"h":497}
    }
}
```

**Example:**

This will take all PNG's in the current directory and generate the texture atlas in the out/ directory.

```bash
find . -name "*.png" | texpack -o out/atlas
```

**Building:**

Building has been tested on Linux, OSX and Windows (with MSYS/mingw-w64). Visual Studio is not supported.

In order to build, you will need to make sure that `libpng` and `zlib` are installed on your system. On Linux, you can use your system package manager to install these libraries. For example:

```bash
# Arch Linux
sudo pacman -S libpng

# Ubuntu
sudo apt-get install libpng12-dev
```

`zlib` is a `libpng` dependency so it should be installed along with it.

Once you have that just run `make` on the texpack project directory. If the libraries aren't installed on the default compiler paths, you can use `CFLAGS` and `LDFLAGS` to give it the required paths. For example, if the libraries were located inside `~/libs`:

```bash
export CFLAGS=-I~/libs/include
export LDFLAGS=-L~/libs/lib
make
```

The resulting binary file will be inside the `bin/` directory.

If you don't have a package manager you may have to download and compile both `zlib` and `libpng` yourself. Here's an example of how you could do this (do not just copy paste this):

```bash
# go to project directory
cd texpack

# create a directory for the libraries and move in there
mkdir ext
cd ext

# download required files
curl -L -O http://prdownloads.sourceforge.net/libpng/libpng-1.6.14.tar.gz?download
curl -L -O http://zlib.net/zlib-1.2.8.tar.gz

# extract them
tar xf libpng*
tar xf zlib*

# compile and install zlib
cd zlib*
./configure --prefix=`pwd`/..
make
make install
cd ..

# note: that won't work on MSYS (windows). you can do this instead:
cd zlib*
export BINARY_PATH="`pwd`/../bin"
export INCLUDE_PATH="`pwd`/../include"
export LIBRARY_PATH="`pwd`/../lib"
make -f win32/Makefile.gcc
make -f win32/Makefile.gcc install
cd ..

# compile and install libpng
cd libpng*
./configure --prefix=`pwd`/.. --with-zlib-prefix=`pwd`/..
make
make install
cd ..

# get back to the texpack directory and compile it
cd ..
export CFLAGS=-I`pwd`/ext/include
export LDFLAGS=-L`pwd`/ext/lib
make
```

**Building on Windows via CMake and vcpkg:**

```bash
# install vcpkg from an administrator console
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg integrate install 

# install dependencies
vcpkg install libpng --triplet x64-windows
vcpkg install zlib --triplet x64-windows
# or for static linking
vcpkg install libpng:x64-windows-static
vcpkg install zlib:x64-windows-static

# generate project and compile
git clone https://github.com/kerskuchen/texpack.git
cd texpack
mkdir solution
cd solution
cmake .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="D:/UserLib/vcpkg/scripts/buildsystems/vcpkg.cmake"
# or for static linking
cmake .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="D:/UserLib/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static

# Then we can open the solution and build it or run the following in a developer console:
msbuild texpack.vcxproj /nologo /p:configuration=Release /p:platform=x64 /p:OutDir=..\bin\
```