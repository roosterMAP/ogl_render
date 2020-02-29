#version 330 core

out vec4 FragColor;

uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoRough;
uniform sampler2D gSpecular;
uniform sampler2D gIBL;

uniform sampler2DShadow shadowAtlas;

const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const float SHADOWMAPSIZE = 512.0;
const int MAX_LIGHTS = 16;
const float MAX_REFLECTION_LOD = 4.0;

//light uniforms
struct Light {
	int typeIndex;
	vec3 position;
	vec3 color;
	vec3 direction;
	float angle;
	float radius;
	int shadowIdx;
};
uniform Light lights[MAX_LIGHTS];

struct Shadow {
	mat4 matrix;
	vec2 loc;
};
uniform Shadow shadows[MAX_LIGHTS];

//scene uniforms
uniform int shadowMapPartitionSize;
uniform int lightCount;
uniform vec3 camPos;
uniform mat4 projMatrixInv;
uniform mat4 viewMatrixInv;

in vec2 TexCoord;

vec3 WorldPosFromDepth( float depth ) {
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4( TexCoord * 2.0 - 1.0, z, 1.0 );
	vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	vec4 worldSpacePosition = viewMatrixInv * viewSpacePosition;

	return worldSpacePosition.xyz;
}

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

float ShadowCalculation( float bias, int lightIdx, vec3 fragPos ) {
	vec2 atlasSize = vec2( textureSize( shadowAtlas, 0 ) );
	int shadowIdx = lights[lightIdx].shadowIdx;

	int faceIdx = 0;
	if ( lights[lightIdx].typeIndex == 3 ) {
		faceIdx = GetFaceIdx( normalize( fragPos - lights[lightIdx].position ) ); //get which face to sample shadows from
	}

	vec4 FragPosLightSpace = shadows[shadowIdx + faceIdx].matrix * vec4( fragPos, 1.0 );	
	vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w; //perform perspective divide
	projCoords = projCoords * 0.5 + 0.5; //transform to [0,1] range	
	float tileSize_normalized = float( shadowMapPartitionSize ) / atlasSize.x;
	projCoords.x = projCoords.x * tileSize_normalized + shadows[shadowIdx + faceIdx].loc.x;
	projCoords.y = projCoords.y * tileSize_normalized + shadows[shadowIdx + faceIdx].loc.y;

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

float LightAttenuation( vec3 P, vec3 N, vec3 lightCenter, float lightRadius, float cutoff ) {
	//https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/

	//calculate normalized light vector and distance to sphere light surface
	float r = lightRadius;
	vec3 L = lightCenter - P;
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
	vec4 sample = texture( gAlbedoRough, TexCoord );

	//calculate FragPos from gDepth
	float depth = texture( gDepth, TexCoord ).x;
	vec3 FragPos = WorldPosFromDepth( depth );

	//sample texture value for current fragment
	vec3 albedo = sample.rgb;
	vec3 specular = texture( gSpecular, TexCoord ).rgb;
	vec3 normal = texture( gNormal, TexCoord ).rgb;
	float roughness = sample.a;
	vec3 ambient = texture( gIBL, TexCoord ).rgb;

	//initialize important vectors necisary for BRDF
	vec3 V = normalize( camPos - FragPos );
	vec3 N = normalize( normal );
	float NdotV = clamp( dot( N, V ), 0.0, 1.0 );

	vec3 totalRadiance = vec3( 0.0, 0.0, 0.0 );
	for( int n = 0; n < lightCount; n++ ) {
		float attenuation = 1.0;
		attenuation = LightAttenuation( FragPos, N, lights[n].position, lights[n].radius, 0.001 );

		vec3 L;
		if ( lights[n].typeIndex == 1 ) { //directional light
			L = -lights[n].direction;
		} else {
			L = normalize( lights[n].position - FragPos );
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

		//for spot lights, but everything outside of radius in shadow by scaling the attenuation
		if ( lights[n].typeIndex == 2 ) { //spot light
			float theta = dot( -lights[n].direction, L );
			float intensity = clamp( ( theta - lights[n].angle ) / 0.01, 0.0, 1.0 );
			attenuation *= intensity;
		}

		//light radiance
		vec3 radiance = lights[n].color * NdotL * attenuation;

		//diffuse
		vec3 diffuseComponent = ( 1.0 - F ) * ( albedo / PI ); //Lambert diffuse component

		//radiance
		vec3 outgoingRadiance = ( diffuseComponent + specularComponent ) * radiance * NdotL;

		//shadow
		if ( lights[n].shadowIdx > -1 ){
			float bias = max( 0.01 * ( 1.0 - NdotL ), 0.001 );
			float shadow = ShadowCalculation( bias, n, FragPos );
			outgoingRadiance *= shadow;
		}

		totalRadiance += outgoingRadiance;
	}

	totalRadiance += ambient;

	totalRadiance = totalRadiance;

	FragColor = vec4( totalRadiance, 1.0 );
}