#ifndef TOY_GLSL_INCLUDED
#define TOY_GLSL_INCLUDED

layout(set = 0, binding = 0) uniform Toy {
  vec2  resolution;
  float time;
  uint  mode;
  vec4  mouse;
  uint  octaves;
  uint  knob;
  float knobf;
  float pad0;
  mat4  view;
  mat4  proj;
} u;

#endif
