#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>
#include <noise.glsl>

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
  vec2 p = (gl_FragCoord.xy - 0.5) / 100.0;

  vec3 col;
  if (u.mode == 1u)
    col = vec3(fbm(p, u.octaves));
  else if (u.mode == 2u)
    col = vec3(value_noise(p));
  else if (u.mode == 3u)
    col = vec3(0.5 + 0.5 * sin(p.x * 3.0 + fbm(p, u.octaves) * 6.0));
  else
    col = bands(fbm(p + vec2(u.time * 0.1, 0.0), u.octaves));

  outColor = vec4(col, 1.0);
}
