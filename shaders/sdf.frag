#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) out vec4 outColor;

float sdf_circle(vec2 p, float r)      { return length(p) - r; }
float op_union(float d1, float d2)     { return min(d1, d2); }
float op_subtract(float d1, float d2)  { return max(d1, -d2); }
float op_intersect(float d1, float d2) { return max(d1, d2); }

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

// TODO(TASK 3a)
float sdf_box(vec2 p, vec2 hs) {
  return 1.0;
}

// TODO(TASK 3b)
float sdf_rounded_box(vec2 p, vec2 hs, float r) {
  return 1.0;
}

// TODO(TASK 3c)
float op_smooth_union(float d1, float d2, float k) {
  return op_union(d1, d2);
}

// TODO(TASK 3d)
float scene(vec2 p) {
  return 1.0;
}

void main() {
  vec2  p = (2.0 * gl_FragCoord.xy - u.resolution) / u.resolution.y;
  float d = scene(p);
  float w = fwidth(d);

  vec3 col = vec3(0.0);
  if (u.mode == 0u) {
    // TODO(TASK 3e)
  }
  else if (u.mode == 1u) col = contours(d);
  else if (u.mode == 2u) col = vec3(clamp(d * 0.5 + 0.5, 0.0, 1.0));
  else if (u.mode == 3u) col = vec3(w * 50.0);

  outColor = vec4(col, 1.0);
}
