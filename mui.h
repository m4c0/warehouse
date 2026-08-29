#ifndef MUI_H
#define MUI_H

typedef struct mui_upc_s {
  float rect[4];
  float colour[4];
  float uv[4];
  float extent[2];
} mui_upc_t;
typedef struct mui_api_s {
  unsigned sw, sh;
  void * ptr;
  void (*draw)(void * ptr, const mui_upc_t * pc);
  void (*scissor)(void * ptr, unsigned x, unsigned y, unsigned w, unsigned h);
} mui_api_t;

extern int mui_overlay;

void mui_run(const mui_api_t * t);

void mui_mouse_down(int x, int y);
void mui_mouse_move(int x, int y);
void mui_mouse_up(int x, int y);

#ifdef MUI_IMPL
#include "gme.h"
#include "lvl.h"
#include "sav.h"
#include "sfx.h"

#include <math.h>

int mui_overlay = 0;

static int mui_mx, mui_my;
static int mui_md;
static int mui_mu;
static int mui_mid;

static int mui_font_width(char c) {
  if ((c | 0x20) == 'i') return 1;
  if ((c | 0x20) == 'm') return 5;
  if ((c | 0x20) == 'n') return 4;
  return 3;
}

static int mui_font_height() {
  return 5;
}

void mui_mouse_down(int x, int y) {
  mui_mouse_move(x, y);
  mui_md = 1;
}
void mui_mouse_move(int x, int y) {
  mui_mx = x; mui_my = y;
}
void mui_mouse_up(int x, int y) {
  mui_mouse_move(x, y);
  mui_mu = 1;
}

static float cuv(char c, char base) {
  float u = 0;
  for (char cc = base; cc < c; cc++) u += mui_font_width(cc) + 1;
  return u;
}
static void uv(float * uv, char c) {
  uv[2] = mui_font_width(c) / 128.f;
  uv[3] = mui_font_height() / 32.f;

  if (c >= 'A' && c <= 'Z') c |= 0x20;
  if (c >= 'a' && c <= 'z') {
    uv[0] = (32 + cuv(c, 'a')) / 128.f;
    uv[1] = 1.f / 32.f;
    return;
  }

  if (c >= '0' && c <= '5') {
    uv[0] = (8 + cuv(c, '0')) / 128.f;
    uv[1] = 9.f / 32.f;
    return;
  }
  if (c >= '6' && c <= '9') {
    uv[0] = (8 + cuv(c, '6')) / 128.f;
    uv[1] = 17.f / 32.f;
    return;
  }

  uv[0] = uv[1] = uv[2] = uv[3] = 0;
}

static int mui_strlen(const char * str) {
  int len = 0;
  for (const char * c = str; *c; c++) len += mui_font_width(*c) * 3 + 2;
  return len - 2;
}

static int mui_hover(float rect[4]) {
  return
    mui_mx >= rect[0] && mui_mx < (rect[0] + rect[2]) &&
    mui_my >= rect[1] && mui_my < (rect[1] + rect[3]);
}

static void mui_draw_box(const mui_api_t * t, float rect[4], float c0[4], float c1[4]) {
  mui_upc_t pc = {
    .rect   = { rect[0] - 2, rect[1] - 2, rect[2] + 4, rect[3] + 4 },
    .colour = { c0[0], c0[1], c0[2], c0[3] },
    .extent = { t->sw, t->sh },
  };
  t->draw(t->ptr, &pc);

  pc = (mui_upc_t) {
    .rect   = { rect[0], rect[1], rect[2], rect[3] },
    .colour = { c1[0], c1[1], c1[2], c1[3] },
    .extent = { t->sw, t->sh },
  };
  t->draw(t->ptr, &pc);
}

static int mui_draw_icon(const mui_api_t * t, float rect[4], float dim, float id) {
  int hover = mui_hover(rect);
  if (hover) {
    dim = 1;

    if (mui_md) mui_mid = id;
  }

  mui_upc_t pc = {
    .rect   = { rect[0], rect[1], rect[2], rect[3] },
    .colour = { dim, dim, dim, id },
    .extent = { t->sw, t->sh },
  };
  t->draw(t->ptr, &pc);

  return hover && mui_mu && (mui_mid == id);
}

static int mui_draw_window(const mui_api_t * t, float rect[4]) {
  int hover_out = !mui_hover(rect);
  if (hover_out && mui_md) mui_mid = 1;
  if (hover_out && mui_mu && mui_mid == 1) {
    return 0;
  }

  mui_draw_box(t, rect, 
      (float[4]) { 0.27, 0.47, 0.35, 0xFFFF },
      (float[4]) { 0.04, 0.12, 0.08, 0xFFFF });

  return 1;
}

