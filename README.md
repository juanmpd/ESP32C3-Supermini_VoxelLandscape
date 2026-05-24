# PoC: Retro voxel landscape render with ESP32-C3 Supermini

This is a voxel-space renderer running on an ESP32-C3 Supermini with an ST7789V display.

<video width="320" height="240" controls>
  <source src="./demo.mp4" type="video/mp4">
</video>

I'm using a pin assignment that allows me to plug-and-play the display directly over the microcontroller on a protoboard without any cables at all, or to solder it directly on its belly without any wiring (except for the BackLight control if supported by the TFT display, as in this case).

Its algorithm was initially taken from a 1994 Pascal landscape generator, which was part of SWAG (a collection of source code and program examples for the Pascal programming language from the MS-DOS era). So, the credits for the algorithm should go to Marcin Borkowski.

You can find a copy of the original Pascal program here: 
https://github.com/ncabanes/swag-pascal/blob/master/TurboPascal-untested/graphics/Landscape.pas  


The algorithm is a voxel-space renderer in the style of the old Comanche/NovaLogic games: a 2D height map rendered from back to front as pseudo-3D columns, with horizon and simple hiding based on scan lines.
The terrain is a wrap-around toroidal map, with heights computed randomnly on start using a diamond-square fractal plasma.

I transformed this demo into a Java applet long, long ago, and now I’ve turned it into a C++ demo to run on my ESP32-C3 Supermini. It's been compiled with VisualStudio Code with the PioArduino extension, using the Lovyan display library.

Due to its reduced amount of RAM and fragmentation limitations, I’ve had to allocate the memory for the terrain by rows to maximize the size of the displayed image, which now gets very close to the full resolution.

