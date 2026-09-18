#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) in vec3  inWorld;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in float inHeight;

layout(location = 0) out vec4 outColor;

// TODO(TASK 4c)
vec3 bands(float h) {
  return vec3(0.5);
}

void main() {
  vec3 n = normalize(inNormal);

  vec3 col = vec3(0.5);
  if (u.mode == 0u) {
    // TODO(TASK 4c)
  }
  else if (u.mode == 1u) col = n * 0.5 + 0.5;
  else if (u.mode == 2u) {
    vec2 cells = inWorld.xz * float(u.knob) * 0.5;
    vec2 edge  = abs(fract(cells) - 0.5);
    vec2 wide  = max(fwidth(cells), 1e-5);
    float line = min((0.5 - edge.x) / wide.x, (0.5 - edge.y) / wide.y);
    col = vec3(smoothstep(0.0, 1.5, line));
  }
  else if (u.mode == 3u) col = vec3(inHeight);

  outColor = vec4(col, 1.0);
}