static void mui_draw_str(const mui_api_t * t, const char * str, float x, float y) {
  mui_upc_t pc = {
    .rect   = { x, y, 0, 15 },
    .colour = { 0.9, 0.9, 0.9, 0 },
    .extent = { t->sw, t->sh },
  };
  for (const char * c = str; *c; c++) {
    pc.rect[2] = mui_font_width(*c) * 3;
    uv(pc.uv, *c);
    t->draw(t->ptr, &pc);
    pc.rect[0] += pc.rect[2] + 2;
  }
}

static int mui_draw_btn(const mui_api_t * t, int id, const char * str, float rect[4]) {
  float dim = 0.8;
  int hover = mui_hover(rect);
  if (hover) {
    dim = 1;
    if (mui_md) mui_mid = id;
  }

  mui_draw_box(t, rect, 
      (float[4]) { dim * 0.08, dim * 0.24, dim * 0.16, 0xFFFF },
      (float[4]) { dim * 0.27, dim * 0.47, dim * 0.35, 0xFFFF });

  float w = mui_strlen(str);
  mui_draw_str(t, str, rect[0] + (rect[2] - w) / 2, rect[1] + (rect[3] - 15) / 2);
  return hover && mui_mu && (mui_mid == id);
}

static void mui_draw_lvl(const mui_api_t * t, float rect[4]) {
  float dim = 0.8;
  int hover = mui_hover(rect);
  if (hover) {
    dim = 1.0;
    if (mui_md) mui_mid = 0xF;
  }
  if (mui_mid == 0xF) {
    dim = 1.0;
    float l = roundf(sav_data.max_level * (mui_mx - rect[0]) / rect[2]);
    gme_level((l < 1 ? 1 : l >= sav_data.max_level ? sav_data.max_level : l) - 1);
  }

  mui_draw_box(t, rect, 
      (float[4]) { dim * 0.08, dim * 0.24, dim * 0.16, 0xFFFF },
      (float[4]) { 0.04, 0.12, 0.08, 0xFFFF });

  char str[16];
  snprintf(str, 16, "Level %02d", lvl_current + 1);
  float w = mui_strlen(str);
  mui_draw_str(t, str, rect[0] + (rect[2] - w) / 2, rect[1] + (rect[3] - 15) / 2);

  float x = (rect[2] - 8) * lvl_current / ((float)sav_data.max_level - 1);

  mui_draw_box(t,
      (float[4]) { rect[0] + x, rect[1], 8, rect[3] },
      (float[4]) { dim * 0.08, dim * 0.24, dim * 0.16, 0xFFFF },
      (float[4]) { dim * 0.27, dim * 0.47, dim * 0.35, 0xFFFF });
}

static int mui_st_options = 0;
static void mui_guarded_run(const mui_api_t * t) {
  if (mui_draw_icon(t, (float[4]) { t->sw - 48 - 12, 12, 48, 48 }, 0.8, 0xEE00)) {
    mui_st_options = !mui_st_options;
  }
  if (!mui_st_options) return;

  int wx = (t->sw - 340) / 2;
  int wy = (t->sh - 200) / 2;
  if (!mui_draw_window(t, (float[4]) { wx, wy, 340, 200 })) {
    mui_st_options = 0;
    return;
  }

  float cl = wx + 16;
  float cr = wx + 340 - 16;
  float r1 = wy + 32;
  float r2 = r1 + 50;
  float r3 = r2 + 60;

  mui_draw_str(t, "Sound", cl, r1);

  const char * sfx = sfx_enabled() ? "ON" : "OFF";
  if (mui_draw_btn(t, 2, sfx, (float[4]) { cr - 60, r1 - 8, 60, 15 + 16 })) {
    sfx_toggle();
  }

  mui_draw_lvl(t, (float[4]) { cl, r2, cr - cl, 15 + 16 });

  if (mui_draw_btn(t, 4, "Restart Level", (float[4]) { cl, r3, cr - cl, 15 + 16 })) {
    gme_level(lvl_current);
    mui_st_options = 0;
    return;
  }
}

void mui_run(const mui_api_t * t) {
  mui_guarded_run(t);

  if (mui_mu) mui_mid = 0;
  mui_md = mui_mu = 0;

  mui_overlay = mui_st_options;
  gme_enabled = !mui_st_options;
}

#endif
#endif
