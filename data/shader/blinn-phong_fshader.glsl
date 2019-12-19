#version 330 core

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D powerTexture;

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
	vec3 lighting;

	//sample texture value for current fragment
	vec3 diffuse = texture( diffuseTexture, TexCoord ).rbg; //convert diffuse to linear space
	vec3 specular = texture( specularTexture, TexCoord ).rgb;
	vec3 normal = texture( normalTexture, TexCoord ).rgb;
	float power = texture( powerTexture, TexCoord ).r * 7.0 + 1.0; //scale the power val to a range of 1.0-8.0.

	//gamma correct the diffuse fragment
	float gamma = 2.2;
	diffuse = pow( diffuse, vec3( gamma ) );

	//transform the sampled normal into worldspace
	normal = normalize( normal * 2.0 - 1.0 );
	normal *= TBN;

	if ( light.typeIndex == 4 ) { //ambient light
		lighting = diffuse * light.color * light.ambient;

	} else {
		vec3 L = normalize( light.position - FragPos );
		vec3 N = normalize( normal );

		if ( light.typeIndex == 3 ) { //point light
			float attenuation = LightAttenuation( FragPos, N, light.position, light.radius, 0.001 );
			diffuse *= attenuation;
			specular *= attenuation;
		} else if ( light.typeIndex == 1 ) { //directional light
			L = -light.direction;
		} else if ( light.typeIndex == 2 ) { //spot light
			float theta = dot( -light.direction, L );
			float epsilon = 0.01;
			float intensity = clamp( ( theta - light.angle ) / epsilon, 0.0, 1.0 );
			float attenuation = LightAttenuation( FragPos, N, light.position, light.radius, 0.001 );
			diffuse *= intensity * attenuation;
			specular *= intensity * attenuation;
		}

		//diffuse contribution is based on how alined light vector is to the normal vector.
		float NdotL = max( dot( N, L ), 0.0 );
		diffuse *= light.color * NdotL;

		//specular contribution is based on how alighed light reflection is to the camera/view vector.
		vec3 V = normalize( camPos - FragPos );
		vec3 H = normalize( L + V );

		float shininess = pow( 2.0, power ); //controlls the roughness of the specular highlight. ranges from 2.0-256.0
		float reflectionIntensity = pow( max( dot( N, H ), 0.0 ), shininess );
		specular *= light.color * reflectionIntensity;

		lighting = diffuse + specular;

		//shadow
		if ( light.shadow ){
			float Frag_NdotL = max( dot( FragNormal, L ), 0.0 );
			float bias = max( 0.05 * ( 1.0 - Frag_NdotL ), 0.005 );
			float shadow = ShadowCalculation( bias );
			lighting = shadow * lighting;
		}
	}
	FragColor = vec4( lighting, 1.0 );
}