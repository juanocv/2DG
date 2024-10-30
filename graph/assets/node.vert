#version 300 es
precision mediump float;

uniform vec2 translation;
uniform float scale;
uniform mat4 projMatrix;

layout(location = 0) in vec2 inPosition;

void main() {
  vec2 position = inPosition * scale + translation;
  gl_Position = projMatrix * vec4(position, 0.0, 1.0);
}
