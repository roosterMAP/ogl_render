#pragma once
#ifndef __DECL_H_INCLUDE__
#define __DECL_H_INCLUDE__

#include <map>

#include "Shader.h"
#include "Texture.h"
#include "String.h"

//in the material decl, a texture entry is a uniform name pointing to a type and a relative path
struct TextureSpecs {
	std::string type;
	std::string path;
};

class MaterialDecl;
typedef std::map< std::string, MaterialDecl* > resourceMap_t;
typedef std::map< std::string, TextureSpecs* > texturePathMap;
typedef std::map< std::string, Texture* > textureMap;

/*
================================
Decl
================================
*/
class Decl {
	public:
		std::string getName() { return name; }
		void setName( std::string s ) { name = s; }
		std::string getType() { return type; }
		void setType( std::string s ) { type = s; }

		virtual void DeleteAllDecls() {};

		Shader * shader;

	protected:
		std::string name;
		std::string type;
};

/*
================================
Material
================================
*/
class MaterialDecl : public Decl {
	public:
		MaterialDecl() { setType( "material" ); }
		~MaterialDecl() {};
		void Delete();
		void DeleteAllDecls() override;

		static MaterialDecl * GetMaterialDecl( const char * name );
		bool CompileShader();
		void BindTextures();

		std::string m_shaderProg;
		textureMap m_textures;		

		static resourceMap_t s_matDecls;

	private:
		static MaterialDecl * LoadMaterialDecl( const char * name );
		static bool LoadPathsFromFile( const char * decl_relative, std::string &shaderProg, texturePathMap & texturePaths );				
};

#endif