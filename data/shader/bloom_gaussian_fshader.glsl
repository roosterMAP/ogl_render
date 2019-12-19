#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform float offsets[3];
uniform float weights[3];

void main() {
	vec2 tex_offset = 1.0 / textureSize( image, 0 ); // gets size of single texel
	vec3 result = texture( image, TexCoords ).rgb * weights[0];
	if( horizontal ) {
		for( int i = 1; i < 3; i++ ) {
			result += texture( image, TexCoords + vec2( tex_offset.x * offsets[i], 0.0 ) ).rgb * weights[i];
			result += texture( image, TexCoords - vec2( tex_offset.x * offsets[i], 0.0 ) ).rgb * weights[i];
		}
	} else {
		for( int i = 1; i < 3; i++ ) {
			result += texture( image, TexCoords + vec2( 0.0, tex_offset.y * offsets[i] ) ).rgb * weights[i];
			result += texture( image, TexCoords - vec2( 0.0, tex_offset.y * offsets[i] ) ).rgb * weights[i];
		}
	}
	FragColor = vec4( result, 1.0 );
}