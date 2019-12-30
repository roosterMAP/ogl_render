#include "Texture.h"
#include "Fileio.h"
#include "Framebuffer.h"
#include "Mesh.h"

#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

/*
===============================
Texture::Texture
===============================
*/
Texture::Texture() :
mName( 0 ),
mWidth( 0 ),
mHeight( 0 ),
mChanCount( 0 ) {
	m_empty = true;
	mTarget = GL_TEXTURE_2D;
}

/*
===============================
Texture::~Texture
===============================
*/
Texture::~Texture() {
	if ( mName > 0 ) {
		glDeleteTextures( 1, &mName );
		mName = 0;
	}
}

/*
===============================
Texture::InitWithData
===============================
*/
void Texture::InitWithData( const unsigned char * data, const int width, const int height, const int chanCount, const char * ext ) {
	// Store the width and height of the texture
	mWidth = width;
	mHeight = height;
	mChanCount = chanCount;

	// Generate the texture
	glGenTextures( 1, &mName );

	// Bind this texture so that we can modify and set it up
	glBindTexture( GL_TEXTURE_2D, mName );
	
	// Set the texture wrapping to clamp to edge
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	// Setup the filtering between texels
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if ( strcmp( ext, "tga" ) == 0 ) {
		assert( chanCount > 0 && chanCount <= 4 );
		if( chanCount == 1 ) {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, mWidth, mHeight, 0, GL_RED, GL_UNSIGNED_BYTE, data );
		} else if ( chanCount == 2 ) {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RG8, mWidth, mHeight, 0, GL_RG, GL_UNSIGNED_BYTE, data );
		} else if ( chanCount == 3 ) {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
		} else {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	} else if ( strcmp( ext, "hdr" ) == 0 ) {
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
	}

	//generate mipmaps
	glGenerateMipmap( GL_TEXTURE_2D );
		
	// Reset the bound texture to nothing
	glBindTexture( GL_TEXTURE_2D, 0 );
}

/*
===============================
Texture::InitFromFile
===============================
*/
bool Texture::InitFromFile( const char * relativePath ) {
	strcpy( mStrName, relativePath );

	//get file extension
	const std::string fileExtension = GetExtension( relativePath );
	if( fileExtension.empty() ) {
		return false;
	}

	//get absolute path
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath, imgPath );

	//read data from image file and pass to GPU
	int width, height, chanCount;
	stbi_set_flip_vertically_on_load( true ); //image needs to be flipped because OpenGL expects the 0.0 coordinate on the y-axis to be on the bottom
	unsigned char *data = stbi_load( imgPath, &width, &height, &chanCount, 0 );
	if ( data ) {
		InitWithData( data, width, height, chanCount, fileExtension.c_str() ); //pass to gpu
		stbi_image_free( data );
		m_empty = false;
	} else {
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}
	return true;
}

/*
===============================
CubemapTexture::InitWithData
===============================
*/
void CubemapTexture::InitWithData( const unsigned char * data, const int face, const int height, const int chanCount ) {
	//Map a face of the cube
	unsigned char * face_data = new unsigned char[ height * height * chanCount ];
	for ( unsigned int i = 0; i < height; i++ ) {
		for ( unsigned int j = 0; j < height; j++ ) {
			unsigned int k = ( unsigned int )height * ( unsigned int )face + j;
			for ( unsigned int n = 0; n < chanCount; n++ ) {
				face_data[ ( height * i + j ) * chanCount + n ] = data[ ( height * 6 * i + k ) * chanCount + n ];
			}
		}
	}

	//pass to gpu
	assert( chanCount > 0 && chanCount <= 4 );
	if( chanCount == 1 ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_R8, height, height, 0, GL_RED, GL_UNSIGNED_BYTE, face_data );
	} else if ( chanCount == 2 ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RG8, height, height, 0, GL_RG, GL_UNSIGNED_BYTE, face_data );
	} else if ( chanCount == 3 ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8, height, height, 0, GL_RGB, GL_UNSIGNED_BYTE, face_data );
	} else {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, height, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_data );
	}

	delete[] face_data; //delete
}

/*
===============================
CubemapTexture::InitWithData
===============================
*/
void CubemapTexture::InitWithData( const float * data, const int face, const int height, const int chanCount ) {
	//Map a face of the cube
	float * face_data = new float[ height * height * chanCount ];
	for ( unsigned int i = 0; i < height; i++ ) {
		for ( unsigned int j = 0; j < height; j++ ) {
			unsigned int k = ( unsigned int )height * ( unsigned int )face + j;
			for ( unsigned int n = 0; n < chanCount; n++ ) {
				face_data[ ( height * i + j ) * chanCount + n ] = data[ ( height * 6 * i + k ) * chanCount + n ];
			}
		}
	}

	//pass to gpu
	glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F, height, height, 0, GL_RGB, GL_FLOAT, face_data );

	delete[] face_data; //delete
}

