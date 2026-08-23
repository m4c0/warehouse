#define CFLAGS "-g"

#include "build.h"

#define CROSS(X) RUN("spirv-cross", X".spv", "--msl", "--output", APP".app/Contents/Resources/"X".metal", "--flip-vert-y");

static void print_key(FILE * f, const char * key) {}

static int pch() {
  RUN("clang", "-Wall", "-x", "c-header", "-o", "pch.pch", "pch.h", CFLAGS);
  return 0;
}

static int link_exe() {
  RUN("clang", "-Wall", "-o", APP".app/Contents/MacOS/main", OBJS, "sokoban-osx.o");
  return 0;
}

static int shots_exe() {
  RUN("clang", "-Wall", "-o", APP".app/Contents/MacOS/shots", OBJS, "shots.o");
  return 0;
}

static int bited_exe() {
  RUN("clang", "-Wall", "-o", APP".app/Contents/MacOS/bited", "bited.o");
  return 0;
}

static int maped_exe() {
  RUN("clang", "-Wall", "-o", APP".app/Contents/MacOS/maped", "lvl.o", "maped.o");
  return 0;
}

int main(int argc, char ** argv) {
  mkdir(APP".app", 0777);
  mkdir(APP".app/Contents", 0777);
  mkdir(APP".app/Contents/MacOS", 0777);
  mkdir(APP".app/Contents/Resources", 0777);

  if (pch()) return 1;

  CM("sokoban-osx");
  if (compile_and_link_exe()) return 1;
  if (shaders()) return 1;
  CROSS("bited.frag");
  CROSS("bited.vert");
  CROSS("mui-vlk.frag");
  CROSS("mui-vlk.vert");
  CROSS("sokoban.frag");
  CROSS("sokoban.vert");

  CM("bited");
  if (bited_exe()) return 1;

  CM("maped");
  if (maped_exe()) return 1;

  CM("shots");
  if (shots_exe()) return 1;

  RUN("cp", "atlas.img",  APP".app/Contents/Resources/");
  RUN("cp", "levels.txt", APP".app/Contents/Resources/");

  return 0;
}
