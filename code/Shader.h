#pragma once
#ifndef __SHADER_H_INCLUDE__
#define __SHADER_H_INCLUDE__

#include <stdio.h>
#include <map>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GL/freeglut.h>

class Shader;

typedef std::map< std::string, Shader * > resourceMap_s;

/*
==============================
Shader
==============================
*/
class Shader {
public:
	Shader();
	~Shader() {};
	
	Shader * GetShader( const char *sprefix );
	Shader * LoadShader( const char *sprefix );

	bool CompileShaderFromCSTR( const char * vshaderSource, const char * fshaderSource );
	bool CompileShaderFromCSTR( const char * vshaderSource, const char *gshaderFile, const char * fshaderSource );
	GLuint GetShaderProgram() const { return mShaderProgram; }
	void UseProgram();
	bool IsCurrentProgram() const;

	void DeleteProgram();
	static void DeleteAllPrograms();

	GLuint GetUniform( const char * name );
	int GetAttribute( const char * name );

	void SetUniform1i( const char * uniform, const int count, const int * values );
	void SetUniform2i( const char * uniform, const int count, const int * values );
	void SetUniform3i( const char * uniform, const int count, const int * values );
	void SetUniform4i( const char * uniform, const int count, const int * values );
	void SetUniform1f( const char * uniform, const int count, const float * values );
	void SetUniform2f( const char * uniform, const int count, const float * values );
	void SetUniform3f( const char * uniform, const int count, const float * values );
	void SetUniform4f( const char * uniform, const int count, const float * values );
	void SetUniformMatrix2f( const char * uniform, const int count, const bool transpose, const float * values );
	void SetUniformMatrix3f( const char * uniform, const int count, const bool transpose, const float * values );
	void SetUniformMatrix4f( const char * uniform, const int count, const bool transpose, const float * values );
	void SetAndBindUniformTexture( const char * uniform, const int textureSlot, const GLenum textureTarget, const GLuint textureID );
	void SetVertexAttribPointer( const char * attribute, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer );
	
private:
	GLuint mShaderProgram;
	void PrintLog( const GLuint shader, const char * logname ) const;

	unsigned int CompileVertexShader( const char *vshaderFile );
	unsigned int CompileGeometryShader( const char *gshaderFile );
	unsigned int CompileFragmentShader( const char *fshaderFile );
	bool CompileShaderFromFile( const char *vshaderFile, const char *fshaderFile );
	bool CompileShaderFromFile( const char *vshaderFile, const char *gshaderFile, const char *fshaderFile );

	static resourceMap_s s_shaders;
};

#endif