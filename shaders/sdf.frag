#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) out vec4 outColor;

float sdf_circle(vec2 p, float r) {
  return length(p) - r;
}
float op_union(float d1, float d2) {
  return min(d1, d2);
}
float op_subtract(float d1, float d2) {
  return max(d1, -d2);
}
float op_intersect(float d1, float d2) {
  return max(d1, d2);
}

vec3 contours(float d) {
  vec3 col = (d < 0.0) ? vec3(0.90, 0.55, 0.25) : vec3(0.25, 0.45, 0.80);
  col *= 1.0 - exp(-6.0 * abs(d));
  col *= 0.85 + 0.15 * cos(d * 62.831853);
  return mix(col, vec3(1.0), 1.0 - smoothstep(0.0, 0.012, abs(d)));
}

vec2 ball_center() {
  if (u.mouse.z > 0.0) return (2.0 * u.mouse.xy - u.resolution) / u.resolution.y;
  return vec2(0.45, 0.15 * sin(u.time));
}

float sdf_box(vec2 p, vec2 hs) {
  vec2 d = abs(p) - hs;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdf_rounded_box(vec2 p, vec2 hs, float r) {
  return sdf_box(p, hs - r) - r;
}

float op_smooth_union(float d1, float d2, float k) {
  float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0, 1);
  return mix(d2, d1, h) - k * h * (1 - h);
}

float scene(vec2 p) {
  vec2 centre = vec2(-0.35, 0.00);
  float body = sdf_rounded_box(p - centre, vec2(0.40, 0.25), 0.08);
  float ball = sdf_circle(p - ball_center(), 0.30);
  float d = op_smooth_union(body, ball, 0.20);
  float hole = sdf_circle(p - centre, 0.12);

  return op_subtract(d, hole);
}

void main() {
  vec2 p = (2.0 * gl_FragCoord.xy - u.resolution) / u.resolution.y;
  float d = scene(p);
  float w = fwidth(d);

  vec3 col = vec3(0.0);
  if (u.mode == 0u) {
    float edge = smoothstep(w, -w, d);
    col = mix(vec3(0.0), vec3(0.9, 0.5, 0.2), edge);
  }
  else if (u.mode == 1u) col = contours(d);
  else if (u.mode == 2u) col = vec3(clamp(d * 0.5 + 0.5, 0.0, 1.0));
  else if (u.mode == 3u) col = vec3(w * 50.0);

  outColor = vec4(col, 1.0);
}
