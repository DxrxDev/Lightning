#version 460

layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2D texSampler2;

layout(location = 0) in vec3 f_pos;
layout(location = 1) in vec2 f_texcoords;
layout(location = 2) in flat uint f_mat;

layout(location = 0) out vec4 oCol;

void main(){
    // vec4(0.15, 0.45, 0.6, 1);
    if (f_mat == 0){
        oCol = texture(texSampler, f_texcoords);
    }
    else {
        oCol = texture(texSampler2, f_texcoords);
    }
}
