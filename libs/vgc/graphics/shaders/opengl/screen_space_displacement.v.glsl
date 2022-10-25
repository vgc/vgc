#version 330 core

in vec2 pos;
in vec2 disp;
in vec4 col;
in vec3 ipos;

layout(std140) uniform BuiltinConstants {
    mat4 proj;
    mat4 view;
    vec4 viewport;
    uint frameStartTimeInMs;
};

//out vec2 fUv;
out vec4 fCol;

void main() {
    vec4 viewPos = view * vec4(pos + ipos.xy, 0.f, 1.f);
    float dispMag = length(disp);
    vec2 dispDir = disp;
    if (ipos.z > 0) {
        dispDir = (view * vec4(dispDir, 0.f, 0.f)).xy;
    }
    dispDir = normalize(dispDir);

    gl_Position = proj * viewPos;
    gl_Position.xy += dispMag * dispDir / vec2(0.5 * viewport.z, -0.5 * viewport.w);
    fCol = col;
}
