struct VSOutput
{
};

float4 main(VSOutput input) : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
