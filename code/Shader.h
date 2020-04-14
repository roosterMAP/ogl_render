#pragma once
#ifndef __SHADER_H_INCLUDE__
#define __SHADER_H_INCLUDE__

#include <stdio.h>
#include <map>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GL/freeglut.h>

class Buffer;
class Shader;

typedef std::map< std::string, Shader * > resourceMap_s;
typedef std::map< std::string, Buffer * > resourceMap_b;
typedef std::map< GLuint, Buffer * > bufferMap;

class Buffer {
	public:
		Buffer() { m_initialized = false; m_pinned = false; };
		~Buffer() {};

		void DeleteBuffer();

		void Initialize( GLsizeiptr size, const void * data, GLenum usage );
		Buffer * GetBuffer( const char *name );
		const GLuint GetID() const { return m_id; }
		const char * GetName() const { return m_name; }
		void SetName( const char * name ) { strcpy( m_name, name ); }
		const GLuint GetBindingPoint() const { return m_bindingPoint; }

	private:
		bool m_initialized;
		GLuint m_id;
		char m_name[64];
		GLuint m_bindingPoint;
		GLsizeiptr m_size;
		bool m_pinned;

		static unsigned int s_count;
		static resourceMap_b s_buffers;

	friend Shader;
};

/*
==============================
Shader
==============================
*/
class Shader {
public:
	Shader();
	~Shader() {};

	static void myglGetError();
	
	Shader * GetShader( const char *sprefix );
	Shader * LoadShader( const char *sprefix );
	const GLuint IDByName( const char * sprefix );

	bool CompileShaderFromCSTR( const char * cshaderSource );
	bool CompileShaderFromCSTR( const char * vshaderSource, const char * fshaderSource );
	bool CompileShaderFromCSTR( const char * vshaderSource, const char *gshaderFile, const char * fshaderSource );
	GLuint GetShaderProgram() const { return mShaderProgram; }
	void UseProgram();
	bool IsCurrentProgram() const;

	void DispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) const;

	void DeleteProgram();
	static void DeleteAllPrograms();

	void PinShader();
	void UnpinShader();

	GLuint GetUniform( const char * name );
	int GetAttribute( const char * name );

	void AddBuffer( Buffer * buffer );
	unsigned int BufferCount() const { return m_buffers.size(); }
	Buffer * BufferByBlockIndex( GLuint blockIdx );
	const GLuint BufferBlockIndexByName( const char * name );

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

	bufferMap m_buffers;

	unsigned int CompileVertexShader( const char *vshaderFile );
	unsigned int CompileGeometryShader( const char *gshaderFile );
	unsigned int CompileFragmentShader( const char *fshaderFile );
	unsigned int CompileComputeShader( const char *cshaderFile );
	bool CompileShaderFromFile( const char *cshaderFile );
	bool CompileShaderFromFile( const char *vshaderFile, const char *fshaderFile );
	bool CompileShaderFromFile( const char *vshaderFile, const char *gshaderFile, const char *fshaderFile );

	static resourceMap_s s_shaders;

	bool m_pinned;
};

#endif