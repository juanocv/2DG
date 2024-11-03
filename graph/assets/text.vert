#version 300 es
precision mediump float;

layout(location = 0) in vec2 inPosition;

uniform mat4 projMatrix;
uniform mat4 modelMatrix;

void main() {
  gl_Position = projMatrix * modelMatrix * vec4(inPosition, 0.0, 1.0);
}