/*
===============================
CubemapTexture::InitFromFile
===============================
*/
bool CubemapTexture::InitFromFile( const char * relativePath ) {
	strcpy( mStrName, relativePath ); //initialize mStrName

	//get file extension
	const std::string fileExtension = GetExtension( relativePath );
	if( fileExtension.empty() ) {
		return false;
	}

	/*
	relativePaths[0] = relativePath_ft;
	relativePaths[1] = relativePath_bk;
	relativePaths[2] = relativePath_up;
	relativePaths[3] = relativePath_dn;
	relativePaths[4] = relativePath_rt;
	relativePaths[5] = relativePath_lf;
	*/

	//get absolute path
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath, imgPath );

	//read data from image file and pass currently bound cubemap
	int width, height, chanCount;
	stbi_set_flip_vertically_on_load( false );

	unsigned char * ldr_data;
	float * hdr_data;
	bool isHDR = false;
	if ( strcmp( fileExtension.c_str(), "tga" ) == 0 ) {
		ldr_data = stbi_load( imgPath, &width, &height, &chanCount, 0 );
	} else if ( strcmp( fileExtension.c_str(), "hdr" ) == 0 ) {
		hdr_data = stbi_loadf( imgPath, &width, &height, &chanCount, 0 );
		isHDR = true;
	} else {
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}	

	if ( ldr_data || hdr_data ) {
		if (width < 0 || height < 0) {
			printf( "Failed to load texture: %s\n", relativePath );
			return false;
		}
		if ( height * 6 != width ) {
			printf( "Failed to load texture: %s\n\tIncorrect dimensions!!!\n", relativePath );
			return false;
		}

		mWidth = width;
		mHeight = height;
		mChanCount = chanCount;
		mTarget = GL_TEXTURE_CUBE_MAP;

		//create and bind cubemap texture
		glGenTextures( 1, &mName );
		glBindTexture( GL_TEXTURE_CUBE_MAP, mName );

		//Set the texture wrapping to clamp to edge
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

		//Setup the filtering between texels
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

		//pass to gpu
		for ( int face = 0; face < 6; face++ ) {
			if ( isHDR ) {
				InitWithData( hdr_data, face, height, chanCount );
			} else {
				InitWithData( ldr_data, face, height, chanCount );
			}
		}

		// Reset the bound texture to nothing
		glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

		if ( isHDR ) {
			stbi_image_free( hdr_data );
		} else {
			stbi_image_free( ldr_data );
		}
		m_empty = false;
	} else {
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}

	return true;
}

