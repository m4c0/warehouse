#ifndef SAV_H
#define SAV_H

typedef struct sav_s {
  unsigned cur_level; // zero-based
  unsigned max_level; // one-based, for reasons
} sav_t;

extern sav_t sav_data;

void sav_load();
void sav_save();

#ifdef SAV_IMPL
#ifdef _WIN32
#  include <direct.h>
#  define SEP "\\"
#else
#  include <sys/stat.h>
#  define SEP "/"
#  define _mkdir(X) mkdir(X, 0777)
#endif

sav_t sav_data;

void sav_get_path(char * buf, unsigned sz);

static FILE * sav_open(const char * mode) {
  char path[10240];
  sav_get_path(path, sizeof(path));

  int len = strlen(path);
  strncat(path + len, SEP "sokoban", sizeof(path) - len);
  _mkdir(path);

  len = strlen(path);
  strncat(path + len, SEP "save.dat", sizeof(path) - len);
  return fopen(path, mode);
}

void sav_load() {
  FILE * f = sav_open("rb");
  if (!f) {
    sav_data.cur_level = 0;
    sav_data.max_level = 1;
    return;
  }
  fread(&sav_data, sizeof(sav_t), 1, f);
  fclose(f);
}

void sav_save() {
  FILE * f = sav_open("wb");
  fwrite(&sav_data, sizeof(sav_t), 1, f);
  fclose(f);
}

#endif
#endif
