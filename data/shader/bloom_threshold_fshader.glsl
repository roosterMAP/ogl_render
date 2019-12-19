#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 threshold;
uniform sampler2D screenTexture;

void main() {
	vec3 col = texture( screenTexture, TexCoords ).rgb;
	if( dot( col, threshold ) > 1.0 ) {
		FragColor = vec4( col, 1.0 );
	} else {
		FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
	}
}