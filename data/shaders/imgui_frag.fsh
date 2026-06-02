#version 450 core

layout (location = 0) in vec2 uv;
layout (location = 1) in vec4 color;

layout (binding = 0) uniform sampler2D tex;
layout (location = 0) out vec4 outColor;

void main() {
    outColor = color * texture(tex, uv);
}
