#version 330 core

layout ( location = 0 ) out vec3 gNormal;
layout ( location = 1 ) out vec4 gAlbedoRough;
layout ( location = 2 ) out vec3 gSpecular;
layout ( location = 3 ) out vec3 gIBL;

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D glossTexture;
uniform sampler2D brdfLUT;
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredEnvMap;

uniform vec3 camPos;

in vec3 FragPos;
in vec2 TexCoord;
in mat3 TBN;

const float MAX_REFLECTION_LOD = 4.0;

//Shlick approximation of the Fresnel equation
vec3 FresnelShlickIBL( float cosTheta, vec3 F0, float roughness ) {
	//Sébastien Lagarde describes a way to inject the roughness term into the Shlick approximation:
	//	https://seblagarde.wordpress.com/2011/08/17/hello-world/
	return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}

void main() {
	vec3 albedo = texture( albedoTexture, TexCoord ).rgb;
	vec3 specular = texture( specularTexture, TexCoord ).rgb;
	vec3 normal = texture( normalTexture, TexCoord ).rgb;
	float roughness = 1.0 - texture( glossTexture, TexCoord ).r;

	//gamma correct the albedo fragment
	albedo = pow( albedo, vec3( 2.2 ) );

	normal = normalize( normal * 2.0 - 1.0 );
	normal *= TBN;

	gNormal = normal;
	gAlbedoRough.rgb = albedo;
	gAlbedoRough.a = roughness;
	gSpecular = specular;
	
	vec3 V = normalize( camPos - FragPos );
	vec3 N = normalize( normal );
	float NdotV = clamp( dot( N, V ), 0.0, 1.0 );
	vec3 R = reflect( -V, N );

	vec3 irradiance = texture( irradianceMap, N ).rgb;
	vec3 prefilteredColor = textureLod( prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD ).rgb;
	vec2 envBRDF = texture( brdfLUT, vec2( NdotV, roughness ) ).rg;

	vec3 diffuse_IBL = irradiance * albedo;	
	vec3 F_IBL = FresnelShlickIBL( NdotV, specular, roughness );
	vec3 specularIBL = prefilteredColor * ( F_IBL * envBRDF.x + envBRDF.y );
	gIBL = ( 1.0 - F_IBL ) * diffuse_IBL + specularIBL;
}