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
-f, --allow-flip      Allows sprites to be flipped/rotated for better packing.
-m, --metadata        Input metadata file in json format. (*)
-e, --pretty          Generated json file will be human readable.
-i, --indentation     String for json indentation. Used along with --pretty.
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
      "anotherimage.png": {"param1": "some-value", "param2": 0, ...},
      ...
    }

    Each file name should match one in the <input-file> list.
```

**Output:**

At least one image (the texture atlas) and its corresponding json file will be generated. If the sprites don't fit in the atlas (when using --size or --max-size), a set of images with its json file will be generated.

The generated json file will have the following format:

```
{
    "width": <atlas width>,
    "height": <atlas height>,
    "sprites": [
        "image1.png": {
            "w": <width of the original image>,
            "h": <height of the original image>,
            "tl": {"x": <top-left-x>,     "y": <top-left-y>},
            "br": {"x": <bottom-right-x>, "y": <bottom-right-y>},
            ... (metadata values if given)
        },
        "image2.png": { ... },
        ...
    ]
}
```

Note: in case that --allow-flip is used, the (tl,br) coordinates of each sprite get flipped along with the image. You can use this fact to detect if a sprite is flipped or not.

**Example:**

This will take all PNG's in the current directory and generate the texture atlas in the out/ directory.
```
mkdir -p out
find . -name "*.png" | texpack -o out/atlas
```

The packer won't create any directories, so make sure they already exist before running it if needed.

**Building:**

Adapt makefile if needed. External dependencies are `libpng` and `zlib`. It also depends on `jsoncpp` and code from `RectangleBinPack` but that's embedded in the source.
