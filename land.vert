#version 460

layout (push_constant) uniform pushconstantuniform{
    mat4 viewproj;
    vec2 mouseworldpos;
};

layout (location = 0) in vec3  v_pos;
layout (location = 1) in uint  v_typ;
layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec2 f_mpos;

void main(){
    gl_Position = vec4(v_pos, 1.0) * viewproj;
    f_pos = v_pos;
    f_mpos = mouseworldpos;
}
