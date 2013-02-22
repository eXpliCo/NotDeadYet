/*
struct VsIn
{
	float4 Position : POSITION;
};

struct PsIn
{
	float4 Position : SV_POSITION;
};

//[Vertex shader]

matrix ViewProj;
float3 Pos;
float size;

PsIn main(VsIn In)
{
	PsIn Out;

	float4 position = In.Position;
	position.xyz *= size;
	position.xyz += Pos;

	Out.Position = mul(ViewProj, position);

	return Out;
}


//[Fragment shader]

Texture2D Depth;
SamplerState DepthFilter;

Texture3D <float4> Decal;
SamplerState DecalFilter;

float4x4 ScreenToLocal;
float3 Color;
float2 PixelSize;

float4 main(PsIn In) : SV_Target
{
    // Compute normalized screen position
	float2 texCoord = In.Position.xy * PixelSize;

    // Compute local position of scene geometry
	float depth = Depth.Sample(DepthFilter, texCoord);
	float4 scrPos = float4(texCoord, depth, 1.0f);
	float4 wPos = mul(ScreenToLocal, scrPos);

    // Sample decal
	float3 coord = wPos.xyz / wPos.w;
	float decal = Decal.Sample(DecalFilter, coord).r;

	return float4(Color, decal);
}
*/