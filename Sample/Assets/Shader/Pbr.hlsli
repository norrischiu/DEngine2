// Pbr.hlsli: header file for pbr related calculation
#ifndef PBR_HLSLI
#define PBR_HLSLI
#pragma pack_matrix( row_major )

#include "Clustering.hlsli"
#include "Light.hlsli"

static const float PI = 3.14159265359;
static const float EPSILON = 1.0e-05;

// Disney’s reparameterization
float RoughnessToAlpha(float roughness)
{
    return roughness * roughness;
}

float NDF_GGX(float3 N, float3 H, float roughness)
{
    float a = RoughnessToAlpha(roughness);
    float a2 = a * a;
	float NdotH = saturate(dot(N, H));
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float Geometry_SchlickGGX(float NdotV, float roughness)
{
#ifdef IBL
    // No reparameterization because it will be too dim for IBL
    // a = roughness^2
    // k = a / 2
    float a = RoughnessToAlpha(roughness);
    float k = a / 2.0;
#else
    // Disney’s reparameterization to reduce extreme
    // a = ((roughness + 1) / 2) ^ 2
    // k = a / 2
    float tmp = (roughness + 1.0);
    float k = (tmp * tmp) / 8.0;
#endif
	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
// height correlated Smith masking and shadowing
float Geometry_Smith(float3 N, float3 V, float3 L, float k)
{
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float ggx1 = Geometry_SchlickGGX(NdotV, k);
	float ggx2 = Geometry_SchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

float3 Fresnel_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float3 Fresnel_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0 - roughness, F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 RandomFloat2_Hammersley(uint i, uint N)
{
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

// https://hal.archives-ouvertes.fr/hal-01509746/document
float3 ImportanceSample_VNDF_GGX(float2 U, float3 N, float roughness)
{
	// -- Stretch the view vector so we are sampling as though
	// -- roughness==1
	float3 v = normalize(float3(N.x * roughness, N.y, N.z * roughness));

	// -- Build an orthonormal basis with v, t1, and t2
	float3 t1 = (v.y < 0.999f) ? normalize(cross(float3(0, 1, 0), v)) : float3(1, 0, 0);
	float3 t2 = cross(v, t1);

	// -- Choose a point on a disk with each half of the disk weighted
	// -- proportionally to its projection onto direction v
	float a = 1.0f / (1.0f + v.y);
	float r = sqrt(U.x);
	float phi = (U.y < a) ? (U.y / a) * PI : PI + (U.y - a) / (1.0f - a) * PI;
	float p1 = r * cos(phi);
	float p2 = r * sin(phi) * ((U.y < a) ? 1.0f : v.y);

	// -- Calculate the normal in this stretched tangent space
	float3 n = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

	// -- unstretch and normalize the normal
	float3 H = normalize(float3(roughness * n.x, max(0.0f, n.y), roughness * n.z));

	// from tangent-space vector to world-space sample vector
	float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(N, up));
	float3 bitangent = cross(tangent, N);

	float3 sampleVec = tangent * H.x + N * H.y + bitangent * H.z;
	return normalize(sampleVec);
}
float3 ImportanceSample_InverseCDF_GGX(float2 U, float3 N, float roughness)
{
    float a = RoughnessToAlpha(roughness);
	
    // stretch vector using inverse of CDF
    float phi = 2.0 * PI * U.x;
    float cosTheta = sqrt((1.0 - U.y) / (1.0 + (a*a - 1.0) * U.y)); 
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = cosTheta;
    H.z = sin(phi) * sinTheta;
	
    // from tangent-space vector to world-space sample vector
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(N, up));
    float3 bitangent = cross(tangent, N);
	
    float3 sampleVec = tangent * H.x + N * H.y + bitangent * H.z;
    return normalize(sampleVec);
}
float3 ImportanceSample_DiffuseLobe(float2 u, float3 N)
{
	float phi = 2.0 * PI * u.y;
	float cos_theta = sqrt(1.0 - u.x);
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

	float3 H;
	H.x = sin_theta * cos(phi);
	H.y = cos_theta;
	H.z = sin_theta * sin(phi);

	float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(N, up));
	float3 bitangent = cross(tangent, N);

	float3 sampleVec = tangent * H.x + N * H.y + bitangent * H.z;
	return normalize(sampleVec);
}

#define LUT_SIZE (64.0f)
#define LUT_SCALE ((64.0f - 1.0f) / 64.0f)
#define LUT_BIAS (0.5 / 64.0f)

float3 EvaluatePointLightIrradiance(
	PointLight light,
	float3 posWS,
	float3 N,
	float3 V,
	float roughness,
	float3 refractedDiffuse,
	float3 F)
{
	float3 L = (light.positionWS - posWS).xyz;
	float distanceSquared = dot(L, L);
	float fallOffSquared = light.falloffRadius * light.falloffRadius;
	if (distanceSquared < EPSILON * EPSILON || distanceSquared > fallOffSquared)
	{
		return float3(0.0f, 0.0f, 0.0f);
	}

	L *= rsqrt(distanceSquared);
	float3 H = normalize(V + L);
	float attenuation = max(0, pow(1.0f - pow(distanceSquared * rcp(fallOffSquared), 2), 2)) * rcp(distanceSquared);
	float3 radiance = light.color * light.intensity * attenuation;

	// cook-torrance brdf
	float NDF = NDF_GGX(N, H, roughness);
	float G = Geometry_Smith(N, V, L, roughness);

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
	float3 specular = numerator / max(denominator, 0.001);

	// add to outgoing radiance Lo
	float NdotL = saturate(dot(N, L));
	return (refractedDiffuse + specular) * radiance * NdotL;
}

// https://blog.selfshadow.com/publications/s2016-advances/s2016_ltc_rnd.pdf
float3 IntegrateEdge(float3 v1, float3 v2)
{
	float x = dot(v1, v2);
	float y = abs(x);

	float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
	float b = 3.4175940 + (4.1616724 + y)*y;
	float v = a / b;

	float theta_sintheta = (x > 0.0) ? v : 0.5*rsqrt(max(1.0 - x * x, 1e-7)) - v;

	return cross(v1, v2)*theta_sintheta;
}

float3 EvaluateLTC(
	Texture2D LTCNormMap,
	Texture2D AreaLightTexture,
	sampler SurfaceSampler,
	float3x3 transform,
	float4 vertices[4],
	float3 posWS,
	float3 N,
	float3 V
)
{
	// construct orthonormal basis around N
	float3 T1 = normalize(V - N * dot(V, N));
	float3 T2 = cross(T1, N);

	// rotate area light in (T1, T2, R) basis
	float3x3 basis = transpose(float3x3(T1, T2, N));
	transform = mul(basis, transform);

	// tranform light polygon to cosine space
	float3 L[4];
	L[0] = mul(vertices[0].xyz - posWS, transform);
	L[1] = mul(vertices[1].xyz - posWS, transform);
	L[2] = mul(vertices[2].xyz - posWS, transform);
	L[3] = mul(vertices[3].xyz - posWS, transform);

	float3 dir = posWS - vertices[0].xyz;
	float3 lightNormal = cross(vertices[1].xyz - vertices[0].xyz, vertices[3].xyz - vertices[0].xyz);
	bool behind = (dot(dir, lightNormal) < 0.0);
	if (behind) return float3(0.0f, 0.0f, 0.0f);

	// texturing
	float3 V1 = L[1] - L[0];
	float3 V2 = L[3] - L[0];
	float3 L0 = L[0];

	// project onto sphere
	L[0] = normalize(L[0]);
	L[1] = normalize(L[1]);
	L[2] = normalize(L[2]);
	L[3] = normalize(L[3]);

	// integration by edges
	float3 vectorIrradiance = 0.0;
	vectorIrradiance += IntegrateEdge(L[0], L[1]);
	vectorIrradiance += IntegrateEdge(L[1], L[2]);
	vectorIrradiance += IntegrateEdge(L[2], L[3]);
	vectorIrradiance += IntegrateEdge(L[3], L[0]);

	float len = length(vectorIrradiance);
	float z = vectorIrradiance.z / len;
	//if (behind) z = -z;

	float2 uv = float2(z*0.5 + 0.5, len);
	uv = uv * LUT_SCALE + LUT_BIAS;
	float scale = LTCNormMap.Sample(SurfaceSampler, uv).w;
	float sum = len * scale;

	float3 planeOrtho = (cross(V1, V2));
	float planeAreaSquared = dot(planeOrtho, planeOrtho);
	float planeDistxPlaneArea = dot(planeOrtho, L0);
	float3 P = vectorIrradiance - L0;

	// find tex coords of P
	float dot_V1_V2 = dot(V1, V2);
	float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
	float3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
	float2 Puv;
	Puv.y = dot(V2_, P) / dot(V2_, V2_);
	Puv.x = dot(V1, P)*inv_dot_V1_V1 - dot_V1_V2 * inv_dot_V1_V1*Puv.y;

	// LOD
	float d = abs(planeDistxPlaneArea) / pow(planeAreaSquared, 0.75);
	float3 textureColor = AreaLightTexture.SampleLevel(SurfaceSampler, Puv, log(2048.0*d) / log(3.0)).rgb;

	return float3(sum, sum, sum) * textureColor;
}

float3 EvaluateQuadLightIrradiance(
	QuadLight quadLight,
	Texture2D LTCInverseMatrixMap,
	Texture2D LTCNormMap,
	Texture2D AreaLightTexture,
	sampler SurfaceSampler,
	float3 posWS,
	float3 N,
	float3 V,
	float roughness
)
{
	// getting the LTC inverse matrix and norm
	float2 uv = float2(roughness, sqrt(1.0 - saturate(dot(N, V))));
	uv = uv * LUT_SCALE + LUT_BIAS;

	float4 inv = LTCInverseMatrixMap.Sample(SurfaceSampler, uv);
	float3x3 inverseMatrix = {
		inv.x, 0.0f, inv.y,
		0.0f, 1.0f, 0.0f,
		inv.z, 0.0f, inv.w
	};

	float3 specular = EvaluateLTC(LTCNormMap, AreaLightTexture, SurfaceSampler, inverseMatrix, quadLight.vertices, posWS, N, V);
	float4 norm = LTCNormMap.Sample(SurfaceSampler, uv);
	specular *= quadLight.color * norm.x + (1.0f - quadLight.color) * norm.y;

	float3x3 identity =
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};
	float3 diffuse = EvaluateLTC(LTCNormMap, AreaLightTexture, SurfaceSampler, identity, quadLight.vertices, posWS, N, V);

	return quadLight.intensity * (specular + quadLight.color * diffuse);
}


