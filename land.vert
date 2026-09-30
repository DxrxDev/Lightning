#version 460

layout (push_constant) uniform pushconstantuniform{
    mat4 viewproj;
    vec3 sun;
    vec2 mouseworldpos;
};

layout (location = 0) in vec3  v_pos;
layout (location = 1) in vec3  v_nrm;
layout (location = 2) in uint  v_typ;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec2 f_mpos;
layout (location = 2) out flat vec4 f_col;

void main(){
    gl_Position = vec4(v_pos, 1.0) * viewproj;
    f_pos = v_pos;
    f_mpos = mouseworldpos;

    float f = v_pos.y;
    if (f < 0.1) f_col = vec4(1, 1, 1, 1);
    else if (f < 0.3) f_col = vec4(0.5, 0.5, 0.5, 1.0);
    else if (f < 0.5) f_col = vec4(0.05, 0.3, 0.15, 1.0);
    else if (f < 0.75) f_col = vec4(0.05, 0.2, 0.05, 1.0);
    else if (f < 0.85) f_col = vec4(0.05, 0.2, 0.05, 1.0);
    else if (f < 0.95) f_col = vec4(0.35, 0.35, 0.05, 1.0);
    else f_col = vec4( 0, 0, v_pos.y, 1 );

    float ambient = 0.05;

    float sim = dot( v_nrm, sun ); // * (1.0 - ambient);
    sim = max(ambient, sim);

    /*
    This had the effect of increasing the sun to night ratio, instead of adding ambient light
    
    float sim = dot( v_nrm, sun ) * (1.0 - ambient);
    sim += ambient;
    */

    f_col.xyz *= sim;
}
