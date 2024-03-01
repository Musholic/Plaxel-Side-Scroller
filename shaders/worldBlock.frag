Texture2D texture : register(t1);
SamplerState texSampler : register(s1);

struct VSInput
{
    [[vk::location(0)]] float2 texCoord : TEXCOORD0;
};

float4 main(VSInput input) : SV_TARGET {
    return texture.Sample(texSampler, input.texCoord);
}
