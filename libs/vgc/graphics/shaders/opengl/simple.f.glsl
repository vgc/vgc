#version 330 core

layout(origin_upper_left) in vec4 gl_FragCoord;

in vec4 fCol;

out highp vec4 fragColor;

void main() {
    fragColor = fCol;
}
