#version 430 core

out vec4 FragColor;

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D glossTexture;
uniform vec3 emissiveColor;

uniform sampler2DShadow shadowAtlas;

uniform sampler2D brdfLUT;
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredEnvMap;

const float E = 2.71828182846;
const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

const int maxLightsPerTile = 32;
struct lightIDs {
	uint count;
	int ids[maxLightsPerTile];
};
layout ( std430 ) buffer light_LUT {
	lightIDs lightLists[];
};

struct Light {
	int typeIndex;
	float pos_x, pos_y, pos_z;
	float col_r, col_g, col_b;
	float dir_x, dir_y, dir_z;
	float angle; //splot light only
	float radius;
	float max_radius;
	float dir_radius; //directional light only
	float brightness;
	int shadowIdx;
};
layout ( std430 ) buffer light_buffer {
	Light light_data[];
};

struct Shadow {
	vec4 m_row1;
	vec4 m_row2;
	vec4 m_row3;
	vec4 m_row4;
	vec4 loc;
};
layout ( std430 ) buffer shadow_buffer {
	Shadow shadow_data[];
};

//scene uniforms
#define WORK_GROUP_SIZE 16
uniform int screenWidth;
//uniform int screenHeight;
uniform int shadowMapPartitionSize;
uniform vec3 camPos;

in vec3 FragPos;
in mat3 TBN;
in vec2 TexCoord;

//Trowbridge-Reitz microfacet distribution function
float NormalDistribution( float NdotH, float roughness ) {
	float alpha = roughness * roughness;
	float denom = NdotH * NdotH * ( alpha * alpha - 1.0 ) + 1.0;
	return ( alpha * alpha ) / ( PI * denom * denom );
}

