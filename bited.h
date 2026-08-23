#ifndef BITED_H
#define BITED_H

typedef struct btd_upc {
  int x, y;
} btd_upc_t;
static btd_upc_t btd_pc;

static uint8_t btd_atlas[128 * 32];

void btd_replace_atlas();

void btd_cursor(int dx, int dy) {
  int x = btd_pc.x + dx;
  if (x >= 0 && x < 128) btd_pc.x = x;

  int y = btd_pc.y + dy;
  if (y >= 0 && y < 128) btd_pc.y = y;
}

void btd_toggle() {
  int i = btd_pc.y * 128 + btd_pc.x;
  btd_atlas[i] = btd_atlas[i] ? 0 : 255;
  btd_replace_atlas();
}

void btd_load() {
  FILE * f = fopen("atlas.img", "rb");
  fread(btd_atlas, 128 * 32, 1, f);
  fclose(f);
  btd_replace_atlas();
}
void btd_save() {
  FILE * f = fopen("atlas.img", "wb");
  fwrite(btd_atlas, 128 * 32, 1, f);
  fclose(f);
}

#endif
