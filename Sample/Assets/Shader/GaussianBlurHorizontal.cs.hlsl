#pragma pack_matrix( row_major )

#define KERNAL_SIZE (7)
#define KERNAL_SIZE_MINUS_ONE (6)
#define KERNAL_HALF_SIZE (3)
#define DIM_X (256)

cbuffer CBuffer : register(b0)
{
	float2 g_resolution;
	float2 g_invResolution;
	float g_standardDeviation;
};

Texture2D<float4> source : register(t0);
RWTexture2D<float4> target : register(u0);

groupshared float3 pixels[DIM_X + KERNAL_SIZE_MINUS_ONE];

float gaussian(int x)
{
	float sigmaSqu = g_standardDeviation * g_standardDeviation;
	return (1 / sqrt(6.28319f * sigmaSqu)) * pow(2.71828f, -(x * x) / (2 * sigmaSqu));
}

[numthreads(DIM_X, 1, 1)]
void main(int3 localThreadId : SV_GroupThreadID, int3 globalThreadId : SV_DispatchThreadID)
{
	float3 color = float3(0.0f, 0.0f, 0.0f);
	float kernelSum = 0.0;

	if (localThreadId.x < KERNAL_HALF_SIZE)
	{
		int x = max(globalThreadId.x - KERNAL_HALF_SIZE, 0);
		pixels[localThreadId.x] = source[int2(x, globalThreadId.y)].rgb;
	}
	if (localThreadId.x >= DIM_X - KERNAL_HALF_SIZE)
	{
		int x = min(globalThreadId.x + KERNAL_HALF_SIZE, g_resolution.x - 1);
		pixels[localThreadId.x + KERNAL_SIZE_MINUS_ONE] = source[int2(x, globalThreadId.y)].rgb;
	}
	pixels[localThreadId.x + KERNAL_HALF_SIZE] = source[min(globalThreadId.xy, int2(g_resolution) - 1)].rgb;

	GroupMemoryBarrierWithGroupSync();

	if (any(globalThreadId.xy > uint2(g_resolution.x, g_resolution.y)))
	{
		return;
	}

	int upper = KERNAL_HALF_SIZE;
	int lower = -upper;

	[unroll]
	for (int x = lower; x <= upper; ++x)
	{
		float gauss = gaussian(x);
		kernelSum += gauss;
		color += gauss * pixels[localThreadId.x + x + KERNAL_HALF_SIZE];
	}

	color /= kernelSum;

	target[globalThreadId.xy] = float4(color, 1.0f);
}