#ifndef GLU_H
#define GLU_H

#include "gme.h"
#include "lvl.h"
#include "mui.h"
#include "sav.h"
#include "sfx.h"
#include "snd.h"
#include "tim.h"

#define GLU_BUF_SIZE (LVL_SZ * 4)

typedef struct glu_upc_s {
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
} glu_upc_t;
glu_upc_t glu_pc;

typedef struct glu_init_s {
  const void * level;
  unsigned level_sz;

  int sound;
  int scr_w, scr_h;
} glu_init_t;

static unsigned glu_sw;
static unsigned glu_sh;

void glu_resize(unsigned w, unsigned h) {
  glu_sw = w;
  glu_sh = h;
  glu_pc.aspect = (float)w / (float)h;
}
void glu_init(const glu_init_t * t) {
  glu_pc.cursor_x = glu_pc.cursor_y = 10000;

  lvl_init(t->level, t->level_sz);

  sfx_init(t->sound);
  glu_resize(t->scr_w, t->scr_h);

  gme_init();

  mui_init();
  snd_init(&sfx_filler);
}

void glu_deinit(void) {
  snd_deinit();
}

void glu_load(void * into) {
  unsigned * d = into;
  const char * m = gme_map_ptr();
  for (int i = 0; i < LVL_SZ; i++) d[i] = m[i];
}

void glu_frame(void) {
  glu_pc.label_pos_x  = lvl_min_x;
  glu_pc.label_pos_y  = lvl_min_y - 1;
  glu_pc.overlay      = mui_overlay ? 0.3 : 0.0;
  glu_pc.player_pos_x = lvl_px;
  glu_pc.player_pos_y = lvl_py;
  glu_pc.level        = lvl_current + 1;
  glu_pc.time         = tim_now();
}

void glu_ui(const mui_api_t * t) {
  mui_api_t tt = *t;
  tt.sw = glu_sw;
  tt.sh = glu_sh;
  mui_run(&tt);
}

void glu_move(int dx, int dy) {
  gme_move(dx, dy);
}

void glu_mouse_move(int x, int y) {
  mui_mouse_move(x, y);
}
void glu_mouse_down(int x, int y) {
  mui_mouse_down(x, y);
}
void glu_mouse_up(int x, int y) {
  mui_mouse_up(x, y);
}

#endif

