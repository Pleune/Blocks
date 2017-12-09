cmake_minimum_required(VERSION 3.1)
project(CDSS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(ZLIB REQUIRED)

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf)

set(SOURCE_FILES
  src/block.c
  src/blockpick.c
  src/chunk.c
  src/custommath.c
  src/debug.c
  src/entity.c
  src/gl.c
  src/hash.c
  src/hmap.c
  src/interface.c
  src/main.c
  src/noise.c
  src/octree.c
  src/save.c
  src/stack.c
  src/state.c
  src/state_game.c
  src/state_inventory.c
  src/state_menu.c
  src/state_world.c
  src/textbox.c
  src/update.c
  src/world.c
  src/worldgen.c
  src/block.h
  src/blockpick.h
  src/cat.h
  src/chunk.h
  src/custommath.h
  src/debug.h
  src/defines.h
  src/directions.h
  src/entity.h
  src/gl.h
  src/hash.h
  src/hmap.h
  src/interface.h
  src/minmax.h
  src/modulo.h
  src/noise.h
  src/octree.h
  src/save.h
  src/stack.h
  src/standard.h
  src/state.h
  src/state_game.h
  src/textbox.h
  src/update.h
  src/world.h
  src/worldgen.h)

include_directories(
  ${SDL2_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS})

add_executable(blocks ${SOURCE_FILES})
target_link_libraries(blocks
  m
  ${CMAKE_THREAD_LIBS_INIT}
  ${SDL2_LIBRARIES}
  ${SDL2TTF_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES})
