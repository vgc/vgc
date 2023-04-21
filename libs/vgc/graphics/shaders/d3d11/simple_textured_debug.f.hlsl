
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 clipPos : POSITION0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

float avg(float3 x) {
    return (x.x + x.y + x.z) / 3;
}

float4 main(PS_INPUT input)
    : SV_Target {

    // gradient in V (-1:red, 0: black, 1:green) with 0.5 opacity
    //if (input.uv.y > 0) {
    //    return float4(input.uv.y, 0.f, 0.f, 0.5f);
    //}
    //else {
    //    return float4(0.f, 0.f, -input.uv.y, 0.5f);
    //}

    // smooth brush test (easings.net/#easeInOutQuad)
    //float x = 1.f - abs(input.uv.y);
    //float k = -2 * x + 2;
    //float ca = x < 0.5 ? 2 * x * x : 1 - k * k / 2;
    //return float4(input.col.rgb, input.col.a * ca);

    const float square_size = 0.05;
    const float m = square_size * 2;
    float2 uvm = (m + ((input.uv + 2.0) % m)) % m;

    if ((uvm.x < square_size) == (uvm.y < square_size)) {
        return float4(0, 0, 0, input.col.a * 2);
    }
    float3 c =
        input.col.rgb; // lerp(input.col.rgb, input.col.gbr, input.uv.x / 160.0 % 0.5);
    if (input.uv.y < -0.1) {
        c += (round(avg(c)) * 2 - 1) * 0.5;
    }

    return float4(c, input.col.a);
}
