#version 450
#extension GL_GOOGLE_include_directive : require
#include "metallic-floor.frag"

layout(push_constant) uniform upc {
  vec4 sel_rect;
  vec2 player_pos;
  vec2 label_pos;
  vec2 cursor;
  float level;
  float aspect;
  float time;
  float overlay;
  float back_btn_dim;
  float menu_btn_dim;
} pc;

layout(set = 0, binding = 0) readonly buffer u_map { uint map[]; };
layout(set = 0, binding = 1) uniform sampler2D u_atlas;

layout(location = 0) in vec2 q_pos;

layout(location = 0) out vec4 frag_color;

const float pi = 3.14159265358979323;
float aw;
float aww;

float sd_box(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}
float sd_rnd_box(vec2 p, vec2 b, float r) {
  return sd_box(p, b) - r;
}

float sd_line(vec2 p, vec2 a, vec2 b) {
  vec2 pa = p - a;
  vec2 ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

float sd_cut_dist(vec2 p, float r, float h) {
  float w = sqrt(r * r - h * h);
  p.x = abs(p.x);
  float s = max((h - r) * p.x * p.x + w * w * (h + r - 2.0 * p.y),
                h * p.x - w * p.y);
  return (s < 0.0) ? length(p) - r : 
         (p.x < w) ? h - p.y :
         length(p - vec2(w, h));
}

float op_xor(float a, float b) {
  return max(min(a, b), -max(a, b));
}

vec3 brick(vec2 p) {
  vec2 b = p * vec2(aw, aww);
  b.x += 0.5 * step(1.0, mod(b.y, 2));

  vec2 i = floor(b);
  float h = hash(i);
  h = smoothstep(0.2, 0.8, h) * 0.3 + 0.5;
  h = h * (noise(b) * 0.3 + 0.7);

  vec2 f = fract(b) - 0.5;
  float d = sd_box(f, vec2(0.5, 0.5));
  d = 1.0 - exp(-16.0 * abs(d));

  vec3 c = vec3(0.6, 0.3, 0.1) * h * d;
  return c;
}

uint map_at(vec2 p, vec2 d) {
  vec2 uv = p;
  uv = floor(d + uv * aww - vec2(0, 8) - 0.001) / 32.0;
  uv = uv * 0.5 + 0.5;
  uvec2 id = clamp(uvec2(uv * 32), uvec2(0), uvec2(31, 23));
  return map[id.x + 32 * id.y];
}

float shadow_side(vec2 p, vec2 d, float b, float m) {
  uint map = map_at(p, d);
  float f = (map == 88) ? 0.0 : 1.0;
  f = mix(f, 1.0, b); 
  return min(m, f);
}

float shadow(vec2 p) {
  vec2 b = fract(p * vec2(aww));
  float m = 1.0f;

  m = shadow_side(p, vec2(-1, 0), b.x, m);
  m = shadow_side(p, vec2(+1, 0), 1.0 - b.x, m);
  m = shadow_side(p, vec2(0, -1), b.y, m);
  m = shadow_side(p, vec2(0, +1), 1.0 - b.y, m);

  m = shadow_side(p, vec2(-1, -1), max(b.x, b.y), m);
  m = shadow_side(p, vec2(-1, +1), max(b.x, 1.0 - b.y), m);
  m = shadow_side(p, vec2(+1, -1), max(1.0 - b.x, b.y), m);
  m = shadow_side(p, vec2(+1, +1), max(1.0 - b.x, 1.0 - b.y), m);

  return m;
}

vec3 outside(vec2 p) {
  float m = shadow(p);
  m = 1.0 - exp(-6.0 * abs(m));

  vec3 c = metal_floor(p).rgb;
  c = c * 0.2 * m;
  return c;
}

vec3 empty(vec2 p) {
  float m = shadow(p);

  float s = 1.0 - exp(-7.0 * abs(m));

  float csel = step(0.5, m);
  csel += 1.0 - step(0.3, m);

  const vec3 flr = vec3(0.05, 0.15, 0.1);
  const vec3 ylw = vec3(0.5, 0.5, 0.1) * 0.5;

  vec3 c = mix(ylw, flr, csel);
  c.rgb = c.rgb * s;
  return c;
}

vec3 target(vec2 p, vec2 b) {
  float d = length(b);
  d = exp(-d * d * 20) * sin(abs(d) * 30 - pc.time * 2);

  const vec3 t = vec3(4.0, 0.2, 0.1);

  vec3 c = empty(p);
  c = mix(c, t, d);
  return c;
}

vec3 box(vec2 p, vec2 b, bool on_tgt) {
  float d = sd_rnd_box(b, vec2(0.25), 0.1);

  vec3 ins = on_tgt ? vec3(0.5, 0.2, 0.1) : vec3(0.1, 0.4, 0.5);
  ins *= 1.0 - 0.01 / (d * d);
  ins *= smoothstep(0.0, 0.2, fract(b.y * 5.0));

  vec3 brd = on_tgt ? vec3(1.0, 0.3, 0.1) : vec3(0.1, 0.7, 1.0);
  brd *= 0.1 * smoothstep(-0.1, 0.0, d) + 0.9;

  vec3 box = mix(ins, brd, step(-0.125, d));
  box *= 0.5 * hash(b) + 0.5;

  vec3 flr = on_tgt ? target(p, b) : empty(p);
  flr *= 0.5 * smoothstep(0.0, 0.1, d) + 0.5;

  return mix(box, flr, step(0, d));
}

vec2 g2l(vec2 p) {
  vec2 magic = vec2(4.0, 0.0) + (12.0 - aw);
  return q_pos * vec2(aw) + aw - p + magic - vec2(0.5);
}

vec3 player(vec3 c) {
  vec2 b = g2l(pc.player_pos);

  vec2 hd_p = b + vec2(0.0, 0.2);
  float hd_d = sd_circle(hd_p, 0.2);
  float hd_msk = step(0, hd_d);

  float hlm_d = step(0, hd_p.y);
  vec3 hlm = vec3(1, 0.7, 0);
  vec3 fc = vec3(0, 0.3, 0.7);
  vec3 hd = mix(hlm, fc, hlm_d);

  vec2 bd_p = vec2(0.0, 0.2) - b;
  float bd_d = sd_cut_dist(bd_p, 0.3, -0.2);
  float bd_msk = step(0, bd_d);
  vec3 clt = vec3(0.4, 0.8, 1.0);
  vec3 jkt = vec3(1.0, 0.5, 0.0);
  float jkt_d = sd_box(bd_p, vec2(0.2, 0.3));
  vec3 bd = mix(jkt, clt, step(0, jkt_d));

  float d = min(hd_d, bd_d);
  d = smoothstep(0.0, 0.1, d) * 0.7 + 0.3;

  c = c * d;
  c = mix(bd, c, bd_msk);
  c = mix(hd, c, hd_msk);
  return c;
}

vec3 cursor(vec3 c) {
  vec2 p = g2l(pc.cursor);
  float d = sd_box(p, vec2(0.6));
  d = abs(d);
  d = smoothstep(0.0, 0.1, d);
  return mix(vec3(1), c, d);
}

vec4 atlas(vec2 p, vec2 sz, vec2 uv0, vec2 uv1) {
  const vec2 uv_sz = vec2(128, 32);

  // TODO: find where it got misaligned
  vec2 pp = g2l(p);
  vec2 uv = (pp + sz * 0.5) / sz;
  float d = sd_box(pp, sz * 0.5);

  uv = floor(8 * mix(uv0, uv1, uv)) / uv_sz;
  return mix(texture(u_atlas, uv).rrrr, vec4(0), step(0, d));
}
vec3 atlas_d(vec2 p, vec2 sz, vec2 uv0, vec2 uv1, vec3 f) {
  vec4 c = atlas(p, sz, uv0, uv1);
  float x = c.x;
  f = mix(f, vec3(1.0, 1.0, 0.0), step(0.7, x));
  f = mix(f, vec3(0.0), smoothstep(0.3, 1.0, sin(x * 3.14 - 0.6)));
  return f;
}
vec3 atlas_digit(vec2 p, int digit, vec3 f) {
  vec2 uv = vec2((digit % 6) * 0.5, digit / 6);
  return atlas_d(p, vec2(0.5, 1.0), vec2(1, 1) + uv, vec2(1.5, 2) + uv, f);
}

vec3 level_label(vec3 f) {
  vec2 p = pc.label_pos + vec2(1.0, 0.0);
  f = atlas_d(p, vec2(3.0, 1.0), vec2(1, 0), vec2(4, 1), f);

  int d = int(pc.level) % 10;
  p = pc.label_pos + vec2(3.0, 0.0);
  f = atlas_digit(p, d, f);

  d = int(pc.level) / 10;
  p = pc.label_pos + vec2(2.5, 0.0);
  f = atlas_digit(p, d, f);

  return f;
}

vec3 menu(vec3 f) {
  return mix(f, vec3(0.1, 0.3, 0.2), pc.overlay);
}

vec3 selection(vec3 f) {
  float d = sd_box(q_pos - pc.sel_rect.xy / 8, pc.sel_rect.zw / 8);

  vec3 c = vec3(0.1, 0.4, 0.3);

  float a = 0.3 - 0.3 * step(0, d);
  a *= step(0.001, length(pc.sel_rect.zw));
  return mix(f, c, a);
}

vec3 btn(vec3 f, float d, float dim) {
  float x = smoothstep(0.015, 0.0, d) * min(1.0, dim);
  f = mix(f, vec3(0.1, 0.7, 1.0), step(0.7, x));
  f = mix(f, vec3(0.0), smoothstep(0.3, 1.0, sin(x * 3.14 - 0.6)));
  return f;
}

vec3 back_btn(vec3 f) {
  vec2 center = -vec2(pc.aspect - 0.2, 0.8);
  float d0 = sd_line(q_pos, center - vec2(0.03, 0), center + vec2(0.03, -0.05));
  float d1 = sd_line(q_pos, center - vec2(0.03, 0), center + vec2(0.03, +0.05));
  float d = min(d0, d1) - 0.005;
  return btn(f, d, pc.back_btn_dim);
}

vec3 menu_btn(vec3 f) {
  vec2 center = vec2(pc.aspect - 0.2, -0.8);
  float dc = sd_circle(q_pos - center, 0.035);

  vec2 p = q_pos - center;
  float angle = atan(p.y / p.x);
  float gr = smoothstep(-0.6, 0.6, sin(angle * 8));
  float dg = sd_circle(p, mix(0.045, 0.06, gr));

  float d = op_xor(dc, dg);
  return btn(f, d, pc.menu_btn_dim);
}

void main() {
  aw = clamp(16 - pc.label_pos.y, 8, 12);
  aww = aw * 2;

  uint map = map_at(q_pos, vec2(0));
  vec2 b = fract(q_pos * vec2(aw)) - 0.5;

  vec3 f;
  if (map == 88) { // 'X' - wall
    f = brick(q_pos);
  } else if (map == 32) { // ' ' - outside
    f = outside(q_pos);
  } else if (map == 42) { // '*' - target
    f = target(q_pos, b);
  } else if (map == 48) { // '0' - target_box
    f = box(q_pos, b, true);
  } else if (map == 79) { // 'O' - box
    f = box(q_pos, b, false);
  } else {
    f = empty(q_pos);
  }

  f = player(f);
  f = level_label(f);
  f = menu(f);
  f = selection(f);
  f = back_btn(f);
  f = menu_btn(f);
  f = cursor(f);

  f = pow(f, 1.0 / vec3(2.2));

  frag_color = vec4(f, 1);
}
