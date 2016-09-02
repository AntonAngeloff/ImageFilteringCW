A brief project on image filtering and convolution matrices based on exercise on Digital Image Processing class in university.

SDL2 is used for window management so it is required to be installed on the system in order to compile the project. Compilation could be done either via build.sh script or through Eclipse by opening the .cproject file.

Generally the project implements `filter_apply()` routine (inside filters.c) which applies arbitrary convolution matrix on a arbitrary-sized 32-bit bitmap. Edge pixels on the bitmap are handled through wrapping. 
In filter.c are provided 9 built-in matrices like blur with 5x5 kernel, sharpen, emboss and Sobel operator.

Two input formats are supported - PGM and BMP, while the output is only in PGM.

For license information, see LICENSE.txt.