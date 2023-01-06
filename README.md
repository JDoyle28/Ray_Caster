# Ray_Caster
Given an input scene file, the ray caster uses various light equations to shoot and calculate 
the amount of light at a single pixel, for every pixel in the output image. Output is written in the form of a P3 ppm file.

Usage:
make
./raycast 1000 1000 input.csv output.ppm

The given make file compiles the code through the first command
The second command executes the code, parameters two and three represent the width and height respectively,
The third parameter is the input scene file and the forth is the output image

Note:
Input scene file can be altered to create differing images. For example, we can add multiple spheres, planes or lights
to the image, with the fredom of location and size. However, altering the image can have interesting effects to the perspective
using the given equations. 
