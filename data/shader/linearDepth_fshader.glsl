#version 330 core

uniform float far_plane;
uniform vec3 lightPos;

in vec3 FragPos;

void main() {
	float dist = length( FragPos - lightPos );
	dist = dist / far_plane;
	gl_FragDepth = dist;
}