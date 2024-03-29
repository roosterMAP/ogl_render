#pragma once
#ifndef __POSTPROCESS_H_INCLUDE__
#define __POSTPROCESS_H_INCLUDE__

#include "Framebuffer.h"
#include "Texture.h"
#include "Vector.h"
#include "String.h"
#include "Camera.h"

#define SSAO_KERNEL_SIZE 48 //must be same as in ssao fshader

/*
===============================
PostProcessManager
	-Object manages the chain of post process effects.
	-It blits the contents of an input framebuffer to its own.
===============================
*/
class PostProcessManager {
	public:
		PostProcessManager() { m_bloomEnabled = false; m_lutEnabled = false; };
		PostProcessManager( unsigned int width, unsigned int height, const char * shaderPrefix );
		~PostProcessManager() {};

		void SetBlitParams( Vec2 srcMin, Vec2 srcMax, Vec2 dstMin, Vec2 dstMax );
		void BlitFramebuffer( Framebuffer * inputFBO );

		bool BloomEnable( float resScale=0.5 );
		void Bloom();
		void SetBloom( bool val ) { m_bloomEnabled = val; }

		bool SSAOEnable( const Camera * camera );
		void SSAO();
		void SetSSAO( bool val ) { m_ssaoEnabled = val; }

		void LUTEnable( Str lut_image );

		void Draw( const float exposure );
		void DrawBloomOnly( const float exposure );
		void DrawSSAOOnly( const float exposure );

	protected:
		Framebuffer m_PostProcessFBO;

		unsigned int m_blit_srcX, m_blit_srcY, m_blit_srcW, m_blit_srcH;
		unsigned int m_blit_dstX, m_blit_dstY, m_blit_dstW, m_blit_dstH;

		bool m_bloomEnabled;
		float m_bloomThreshold;
		Framebuffer m_bloomFBOs[2];
		Framebuffer m_bloomThresholdFBO;
		unsigned int m_bloomRenderTarget_idx;

		bool m_ssaoEnabled;
		Framebuffer m_ssaoFBO;

		bool m_lutEnabled;
		LUTTexture * m_lutTexture;

		const Camera * m_camera;
};

#endif