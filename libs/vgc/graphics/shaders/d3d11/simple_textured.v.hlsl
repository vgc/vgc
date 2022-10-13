
cbuffer vertexBuffer : register(b0) {
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewport;
    unsigned int frameStartTimeInMs;
};

struct VS_INPUT {
    float2 pos : POSITION0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    float4 viewPos = mul(viewMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.pos = mul(projMatrix, viewPos);
    output.clipPos = output.pos;
    output.uv = input.uv;
    output.col = input.col;
    return output;
}
