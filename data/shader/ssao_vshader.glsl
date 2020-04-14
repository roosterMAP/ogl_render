#version 330

layout ( location = 0 ) in vec3 aPos;
layout ( location = 1 ) in vec2 aTexCoords;

uniform float aspect;
uniform float tanHalfFOV;

out vec2 TexCoord;
out vec2 ViewRay;

void main() {
	gl_Position = vec4( aPos, 1.0 );
	TexCoord = aTexCoords;
	ViewRay.x = aPos.x * aspect * tanHalfFOV;
	ViewRay.y = aPos.y * tanHalfFOV;
}