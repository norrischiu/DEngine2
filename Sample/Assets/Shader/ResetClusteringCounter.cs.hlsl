RWStructuredBuffer<uint> visibleClusterCount : register(u0);
RWStructuredBuffer<uint> lightIndexListCounter : register(u1);

[numthreads(1, 1, 1)]
void main()
{
	visibleClusterCount[0] = 0;
	lightIndexListCounter[0] = 0;
}