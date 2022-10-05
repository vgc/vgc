
Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

float4 main(PS_INPUT input) : SV_Target
{
    return input.col * tex0.Sample(sampler0, input.uv);
}
