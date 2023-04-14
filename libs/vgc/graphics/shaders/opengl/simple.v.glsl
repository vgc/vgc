#version 330 core

in vec4 pos;
in vec4 col;

layout(std140) uniform BuiltinConstants {
    mat4 proj;
    mat4 view;
    vec4 viewport;
    uint frameStartTimeInMs;
};

//out vec2 fUv;
out vec4 fCol;
out vec4 fClipPos;

void main() {
    gl_Position = proj * view * pos;
    fClipPos = gl_Position;
    fCol = col;
}
