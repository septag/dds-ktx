glslcc -i quad.vert --lang hlsl --bin -o quad_hlsl.vert.h --cvar quad
glslcc -i quad.vert --lang glsl --flatten-ubos --profile 330 -o quad_glsl.vert.h --cvar quad
glslcc -i quad.vert --lang metal -o quad_metal.vert.h --cvar quad

glslcc -i quad.frag --lang hlsl --bin -o quad_hlsl.frag.h --cvar quad
glslcc -i quad.frag --lang glsl --flatten-ubos --profile 330 -o quad_glsl.frag.h --cvar quad
glslcc -i quad.frag --lang metal -o quad_metal.frag.h --cvar quad

glslcc -i quad.frag --lang hlsl --bin -o quad_cubemap_hlsl.frag.h --cvar quad_cubemap --defines CUBEMAP
glslcc -i quad.frag --lang glsl --flatten-ubos --profile 330 -o quad_cubemap_glsl.frag.h --cvar quad_cubemap --defines CUBEMAP
glslcc -i quad.frag --lang metal -o quad_cubemap_metal.frag.h --cvar quad_cubemap --defines CUBEMAP
