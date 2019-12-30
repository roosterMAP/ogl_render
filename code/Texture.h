#pragma once
#ifndef __TEXTURE_H_INCLUDE__
#define __TEXTURE_H_INCLUDE__

#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>

/*
===============================
Texture
===============================
*/
class Texture {
	public:
		Texture();
		~Texture();
	
		virtual bool InitFromFile( const char * relativePath );
		unsigned int GetName()	const { return mName; }
		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }
		int GetChanCount() const { return mChanCount; }
		GLenum GetTarget() const { return mTarget; }

		static std::vector< Texture* > s_textures;
		char mStrName[1024];
		bool m_empty;

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
		bool InitFromFile( const char * relativePath );
		void PrefilterSpeculateProbe();

	private:
		void InitWithData( const unsigned char * data, const int face, const int height, const int chanCount );
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