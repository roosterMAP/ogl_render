#version 330 core

in vec2 TexCoord;

uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoRough;
uniform sampler2D gSpecular;
uniform sampler2D gIBL;
uniform int mode;
uniform float near;
uniform float far;

out vec4 FragColor;

void main() {
	if ( mode == 1 ) {
		FragColor = vec4( texture( gAlbedoRough, TexCoord ).rgb, 1.0 );
	} else if ( mode == 2 ) {
		FragColor = vec4( texture( gSpecular, TexCoord ).rgb, 1.0 );
	} else if ( mode == 3 ) {
		FragColor = vec4( texture( gNormal, TexCoord ).rgb, 1.0 );
	} else if ( mode == 4 ) {
		float rough = texture( gAlbedoRough, TexCoord ).a;
		FragColor = vec4( rough, rough, rough, 1.0 );
	} else if ( mode == 5 ) {
		FragColor = vec4( texture( gIBL, TexCoord ).rgb, 1.0 );
	} else if ( mode == 6 ) {
		float depth = texture( gDepth, TexCoord ).r;
		float ndc_depth = depth * 2.0 - 1.0; // back to NDC
		float clip_z = ( 2.0 * near * far ) / ( far + near - ndc_depth * ( far - near ) );	
		FragColor = vec4( clip_z, clip_z, clip_z, 1.0 );
	} else {
		FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
	}
}