#include "Texture.h"
#include "Fileio.h"
#include "Framebuffer.h"
#include "Mesh.h"

#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define	BC7A_COMPRESSED	1
#define BC7_COMPRESSED	2
#define BC6H_COMPRESSED	3

struct bc_header {
	uint8_t type;
	uint8_t blockdim_x;
	uint8_t blockdim_y;
	unsigned int xsize;
	unsigned int ysize;
};

//static members initialized in glSetup()
unsigned int Texture::s_errorTexture = 0;
unsigned int CubemapTexture::s_errorCube = 0;

/*
===============================
Texture::InitErrorTexture2D
	-used to initialize static debug pink texture for error cases
===============================
*/
unsigned int Texture::InitErrorTexture() {
	const unsigned int pixCount = 4 * 4;
	unsigned char * errorData = new unsigned char[ pixCount * 3 ];
	for ( unsigned int i = 0; i < pixCount; i++ ) {
		errorData[ i * 3 + 0 ] = 128;
		errorData[ i * 3 + 1 ] = 128;
		errorData[ i * 3 + 2 ] = 255;
	}
	
	// Generate the texture
	unsigned int textureID;
	glGenTextures( 1, &textureID );

	// Bind this texture so that we can modify and set it up
	glBindTexture( GL_TEXTURE_2D, textureID );
	
	// Set the texture wrapping to clamp to edge
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	// Setup the filtering between texels
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, errorData );

	glBindTexture( 	GL_TEXTURE_2D, 0 );

	delete[] errorData;
	errorData = nullptr;
	return textureID;
}

/*
===============================
CubemapTexture::InitErrorCube
	-used to initialize static debug pink texture for error cases
===============================
*/
unsigned int CubemapTexture::InitErrorCube() {
	const unsigned int pixCount = 4 * 4;
	unsigned char * errorData = new unsigned char[ pixCount * 3 ];
	for ( unsigned int i = 0; i < pixCount; i++ ) {
		errorData[ i * 3 + 0 ] = 128;
		errorData[ i * 3 + 1 ] = 128;
		errorData[ i * 3 + 2 ] = 255;
	}

	//create and bind cubemap texture
	unsigned int cubemapTextureID;
	glGenTextures( 1, &cubemapTextureID );
	glBindTexture( GL_TEXTURE_CUBE_MAP, cubemapTextureID );

	//Set the texture wrapping to clamp to edge
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

	//Setup the filtering between texels
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	
	for ( unsigned int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, errorData );
	}

	//glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

	delete[] errorData;
	errorData = nullptr;
	return cubemapTextureID;
}

/*
===============================
Texture::Texture
===============================
*/
Texture::Texture() {
	mName = 0;
	mWidth = 0;
	mHeight = 0;
	mChanCount = 0;
	m_empty = true;
	m_compressed = false;
	mTarget = GL_TEXTURE_2D;
}

/*
===============================
Texture::Delete
	-delete texture from gpu
===============================
*/
void Texture::Delete() {
	if ( mName > 0 ) {
		if ( mName != s_errorTexture ) {
			glDeleteTextures( 1, &mName );
		}
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
	} else if ( strcmp( ext, "bc7" ) == 0 || strcmp( ext, "bc7a" ) == 0 ) {
		const GLsizei imgSize = ( mWidth / 4 ) * ( mHeight / 4 ) * 16; //total number of blocks at 16bytes per block
		glCompressedTexImage2D( GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, mWidth, mHeight, 0, imgSize, data );
	} else if ( strcmp( ext, "bc6h" ) == 0 ) {
		const GLsizei imgSize = ( mWidth / 4 ) * ( mHeight / 4 ) * 16; //total number of blocks at 16bytes per block
		glCompressedTexImage2D( GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, mWidth, mHeight, 0, imgSize, data );
	}

	//generate mipmaps
	glGenerateMipmap( GL_TEXTURE_2D );
		
	// Reset the bound texture to nothing
	glBindTexture( GL_TEXTURE_2D, 0 );
}

/*
===============================
Texture::InitFromFile_Uncompressed
===============================
*/
bool Texture::InitFromFile_Uncompressed( const char * relativePath ) {
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
		UseErrorTexture();
		printf( "Failed to load uncompressed texture: %s\n", relativePath );
		return false;
	}
	return true;
}

