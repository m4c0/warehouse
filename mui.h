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

void mui_init();
void mui_run(const mui_api_t * t);

int mui_font_width(char c);
int mui_font_height();

void mui_mouse_down(int x, int y);
void mui_mouse_move(int x, int y);
void mui_mouse_up(int x, int y);

#ifdef MUI_IMPL
#include "gme.h"
#include "lvl.h"
#include "sav.h"
#include "sfx.h"

#include "microui.h"

mu_Context mui_ctx = {0};
int mui_overlay = 0;

int mui_font_width(char c) {
  if ((c | 0x20) == 'i') return 1;
  if ((c | 0x20) == 'm') return 5;
  if ((c | 0x20) == 'n') return 4;
  return 3;
}

int mui_font_height() {
  return 5;
}

static int font_width(mu_Font f, const char * txt, int len) {
  int w = 0;
  for (; *txt; txt++) w += mui_font_width(*txt) * 3 + 2;
  return w;
}
static int font_height(mu_Font f) {
  return mui_font_height() * 3;
}

void mui_init() {
  mu_init(&mui_ctx);

  mui_ctx.text_width  = &font_width;
  mui_ctx.text_height = &font_height;

  mui_ctx.style->colors[MU_COLOR_WINDOWBG] = mu_color(10,  30, 20, 255);
  mui_ctx.style->colors[MU_COLOR_BUTTON]   = mu_color(70, 120, 90, 255);
}

void mui_mouse_down(int x, int y) {
  mu_input_mousedown(&mui_ctx, x, y, 1);
}
void mui_mouse_move(int x, int y) {
  mu_input_mousemove(&mui_ctx, x, y);
}
void mui_mouse_up(int x, int y) {
  mu_input_mouseup(&mui_ctx, x, y, 1);
}

static void mui_label(const char * txt) {
  int pad = mui_ctx.style->padding;

  mui_ctx.style->padding = 0;
  mu_label(&mui_ctx, txt);
  mui_ctx.style->padding = pad;
}
static void mui_vspace(int n) {
  mu_layout_row(&mui_ctx, 1, (int[]) { -1 }, n);
  mu_layout_next(&mui_ctx);
}

static float mui_lvl = 1;
static int mui_first_open_ever = 1;

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

void mui_run(const mui_api_t * t) {
  mu_begin(&mui_ctx);

  mui_ctx.style->padding = 12;
  mui_ctx.style->spacing = 8;

  int toggle_options = 0;

  int opt = MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME | MU_OPT_NOSCROLL;
  if (mu_begin_window_ex(&mui_ctx, "!main", mu_rect(0, 0, t->sw, 70), opt)) {
    mu_layout_row(&mui_ctx, 2, (int[]) { -56, -1 }, 48);
    mu_layout_next(&mui_ctx);
    if (mu_button_ex(&mui_ctx, "", 0xEE00, opt)) toggle_options = 1;
    mu_end_window(&mui_ctx);
  }

  if (toggle_options) {
    mu_Container * cnt = mu_get_container(&mui_ctx, "!options");
    if (mui_first_open_ever) {
      // mu_get_container always "open" the container when it creates and we
      // don't have a clear way of detecting this.
      mui_first_open_ever = 0;
    } else {
      cnt->open = 1 - cnt->open;
    }
    mui_lvl = lvl_current + 1;
    mui_overlay = cnt->open;
    gme_enabled = !cnt->open;
  }

  int wx = (t->sw - 300) / 2;
  int wy = (t->sh - 200) / 2;
  opt = MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_CLOSED;
  if (mu_begin_window_ex(&mui_ctx, "!options", mu_rect(wx, wy, 300, 200), opt)) {
    mui_vspace(6);

    mu_layout_row(&mui_ctx, 3, (int[]) { -60, -1 }, 32);
    mui_label("Sound");
    if (mu_button(&mui_ctx, sfx_enabled() ? "ON" : "")) sfx_toggle();

    mui_vspace(12);

    mu_layout_row(&mui_ctx, 1, (int[]) { -1 }, 32);
    if (mu_slider_ex(&mui_ctx, &mui_lvl, 1, sav_data.max_level + 1, 1, "Level %.0f", MU_OPT_ALIGNCENTER)) {
      gme_level(mui_lvl - 1);
    }

    mui_vspace(12);

    mu_layout_row(&mui_ctx, 1, (int[]) { -1 }, 32);
    if (mu_button(&mui_ctx, "Restart level")) {
      gme_level(lvl_current);

      mu_Container * cnt = mu_get_current_container(&mui_ctx);
      cnt->open = 0;
      mui_overlay = 0;
      gme_enabled = 1;
    }

    mu_end_window(&mui_ctx);
  }

  mu_end(&mui_ctx);

  // TODO: batch these into fewer calls
  mu_Command * cmd = NULL;
  while (mu_next_command(&mui_ctx, &cmd)) {
    switch (cmd->type) {
      case MU_COMMAND_TEXT: {
        mui_upc_t pc = {
          .rect   = { cmd->text.pos.x, cmd->text.pos.y, 0, mui_font_height() * 3 },
          .colour = {
            cmd->text.color.r / 255.f,
            cmd->text.color.g / 255.f,
            cmd->text.color.b / 255.f,
            0,
          },
          .extent = { t->sw, t->sh },
        };
        for (char * c = cmd->text.str; *c; c++) {
          pc.rect[2] = mui_font_width(*c) * 3;
          uv(pc.uv, *c);
          t->draw(t->ptr, &pc);
          pc.rect[0] += pc.rect[2] + 2;
        }
        break;
      }
      case MU_COMMAND_CLIP: {
        t->scissor(t->ptr,
          cmd->clip.rect.x, cmd->clip.rect.y,
          cmd->clip.rect.w, cmd->clip.rect.h);
        break;
      }
      case MU_COMMAND_RECT: {
        mui_upc_t pc = {
          .rect   = {
            cmd->rect.rect.x,
            cmd->rect.rect.y,
            cmd->rect.rect.w,
            cmd->rect.rect.h,
          },
          .colour = {
            cmd->rect.color.r / 255.f,
            cmd->rect.color.g / 255.f,
            cmd->rect.color.b / 255.f,
            0xFFFF,
          },
          .extent = { t->sw, t->sh },
        };
        t->draw(t->ptr, &pc);
        break;
      }
      case MU_COMMAND_ICON: {
        mui_upc_t upc = {
          .rect   = {
            cmd->icon.rect.x,
            cmd->icon.rect.y,
            cmd->icon.rect.w,
            cmd->icon.rect.h,
          },
          .colour = {
            cmd->icon.color.r / 255.f,
            cmd->icon.color.g / 255.f,
            cmd->icon.color.b / 255.f,
            cmd->icon.id,
          },
          .extent = { t->sw, t->sh },
        };
        t->draw(t->ptr, &upc);
        break;
      }
    }
  }
}

#endif
#endif
