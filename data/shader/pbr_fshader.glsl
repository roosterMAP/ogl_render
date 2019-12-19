#version 330 core

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;
uniform sampler2D smoothnessTexture;

in vec3 vColor;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
	vec4 imgSample1 = texture( albedoTexture, TexCoord );
	vec4 imgSample2 = texture( specularTexture, TexCoord );
	vec4 imgSample3 = texture( normalTexture, TexCoord );
	vec4 imgSample4 = texture( smoothnessTexture, TexCoord );

	if( imgSample1.a < 1.0 ) {
		discard;
	}

	FragColor = imgSample1 * imgSample2 * imgSample3 * imgSample4;
} 