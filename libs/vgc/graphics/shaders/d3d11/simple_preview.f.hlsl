
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float4 col : COLOR0;
};

float4 main(PS_INPUT input)
    : SV_Target {

    if (all(int2(input.pos.xy) % 2 == 0)) {
        return input.col;
    }
    else {
        discard;
    }

    return float4(0, 0, 0, 0);
}
