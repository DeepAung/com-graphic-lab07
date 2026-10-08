#version 450
#extension GL_GOOGLE_include_directive : require

#include <shadertoy.glsl>
#include <noise.glsl>

float sdf_circle(vec2 p, float r) {
  return length(p) - r;
}

float sdf_box(vec2 p, vec2 hs) {
  vec2 d = abs(p) - hs;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdf_rounded_box(vec2 p, vec2 hs, float r) {
  return sdf_box(p, hs - r) - r;
}

float op_smooth_union(float d1, float d2, float k) {
  float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
  return mix(d2, d1, h) - k * h * (1.0 - h);
}

vec2 toP(vec2 fragCoord) {
  return (2.0 * fragCoord - iResolution.xy) / iResolution.y;
}

vec2 mouseP() {
  if (iMouse.z > 0.0)
    return toP(iMouse.xy);
  return vec2(0.55, 0.10 * sin(iTime * 1.3));
}

float scene(vec2 p) {
  float body = sdf_rounded_box(p - vec2(-0.25, -0.05), vec2(0.38, 0.22), 0.07);

  vec2 c1 = vec2(0.35 * cos(iTime * 0.9), 0.28 * sin(iTime * 1.1));
  vec2 c2 = vec2(0.30 * cos(iTime * 1.4 + 2.1), 0.32 * sin(iTime * 0.8 + 1.0));

  float d = op_smooth_union(body, sdf_circle(p - c1, 0.22), 0.18);
  d = op_smooth_union(d, sdf_circle(p - c2, 0.18), 0.16);
  d = op_smooth_union(d, sdf_circle(p - mouseP(), 0.20), 0.18);
  return d;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 p = toP(fragCoord);

  // cheap domain warp so the melt is not a hard outline
  float n = fbm(p * 1.4 + vec2(iTime * 0.08, 0.0), u.octaves);
  vec2 q = p + 0.08 * (n - 0.5);
  float d = scene(q);

  float w = fwidth(d);
  float edge = smoothstep(w, -w, d);

  vec3 fill = mix(vec3(0.15, 0.35, 0.85), vec3(0.95, 0.55, 0.20), n);
  fill *= 0.55 + 0.45 * fbm(q * 2.0 + iTime * 0.05, min(u.octaves, 4u));

  vec3 col = mix(vec3(0.04, 0.05, 0.08), fill, edge);

  if (iMode == 1u) {
    col = (d < 0.0) ? vec3(0.90, 0.55, 0.25) : vec3(0.25, 0.45, 0.80);
    col *= 1.0 - exp(-6.0 * abs(d));
    col *= 0.85 + 0.15 * cos(d * 62.831853);
    col = mix(col, vec3(1.0), 1.0 - smoothstep(0.0, 0.012, abs(d)));
  } else if (iMode == 2u) {
    col = vec3(n);
  } else if (iMode == 3u) {
    col = vec3(edge);
  }

  fragColor = vec4(col, 1.0);
}
