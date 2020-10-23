#version 450

precision mediump float;

layout (location = TEXCOORD0)  in  vec2 f_uv;

layout (location = SV_Target0) out vec4 frag_color;

layout (binding = 0) uniform sampler2D tex_image;

layout (binding = 0, std140) uniform globals {
    vec4 color;
	vec4 target_lod;
};

void main() 
{
    frag_color = textureLod(tex_image, f_uv, target_lod.x) * vec4(color.rgb, 1.0);
    frag_color.a *= step(1.0, color.a);
}
