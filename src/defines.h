#ifndef DEFINES_H
#define DEFINES_H

#define PROGRAM_ORG "Pleune"
#define PROGRAM_NAME "Blocks"

#define RENDER_WOBBLE 0.1

#define INIT_WORLDGEN_THREADS 4

#define CHUNK_LEVELS 5

#define WORLD_CHUNKS_PER_EDGE 20

#define WORLDGEN_BUMPYNESS 3
#define WORLDGEN_RANGE 0.5
#define WORLDGEN_DIAMONDSQUARE_LEVELS 11

#define CHUNK_UNCOMPRESS 200
#define CHUNK_RECOMPRESS 100

#define PLAYER_FLY_SPEED 55
#define PLAYER_FRICTION 20.0
#define PLAYER_WALK_MAX_FORCE 150
#define PLAYER_JUMPSPEED 7

#define PLAYER_HEIGHT 3.7
#define PLAYER_EYEHEIGHT 3.4
#define PLAYER_MASS 30
#define PLAYER_WIDTH 1.8

#define SIM_WATER_LEVELS 10
#define SIM_WATER_DELAY (5 / (avg + rem))
#define GRAVITY 20

#define MOUSE_SENSITIVITY (1.1/800.0)

#define JOYSTICK_SENSITIVITY_LOOK 10

#define SAVE_PATH_MAX_LEN 512
#define SAVE_EXTENSION_CHUNKS ".cnks"
#define SAVE_EXTENSION_CHUNKS_LEN 5
#define SAVE_EXTENSION_CHUNKTABLE ".ctab"
#define SAVE_EXTENSION_CHUNKTABLE_LEN 5

#endif
