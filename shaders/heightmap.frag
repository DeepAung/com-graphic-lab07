#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) in vec3 inWorld;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in float inHeight;

layout(location = 0) out vec4 outColor;

vec3 bands(float h) {
  vec3 col = mix(vec3(0.02, 0.08, 0.30), vec3(0.10, 0.45, 0.65), smoothstep(0.20, 0.35, h));
  col = mix(col, vec3(0.80, 0.74, 0.50), smoothstep(0.35, 0.40, h));
  col = mix(col, vec3(0.20, 0.45, 0.15), smoothstep(0.40, 0.55, h));
  col = mix(col, vec3(0.42, 0.38, 0.35), smoothstep(0.55, 0.70, h));
  col = mix(col, vec3(0.95, 0.95, 0.98), smoothstep(0.70, 0.80, h));
  return col;
}

void main() {
  vec3 n = normalize(inNormal);

  vec3 col = vec3(0.5);
  if (u.mode == 0u) {
    vec3 L = normalize(vec3(0.5, 1.0, 0.3));
    col = bands(inHeight) * (0.15 + 0.85 * max(dot(n, L), 0.0));
  }
  else if (u.mode == 1u) col = n * 0.5 + 0.5;
  else if (u.mode == 2u) {
    vec2 cells = inWorld.xz * float(u.knob) * 0.5;
    vec2 edge = abs(fract(cells) - 0.5);
    vec2 wide = max(fwidth(cells), 1e-5);
    float line = min((0.5 - edge.x) / wide.x, (0.5 - edge.y) / wide.y);
    col = vec3(smoothstep(0.0, 1.5, line));
  }
  else if (u.mode == 3u) col = vec3(inHeight);

  outColor = vec4(col, 1.0);
}
