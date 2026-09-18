#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) out vec4 outColor;

void main() {
  // TODO(TASK 1a)
  vec2 uv = vec2(0.0);

  // TODO(TASK 1b)
  vec2 p = vec2(0.0);

  // TODO(TASK 1c)
  float d = 1.0;

  // TODO(TASK 1d)
  vec3 col = vec3(0.0);

  if      (u.mode == 1u) col = vec3(uv, 0.0);
  else if (u.mode == 2u) col = vec3(d * 0.5 + 0.5);
  else if (u.mode == 3u) col = vec3(fract(d * 10.0));

  outColor = vec4(col, 1.0);
}
