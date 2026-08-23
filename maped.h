#ifndef MAPED_H
#define MAPED_H

#include "gme.h"
#include "lvl.h"
#include "tim.h"

typedef struct mpd_upc {
  float sel_rect_x, sel_rect_y, sel_rect_w, sel_rect_h;
  float player_pos_x, player_pos_y;
  float label_pos_x, label_pos_y;
  float cursor_x, cursor_y;
  float level;
  float aspect;
  float time;
  float overlay;
  float back_btn_dim;
  float menu_btn_dim;
} mpd_upc_t;

static mpd_upc_t mpd_pc;
static int mpd_cur_x = LVL_WIDTH / 2;
static int mpd_cur_y = LVL_HEIGHT / 2;

static char mpd_ptr[LVL_SZ];

void mpd_update_map();

static void mpd_load_map(int lvl) {
  lvl_load(lvl, mpd_ptr);
  mpd_update_map();
}
void mpd_load_next_level() {
  int lvl = lvl_current + 1;
  if (lvl > lvl_max_level) lvl = 0;
  mpd_load_map(lvl);
}
void mpd_load_prev_level() {
  int lvl = lvl_current - 1;
  if (lvl < 0) lvl = lvl_max_level;
  mpd_load_map(lvl);
}

void mpd_cursor(int dx, int dy) {
  mpd_cur_x += dx;
  if (mpd_cur_x < 0) mpd_cur_x = 0;
  if (mpd_cur_x >= LVL_WIDTH) mpd_cur_x = LVL_WIDTH;

  mpd_cur_y += dy;
  if (mpd_cur_y < 0) mpd_cur_y = 0;
  if (mpd_cur_y >= LVL_HEIGHT) mpd_cur_x = LVL_HEIGHT;
}

static void mpd_clear_player() {
  char * p = mpd_ptr + lvl_py * LVL_WIDTH + lvl_px;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_player:        *p = gme_b_empty;  break;
    case gme_b_player_target: *p = gme_b_target; break;
    default: assert(0 && "invalid map state in old player pos"); // unreachable
  }
}
static void mpd_update_player(char * p, char c) {
  mpd_clear_player();
  *p = c;        
  lvl_px = mpd_cur_x;
  lvl_py = mpd_cur_y;
}
void mpd_player() {
  char * p = mpd_ptr + mpd_cur_y * LVL_WIDTH + mpd_cur_x;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_empty:  mpd_update_player(p, gme_b_player);        break;
    case gme_b_target: mpd_update_player(p, gme_b_player_target); break;
  }
  mpd_update_map();
}

void mpd_empty() {
  char * p = mpd_ptr + mpd_cur_y * LVL_WIDTH + mpd_cur_x;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_outside: *p = gme_b_empty;   break;
    case gme_b_empty:   *p = gme_b_outside; break;
    case gme_b_wall:    *p = gme_b_empty;   break;
  }
  mpd_update_map();
}

void mpd_wall() {
  char * p = mpd_ptr + mpd_cur_y * LVL_WIDTH + mpd_cur_x;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_empty:
    case gme_b_outside: *p = gme_b_wall;    break;
    case gme_b_wall:    *p = gme_b_outside; break;
  }
  mpd_update_map();
}

void mpd_box() {
  char * p = mpd_ptr + mpd_cur_y * LVL_WIDTH + mpd_cur_x;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_outside:
    case gme_b_empty:      *p = gme_b_box;        break;
    case gme_b_box:        *p = gme_b_empty;      break;
    case gme_b_target:     *p = gme_b_target_box; break;
    case gme_b_target_box: *p = gme_b_target;     break;
  }
  mpd_update_map();
}
void mpd_target() {
  char * p = mpd_ptr + mpd_cur_y * LVL_WIDTH + mpd_cur_x;
  if (p < mpd_ptr || p >= mpd_ptr + LVL_SZ) return;
  switch (*p) {
    case gme_b_outside:
    case gme_b_empty:         *p = gme_b_target;        break;
    case gme_b_box:           *p = gme_b_target_box;    break;
    case gme_b_target:        *p = gme_b_empty;         break;
    case gme_b_target_box:    *p = gme_b_box;           break;
    case gme_b_player:        *p = gme_b_player_target; break;
    case gme_b_player_target: *p = gme_b_player;        break;
  }
  mpd_update_map();
}

void mpd_save() {
  FILE * f = fopen("levels.txt", "r+");
  assert(0 == fseek(f, (LVL_SZ + 3) * lvl_current + 1, SEEK_SET));
  assert(1 == fwrite(mpd_ptr, LVL_SZ, 1, f));
  fclose(f);
}

void mpd_frame(void) {
  mpd_pc.cursor_x = mpd_cur_x;
  mpd_pc.cursor_y = mpd_cur_y;
  mpd_pc.label_pos_x = lvl_min_x;
  mpd_pc.label_pos_y = lvl_min_y - 1;
  mpd_pc.player_pos_x = lvl_px;
  mpd_pc.player_pos_y = lvl_py;
  mpd_pc.level = lvl_current + 1;
  mpd_pc.aspect = (float)800 / (float)600;
  mpd_pc.time = tim_now();
}

#endif
