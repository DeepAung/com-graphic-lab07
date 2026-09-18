#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) out vec4 outColor;

void main() {
  vec2 uv = gl_FragCoord.xy / u.resolution;

  vec2 p = (2 * gl_FragCoord.xy - u.resolution) / u.resolution.y;

  float d = length(p) - 0.5;

  vec3 col = smoothstep(0.02, -0.02, d) * (0.5 + 0.5 * cos(u.time + d * 10.0 + vec3(0.0, 2.0, 4.0)));

  if (u.mode == 1u) col = vec3(uv, 0.0);
  else if (u.mode == 2u) col = vec3(d * 0.5 + 0.5);
  else if (u.mode == 3u) col = vec3(fract(d * 10.0));

  outColor = vec4(col, 1.0);
}
