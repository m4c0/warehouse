//#define OPT "-gdwarf"
#define OPT "-O3"

#define CFLAGS OPT
#define RES_PATH(X) "."
#include "build.h"

#define CROSS(X) RUN("spirv-cross", X".spv", "--hlsl", "--output", X".hlsl", "--shader-model", "50", "--flip-vert-y");

static int pch() {
  RUN("clang", "-Wall", "-x", "c-header", CFLAGS, "-o", "pch.pch", "pch.h");
  return 0;
}

static int bited_exe() {
  RUN("clang", "-Wall", OPT, "-o", "bited.exe", "bited.o");
  return 0;
}
static int maped_exe() {
  RUN("clang", "-Wall", OPT, "-o", "maped.exe", "maped.o", "lvl.o");
  return 0;
}
static int link_exe() {
  RUN("clang", "-Wall", OPT, "-o", APP".exe", "main.res", "sokoban-win.o", OBJS);
  return 0;
}

static void print_key(FILE * f, const char * p) {
  char * env = getenv(p);
  if (strncmp(p, "WIN_", 4)) {
    assert(fprintf(f, "&%s;", p));
  } else if (env) {
    assert(fprintf(f, "%s", env));
  } else {
    fprintf(stderr, "Missing environment: %s\n", p);
    exit(1);
  }
}
static int pack() {
  if (getenv("WIN_BUILD_ONLY")) return 0;

  // https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/how-to-create-a-basic-package-manifest
  if (apply("AppxManifest.xml.in", "AppxManifest.xml")) return 1;

  unlink(APP".msix");

  char argv0[1024];
  snprintf(argv0, 1024,
      "c:\\Program Files (x86)\\Windows Kits\\10\\bin\\%s\\x64\\makeappx.exe",
      getenv("WIN_KIT_VERSION"));

  char argv1[1024];
  snprintf(argv1, 1024, "\"%s\"", argv0);
  return _spawnl(_P_WAIT, argv0, argv1, "pack", "/f", "AppxMapping.ini", "/p", APP".msix", NULL);
}

int icon() {
  unsigned sz;
  char * img = slurp("Assets.xcassets\\AppIcon.appiconset\\Icon-1024.png", &sz);

  FILE * f = fopen("icon.ico", "wb");
  fwrite("\0\0\1\0\1\0", 6, 1, f); // 0=Reserved; 1=ICO; 1 Image
  fwrite("\0\0\0\0\0\0\x20\0", 8, 1, f); // W/H/C/Res. Planes/Bits

  fwrite(&sz, 4, 1, f);
  fwrite("\x16\0\0\0", 4, 1, f); // 20=offset from BOS
  fwrite(img, sz, 1, f);

  fclose(f);
  return 0;
}

int main(int argc, char ** argv) {
  _mkdir("app");

  if (pch()) return 1;

  if (icon())    return 1;
  if (shaders()) return 1;
  CROSS("bited.frag");
  CROSS("bited.vert");
  CROSS("mui-vlk.frag");
  CROSS("mui-vlk.vert");
  CROSS("sokoban.frag");
  CROSS("sokoban.vert");
  RUN("llvm-rc", "/FO", "main.res", "main.rc");

  CC("sokoban-win");
  if (compile_and_link_exe()) return 1;

  CC("bited");
  if (bited_exe()) return 1;

  CC("maped");
  if (maped_exe()) return 1;

  if (pack()) return 1;
  return 0;
}
