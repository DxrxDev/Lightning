#version 460

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec2 f_mpos;

layout (location = 0) out vec4 o_col;

void main(){
    if (f_pos.y < 0.2){
        o_col = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else if (f_pos.y < 0.3){
        o_col = vec4(0.8, 0.8, 0.8, 1.0);
    }
    else if (f_pos.y < 0.6){
        o_col = vec4(0, 0.8, 0.2, 1.0);
    }
    else if (f_pos.y < 0.8){
        o_col = vec4(0.3, 0.2, 0, 1.0);
    }
    else if (f_pos.y < 1.0){
        o_col = vec4(0.3, 0.75, 0.1, 1.0);
    }
    else {
        o_col = vec4(0.0, 0.2, 0.8, 1.0);
    }
}
