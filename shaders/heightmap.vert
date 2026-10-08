#version 450
#extension GL_GOOGLE_include_directive : require

#include <toy.glsl>
#include <noise.glsl>

layout(location = 0) in vec2 inXZ;

layout(location = 0) out vec3 outWorld;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out float outHeight;

float height(vec2 xz) {
  return fbm(xz * 1.5 + vec2(1.7, 9.2) + u.time * 0.05, u.octaves) * u.knobf;
}

void main() {
  vec3 pos = vec3(inXZ.x, height(inXZ), inXZ.y);

  float e = 2.0 / float(u.knob);
  vec3 n = normalize(vec3(
        height(inXZ - vec2(e, 0.0)) - height(inXZ + vec2(e, 0.0)),
        2.0 * e,
        height(inXZ - vec2(0.0, e)) - height(inXZ + vec2(0.0, e))
      ));

  outWorld = pos;
  outNormal = n;
  outHeight = clamp(pos.y / u.knobf, 0.0, 1.0);
  gl_Position = u.proj * u.view * vec4(pos, 1.0);
}
