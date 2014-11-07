### **texpack**

Simple command line texture packer based on the MaxRects algorithm by Jukka Jylänki (https://github.com/juj/RectangleBinPack).

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
-S, --max-size        Max size for the atlas image (ignored if size is used).
-P, --POT             Keep atlas size a power of two (ignored if size is used).
-r, --allow-rotate    Allows sprites to be rotated for better packing.
-m, --metadata        Input metadata file in json format. (*)
-e, --pretty          Generated json file will be human readable.
-i, --indentation     Number of spaces for indentation, 0 to use tabs (default).
-b, --alpha-bleeding  Post-process atlas image with an alpha bleeding algorithm.
-M, --mode            Specifies the packing heuristic. Allowed values are:
                        * auto (default; tries all modes and selects one)
                        * bottom-left
                        * short-side
                        * long-side
                        * best-area
                        * contact-point

(*) The format of the metadata file should be as follows:

    {
      "someimage.png":    {"param1": "some-value", "param2": 0, ...},
      "anotherimage.png": "not necessarily an object",
      ...
    }

    Each file name should match one in the <input-file> list.
```

**Output:**

At least one image (the texture atlas) and its corresponding json file will be generated. If the sprites don't fit in the atlas (when using --size or --max-size), a set of images with its json file will be generated.

The generated json file will have a format like this:

```
{
    "width": 512,              // texture atlas width
    "height": 512,             // texture atlas height

    "sprites": {
        "image1.png": {
            "x": 0,            // coords of sprite rect in atlas
            "y": 0,

            "width": 60,       // size of sprite rect in atlas
            "height": 100,

            "rotated": true,   // whether sprite is rotated or not (clockwise)
                               // available if used --allow-rotate

            "meta": ...        // sprite metadata; available if --metadata
                               // is given and it has data for the sprite
        },
        ...
    }
}
```

**Example:**

This will take all PNG's in the current directory and generate the texture atlas in the out/ directory.
```
find . -name "*.png" | texpack -o out/atlas
```

**Building:**

Adapt makefile if needed. External dependencies are `libpng` and `zlib`. It also depends on `jsoncpp` and code from `RectangleBinPack` but that's embedded in the source.
