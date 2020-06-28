#pragma pack_matrix( row_major )

#define KERNAL_SIZE (7)
#define KERNAL_SIZE_MINUS_ONE (6)
#define KERNAL_HALF_SIZE (3)
#define DIM_Y (256)

cbuffer CBuffer : register(b0)
{
	float2 g_resolution;
	float2 g_invResolution;
	float g_standardDeviation;
};

RWTexture2D<float4> target : register(u0);

groupshared float3 pixels[DIM_Y + KERNAL_SIZE_MINUS_ONE];

float gaussian(int x)
{
	float sigmaSqu = g_standardDeviation * g_standardDeviation;
	return (1 / sqrt(6.28319f * sigmaSqu)) * pow(2.71828f, -(x * x) / (2 * sigmaSqu));
}

[numthreads(1, DIM_Y, 1)]
void main(int3 localThreadId : SV_GroupThreadID, int3 globalThreadId : SV_DispatchThreadID)
{
	float3 color = float3(0.0f, 0.0f, 0.0f);
	float kernelSum = 0.0;

	if (localThreadId.y < KERNAL_HALF_SIZE)
	{
		int y = max(globalThreadId.y - KERNAL_HALF_SIZE, 0);
		pixels[localThreadId.y] = target[int2(globalThreadId.x, y)].rgb;
	}
	if (localThreadId.y >= DIM_Y - KERNAL_HALF_SIZE)
	{
		int y = min(globalThreadId.y + KERNAL_HALF_SIZE, g_resolution.y - 1);
		pixels[localThreadId.y + KERNAL_SIZE_MINUS_ONE] = target[int2(globalThreadId.x, y)].rgb;
	}
	pixels[localThreadId.y + KERNAL_HALF_SIZE] = target[min(globalThreadId.xy, int2(g_resolution) - 1)].rgb;

	GroupMemoryBarrierWithGroupSync();

	if (any(globalThreadId.xy > uint2(g_resolution.x, g_resolution.y)))
	{
		return;
	}

	int upper = KERNAL_HALF_SIZE;
	int lower = -upper;

	[unroll]
	for (int i = lower; i <= upper; ++i)
	{
		float gauss = gaussian(i);
		kernelSum += gauss;
		color += gauss * pixels[localThreadId.y + i + KERNAL_HALF_SIZE];
	}

	color /= kernelSum;

	target[globalThreadId.xy] = float4(color, 1.0f);
}