/*
===============================
Texture::InitFromFile
===============================
*/
bool Texture::InitFromFile( const char * relativePath ) {
	strcpy( mStrName, relativePath );

	//get relative path the generated bc file
	Str output_file_relative = Str( relativePath );
	output_file_relative.ReplaceChar( '/', '\\' );
	output_file_relative.Replace( "data\\texture\\", "data\\generated\\texture\\", false );
	if ( output_file_relative.EndsWith( "tga" ) ) {
		output_file_relative.Replace( ".tga", ".bc", false );
	} else if ( output_file_relative.EndsWith( "hdr" ) ) {
		output_file_relative.Replace( ".hdr", ".bc", false );
	} else {
		UseErrorTexture();
		return false;
	}

	//get absolute path
	char output_file_absolute[ 2048 ];
	RelativePathToFullPath( output_file_relative.c_str(), output_file_absolute );

	//open file
	FILE * fs = fopen( output_file_absolute, "rb" );
	if ( !fs ) {
		printf( "Failed to load uncompressed texture: %s\n", output_file_relative.c_str() );
		UseErrorTexture();
		return false;
	}	

	//load header data
	bc_header file_header;
	fread( &file_header, sizeof( bc_header ), 1, fs );	

	int chanCount;
	const unsigned int imgType = ( unsigned int )file_header.type;
	Str fileExtension;
	if ( imgType == BC6H_COMPRESSED ) {
		chanCount = 3;
		fileExtension = "bc6h";
	} else if ( imgType == BC7_COMPRESSED ) {
		chanCount = 3;
		fileExtension = "bc7";
	} else if ( imgType == BC7A_COMPRESSED ) {
		chanCount = 4;
		fileExtension = "bc7a";
	} else {
		printf( "Following texture had invalid header: %s\n", output_file_relative.c_str() );
		UseErrorTexture();
		return false;
	}
	const int width = ( int )file_header.xsize;
	const int height = ( int )file_header.ysize;	

	//load img data
	const size_t bytes_per_block = 16;
	const size_t ptr_size = ( width / ( int )file_header.blockdim_x ) * ( height / ( int )file_header.blockdim_y ) * ( int )bytes_per_block;
	unsigned char * data = new unsigned char[ptr_size];
	fread( data, ptr_size, 1, fs );
	InitWithData( data, width, height, chanCount, fileExtension.c_str() ); //pass to gpu
	delete[] data;
	data = nullptr;
	m_empty = false;

	return true;
}



/*
===============================
store_bc6h
	-write compressed img data to disc
===============================
*/
void store_bc6h( const rgba_surface * img, const unsigned int width, const unsigned int height, const char* filename ) {
    FILE * fs = fopen( filename, "wb" );

    bc_header file_header;
	file_header.type = BC6H_COMPRESSED;
    file_header.blockdim_x = 4;
    file_header.blockdim_y = 4;
	file_header.xsize = width;
	file_header.ysize = height;

    fwrite( &file_header, sizeof( bc_header ), 1, fs );

    const unsigned int bytes_per_block = 16;
    fwrite( img->ptr, img->width * img->height * bytes_per_block, 1, fs );

    fclose( fs );
}

/*
===============================
store_bc7
	-write compressed img data to disc
===============================
*/
void store_bc7( const rgba_surface * img, const unsigned int width, const unsigned int height, const unsigned int chanCount, const char* filename ) {
    FILE * fs = fopen( filename, "wb" );

    bc_header file_header;
	if ( chanCount == 3 ) {
		file_header.type = BC7_COMPRESSED;
	} else {
		file_header.type = BC7A_COMPRESSED;
	}
    file_header.blockdim_x = 4;
    file_header.blockdim_y = 4;
	file_header.xsize = width;
	file_header.ysize = height;

    fwrite( &file_header, sizeof( bc_header ), 1, fs );

    const unsigned int bytes_per_block = 16;
    fwrite( img->ptr, img->width * img->height * bytes_per_block, 1, fs );

    fclose( fs );
}

