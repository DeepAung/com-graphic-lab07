#ifndef __TOY_H_INCLUDED__
#define __TOY_H_INCLUDED__

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>

// The one uniform block this whole lab uses. GLSL side: shaders/toy.glsl, which
// must stay in the same order, byte for byte. std140: a vec4 or a mat4 column
// starts on a 16-byte boundary, which is why pad0 exists.
struct ToyParams {                 // GLSL: layout(set = 0, binding = 0) uniform Toy { ... } u;
  glm::vec2     resolution{};      //   0  vec2  resolution  (pixels)
  float         time    = 0.0f;    //   8  float time        (seconds)
  std::uint32_t mode    = 0;       //  12  uint  mode        (0 final, 1..3 debug views)
  glm::vec4     mouse{};           //  16  vec4  mouse       (xy pixels, y DOWN like gl_FragCoord;
                                   //                         z = 1 while the left button is down)
  std::uint32_t octaves = 6;       //  32  uint  octaves     (Part II and Part IV)
  std::uint32_t knob    = 0;       //  36  uint  knob        (integer knob: grid N in Part IV)
  float         knobf   = 0.0f;    //  40  float knobf       (float knob: amplitude in Part IV)
  float         pad0    = 0.0f;    //  44
  glm::mat4     view{1.0f};        //  48  mat4  view
  glm::mat4     proj{1.0f};        // 112  mat4  proj
};                                 // 176

static_assert(sizeof(ToyParams) == 176,          "ToyParams must match the std140 block in toy.glsl");
static_assert(offsetof(ToyParams, resolution) ==   0, "resolution must start at byte 0");
static_assert(offsetof(ToyParams, time)       ==   8, "time must start at byte 8");
static_assert(offsetof(ToyParams, mode)       ==  12, "mode must start at byte 12");
static_assert(offsetof(ToyParams, mouse)      ==  16, "mouse must start at byte 16");
static_assert(offsetof(ToyParams, octaves)    ==  32, "octaves must start at byte 32");
static_assert(offsetof(ToyParams, knob)       ==  36, "knob must start at byte 36");
static_assert(offsetof(ToyParams, knobf)      ==  40, "knobf must start at byte 40");
static_assert(offsetof(ToyParams, view)       ==  48, "view must start at byte 48");
static_assert(offsetof(ToyParams, proj)       == 112, "proj must start at byte 112");

#endif
