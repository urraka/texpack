### **texpack**

Simple command line texture packer based on the MaxRects algorithm by Jukka Jylänki (https://github.com/juj/RectangleBinPack).

**Usage:**
```
{file-list} | texpack -o <output> [-p <padding>] [-m <metadata>] [-b]


{file-list}:    List of PNG files separated by new-line's.

-o <output>:    Prefix for the output files. If it contains a directory it must exist prior to
                execution.

-p <padding>:   Amount of padding between individual sprites and the image border.

-m <metadata>:  Json file with data that will be included in the generated json file.
                It must be an object where each key matches a file from {file-list} (must match
                exactly as passed).

-b:             Enables "alpha bleeding", which bleeds the border colors through the fully
                transparent pixels.
```

**Output:**

There will be 5 images generated that correspond to the different methods of the MaxRects algorithm (BestShortSideFit, BestLongSideFit, BestAreaFit, BottomLeftRule, ContactPointRule).

The generated image will have a power of 2 size.

The sprites might get rotated. In that case the `tl` coordinate (top-left) will correspond to the top-right of the sprite as it appears in the resulting image.

The generated json file will have the following format:

```
{
	"image1.png": {
    	"w": <width of the original image>,
        "h": <height of the original image>,
        "tl": {"x": <top-left-x>,     "y": <top-left-y>},
        "br": {"x": <bottom-right-x>, "y": <bottom-right-y>},
        ... metadata ...
    },
    "image2.png": { ... },
    ...
}
```

**Example:**

This will take all PNG's in the current directory and generate the texture atlas in the out/ directory.
```
mkdir -p out
find . -name "*.png" | texpack -o out/atlas
```

**Building:**

Adapt makefile as needed. External dependencies are `libpng` and `zlib`. It also depends on `jsoncpp` and code from `RectangleBinPack` but that's embedded in the source.