/*
===============================
Texture::CompressImage
===============================
*/
bool Texture::CompressFromFile( const char * relativePath ) {
	//get relative path the generated bc file
	Str output_file_relative = Str( relativePath );
	output_file_relative.ReplaceChar( '/', '\\' );
	output_file_relative.Replace( "data\\texture\\", "data\\generated\\texture\\", false );
	if ( output_file_relative.EndsWith( "tga" ) ) {
		output_file_relative.Replace( ".tga", ".bc", false );
	} else if ( output_file_relative.EndsWith( "hdr" ) ) {
		output_file_relative.Replace( ".hdr", ".bc", false );
	} else {
		return false;
	}

	//create windows folders
	int filename_idx = -1;
	for ( unsigned int i = output_file_relative.Length() - 1; i >= 0; i-- ) {
		if ( output_file_relative[i] == '\\' || output_file_relative[i] == '/' ) {
			filename_idx = ( int )i;
			break;
		}
	}	
	assert( filename_idx > -1 );
	Str output_dir = output_file_relative.Substring( 0, filename_idx );
	if ( dirExists( output_dir.c_str() ) == false ) {
		makeDir( output_dir.c_str() );
	}

	//get absolute path
	char output_file_absolute[ 2048 ];
	RelativePathToFullPath( output_file_relative.c_str(), output_file_absolute ); //get absolute path

	//get file extension
	const std::string fileExtension = GetExtension( relativePath );
	if( fileExtension.empty() ) {
		return false;
	}
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath, imgPath ); //get absolute path

	int width, height, chanCount;
	const unsigned int block_width = 4;
	const unsigned int block_height = 4;
	const size_t bytes_per_block = 16;
	stbi_set_flip_vertically_on_load( true );
	if ( fileExtension == "tga" ) { //bc7
		unsigned char * buffer_data = stbi_load( imgPath, &width, &height, &chanCount, 0 );
		if ( !buffer_data ) {
			return false;
		}
		if ( chanCount < 1 || width < 4 || height < 4 ) {
			stbi_image_free( buffer_data );
			return false;
		}

		rgba_surface input_tex;
		input_tex.width = width;
		input_tex.height = height;
		input_tex.stride = width * sizeof( unsigned char ) * 4; //LDR input is 32 bit/pixel (sRGB)
		input_tex.ptr = buffer_data;
		unsigned char * data = NULL;

		rgba_surface output_tex;
		const int width_blockCount = width / 4;
		const int height_blockCount = height / 4;
		output_tex.width = width_blockCount;
		output_tex.height = height_blockCount;
		output_tex.stride = width_blockCount * bytes_per_block;
		output_tex.ptr = new uint8_t[ width_blockCount * height_blockCount * bytes_per_block ];

		//compress
		bc7_enc_settings settings;
		if ( chanCount == 1 ) {
			data = new unsigned char[ width * height * 4 ];
			for ( unsigned int i = 0; i < width * height; i++ ) {
				data[ i * 4 + 0 ] = buffer_data[ i ];
				data[ i * 4 + 1 ] = buffer_data[ i ];
				data[ i * 4 + 2 ] = buffer_data[ i ];
				data[ i * 4 + 3 ] = 0;
			}
			input_tex.ptr = data;
			chanCount = 4;
			GetProfile_alpha_basic( &settings );
		} else if ( chanCount == 2 ) {
			data = new unsigned char[ width * height * 4 ];
			for ( unsigned int i = 0; i < width * height; i++ ) {
				data[ i * 4 + 0 ] = buffer_data[ i * 2 + 0 ];
				data[ i * 4 + 1 ] = buffer_data[ i * 2 + 1 ];
				data[ i * 4 + 2 ] = 0;
				data[ i * 4 + 3 ] = 0;
			}
			input_tex.ptr = data;
			chanCount = 4;
			GetProfile_alpha_basic( &settings );
		} else if ( chanCount == 3 ) {
			data = new unsigned char[ width * height * 4 ];
			for ( unsigned int i = 0; i < width * height; i++ ) {
				data[ i * 4 + 0 ] = buffer_data[ i * 3 + 0 ];
				data[ i * 4 + 1 ] = buffer_data[ i * 3 + 1 ];
				data[ i * 4 + 2 ] = buffer_data[ i * 3 + 2 ];
				data[ i * 4 + 3 ] = 0;
			}
			input_tex.ptr = data;
			chanCount = 4;
			GetProfile_alpha_basic( &settings );
		} else if ( chanCount == 4 ) {
			GetProfile_alpha_basic( &settings );				
		} else {
			delete[] output_tex.ptr;
			output_tex.ptr = nullptr;
			printf( "ERROR :: BC7 image must have 3 or 4 channels!!!\n" );
			stbi_image_free( buffer_data );
			return false;
		}
		CompressBlocksBC7( &input_tex,  output_tex.ptr, &settings );

		store_bc7( &output_tex, width, height, chanCount, output_file_absolute );
		delete[] output_tex.ptr;
		output_tex.ptr = nullptr;
		stbi_image_free( buffer_data );
		if ( data != NULL ) {
			delete[] data;
			data = nullptr;
		}

	} else if ( fileExtension == "hdr" ) { //bc6h
		float * data = stbi_loadf( imgPath, &width, &height, &chanCount, 0 );
		if ( !data ) {
			return false;
		}
		if ( chanCount != 3 || width < 4 || height < 4 ) {
			stbi_image_free( data );
			return false;
		}

		//hdr vals for bc6h uses 2 bytes. So we convert each float in data to a short.
		//we also need to have each pixel have 8 bytes (two per value). So we convert to rgba with a==1.
		rgba_surface input_tex;
		input_tex.width = width;
		input_tex.height = height;
		input_tex.stride = width * sizeof( unsigned short ) * 4; //HDR is 64 bit/pixel (half float), thus chanCount for output must be 4
		const unsigned int valCount = width * height * chanCount;
		unsigned short * data_short = new unsigned short [ width * height * 4 ];
		for ( unsigned int i = 0; i < width * height; i++ ) {
			for ( unsigned int j = 0; j < chanCount; j++ ) {
				data_short[ i * 4 + j ] = F32toF16( data[ i * 3 + j ] );
			}
			data_short[ i * 4 + 3 ] = 1; //fill alpha channel
		}
		input_tex.ptr = ( uint8_t * )data_short;
		stbi_image_free( data );

		//flip the image vertically
		input_tex.ptr += ( input_tex.height - 1 ) * input_tex.stride;
		input_tex.stride *= -1;

		rgba_surface output_tex;
		const int width_blockCount = width / 4;
		const int height_blockCount = height / 4;
		output_tex.width = width_blockCount;
		output_tex.height = height_blockCount;
		output_tex.stride = width_blockCount * bytes_per_block;
		output_tex.ptr = new uint8_t[ width_blockCount * height_blockCount * bytes_per_block ];

		//compress
		bc6h_enc_settings settings;
		GetProfile_bc6h_basic( &settings );
		CompressBlocksBC6H( &input_tex,  output_tex.ptr, &settings );

		store_bc6h( &output_tex, width, height, output_file_absolute );
		delete[] output_tex.ptr;
		output_tex.ptr = nullptr;
	}				

	return true;
}

