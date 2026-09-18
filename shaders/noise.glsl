#ifndef NOISE_GLSL_INCLUDED
#define NOISE_GLSL_INCLUDED

uint pcg(uint v) {
  v = v * 747796405u + 2891336453u;
  uint w = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
  return (w >> 22u) ^ w;
}

float hash(uvec2 p) { return float(pcg(p.x ^ pcg(p.y))) / 4294967296.0; }

// TODO(TASK 2a)
float value_noise(vec2 p) {
  return 0.0;
}

// TODO(TASK 2b)
float fbm(vec2 p, uint octaves) {
  return 0.0;
}

#endif
