#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomTexture;
uniform sampler2D ssaoTexture;
uniform sampler3D lookup;
uniform float exposure;
uniform bool bloomEnabled;
uniform bool ssaoEnabled;

void main() {
	/*
	const float offset = 1.0 / 300.0;
	vec2 offsets[9] = vec2[](
		vec2(-offset,  offset), // top-left
		vec2( 0.0f,    offset), // top-center
		vec2( offset,  offset), // top-right
		vec2(-offset,  0.0f),   // center-left
		vec2( 0.0f,    0.0f),   // center-center
		vec2( offset,  0.0f),   // center-right
		vec2(-offset, -offset), // bottom-left
		vec2( 0.0f,   -offset), // bottom-center
		vec2( offset, -offset)  // bottom-right    
	);

	float kernel[9] = float[](
		 0, -1,  0,
		-1,  5, -1,
		 0, -1,  0
	);

	vec3 sampleTex[9];
	for( int i = 0; i < 9; i++ ) {
		sampleTex[i] = vec3( texture( screenTexture, TexCoords.st + offsets[i] ) );
	}

	vec3 col = vec3( 0.0 );
	for( int i = 0; i < 9; i++ ) {
		col += sampleTex[i] * kernel[i];
	}
	*/
	vec3 hdrColor = texture( screenTexture, TexCoords ).rgb;

	//Gaussian bloom
	if ( bloomEnabled ) {
		vec3 bloomColor = texture( bloomTexture, TexCoords ).rgb;
		hdrColor += bloomColor / 3.0;
	}

	//ssao
	if ( ssaoEnabled ) {
		float ao = texture( ssaoTexture, TexCoords ).r;
		hdrColor *= ao;
	}

	//Reinhard tone mapping
	vec3 mapped = vec3( 1.0 ) - exp( -hdrColor * exposure );

	//Gamma correction
	const float gamma = 2.2;
	mapped = pow( mapped, vec3( 1.0 / gamma ) );

	//LUT applied after colors have been tonemapped back to LDR and gamma corrected to sRGB
	mapped = texture( lookup, clamp( mapped.rgb, 0.0, 1.0 ) ).rgb;

	FragColor = vec4( mapped, 1.0 );
}