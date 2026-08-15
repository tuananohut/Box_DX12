cbuffer constant_buffer_per_object: register(b0)
{
	float4x4 world_view_proj; 
};

struct VertexInputType
{
	float3 pos: POSITION;
	float4 color: COLOR; 
};

struct PixelInputType
{
	float4 pos: SV_POSITION;
	float4 color: COLOR; 
};

PixelInputType ColorVertexShader (VertexInputType input)
{
	PixelInputType output;

	output.pos = mul(float4(input.pos, 1.f), world_view_proj);

	output.color = input.color;

	return output; 
}

float4 ColorPixelShader(PixelInputType input): SV_TARGET
{
	return input.color; 
}