#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>
#include <noise.glsl>

layout(location = 0) in vec2 inXZ;

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
  float h = fbm(inXZ * 1.5 + vec2(1.7, 9.2) + u.time * 0.05, u.octaves);
  outColor = vec4(bands(h), 1.0);
}
