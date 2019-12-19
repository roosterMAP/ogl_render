#version 330 core

layout ( location = 0 ) in vec3 aPos;
layout ( location = 1 ) in vec3 aNormal;
layout ( location = 2 ) in vec3 aTangent;
layout ( location = 3 ) in vec2 aUV;
layout ( location = 4 ) in float aFSign;
layout ( location = 5 ) in mat4 model;

uniform mat4 view;
uniform mat4 projection;

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

out vec3 FragPos;
out vec3 FragNormal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;
out mat3 TBN;

void main() {
	//pass frag data
	FragPos = aPos;
	FragNormal = aNormal;
	TexCoord = aUV;

	//pass frag pos data in light spase (for 2d shadow maps)
	FragPosLightSpace = light.matrix * vec4( FragPos, 1.0 );

	//compute tangent space matrix
	mat3 model_noScale = mat3( transpose( inverse( model ) ) );
	vec3 T = normalize( model_noScale * aTangent );
	vec3 N = normalize( model_noScale * aNormal );
	T = normalize( T - dot( T, N ) * N ); //re-orthogonalize T with respect to N
	vec3 B = normalize( aFSign * cross( N, T ) );
	TBN = transpose( mat3( T, B, N ) );

	gl_Position = projection * view * model * vec4( aPos, 1.0 );
}