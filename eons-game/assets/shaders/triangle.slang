struct PSInput
{
	float4 Pos : SV_POSITION;
	float3 Color : COLOR;
};

struct PSOutput
{
	float4 Color : SV_TARGET;
};

void main(in uint VertId : SV_VertexID,
          out PSInput PSIn)
{
	float4 Pos[3];
	Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
	Pos[1] = float4(0.0, +0.5, 0.0, 1.0);
	Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

	float3 Col[3];
	Col[0] = float3(1.0, 0.0, 0.0); // red
	Col[1] = float3(0.0, 1.0, 0.0); // green
	Col[2] = float3(0.0, 0.0, 1.0); // blue

	PSIn.Pos = Pos[VertId];
	PSIn.Color = Col[VertId];
}

void main(in PSInput PSIn,
          out PSOutput PSOut)
{
	PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
