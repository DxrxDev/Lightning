#version 460

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 f_pos;
layout(location = 1) in vec2 f_texcoords;

layout(location = 0) out vec4 oCol;

void main(){
    // vec4(0.15, 0.45, 0.6, 1);
    oCol = texture(texSampler, f_texcoords);
}