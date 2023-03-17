
cbuffer vertexBuffer : register(b0) {
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewport;
    unsigned int frameStartTimeInMs;
};

struct VS_INPUT {
    float2 pos : POSITION0;
    float2 disp : DISP0;
    float4 col : COLOR0;
    float2 ipos : POSITION1;
    float rotate : ROT0;
    float offset : OFFSET0;
    uint vid : SV_VertexID;
    uint iid : SV_InstanceID;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float4 col : COLOR0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;

    float4 viewPos = mul(viewMatrix, float4(input.pos + input.ipos, 0.f, 1.f));
    float dispMag = length(input.disp) * input.offset;
    float2 dispDir = input.disp;
    // input.rotate is "Rot" and is a "float" boolean to enable the rotation of the displacement by the view matrix.
    if (input.rotate > 0) {
        dispDir = mul(viewMatrix, float4(dispDir, 0.f, 0.f)).xy;
    }
    dispDir = normalize(dispDir);

    output.pos = mul(projMatrix, viewPos);
    output.pos.xy += dispMag * dispDir / float2(0.5 * viewport.z, -0.5 * viewport.w);
    output.clipPos = output.pos;
    output.col = input.col;

    return output;
}
