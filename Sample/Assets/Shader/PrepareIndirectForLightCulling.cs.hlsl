RWStructuredBuffer<uint> visibleClusterCount : register(u0);
RWStructuredBuffer<uint> indirectDispatchArgument : register(u1);

[numthreads(1, 1, 1)]
void main()
{
	indirectDispatchArgument[0] = (visibleClusterCount[0] + 63) / 64;
	indirectDispatchArgument[1] = 1;
	indirectDispatchArgument[2] = 1;
}