/*
===============================
CubemapTexture::PrefilterSpeculateProbe
===============================
*/
void CubemapTexture::PrefilterSpeculateProbe() {
	glBindTexture( GL_TEXTURE_CUBE_MAP, mName );

	//set new filtering between texels because we plan on samplint the textures mipmaps and have trilinear filtering
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ); 
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glGenerateMipmap( GL_TEXTURE_CUBE_MAP ); //allocate enough memory for the mip levels
	
	//create framebuffer whose color attachement we are drawing to
	Framebuffer prefilterFBO( "cubemap" );
	const unsigned int cubeSize = 128;
	prefilterFBO.CreateNewBuffer( cubeSize, cubeSize, "cubemap" );
    prefilterFBO.AttachCubeMapTextureBuffer( GL_RGB16F, GL_COLOR_ATTACHMENT0, GL_RGB, GL_FLOAT );
	if ( !prefilterFBO.Status() ) {
		assert( false );
	}

	//specify storage for all mips of cubemap color attachement of prefilterFBO
	const unsigned int maxMipLevelCount = 5;
	glBindTexture( GL_TEXTURE_CUBE_MAP, prefilterFBO.m_attachements[0] );
	glTexStorage2D( GL_TEXTURE_CUBE_MAP, maxMipLevelCount, GL_RGB16F, cubeSize, cubeSize ); //allocate the memory for maxMipLevelCount mips
	glGenerateMipmap( GL_TEXTURE_CUBE_MAP ); //generate the mip data
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ); //required to be able to sample specific mips in the frag shader
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		
	//compile prefilter shader and pass uniforms
	Shader * shaderProg = new Shader();
	shaderProg = shaderProg->GetShader( "prefilter_envMap" );
	shaderProg->UseProgram();
	shaderProg->SetAndBindUniformTexture( "environmentMap", 0, GL_TEXTURE_CUBE_MAP, mName );
	Mat4 projection = Mat4();
	projection.Perspective( to_radians( 90.0f ), 1.0f, 0.1f, 10.0f );
	shaderProg->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );

	//create cube geo and views to render cubemap to
	Cube renderBox = Cube( "" );
	Mat4 views[] = { Mat4(), Mat4(), Mat4(), Mat4(), Mat4(), Mat4() };
	views[0].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[1].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( -1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[2].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 1.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ) );
	views[3].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ) );
	views[4].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[5].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );

	//draw to prefilterFBO by convoluting each face of env cubemap for each mip level
	prefilterFBO.Bind();
	shaderProg->UseProgram();
	for ( unsigned int mip = 0; mip < maxMipLevelCount; mip++ ) {
		unsigned int mipDimension = cubeSize * std::pow( 0.5, mip );
		glViewport( 0, 0, mipDimension, mipDimension );

		const float roughness = ( float )mip / ( float )( maxMipLevelCount - 1 );
		shaderProg->SetUniform1f( "roughness", 1, &roughness );

		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			shaderProg->SetUniformMatrix4f( "view", 1, false, views[faceIdx].as_ptr() );
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, prefilterFBO.m_attachements[0], mip );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			renderBox.DrawSurface( true );
		}
	}
	prefilterFBO.Unbind();

	//delete old cubemap texture
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
	glDeleteTextures( 1, &mName );

	//replace the current cubemap with the FBO color attachement
	mName = prefilterFBO.m_attachements[0];
	mHeight = cubeSize;
	mWidth = cubeSize * 6;

	//DEBUG: SAVE OUT prefiltered env and its mips
	/*
	char imgPath[ 2048 ];
	if ( RelativePathToFullPath( mStrName, imgPath ) == false ) {
		assert( false );
	}
	Str prefilteredMap_absolute = Str( imgPath );

	for ( unsigned int mip = 0; mip < maxMipLevelCount; mip++ ) {
		unsigned int mipDimension = cubeSize * std::pow( 0.5, mip );

		//copy prefiltered env texture data from gpu to cpu
		float * output_data = new float[ mipDimension * 6 * mipDimension * mChanCount ];
		float * cubemapFace_data = new float[ mipDimension * mipDimension * mChanCount ];
		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			//copy from gpu to cubemapFace_data
			glBindTexture( GL_TEXTURE_CUBE_MAP, prefilterFBO.m_attachements[0] );
			glGetTexImage( GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, mip, GL_RGB,  GL_FLOAT, cubemapFace_data );

			//write from cubemapFace_data to output_data
			for ( int i = 0; i < mipDimension; i++ ) {
				for ( int j = 0; j < mipDimension; j++ ) {
					const int k = faceIdx * mipDimension + j;
					const int output_idx = ( i * mipDimension * 6 * mChanCount ) + ( k * mChanCount );
					const int cubemapFace_idx = ( i * mipDimension * mChanCount ) + ( j * mChanCount );
					output_data[ output_idx + 0 ] = cubemapFace_data[ cubemapFace_idx + 0 ];
					output_data[ output_idx + 1 ] = cubemapFace_data[ cubemapFace_idx + 1 ];
					output_data[ output_idx + 2 ] = cubemapFace_data[ cubemapFace_idx + 2 ];
				}
			}			
		}

		//generate output image path
		prefilteredMap_absolute = prefilteredMap_absolute.Substring( 0, prefilteredMap_absolute.Length() - 9 );
		prefilteredMap_absolute += "_env";
		prefilteredMap_absolute += std::to_string( mip ).c_str();
		prefilteredMap_absolute += ".hdr";
		stbi_write_hdr( prefilteredMap_absolute.c_str(), mipDimension * 6, mipDimension, mChanCount, output_data ); //save image
		delete[] cubemapFace_data;
		delete[] output_data;
	}
	*/
}

/*
===============================
LUTTexture::InitFromFile
===============================
*/
bool LUTTexture::InitFromFile( const char * relativePath ) {
	strcpy( mStrName, relativePath );

	//get file extension
	const std::string fileExtension = GetExtension( relativePath );
	if( fileExtension.empty() ) {
		return false;
	}

	//get absolute path
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath, imgPath );

	//read data from image file
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load( false );
	unsigned char *imgData = stbi_load( imgPath, &width, &height, &nrChannels, 0 );

	if ( imgData ) {
		assert( width == 256 && height == 16 );

		//load image data into a GL_TEXTURE_3D array
		unsigned char data[16][16][16][3];
		for ( unsigned int i = 0; i < 16; i++ ) {
			for ( unsigned int j = 0; j < 16; j++ ) {
				for ( unsigned int k = 0; k < 16; k++ ) {
					data[j][k][i][0] = imgData[ ( i * 256 + j * 16 + k ) * 3 + 0 ];
					data[j][k][i][1] = imgData[ ( i * 256 + j * 16 + k ) * 3 + 1 ];
					data[j][k][i][2] = imgData[ ( i * 256 + j * 16 + k ) * 3 + 2 ];
				}
			}
		}

		// Store the width and height of the texture
		mWidth = 16;
		mHeight = 16;
		mDepth = 16;
		mChanCount = nrChannels;

		// Generate the texture
		glGenTextures( 1, &mName );

		// Bind this texture so that we can modify and set it up
		glBindTexture( GL_TEXTURE_3D, mName );
	
		// Set the texture wrapping to clamp to edge
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

		// Setup the filtering between texels
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

		glBindTexture( GL_TEXTURE_3D, mName );
		glTexImage3D( GL_TEXTURE_3D, 0, GL_RGB8, 16, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

		stbi_image_free( imgData );
		m_empty = false;
	} else {
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}
	return true;
}