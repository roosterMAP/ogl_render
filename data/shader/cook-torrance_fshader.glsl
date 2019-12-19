#version 330 core

out vec4 FragColor;

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D glossTexture;

struct Light {
	int typeIndex;
	vec3 position;
	vec3 color;
	vec3 direction;
	float angle;
	float radius;
	float ambient;
	bool shadow;
	sampler2DShadow shadowMap;
	samplerCubeShadow shadowCubeMap;
	mat4 matrix;
	float far_plane;
};
uniform Light light;

uniform vec3 camPos;

in vec3 FragPos;
in vec3 FragNormal;
in mat3 TBN;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

const float PI = 3.14159265359;

//Trowbridge-Reitz microfacet distribution function
float NormalDistribution( float NdotH, float roughness ) {
	float alpha = roughness * roughness;
	float denom = NdotH * NdotH * ( alpha * alpha - 1.0 ) + 1.0;
	return ( alpha * alpha ) / ( PI * denom * denom );
}

//Shlick approximation of the Fresnel equation
vec3 FresnelShlick( float cosTheta, vec3 F0 ) {
	return F0 + ( 1.0 - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}

//Lazarov [2011] does a substitution that combines the denom of the BRDF and the Geometry masking term.
float VisibilityTerm( float NdotV, float roughness ) {
	//Schlick-Beckmann except with k = a / 2;
	if ( NdotV > 0.0 ) {
		float alpha2 = roughness * roughness * roughness * roughness;
		float NdotV2 = NdotV * NdotV;
		float denom = NdotV + ( sqrt( alpha2 + pow( 1.0 - alpha2, 2.0 ) * NdotV2 ) );
		return NdotV2 / denom;
	} else {
		return 0.0;
	}
}

// array of offset direction for sampling cube shadowmap
vec3 gridSamplingDisk[20] = vec3[] (
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float ShadowCalculation( float bias ) {
	float cumulativeShadow = 0.0;

	if ( light.typeIndex == 3 ) { //point light
		vec3 fragToLight = FragPos - light.position; //get vector between frag and light pos
		float currentDepth = length( fragToLight ); //get current linear depth between frag and light pos

		float viewDistance = length( camPos - FragPos );
		float diskRadius = ( 1.0 + ( viewDistance / light.far_plane ) ) / 50.0;
		vec4 P;
		for( int i = 0; i < 20; i++ ) {
			P = vec4( fragToLight + gridSamplingDisk[i] * diskRadius, ( currentDepth - bias * 15.0 ) / light.far_plane );
			cumulativeShadow += texture( light.shadowCubeMap, P );		
		}
		cumulativeShadow /= 20.0;

	} else {
		vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w; //perform perspective divide and transform to [0,1] range
		projCoords = projCoords * 0.5 + 0.5; //get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)

		vec3 samplePos;
		int sampleCount = 0;
		vec2 texelSize = 1.0 / vec2( textureSize( light.shadowMap, 0 ) );
		for ( int x = -10; x <= 10; x++ ) {
			for ( int y = -10; y <= 10; y++ ) {
				samplePos = vec3( projCoords.xy + ( vec2( x, y ) * texelSize ), projCoords.z - bias );
				cumulativeShadow += texture( light.shadowMap, samplePos, 0 );
				sampleCount += 1;
			}
		}
		cumulativeShadow /= float( sampleCount );
	}

	return cumulativeShadow;
}

float LightAttenuation( vec3 P, vec3 N, vec3 lightCentre, float lightRadius, float cutoff ) {
	//https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/

	//calculate normalized light vector and distance to sphere light surface
	float r = lightRadius;
	vec3 L = lightCentre - P;
	float distance = length( L );
	float d = max( distance - r, 0.0 );
	L /= distance;
	 
	//calculate basic attenuation
	float denom = d / r + 1.0;
	float attenuation = 1.0 / ( denom * denom );
	 
	//scale and bias attenuation such that:
	//	attenuation == 0 at extent of max influence
	//	attenuation == 1 when d == 0
	attenuation = ( attenuation - cutoff ) / ( 1.0 - cutoff );
	attenuation = max( attenuation, 0.0 );

	return attenuation;
}

void main() {
	vec3 finalColor;

	//sample texture value for current fragment
	vec3 albedo = texture( albedoTexture, TexCoord ).rbg;
	vec3 specular = texture( specularTexture, TexCoord ).rgb;
	vec3 normal = texture( normalTexture, TexCoord ).rgb;
	float roughness = 1.0 - texture( glossTexture, TexCoord ).r;

	//gamma correct the albedo fragment
	float gamma = 2.2;
	albedo = pow( albedo, vec3( gamma ) );

	//transform the sampled normal into worldspace
	normal = normalize( normal * 2.0 - 1.0 );
	normal *= TBN;

	if ( light.typeIndex == 4 ) { //ambient light
		finalColor = albedo * light.color * light.ambient;
	} else {
		//initialize important vectors necisary for BRDF
		vec3 V = normalize( camPos - FragPos );
		vec3 N = normalize( normal );		
		vec3 L;
		float attenuation = 1.0;
		if ( light.typeIndex == 1 ) { //directional light
			L = -light.direction;
		} else {
			L = normalize( light.position - FragPos );
			attenuation = LightAttenuation( FragPos, N, light.position, light.radius, 0.001 );
		}
		vec3 H = normalize( L + V );		

		//BRDF - cook-torrance specular component
		//D - Trowbridge-Reitz microfacet distribution function
		float NdotH = clamp( dot( N, H ), 0.0, 1.0 );
		float D = NormalDistribution( NdotH, roughness );

		//F - Shlick Approximation for the Fresnel Reflection
		float HdotV = clamp( dot( H, V ), 0.0, 1.0 );
		vec3 F = FresnelShlick( HdotV, specular );
		
		//using substitution described in Lazarov, we can skip the calculation of the specBRDF denom.
		float NdotL = clamp( dot( N, L ), 0.0, 1.0 );
		float NdotV = clamp( dot( N, V ), 0.0, 1.0 );
		float G = VisibilityTerm( NdotL, roughness ) * VisibilityTerm( NdotV, roughness );

		specular = G * D * F;

		//for spot lights, but everything outside of radius in shadow by scaling the attenuation
		if ( light.typeIndex == 2 ) { //spot light
			float theta = dot( -light.direction, L );
			float epsilon = 0.01;
			float intensity = clamp( ( theta - light.angle ) / epsilon, 0.0, 1.0 );
			attenuation *= intensity;
		}

		//light radiance
		vec3 radiance = light.color * NdotL * attenuation;

		//outgoing radiance
		vec3 diffuse = ( 1.0 - F ) * ( albedo / PI ); //Lambert diffuse component
		vec3 outgoingRadiance = ( diffuse + specular ) * radiance * NdotL;

		//shadow
		if ( light.shadow ){
			float bias = max( 0.05 * ( 1.0 - NdotL ), 0.005 );
			float shadow = ShadowCalculation( bias );
			outgoingRadiance *= shadow;
		}
		finalColor = outgoingRadiance;
	}
	FragColor = vec4( finalColor, 1.0 );
}