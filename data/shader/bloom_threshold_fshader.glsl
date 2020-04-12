#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float threshold;
uniform sampler2D screenTexture;

void main() {
	vec3 col = texture( screenTexture, TexCoords ).rgb;
	float mag = length( col );

	if( mag >= threshold ) {
		FragColor = vec4( col, 1.0 );
	} else {
		//prevents hard cutoff at threshold.
		float intensity = 1.0 - clamp( abs( threshold - mag ) / 0.1, 0.0, 1.0 );
		FragColor = vec4( col * intensity, 1.0 );
	}
}