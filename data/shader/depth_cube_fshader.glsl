#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main() {	
	float lightDistance = length( FragPos.xyz - lightPos ); //get distance between fragment and light source	
	lightDistance = lightDistance / far_plane; //map to [0;1] range by dividing by far_plane	
	gl_FragDepth = lightDistance; //write this as modified depth
}