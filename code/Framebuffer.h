#pragma once

#include <vector>
#include <assert.h>

#include "Fileio.h"
#include "Shader.h"

#include <GL/glew.h>
#include <GL/glut.h>

#ifndef __FRAMEBUFFER_H_INCLUDE__
#define __FRAMEBUFFER_H_INCLUDE__

/*
===============================
Framebuffer
===============================
*/
class Framebuffer {
	public:
		Framebuffer() { m_multisample = 0; }
		Framebuffer( std::string colorbufferUniform );
		Framebuffer( const Framebuffer & rhs ) { m_multisample = 0; *this = rhs; }
		~Framebuffer() {};

		unsigned int GetID() { return m_id; }
		unsigned int GetWidth() { return m_width; }
		unsigned int GetHeight() { return m_height; }
		void SetColorBufferUniform( std::string name ) { m_colorbufferUniform = name; }
		bool CreateDefaultBuffer( unsigned int width, unsigned int height );
		bool CreateNewBuffer( unsigned int width, unsigned int height, const char *shaderPrefix );
		void AttachTextureBuffer( GLenum internalformat=GL_RGB, GLenum attachment=GL_COLOR_ATTACHMENT0, GLenum format=GL_RGB, GLenum pixelFormat=GL_UNSIGNED_BYTE );
		void AttachCubeMapTextureBuffer( GLenum internalformat=GL_RGB, GLenum attachment=GL_COLOR_ATTACHMENT0, GLenum format=GL_RGB, GLenum pixelFormat=GL_UNSIGNED_BYTE );
		void AttachRenderBuffer( GLenum internalformat=GL_DEPTH24_STENCIL8, GLenum attachment=GL_DEPTH_STENCIL_ATTACHMENT );
		void DrawToMultipleBuffers();
		bool Status();
		void EnableMultisampledAttachments( unsigned int subsampleCount ) { m_multisample = subsampleCount; }
		void Bind() { glBindFramebuffer( GL_FRAMEBUFFER, m_id ); }
		void Unbind() { glBindFramebuffer( GL_FRAMEBUFFER, 0 ); }
		void CreateScreen( int screen_xPos=0, int screen_yPos=0, int screen_width=-1, int screen_height=-1 );
		void Draw( unsigned int colorBuffer );
		const unsigned int GetScreenVAO() { return m_screenVAO; }
		Shader* GetShader() { return m_shader; }

		std::vector< unsigned int > m_attachements;
		

	protected:
		unsigned int m_width, m_height;
		unsigned int m_id, m_screenVAO;
		std::string m_colorbufferUniform; //the name of the texture uniform being passed to m_shader to be displayed on the screenVAO

		Shader * m_shader; //the shader used to display the render target to the screenVAO

		unsigned int m_colorBufferCount;

		static const char * s_vshader_source;
		static const char * s_fshader_source;
		static const char * s_fshader_source_shadow;

		unsigned int m_multisample;
};

#endif