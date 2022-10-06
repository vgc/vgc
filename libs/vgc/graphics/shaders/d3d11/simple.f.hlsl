
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float4 col : COLOR0;
};

float4 main(PS_INPUT input) : SV_Target {
    return input.col;
}
