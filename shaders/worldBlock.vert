struct UBOMatrices {
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

cbuffer ubo : register(b0) { UBOMatrices ubo; };

struct VSInput
{
[[vk::location(0)]] float3 pos : POSITION0;
[[vk::location(1)]] float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
[[vk::location(0)]] float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
	VSOutput output = (VSOutput)0;
	output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.pos, 1.0))));
	output.texCoord = input.texCoord;
	return output;
}
