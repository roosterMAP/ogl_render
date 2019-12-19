#version 330 core

out vec4 FragColor;

uniform sampler2D sampleTexture;
uniform bool debugNormal;

in vec2 TexCoord;
in mat3 TBN;

void main() {
	if ( debugNormal ) {
		vec3 normal = texture( sampleTexture, TexCoord ).rgb;
		normal = normalize( normal * 2.0 - 1.0 );
		normal *= TBN;
		FragColor = vec4( normal, 1.0 );
	} else {
		vec3 color = texture( sampleTexture, TexCoord ).rgb;
		FragColor = vec4( color, 1.0 );
	}
}