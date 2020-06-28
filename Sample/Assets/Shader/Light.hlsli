// Light.hlsli: header file for light related struct and calculation
#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI
#pragma pack_matrix( row_major )

struct PointLight
{
	float3 positionWS;
	float padding;
	float3 color;
	float intensity;
};

struct QuadLight
{
	float4 vertices[4];
	float3 color;
	float intensity;
};

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
	float3 L = normalize(light.positionWS - posWS).xyz;
	float3 H = normalize(V + L);
	float distance = length(light.positionWS - posWS);
	float attenuation = 1.0 / (distance * distance);
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

    float theta_sintheta = (x > 0.0) ? v : 0.5*rsqrt(max(1.0 - x*x, 1e-7)) - v;

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

#endif