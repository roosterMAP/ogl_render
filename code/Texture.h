#pragma once
#ifndef __TEXTURE_H_INCLUDE__
#define __TEXTURE_H_INCLUDE__

#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstdio>
#include <cassert>
#include <windows.h>
#include <vector>
#include "ispc_texcomp.h"

/*
========================
F32toF16
========================
*/
inline unsigned short F32toF16( float a ) {
	unsigned int f = *(unsigned *)( &a );
	unsigned int signbit  = ( f & 0x80000000 ) >> 16;
	int exponent = ( ( f & 0x7F800000 ) >> 23 ) - 112;
	unsigned int mantissa = ( f & 0x007FFFFF );

	if ( exponent <= 0 ) {
		return 0;
	}
	if ( exponent > 30 ) {
		return ( unsigned short )( signbit | 0x7BFF );
	}

	return ( unsigned short )( signbit | ( exponent << 10 ) | ( mantissa >> 13 ) );
}

/*
===============================
Texture
===============================
*/
class Texture {
	public:
		Texture();
		~Texture() {};
		void Delete();
	
		virtual bool InitFromFile( const char * relativePath );
		virtual bool InitFromFile_Uncompressed( const char * relativePath );
		unsigned int GetName()	const { return mName; }
		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }
		int GetChanCount() const { return mChanCount; }
		GLenum GetTarget() const { return mTarget; }

		static bool CompressFromFile( const char * relativePath );

		static std::vector< Texture* > s_textures;
		char mStrName[1024];
		bool m_empty;
		bool m_compressed;

		static unsigned int s_errorTexture;
		static unsigned int InitErrorTexture();
		virtual void UseErrorTexture();

	protected:
		unsigned int mName;
		int mWidth;
		int mHeight;
		int mChanCount;
		GLenum mTarget;

	private:
		void InitWithData( const unsigned char * data, const int width, const int height, const int chanCount, const char * ext );
};

/*
===============================
CubemapTexture
===============================
*/
class CubemapTexture : public Texture {
	public:
		CubemapTexture();
		~CubemapTexture() {};
		void Delete();

		bool InitFromFile( const char * relativePath );
		bool InitFromFile_Uncompressed( const char * relativePath );
		void PrefilterSpeculateProbe();

		static unsigned int s_errorCube;
		static unsigned int InitErrorCube();
		void UseErrorTexture() override;

	private:
		void InitWithData( const void * data, const int face, const int height, const int chanCount, const char * ext );
		void InitWithData( const float * data, const int face, const int height, const int chanCount );
};

/*
===============================
LUTTexture
===============================
*/
class LUTTexture : public Texture {
	public:
		bool InitFromFile( const char * relativePath );

	private:
		int mDepth;
};

#endif