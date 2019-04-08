Mandelbrot shader


This program draws the Mandelbrot set using several different methods. There's an interactive mode and a benchmark mode. The benchmark can only be run once each time the program is launched.


*** Controls:

Enter - run benchmark using each of the supported methods with 255 iterations per pixel. 

1 - do the math on the GPU using a single loop with dynamic branching. Requires a GPU with SM3 support.

2 - same as above, only without dynamic branching. Should run on a SM2 or SM3 GPU.

3 - same as above, using multipass rendering to a frame buffer object. Every 5 iterations the result is stored in a floating point texture, and read back as input for the next 5 loops. Requires SM2 hardware with support for FBO's and floating point textures.

4 - do all math on the CPU, using multiple threads if multiple processors are detected.

*,/ - decrease/increase the number of iterations per pixel by 5. The minimum is 5. The maximum is 255.

Left mouse button or arrow keys - pan the camera.

Mouse wheel or plus/minus keys - zoom.

Press mouse wheel or space bar to reset the camera.

Tab - show/hide info

Esc - exit the program.


*** Notes:

The resolution is fixed at 1024 x 768. This is used in all cases, except when benchmarking the cpu. The cpu benchmark only renders an image of 512 x 384, in order to measure frames per second and not the other way around.

When multiple threads are used for cpu rendering, each thread renders an interlaced image - meaning that for 2 threads each thread renders every other line. With 4 threads each one does every 4th line and so on. The number of rendering threads is equal to the number of processors detected.

GPU's are limited to 32-bit precision. The CPU renderer uses 64-bit precision, so you can zoom in much more.

By default the application uses OpenGL do display the final image. If OpenGL acceleration is not available, only CPU rendeting can be used, and will be displayed using GDI software rendering.