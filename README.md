# Blocks
![](http://pleune.github.io/Blocks/screenshots/16-04-11-23-01-49.png)

This is a project that I work on very sporadically when I have the time and energy to do so. In the past, development has ceased for entire months at a time. This is just a hobby of mine, and so time gets spent where I think is interesting. Much of the code may be very messy or inefficient, but if it's working for me then it may not get worked on.

# Building / Running

#### NixOS
- Just clone and ./run

#### Everyone Else

- To run:
	- Linux:
		- Make sure glew, SDL2, SDL2_ttf, zlib, and supporting OpenGL packages are installed
	- Windows:
		- Make sure glew32.dll, libfreetype-6.dll, SDL2.dll, SDL2_ttf.dll, and zlib1.dll are availaible (If availaible via PATH, it may need to be early in the PATH)

- To compile:
	- Linux:
		- make sure the devel versions of the above packages are installed,
		along with pkg-config for sdl2, SDL2_ttf, zlib, and glew
	- Windows:
		- Make sure SDL2, SDL2_ttf, zlib, and glew include and lib files are copied into MinGW's folders Make MinGW is installed with pkg-config set up for sdl2, SDL2_ttf, and glew
