#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec4 color;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = color;
}