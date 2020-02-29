#version 330 core

layout ( location = 0 ) in vec3 aPos;
layout ( location = 1 ) in vec3 aNormal;
layout ( location = 2 ) in vec3 aTangent;
layout ( location = 4 ) in float aFSign;
layout ( location = 5 ) in mat4 model;

out VS_OUT {
	vec4 normal;
	vec4 tangent;
	vec4 bitangent;
} vs_out;

uniform mat4 projection;
uniform mat4 view;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0); 
	
	//compute tangent space
	mat3 model_noScale = mat3( transpose( inverse( model ) ) );
	vec3 T = normalize( model_noScale * aTangent );
	vec3 N = normalize( model_noScale * aNormal );
	T = normalize( T - dot( T, N ) * N ); //re-orthogonalize T with respect to N
	vec3 B = cross( N, T ) + ( aFSign * 0.00000001 ) ;

	vs_out.normal = normalize( projection * view * vec4( N, 0.0 ) );
	vs_out.tangent = normalize( projection * view * vec4( T, 0.0 ) );
	vs_out.bitangent = normalize( projection * view * vec4( B, 0.0 ) );
}