#version 330

in vec2 ViewRay;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D depthTexture;
uniform float sampleRadius;
uniform float strength;
uniform mat4 projection;

uniform float near;
uniform float far;

const float bias = 0.025;
const int MAX_KERNEL_SIZE = 48;
uniform vec3 kernel[MAX_KERNEL_SIZE];

float GetViewZ( vec2 coords ) {
	float depth = texture( depthTexture, coords ).r;
	float nom = 2.0 * far * near;
	float denom = 2.0 * depth * ( near - far ) + 2.0 * far;
	return nom / denom;
}

void main() {
	float zView = GetViewZ( TexCoord );
	float xView = ViewRay.x * zView;
	float yView = ViewRay.y * zView;
	vec3 pos_view = vec3( xView, yView, zView );

	float occlusion = 0.0;
	for (int i = 0 ; i < MAX_KERNEL_SIZE ; i++) {
		vec3 samplePos = pos_view + kernel[i];
		vec4 offset = vec4( samplePos, 1.0 );
		offset = projection * offset;
		offset.xy /= -offset.w;
		offset.xy = offset.xy * 0.5 + vec2( 0.5 );
			
		float sampleZView = GetViewZ( offset.xy );
		float rangeCheck = smoothstep( 0.0, 1.0, sampleRadius / abs( zView - sampleZView ) );
		occlusion += ( zView >= sampleZView + bias ? 1.0 : 0.0 ) * rangeCheck; 

		/*
		float sampleZView = GetViewZ( offset.xy );
		if ( abs( pos_view.z - sampleZView ) < sampleRadius ) {
			occlusion += step( sampleZView, samplePos.z );
		}
		*/

	}
	occlusion = 1.0 - occlusion / float( MAX_KERNEL_SIZE );
	occlusion = pow( occlusion, strength );

	FragColor = vec4( occlusion, occlusion, occlusion, 1.0 );
}