/*
===============================
Texture::Error
	-Set texture to use s_errorTexture as mName
===============================
*/
void Texture::UseErrorTexture() {
	mName = s_errorTexture;
	mWidth = 4;
	mHeight = 4;
	mChanCount = 3;
	mTarget = GL_TEXTURE_2D;
}

/*
===============================
CubemapTexture::CubemapTexture
===============================
*/
CubemapTexture::CubemapTexture() {
	mName = 0;
	mWidth = 0;
	mHeight = 0;
	mChanCount = 0;
	m_empty = true;
	m_compressed = false;
	mTarget = GL_TEXTURE_CUBE_MAP;
}

/*
===============================
CubemapTexture::Delete
	-delete texture from gpu
===============================
*/
void CubemapTexture::Delete() {
	if ( mName > 0 ) {
		if ( mName != s_errorCube ) {
			glDeleteTextures( 1, &mName );
		}
		mName = 0;
	}
}

/*
===============================
CubemapTexture::InitWithData
===============================
*/
void CubemapTexture::InitWithData( const void * data, const int face, const int height, const int chanCount, const char * ext ) {
	if ( strcmp( ext, "tga" ) == 0 ) {
		//Map a face of the cube
		unsigned char * src = ( unsigned char * )data;
		unsigned char * face_data = new unsigned char[ height * height * chanCount ];
		for ( unsigned int i = 0; i < height; i++ ) {
			for ( unsigned int j = 0; j < height; j++ ) {
				unsigned int k = ( unsigned int )height * ( unsigned int )face + j;
				for ( unsigned int n = 0; n < chanCount; n++ ) {
					face_data[ ( height * i + j ) * chanCount + n ] = src[ ( height * 6 * i + k ) * chanCount + n ];
				}
			}
		}
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
		delete[] face_data;
		face_data = nullptr;

	} else if ( strcmp( ext, "hdr" ) == 0 ) {
		//Map a face of the cube
		float * src = ( float * )data;
		float * face_data = new float[ height * height * chanCount ];
		for ( unsigned int i = 0; i < height; i++ ) {
			for ( unsigned int j = 0; j < height; j++ ) {
				unsigned int k = ( unsigned int )height * ( unsigned int )face + j;
				for ( unsigned int n = 0; n < chanCount; n++ ) {
					face_data[ ( height * i + j ) * chanCount + n ] = src[ ( height * 6 * i + k ) * chanCount + n ];
				}
			}
		}
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F, height, height, 0, GL_RGB, GL_FLOAT, face_data );
		delete[] face_data;
		face_data = nullptr;

	} else if ( strcmp( ext, "bc7" ) == 0 || strcmp( ext, "bc7a" ) == 0 ) {
		//Map a face of the cube
		const unsigned int bytesPerBlock = 16;
		const unsigned int height_inBlocks = height / 4;
		unsigned char * src = ( unsigned char * )data;
		unsigned char * face_data = new unsigned char[ height_inBlocks * height_inBlocks * bytesPerBlock ];
		for ( unsigned int i = 0; i < height_inBlocks; i++ ) {
			for ( unsigned int j = 0; j < height_inBlocks; j++ ) {
				unsigned int k = height_inBlocks * ( unsigned int )face + j;
				for ( unsigned int n = 0; n < bytesPerBlock; n++ ) {
					face_data[ ( height_inBlocks * i + j ) * bytesPerBlock + n ] = src[ ( height_inBlocks * 6 * i + k ) * bytesPerBlock + n ];
				}
			}
		}
		const GLsizei imgSize = height_inBlocks * height_inBlocks * bytesPerBlock; //total number of blocks at 16bytes per block for this face
		glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, height, height, 0, imgSize, face_data );
		delete[] face_data;
		face_data = nullptr;

	} else if ( strcmp( ext, "bc6h" ) == 0 ) {
		//Map a face of the cube
		const unsigned int bytesPerBlock = 16;
		const unsigned int height_inBlocks = height / 4;
		unsigned char * src = ( unsigned char * )data;
		unsigned char * face_data = new unsigned char[ height_inBlocks * height_inBlocks * bytesPerBlock ];
		for ( unsigned int i = 0; i < height_inBlocks; i++ ) {
			for ( unsigned int j = 0; j < height_inBlocks; j++ ) {
				unsigned int k = height_inBlocks * ( unsigned int )face + j;
				for ( unsigned int n = 0; n < bytesPerBlock; n++ ) {
					face_data[ ( height_inBlocks * i + j ) * bytesPerBlock + n ] = src[ ( height_inBlocks * 6 * i + k ) * bytesPerBlock + n ];
				}
			}
		}
		const GLsizei imgSize = height_inBlocks * height_inBlocks * bytesPerBlock; //total number of blocks at 16bytes per block for this face
		glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, height, height, 0, imgSize, face_data );
		delete[] face_data;
		face_data = nullptr;
	}
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
	face_data = nullptr;
}

