#version 330 core

uniform sampler2D errorTexture;

in vec2 TexCoords;

out vec4 FragColor;

void main() {
	vec3 col = texture( errorTexture, TexCoords ).rgb;
	FragColor = vec4( col, 1.0 );
}