#!/bin/bash -x
cd Release
gcc -O3 -Wall -c -fmessage-length=0 -o filters.o "..\\filters.c" 
gcc -O3 -Wall -c -fmessage-length=0 -o imgutils_bmp.o "..\\imgutils_bmp.c" 
gcc -O3 -Wall -c -fmessage-length=0 -o imgutils_pgm.o "..\\imgutils_pgm.c" 
gcc -O3 -Wall -c -fmessage-length=0 -o histogram.o "..\\histogram.c" 
gcc -O3 -Wall -c -fmessage-length=0 -o imgutils.o "..\\imgutils.c" 
gcc -O3 -Wall -c -fmessage-length=0 -o main.o "..\\main.c" 
gcc -o CourseWork_DIP.exe filters.o histogram.o imgutils.o imgutils_bmp.o imgutils_pgm.o main.o -lmingw32 -lSDL2main -lSDL2 
cd ..
