#version 330 core

in vec4 fCol;
in vec2 fUv;

out highp vec4 fragColor;

void main() {
    fragColor = vec4(
        mix(
            mix(fCol.rgb, vec3(0, 0, 0), fUv.y * 0.001),
            vec3(1, 1, 1) - fCol.rgb,
            fUv.x * 0.001),
        fCol.a);
}
