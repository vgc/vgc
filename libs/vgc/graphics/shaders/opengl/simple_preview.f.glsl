#version 330 core

layout(origin_upper_left) in vec4 gl_FragCoord;

in vec4 fCol;
in vec4 fClipPos;

out highp vec4 fragColor;

void main() {
    vec4 fPos = gl_FragCoord;

    //fragColor = vec4(fClipPos.x, fClipPos.y, 0, 1);
    //fragColor = vec4(fPos.x / 1000, fPos.y / 1000, 0, 1);

    if (mod(ivec2(fPos.xy), 2) == vec2(0, 0)) {
        fragColor = fCol;
    }
    else {
        discard;
    }
}
