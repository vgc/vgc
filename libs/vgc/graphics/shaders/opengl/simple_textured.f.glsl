#version 330 core

uniform sampler2D tex0f;

in vec4 fCol;
in vec2 fUv;

out highp vec4 fragColor;

void main() {
   fragColor = fCol * texture(tex0f, fUv);
}
