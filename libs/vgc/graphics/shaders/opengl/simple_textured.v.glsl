#version 330 core

in vec4 pos;
in vec2 uv;
in vec4 col;

layout(std140) uniform BuiltinConstants {
    mat4 proj;
    mat4 view;
    uint frameStartTimeInMs;
};

out vec4 fCol;
out vec2 fUv;

void main() {
    gl_Position = proj * view * pos;
    fCol = col;
    fUv = uv;
}
