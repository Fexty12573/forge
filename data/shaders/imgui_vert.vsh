#version 450 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;

layout (std140, binding = 0) uniform UBO { mat4 proj; } ubo;

void main() {
    gl_Position = ubo.proj * vec4(pos, 0.0, 1.0);
    outUv = uv;
    outColor = color;
}
