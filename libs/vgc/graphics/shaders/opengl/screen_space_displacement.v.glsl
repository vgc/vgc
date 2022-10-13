#version 330 core

in vec2 pos;
in vec2 disp;
in vec4 col;
in vec2 ipos;

layout(std140) uniform BuiltinConstants {
    mat4 proj;
    mat4 view;
    vec4 viewport;
    uint frameStartTimeInMs;
};

//out vec2 fUv;
out vec4 fCol;

void main() {
    gl_Position = proj * view * vec4(pos + ipos, 0.f, 1.f);
    gl_Position.xy += disp / viewport.zw * 2;
    fCol = col;
}
