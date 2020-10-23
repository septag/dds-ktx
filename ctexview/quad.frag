#version 450

precision mediump float;


layout (location = TEXCOORD0)  in  vec3 f_uv;
layout (location = SV_Target0) out vec4 frag_color;

#ifdef CUBEMAP
layout (binding = 0) uniform samplerCube tex_image;
#else
layout (binding = 0) uniform sampler2D tex_image;
#endif

layout (binding = 0, std140) uniform globals {
    vec4 color;
	vec4 target_lod;
};

void main() 
{
    #ifdef CUBEMAP
        frag_color = textureLod(tex_image, f_uv, target_lod.x) * vec4(color.xyz, 1.0); 
    #else
        frag_color = textureLod(tex_image, f_uv.xy, target_lod.x) * vec4(color.rgb, 1.0);
    #endif
    frag_color.a = color.a < 1.0 ? 1.0 : frag_color.a;
}
