#version 330 core
layout ( location = 0 ) in vec3 aPos;
layout ( location = 5 ) in mat4 model;

uniform mat4 lightSpaceMatrix;

out vec3 FragPos;

void main() {
	FragPos = aPos;
	gl_Position = lightSpaceMatrix * model * vec4( aPos, 1.0 );
}