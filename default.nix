with import <nixpkgs> {}; {
  sdlEnv = stdenv.mkDerivation {
    name = "sdl";
    buildInputs = [ glew SDL2 SDL2_ttf gdb ];
  };
}
