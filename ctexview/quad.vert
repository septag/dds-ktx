#version 450

layout (location = POSITION)  in  vec2 a_pos;
layout (location = TEXCOORD0) in  vec3 a_uv;

layout (location = TEXCOORD0) out vec3 f_uv;

layout (binding = 0, std140) uniform globals {
    mat4 proj_mat;
};
   
void main() {
    gl_Position = proj_mat * vec4(a_pos, 0, 1.0);
    f_uv = a_uv;    
}  