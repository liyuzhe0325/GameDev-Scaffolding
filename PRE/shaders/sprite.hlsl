cbuffer SpriteRect : register(b0, space1) {
    float4 rect;
};

struct VertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOut vertex_main(uint id : SV_VertexID) {
    const float2 corners[6] = {
        float2(0, 0), float2(1, 0), float2(1, 1),
        float2(0, 0), float2(1, 1), float2(0, 1)
    };
    VertexOut output;
    output.uv = corners[id];
    output.position = float4(lerp(rect.xy, rect.zw, output.uv), 0, 1);
    return output;
}

Texture2D<float4> sprite_texture : register(t0, space2);
SamplerState sprite_sampler : register(s0, space2);

float4 pixel_main(VertexOut input) : SV_Target0 {
    return sprite_texture.Sample(sprite_sampler, input.uv);
}