float3 PbrShading(
	TextureCube irradianceMap,
	TextureCube prefilteredEnvMap,
	Texture2D BRDFIntegrationMap,
	Texture2D LTCInverseMatrixMap,
	Texture2D LTCNormMap,
	Texture2D AreaLightTexture,
	sampler SurfaceSampler,
	uint numPointLight,
	uint numQuadLight,
	PointLight pointLights[8],
	QuadLight quadLights[8],
	float4 screenPos,
	float3 posWS,
	float3 eyePosWS,
	float3 N,
	float3 albedo,
	float metallic,
	float roughness,
	float ao,
	ClusterInfo clusterInfo,
	StructuredBuffer<ClusterLightInfo> clusterLightInfoList,
	StructuredBuffer<uint> lightIndexList
)
{
	float3 V = normalize(eyePosWS - posWS);
	float3 R = reflect(-V, N);

	float3 F0 = float3(0.04f, 0.04f, 0.04f); // default F0 for dielectrics, not accurate but close enough
	F0 = lerp(F0, albedo, metallic);

	// cook-torrance brdf
	float3 F = Fresnel_SchlickRoughness(saturate(dot(N, V)), F0, roughness);

	float3 kS = F;
	float3 kD = 1.0f - kS;
	kD *= 1.0 - metallic;
	float3 refractedDiffuse = kD * albedo / PI; // Kd * f lambert

	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	// cluster
	const uint clusterLinearId = ScreenPosToClusterLinearId(screenPos, clusterInfo);
	const ClusterLightInfo info = clusterLightInfoList[clusterLinearId];

	uint i = 0;
	[loop]
	for (i = 0; i < info.numPointLight; ++i)
	{
		const uint id = lightIndexList[info.index + i];
		Lo += EvaluatePointLightIrradiance(pointLights[id], posWS, N, V, roughness, refractedDiffuse, F);
	}
	[loop]
	for (i = 0; i < info.numQuadLight; ++i)
	{
		const uint id = lightIndexList[info.index + info.numPointLight + i];
		Lo += EvaluateQuadLightIrradiance(quadLights[id], LTCInverseMatrixMap, LTCNormMap, AreaLightTexture, SurfaceSampler, posWS, N, V, roughness);
	}

	// indirect specular
	float3 prefilteredSpecular = prefilteredEnvMap.SampleLevel(SurfaceSampler, R, roughness * 4.0f).rgb;
	float2 specularBrdf = BRDFIntegrationMap.Sample(SurfaceSampler, float2(saturate(dot(N, V)), roughness)).rg;
	float3 indirectSpecular = prefilteredSpecular * (F * specularBrdf.x + specularBrdf.y);

	// indirect diffuse
	float3 irradiance = irradianceMap.Sample(SurfaceSampler, N).rgb;
	float3 diffuse = irradiance * albedo * ao;

	float3 ambient = (kD * diffuse + indirectSpecular) * ao;
	return ambient + Lo;
}
#endif