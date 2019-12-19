#version 330 core

layout ( triangles ) in;
layout ( line_strip, max_vertices = 18 ) out;

in VS_OUT {
	vec4 normal;
	vec4 tangent;
	vec4 bitangent;
} gs_in[];

out vec3 fColor;

const float MAGNITUDE = 0.1;

void GenerateLine( int index ) {
	//normal
	fColor = vec3( 0.0, 0.0, 1.0 ); //blue
	gl_Position = gl_in[index].gl_Position;
	EmitVertex();
	gl_Position = gl_in[index].gl_Position + gs_in[index].normal * MAGNITUDE;
	EmitVertex();
	EndPrimitive();

	//tangent
	fColor = vec3( 1.0, 0.0, 0.0 ); //red
	gl_Position = gl_in[index].gl_Position;
	EmitVertex();
	gl_Position = gl_in[index].gl_Position + gs_in[index].tangent * MAGNITUDE;
	EmitVertex();
	EndPrimitive();

	//bitangent
	fColor = vec3( 0.0, 1.0, 0.0 ); //green
	gl_Position = gl_in[index].gl_Position;
	EmitVertex();
	gl_Position = gl_in[index].gl_Position + gs_in[index].bitangent * MAGNITUDE;
	EmitVertex();
	EndPrimitive();
}

void main() {
	GenerateLine(0);
	GenerateLine(1);
	GenerateLine(2);
}