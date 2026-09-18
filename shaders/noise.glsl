#ifndef NOISE_GLSL_INCLUDED
#define NOISE_GLSL_INCLUDED

uint pcg(uint v) {
  v = v * 747796405u + 2891336453u;
  uint w = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
  return (w >> 22u) ^ w;
}

float hash(uvec2 p) {
  return float(pcg(p.x ^ pcg(p.y))) / 4294967296.0;
}

float value_noise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);

  uvec2 ui = uvec2(ivec2(i) + 1000);

  float a = hash(ui);
  float b = hash(ui + uvec2(1u, 0u));
  float c = hash(ui + uvec2(0u, 1u));
  float d = hash(ui + uvec2(1u, 1u));

  vec2 w = f * f * (3.0 - 2.0 * f);
  return mix(mix(a, b, w.x), mix(c, d, w.x), w.y);
}

float fbm(vec2 p, uint octaves) {
  float total = 0.0;
  for (int i = 0; i < octaves; i++) {
    total += pow(0.5, i + 1) * value_noise(p * pow(2.0, i));
  }
  return total;
}

#endif
