#version 150
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform ciBlock0T {
	mat4	ciModelViewProjection;
	mat3	ciNormalMatrix;
} ciBlock0;

layout(location = 0) in vec4 ciPosition;
layout(location = 1) in vec2 ciTexCoord0;
layout(location = 2) in vec3 ciNormal;

layout(location = 0) out vec4 Color;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoord0;

void main( void )
{
	gl_Position	= ciBlock0.ciModelViewProjection * ciPosition;
	TexCoord0	= ciTexCoord0;
	Normal		= ciBlock0.ciNormalMatrix * ciNormal;
}
