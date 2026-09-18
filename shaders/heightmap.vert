#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>
#include <noise.glsl>

layout(location = 0) in vec2 inXZ;

layout(location = 0) out vec3  outWorld;
layout(location = 1) out vec3  outNormal;
layout(location = 2) out float outHeight;

// TODO(TASK 4a)
float height(vec2 xz) {
  return 0.0;
}

void main() {
  vec3 pos = vec3(inXZ.x, height(inXZ), inXZ.y);

  // TODO(TASK 4b)
  vec3 n = vec3(0.0, 1.0, 0.0);

  outWorld    = pos;
  outNormal   = n;
  outHeight   = clamp(pos.y / u.knobf, 0.0, 1.0);
  gl_Position = u.proj * u.view * vec4(pos, 1.0);
}
