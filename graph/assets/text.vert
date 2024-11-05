#version 300 es
precision mediump float;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

uniform mat4 projMatrix;

out vec2 fragTexCoord;

void main() {
  gl_Position = projMatrix * vec4(inPosition, 0.0, 1.0);
  fragTexCoord = inTexCoord;
}