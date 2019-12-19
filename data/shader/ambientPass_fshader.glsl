#version 330 core

out vec4 FragColor;

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D glossTexture;
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredEnvMap;
uniform sampler2D brdfLUT;

uniform vec3 camPos;

in vec3 FragPos;
in mat3 TBN;
in vec2 TexCoord;

//Shlick approximation of the Fresnel equation
vec3 FresnelShlick( float cosTheta, vec3 F0, float roughness ) {
	//Sébastien Lagarde describes a way to inject the roughness term into the Shlick approximation:
	//	https://seblagarde.wordpress.com/2011/08/17/hello-world/
	return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}

void main() {
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
	vec3 N = normalize( normal );

	//calculate the fresnel component
	vec3 V = normalize( camPos - FragPos );
	float NdotV = clamp( dot( N, V ), 0.0, 1.0 );
	vec3 F = FresnelShlick( NdotV, specular, roughness );

	//calculate ambient term
	vec3 irradiance = texture( irradianceMap, N ).rgb;
	vec3 diffuse = irradiance * albedo;

	//compute the specular term
	const float MAX_REFLECTION_LOD = 4.0;
	vec3 R = reflect( -V, N );
	vec3 prefilteredColor = textureLod( prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD ).rgb;
	vec2 envBRDF = texture( brdfLUT, vec2( NdotV, roughness ) ).rg;
	specular = prefilteredColor * ( F * envBRDF.x + envBRDF.y );

	vec3 ambient = ( 1.0 - F ) * diffuse + specular;

	FragColor = vec4( ambient, 1.0 );
}