/*
===============================
CubemapTexture::InitFromFile
===============================
*/
bool CubemapTexture::InitFromFile_Uncompressed( const char * relativePath ) {
	strcpy( mStrName, relativePath ); //initialize mStrName

	//get file extension
	const std::string fileExtension = GetExtension( relativePath );
	if( fileExtension.empty() ) {
		UseErrorTexture();
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

	unsigned char * ldr_data = NULL;
	float * hdr_data = NULL;
	bool isHDR = false;
	if ( strcmp( fileExtension.c_str(), "tga" ) == 0 ) {
		ldr_data = stbi_load( imgPath, &width, &height, &chanCount, 0 );
	} else if ( strcmp( fileExtension.c_str(), "hdr" ) == 0 ) {
		hdr_data = stbi_loadf( imgPath, &width, &height, &chanCount, 0 );
		isHDR = true;
	} else {
		UseErrorTexture();
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}	

	if ( ldr_data || hdr_data ) {
		if ( width < 0 || height < 0 ) {
			UseErrorTexture();
			printf( "Failed to load texture: %s\n", relativePath );
			return false;
		}
		if ( height * 6 != width ) {
			UseErrorTexture();
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
				InitWithData( hdr_data, face, height, chanCount, fileExtension.c_str() );
			} else {
				InitWithData( ldr_data, face, height, chanCount, fileExtension.c_str() );
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
		UseErrorTexture();
		printf( "Failed to load texture: %s\n", relativePath );
		return false;
	}

	return true;
}

/*
===============================
CubemapTexture::InitFromFile
===============================
*/
bool CubemapTexture::InitFromFile( const char * relativePath ) {
	strcpy( mStrName, relativePath ); //initialize mStrName

	//get relative path the generated bc file
	Str output_file_relative = Str( relativePath );
	output_file_relative.ReplaceChar( '/', '\\' );
	output_file_relative.Replace( "data\\texture\\", "data\\generated\\texture\\", false );
	if ( output_file_relative.EndsWith( "tga" ) ) {
		output_file_relative.Replace( ".tga", ".bc", false );
	} else if ( output_file_relative.EndsWith( "hdr" ) ) {
		output_file_relative.Replace( ".hdr", ".bc", false );
	} else {
		UseErrorTexture();
		return false;
	}

	//get absolute path
	char output_file_absolute[ 2048 ];
	RelativePathToFullPath( output_file_relative.c_str(), output_file_absolute );

	//open file
	FILE * fs = fopen( output_file_absolute, "rb" );
	if ( !fs ) {
		printf( "Failed to load texture: %s\n", output_file_relative.c_str() );
		UseErrorTexture();
		return false;
	}	

	//load header data
	bc_header file_header;
	fread( &file_header, sizeof( bc_header ), 1, fs );	

	int chanCount;
	const unsigned int imgType = ( unsigned int )file_header.type;
	Str fileExtension;
	if ( imgType == BC6H_COMPRESSED ) {
		chanCount = 3;
		fileExtension = "bc6h";
	} else if ( imgType == BC7_COMPRESSED ) {
		chanCount = 3;
		fileExtension = "bc7";
	} else if ( imgType == BC7A_COMPRESSED ) {
		chanCount = 4;
		fileExtension = "bc7a";
	} else {
		printf( "Following texture had invalid header: %s\n", output_file_relative.c_str() );
		UseErrorTexture();
		return false;
	}
	const int width = ( int )file_header.xsize;
	const int height = ( int )file_header.ysize;

	if ( height < 4 ) {
		UseErrorTexture();
		printf( "Cubemap width must be 4 or larger: %s\n", relativePath );
		return false;
	}
	if ( height * 6 != width ) {
		UseErrorTexture();
		printf( "Failed to load texture: %s\n\tIncorrect dimensions!!!\n", relativePath );
		return false;
	}

	//load img data
	const size_t bytes_per_block = 16;
	const size_t ptr_size = ( width / ( int )file_header.blockdim_x ) * ( height / ( int )file_header.blockdim_y ) * ( int )bytes_per_block;
	unsigned char * data = new unsigned char[ptr_size];
	fread( data, ptr_size, 1, fs );

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
		InitWithData( data, face, height, chanCount, fileExtension.c_str() );
	}

	// Reset the bound texture to nothing
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

	delete[] data;
	data = nullptr;
	m_empty = false;

	return true;
}

/*
===============================
CubemapTexture::Error
	-Set texture to use s_errorTexture as mName
===============================
*/
void CubemapTexture::UseErrorTexture() {
	mName = s_errorCube;
	mWidth = 4 * 6;
	mHeight = 4;
	mChanCount = 3;
	mTarget = GL_TEXTURE_CUBE_MAP;
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
		unsigned int mipDimension = cubeSize * pow( 0.5, mip );
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
	if ( mName != s_errorCube ) {
		glDeleteTextures( 1, &mName );
	}

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
		cubemapFace_data = nullptr;
		delete[] output_data;
		output_data = nullptr;
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