//Shlick approximation of the Fresnel equation
vec3 FresnelShlickIBL( float cosTheta, vec3 F0, float roughness ) {
	//Sébastien Lagarde describes a way to inject the roughness term into the Shlick approximation:
	//	https://seblagarde.wordpress.com/2011/08/17/hello-world/
	return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( 1.0 - cosTheta, 5.0 );
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

//returns the index of the axis most aligned with dir arg
int GetFaceIdx( vec3 dir ) {
	int returnIdx = 0;
	float largesttDot = -1.0;

	float dots[6];
	dots[0] = dot( dir, vec3( 1.0, 0.0, 0.0 ) );
	dots[1] = dot( dir, vec3( 0.0, 0.0, 1.0 ) );
	dots[2] = dot( dir, vec3( -1.0, 0.0, 0.0 ) );
	dots[3] = dot( dir, vec3( 0.0, 0.0, -1.0 ) );
	dots[4] = dot( dir, vec3( 0.0, 1.0, 0.0 ) );
	dots[5] = dot( dir, vec3( 0.0, -1.0, 0.0 ) );

	for ( int i = 0; i < 6; i++ ) {
		float testDot = dots[i];
		if ( testDot > largesttDot ) {
			largesttDot = testDot;
			returnIdx = i;
		}
	}

	return returnIdx;
}

float ShadowCalculation( float bias, Light light ) {
	vec2 atlasSize = vec2( textureSize( shadowAtlas, 0 ) );
	int shadowIdx = light.shadowIdx;

	int faceIdx = 0;
	if ( light.typeIndex == 3 ) {
		vec3 lightPos = vec3( light.pos_x, light.pos_y, light.pos_z );
		faceIdx = GetFaceIdx( normalize( FragPos - lightPos ) ); //get which face to sample shadows from
	}

	mat4 shadowMatrix;
	shadowMatrix[0] = shadow_data[shadowIdx + faceIdx].m_row1;
	shadowMatrix[1] = shadow_data[shadowIdx + faceIdx].m_row2;
	shadowMatrix[2] = shadow_data[shadowIdx + faceIdx].m_row3;
	shadowMatrix[3] = shadow_data[shadowIdx + faceIdx].m_row4;

	vec4 FragPosLightSpace = shadowMatrix * vec4( FragPos, 1.0 );
	vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w; //perform perspective divide
	projCoords = projCoords * 0.5 + 0.5; //transform to [0,1] range	
	float tileSize_normalized = float( shadowMapPartitionSize ) / atlasSize.x;
	projCoords.x = projCoords.x * tileSize_normalized + shadow_data[shadowIdx + faceIdx].loc.x;
	projCoords.y = projCoords.y * tileSize_normalized + shadow_data[shadowIdx + faceIdx].loc.y;

	vec3 samplePos;
	int sampleCount = 0;
	vec2 texelSize = 1.0 / atlasSize;
	float cumulativeShadow = 0.0;
	for ( int x = -4; x <= 4; x++ ) {
		for ( int y = -4; y <= 4; y++ ) {
			samplePos = vec3( projCoords.xy + ( vec2( x, y ) * texelSize ), projCoords.z - bias );
			cumulativeShadow += texture( shadowAtlas, samplePos, 0 );
			sampleCount += 1;
		}
	}
	cumulativeShadow /= float( sampleCount );

	return cumulativeShadow;
}

float LightAttenuation( vec3 P, vec3 lightCenter, float lightRadius, float maxRadius, float brightness, float cutoff ) {
	//https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/

	//calculate normalized light vector and distance to sphere light surface
	float r = lightRadius;
	float d = max( length( lightCenter - P ) - r, 0.0 );
	
	//calculate basic attenuation
	float denom = d / r + 1.0;
	float attenuation = brightness / ( denom * denom );
	
	//scale and bias attenuation such that:
	//	attenuation == 0 at extent of max influence
	//	attenuation == 1 when d == 0
	attenuation = ( attenuation - cutoff ) / ( 1.0 - cutoff );
	attenuation = max( attenuation, 0.0 );

	//use gausian function to make the attenuation falloff within the lights maxRadius
	float gaussFactor = pow( E, -4.0 * pow( ( d / maxRadius ), 4.0 ) ) - 0.1;

	return attenuation * gaussFactor;
}

float distanceToLine( vec3 a, vec3 ab, vec3 v ) {
	vec3 av = v - a;
	vec3 closestPos = a + ab * dot( av, ab ) / dot( ab, ab );
	return length( v - closestPos );
}

void main() {
	//sample texture value for current fragment
	vec3 albedo = texture( albedoTexture, TexCoord ).rgb;
	float emissive = texture( albedoTexture, TexCoord ).a;
	vec3 specular = texture( specularTexture, TexCoord ).rgb;
	vec3 normal = texture( normalTexture, TexCoord ).rgb;
	float roughness = 1.0 - texture( glossTexture, TexCoord ).r;

	//gamma correct the albedo fragment
	albedo = pow( albedo, vec3( 2.2 ) );

	//transform the sampled normal into worldspace
	normal = normalize( normal * 2.0 - 1.0 );
	normal *= TBN;

	//initialize important vectors necisary for BRDF
	vec3 V = normalize( camPos - FragPos );
	vec3 N = normalize( normal );
	float NdotV = clamp( dot( N, V ), 0.0, 1.0 );

	//ambient
	vec3 irradiance = texture( irradianceMap, N ).rgb;
	vec3 diffuse_IBL = irradiance * albedo;
	vec3 R = reflect( -V, N );
	vec3 prefilteredColor = textureLod( prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD ).rgb;
	vec2 envBRDF = texture( brdfLUT, vec2( NdotV, roughness ) ).rg;	
	vec3 F_IBL = FresnelShlickIBL( NdotV, specular, roughness );
	vec3 specularIBL = prefilteredColor * ( F_IBL * envBRDF.x + envBRDF.y );
	vec3 ambientComponent = ( 1.0 - F_IBL ) * diffuse_IBL + specularIBL;

	ivec2 tiledCoord = ivec2( gl_FragCoord.xy ) / WORK_GROUP_SIZE;
	int workGroupID = tiledCoord.x + tiledCoord.y * screenWidth / WORK_GROUP_SIZE;

	vec3 totalRadiance = vec3( 0.0, 0.0, 0.0 );
	int numLights = int( lightLists[ workGroupID ].count );
	for( int n = 0; n < numLights; n++ ) {
		int lightID = lightLists[workGroupID].ids[n];
		Light currentLight = light_data[lightID];
		vec3 lightPos = vec3( currentLight.pos_x, currentLight.pos_y, currentLight.pos_z );
		vec3 lightCol = vec3( currentLight.col_r, currentLight.col_g, currentLight.col_b );
		vec3 lightDir = vec3( currentLight.dir_x, currentLight.dir_y, currentLight.dir_z );

		float attenuation = 1.0;
		attenuation = LightAttenuation( FragPos, lightPos, currentLight.radius, currentLight.max_radius, currentLight.brightness, 0.001 );

		vec3 L;
		if ( currentLight.typeIndex == 1 ) { //directional light
			L = -lightDir;
		} else {
			L = normalize( lightPos - FragPos );
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
		float G = VisibilityTerm( NdotL, roughness ) * VisibilityTerm( NdotV, roughness );

		//specular
		vec3 specularComponent = G * D * F;

		//for directional and spot lights, put everything outside of radius in shadow by scaling the attenuation
		if ( currentLight.typeIndex == 1 ) { //directional light
			float dist = distanceToLine( lightPos, lightDir, FragPos );
			float intensity = clamp( ( currentLight.dir_radius - dist ) / 0.01, 0.0, 1.0 );
			attenuation *= intensity;
		} else if ( currentLight.typeIndex == 2 ) { //spot light
			float theta = dot( -lightDir, L );
			float intensity = clamp( ( theta - currentLight.angle ) / 0.01, 0.0, 1.0 );
			attenuation *= intensity;
		}

		//light radiance
		vec3 radiance = lightCol * NdotL * attenuation * currentLight.brightness;

		//diffuse
		vec3 diffuseComponent = ( 1.0 - F ) * ( albedo / PI ); //Lambert diffuse component

		//radiance
		vec3 outgoingRadiance = ( diffuseComponent + specularComponent ) * radiance * NdotL;

		//shadow
		if ( currentLight.shadowIdx > -1 ){
			float bias = max( 0.01 * ( 1.0 - NdotL ), 0.001 );
			float shadow = ShadowCalculation( bias, currentLight );
			outgoingRadiance *= shadow;
		}

		totalRadiance += outgoingRadiance;
	}

	totalRadiance += ambientComponent;

	//emmisive
	totalRadiance += emissiveColor * emissive;

	FragColor = vec4( totalRadiance, 1.0 );
}