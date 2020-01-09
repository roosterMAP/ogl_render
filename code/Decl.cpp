#include "Decl.h"
#include "Fileio.h"

//#include <stdio.h>
//#include <sstream>
//#include <iostream>
//#include <iterator>
//#include <fstream>

//initialize static members
resourceMap_t MaterialDecl::s_matDecls;
std::vector< Texture* > Texture::s_textures;

/*
====================================
MaterialDecl::LoadPathsFromFile
====================================
*/
bool MaterialDecl::LoadPathsFromFile( const char * decl_relative, std::string & shaderProg, texturePathMap & texturePaths ) {
	char decl_absolute[ 2048 ];
	RelativePathToFullPath( decl_relative, decl_absolute );
	strcat( decl_absolute, ".txt" );
	
	FILE * fp = fopen( decl_absolute, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", decl_absolute );
		return false;
    }

	char buff[ 512 ] = { 0 };
	char strBuffer1[ 1024 ];
	char strBuffer2[ 1024 ];
	char strBuffer3[ 1024 ];
	std::string uniformName;
	std::string textureType;
	std::string texturePath;
	while ( !feof( fp ) ) {
		fgets( buff, sizeof( buff ), fp );
		
		texturePath.clear();

		if ( sscanf( buff, "shaderProg :: %s", strBuffer1 ) == 1 ) { //identifies what shader to compile
			shaderProg = strBuffer1;
			findAndReplaceAll( shaderProg, "\r\n", "" );

		} else if ( sscanf( buff, "framebuffer %s", strBuffer1 ) == 1 ) { //identifies a uniform as a framebuffer
			//when framebuffer keyword is detected, then the texture will later be fetched from a framebuffer of the same name.
			uniformName = strBuffer1;
			findAndReplaceAll( uniformName, "\r\n", "" );
			TextureSpecs * newTextureSpec = new TextureSpecs();
			newTextureSpec->type = "framebuffer";
			texturePaths.insert( std::make_pair( uniformName, newTextureSpec ) ); //add to texture resource

		} else if ( sscanf( buff, "%s %s %s", strBuffer1, strBuffer2, strBuffer3 ) == 3 ) { //associates a uniform name with a texture
			uniformName = strBuffer1;
			findAndReplaceAll( uniformName, "\r\n", "" );

			textureType = strBuffer2;
			findAndReplaceAll( textureType, "\r\n", "" );

			texturePath = strBuffer3;
			findAndReplaceAll( texturePath, "\r\n", "" );

			TextureSpecs * newTextureSpec = new TextureSpecs();
			newTextureSpec->type = textureType;
			newTextureSpec->path = texturePath;

			texturePaths.insert( std::make_pair( uniformName, newTextureSpec ) ); //add to texture resource
		}
	}
	fclose( fp );

	return true;
}

/*
====================================
MaterialDecl::GetMaterialDecl
====================================
*/
MaterialDecl* MaterialDecl::GetMaterialDecl( const char * name ) {
    // Search for pre-loaded material and return if found
    resourceMap_t::iterator it = s_matDecls.find( name );
    if ( it != s_matDecls.end() ) {
        return it->second;
    }

    // Couldn't find pre-loaded material, so load from file
	MaterialDecl * newDecl = LoadMaterialDecl( name );
    if ( NULL != newDecl ) {
		s_matDecls[name] = newDecl;
		return newDecl;
	}

	return NULL;
}

/*
====================================
MaterialDecl::LoadMaterialDecl
====================================
*/
MaterialDecl * MaterialDecl::LoadMaterialDecl( const char * name ) {
	char shaderDir[128];
	strcpy( shaderDir, "data\\decl\\" );
	strcat( shaderDir, name );

	std::string shaderProg;
	texturePathMap texturePaths;
	textureMap textures;

	// try to load decl, return false if failed
    if ( !LoadPathsFromFile( shaderDir, shaderProg, texturePaths ) ) {
		return NULL;
	}

	texturePathMap::iterator it = texturePaths.begin();
	while ( it != texturePaths.end() ) {
		std::string uniformName = it->first;
		TextureSpecs* textureEntry = it->second;

		Texture * newTexture = new Texture; //create in advance because access to Texture.s_textures is needed

		//check if texture being added already exists in the static member.
		int textureIndex = -1; //used to identify the index of the texture in the static texture resource
		for ( unsigned int j = 0; j < newTexture->s_textures.size(); j++ ) {
			if( strcmp( newTexture->s_textures[j]->mStrName, textureEntry->path.c_str() ) == 0 ) {
				textureIndex = j;
				break;
			}
		}

		//add the texture.
		if ( textureIndex < 0 ) {
			//texture doesnt exist. Load it and add it to static texture list.
			if ( textureEntry->type == "texture2d" ) {
				if ( newTexture->InitFromFile( textureEntry->path.c_str() ) ) {
					newTexture->s_textures.push_back( newTexture );
					textures[uniformName] = newTexture; //add texture to resource
				} else {
					delete newTexture;
					return NULL;
				}
			} else if ( textureEntry->type == "cubemap" ) {
				delete newTexture; //since we need a cubemap, delete newTexture and recreate it as a cubemap obj
				CubemapTexture * newCubemapTexture = new CubemapTexture;

				if ( newCubemapTexture->InitFromFile( textureEntry->path.c_str() ) ) {
					newCubemapTexture->s_textures.push_back( newCubemapTexture );
					textures[uniformName] = newCubemapTexture; //add texture to resource
				} else {
					delete newCubemapTexture;
					return NULL;
				}
			}

		} else {
			//texture already exists. Fetch it to add to decl texture list
			newTexture = newTexture->s_textures[textureIndex];
			textures[uniformName] = newTexture; //add texture to resource
		}

		it++;
	}

	//create and initialize the data for THIS decl obj
	MaterialDecl * decl = new MaterialDecl;
	decl->setName( name );
	decl->m_shaderProg = shaderProg;
	decl->m_textures = textures;
	return decl;
}

/*
====================================
MaterialDecl::CompileShaders
====================================
*/
bool MaterialDecl::CompileShader() {
	shader = shader->GetShader( m_shaderProg.c_str() );
	if ( shader != NULL ) {
		return true;
	}
	return false;
}

/*
====================================
MaterialDecl::BindTextures
====================================
*/
void MaterialDecl::BindTextures() {
	shader->UseProgram();

	unsigned int slotCount = 0;
	textureMap::iterator it = m_textures.begin();
	while ( it != m_textures.end() ) {
		std::string uniformName = it->first;
		Texture* texture = it->second;
		shader->SetAndBindUniformTexture( uniformName.c_str(), slotCount, texture->GetTarget(), texture->GetName() );

		slotCount++;
		it++;
	}
}