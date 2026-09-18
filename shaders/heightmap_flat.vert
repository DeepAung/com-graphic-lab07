#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>

layout(location = 0) in vec2 inXZ;

layout(location = 0) out vec2 outXZ;

void main() {
  outXZ       = inXZ;
  gl_Position = u.proj * u.view * vec4(inXZ.x, 0.0, inXZ.y, 1.0);
}
