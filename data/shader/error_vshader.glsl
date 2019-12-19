#version 330 core

layout ( location = 0 ) in vec3 aPos;
layout ( location = 3 ) in vec2 aUV;
layout ( location = 5 ) in mat4 model;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main() {
	TexCoords = aUV;
	gl_Position = projection * view * model * vec4( aPos, 1.0 );
}