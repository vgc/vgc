#version 150

in vec4 pos;
in vec4 col;
uniform mat4 proj;
uniform mat4 view;
out vec4 fcol;

void main()
{
    fcol = col;
    gl_Position = proj * view * pos;
}
