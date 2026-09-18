#ifndef __HASH_H_INCLUDED__
#define __HASH_H_INCLUDED__

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>

// The same functions as shaders/noise.glsl, in C++. Integer arithmetic is bit
// exact on every GPU and on the CPU, so the panel can check one against the other.
// fract(sin(dot(p, ...)) * 43758.5453) could not be checked this way.
inline std::uint32_t pcg(std::uint32_t v) {
  v = v * 747796405u + 2891336453u;
  std::uint32_t w = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
  return (w >> 22u) ^ w;
}

inline float hash(glm::uvec2 p) {
  return static_cast<float>(pcg(p.x ^ pcg(p.y))) / 4294967296.0f;
}

// GLSL's mix is x*(1-a) + y*a, which is not quite glm::mix's x + a*(y-x)
inline float glslMix(float x, float y, float a) { return x * (1.0f - a) + y * a; }

inline float valueNoise(glm::vec2 p) {
  glm::vec2  i = glm::floor(p);
  glm::vec2  f = p - i;
  glm::uvec2 g = glm::uvec2(glm::ivec2(i) + 1000);

  float a = hash(g);
  float b = hash(g + glm::uvec2(1u, 0u));
  float c = hash(g + glm::uvec2(0u, 1u));
  float d = hash(g + glm::uvec2(1u, 1u));

  glm::vec2 w = f * f * (3.0f - 2.0f * f);
  return glslMix(glslMix(a, b, w.x), glslMix(c, d, w.x), w.y);
}

inline float fbm(glm::vec2 p, std::uint32_t octaves) {
  float sum  = 0.0f;
  float amp  = 0.5f;
  float freq = 1.0f;
  for (std::uint32_t i = 0; i < octaves; ++i) {
    sum  += amp * valueNoise(p * freq);
    freq *= 2.0f;
    amp  *= 0.5f;
  }
  return sum;
}

#endif
