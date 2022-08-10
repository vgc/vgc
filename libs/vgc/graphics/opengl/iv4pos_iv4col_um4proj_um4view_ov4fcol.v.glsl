#version 330 core

in vec4 pos;
in vec4 col;

layout(std140) uniform BuiltinConstants {
	mat4 proj;
	mat4 view;
	uint frameStartTimeInMs;
};

out vec4 fcol;

void main()
{
    fcol = col;
    gl_Position = proj * view * pos;
}
