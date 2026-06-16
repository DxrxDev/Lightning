#version 460

/*
layout (set = 0, binding = 0) uniform MVP{
    mat4 models[1024];
};
*/

/*
layout (push_constant) uniform pushconstantuniform{
    mat4 viewproj;
};
*/

layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_tex;
layout (location = 2) in vec4 v_col;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec2 f_texcoords;

void main(){
    gl_Position = vec4(v_pos, 0.0, 1.0);
    f_texcoords = v_tex;
    // f_pos = v_pos;
}
