// Pbr.hlsli: header file for pbr related calculation
#ifndef PBR_HLSLI
#define PBR_HLSLI

#include "Light.hlsli"

static const float PI = 3.14159265359;

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
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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

float3 PbrShading(
	TextureCube irradianceMap,
	TextureCube prefilteredEnvMap,
	Texture2D BRDFIntegrationMap,
	sampler SurfaceSampler,
	uint numLight,
	Light pointLights[8],
	float3 posWS,
	float3 eyePosWS,
	float3 N,
	float3 albedo,
	float metallic,
	float roughness,
	float ao
)
{
	float3 V = normalize(eyePosWS - posWS);
	float3 R = reflect(-V, N);

	float3 F0 = float3(0.04f, 0.04f, 0.04f); // default F0 for dielectrics, not accurate but close enough
	F0 = lerp(F0, albedo, metallic);

	// cook-torrance brdf
	float3 F = Fresnel_SchlickRoughness(saturate(dot(N, V)), F0, roughness);

	float3 kS = F;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	kD *= 1.0 - metallic;

	// indirect specular
	float3 prefilteredSpecular = prefilteredEnvMap.SampleLevel(SurfaceSampler, R, roughness * 4.0f).rgb;
	float2 specularBrdf = BRDFIntegrationMap.Sample(SurfaceSampler, float2(saturate(dot(N, V)), roughness)).rg;
	float3 indirectSpecular = prefilteredSpecular * (F * specularBrdf.x + specularBrdf.y);

	float3 Lo = float3(0.0f, 0.0f, 0.0f);
	for (uint i = 0; i < numLight; ++i)
	{
		float3 L = normalize(pointLights[i].positionWS - posWS).xyz;
		float3 H = normalize(V + L);
		float distance = length(pointLights[i].positionWS - posWS);
		float attenuation = 1.0 / (distance * distance);
		float3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;

		// cook-torrance brdf
		float NDF = NDF_GGX(N, H, roughness);
		float G = Geometry_Smith(N, V, L, roughness);

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
		float3 specular = numerator / max(denominator, 0.001);

		// add to outgoing radiance Lo
		float NdotL = saturate(dot(N, L));
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	// indirect diffuse
	float3 irradiance = irradianceMap.Sample(SurfaceSampler, N).rgb;
	float3 diffuse = irradiance * albedo * ao;

	float3 ambient = (kD * diffuse + indirectSpecular) * ao;
	return ambient + Lo;
}
#endif