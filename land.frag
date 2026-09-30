#version 460

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec2 f_mpos;
layout (location = 2) in flat vec4 f_col;

layout (location = 0) out vec4 o_col;

void main(){
    o_col = f_col;
}
