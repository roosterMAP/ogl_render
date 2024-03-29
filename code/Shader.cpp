#include "Shader.h"
#include <stdio.h>
#include "Fileio.h"
#include <assert.h>

unsigned int Buffer::s_count = 0;
resourceMap_b Buffer::s_buffers;

//initialize static members
resourceMap_s Shader::s_shaders;

/*
 ===================================
 Buffer::DeleteBuffer
 ===================================
 */
void Buffer::DeleteBuffer() {
	if ( m_initialized ) {
		GLuint delBuffers[1] = { m_id };
		glDeleteBuffers( 1, delBuffers );
	}
}

/*
 ===================================
 Buffer::Initialize
 ===================================
 */
void Buffer::Initialize( GLsizeiptr size, const void * data, GLenum usage ) {
	if ( !m_initialized ) {
		GLuint ssbo;
		glGenBuffers( 1, &ssbo );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo );
		glBufferData( GL_SHADER_STORAGE_BUFFER, size, data, usage );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
		m_initialized = true;
		m_id = ssbo;
		m_size = size;
	}
}

/*
 ===================================
 Buffer::GetBuffer
	-fetch an existing buffer from resource map.
	-if it doesnt exist, create it.
 ===================================
 */
Buffer * Buffer::GetBuffer( const char *name ) {
    resourceMap_b::iterator it = s_buffers.find( name );
    if ( it != s_buffers.end() ) {
		return it->second;
    }

	Buffer * buffer = new Buffer();
	buffer->SetName( name );
	buffer->m_bindingPoint = s_count;
	s_buffers[name] = buffer;
	s_count += 1;
	return buffer;
}

/*
 ===================================
 myglGetError
 ===================================
 */
void Shader::myglGetError() {
	// check for up to 10 errors pending
	for ( int i = 0 ; i < 10 ; i++ ) {
		const int err = glGetError();
		if ( err == GL_NO_ERROR ) {
			return;
		}
		switch ( err ) {
			case GL_INVALID_ENUM:
				printf( "GL_INVALID_ENUM\n" );
				break;
			case GL_INVALID_VALUE:
				printf( "GL_INVALID_VALUE\n" );
				break;
			case GL_INVALID_OPERATION:
				printf( "GL_INVALID_OPERATION\n" );
				break;
			case GL_OUT_OF_MEMORY:
				printf( "GL_OUT_OF_MEMORY\n" );
				break;       
			default:
				printf( "unknown GL error\n" );
				break;
		}
		assert( false );
	}
}

/*
 ===================================
gCurrentProgram
 ===================================
 */
static GLuint gCurrentProgram = 0;

/*
 ================================
 Shader::Shader
 ================================
 */
Shader::Shader() :
mShaderProgram( 0 ) {
	m_pinned = false;
}

/*
 ================================
 Shader::DeleteProgram
 ================================
 */
void Shader::DeleteProgram() {
	glDeleteProgram( mShaderProgram );
	m_buffers.clear();
	mShaderProgram = 0;
}

/*
 ================================
 Shader::DeleteProgram
 ================================
 */
void Shader::DeleteAllPrograms() {
	resourceMap_s::iterator it = s_shaders.begin();
	while ( it != s_shaders.end() ) {
		Shader * delShader = it->second;
		if ( delShader->m_pinned == false ) {
			delShader->DeleteProgram();
			for ( unsigned int i = 0; i < delShader->m_buffers.size(); i++ ) {
				delete delShader->m_buffers[i];
				delShader->m_buffers[i] = nullptr;
			}
			resourceMap_s::iterator toErase = it;
			++it;
			s_shaders.erase( toErase );
		} else {
			++it;
		}
	}
	
	//weird shit... some buffers get pinned...
	//empty buffers
	resourceMap_b::iterator b_it = Buffer::s_buffers.begin();
	while ( b_it != Buffer::s_buffers.end() ) {
		Buffer * delBuffer = b_it->second;
		if ( delBuffer->m_pinned == false ) {
			resourceMap_b::iterator toErase = b_it;
			++b_it;
			Buffer::s_buffers.erase( toErase );
		} else {
			++b_it;
		}
	}
	Buffer::s_buffers.clear();
}

/*
 ================================
 Shader::PinShader
	-Prevents this shader and its buffers from being deleted
 ================================
 */
void Shader::PinShader() {
	m_pinned = true;
	for ( unsigned int i = 0; i < m_buffers.size(); i++ ) {
		m_buffers[i]->m_pinned = true;
	}
}

/*
 ================================
 Shader::UnpinShader
	-Makes shaders and their buffers deletable
 ================================
 */
void Shader::UnpinShader() {
	m_pinned = false;
	for ( unsigned int i = 0; i < m_buffers.size(); i++ ) {
		m_buffers[i]->m_pinned = false;
	}
}

/*
 ================================
 Shader::BufferByBlockIndex
	-links the block index in this shader to the binding point of the buffer
 ================================
 */
void Shader::AddBuffer( Buffer * buffer ) {
	GLuint block_index = glGetProgramResourceIndex( mShaderProgram, GL_SHADER_STORAGE_BLOCK, buffer->GetName() );
	assert( block_index != GL_INVALID_INDEX );
	glShaderStorageBlockBinding( mShaderProgram, block_index, buffer->GetBindingPoint() );
	m_buffers[block_index] = buffer;
}

/*
 ================================
 Shader::BufferByBlockIndex
 ================================
 */
Buffer * Shader::BufferByBlockIndex( GLuint blockIdx ) {
	Buffer * buffer = m_buffers[blockIdx];
	GLuint block_index = glGetProgramResourceIndex( mShaderProgram, GL_SHADER_STORAGE_BLOCK, buffer->GetName() );
	assert( block_index != GL_INVALID_INDEX );
	assert( blockIdx == block_index );
	return buffer;
}

/*
 ================================
 Shader::BufferBlockByName
	-returns the block index of the buffer in the shader.
	-only returns it if the buffer is also included in m_buffer member map
 ================================
 */
const GLuint Shader::BufferBlockIndexByName( const char * name ) {
	//GLuint block_index = glGetUniformBlockIndex( mShaderProgram, name );
	GLuint block_index = glGetProgramResourceIndex( mShaderProgram, GL_SHADER_STORAGE_BLOCK, name );
	assert( block_index != GL_INVALID_INDEX ); //name must be present in shader to begine with

	//only return block_index if the buffer is included in m_buffer member map
	bufferMap::iterator it = m_buffers.begin();
	while ( it != m_buffers.end() ) {
		GLuint blkIdx = it->first;
		Buffer * buffer = it->second;
		if ( strcmp( name, buffer->GetName() ) == 0 ) {
			return block_index;
		}
		it++;
	}

	return GL_INVALID_INDEX;
}

/*
 ================================
 Shader::PrintLog
 ================================
 */
void Shader::PrintLog( const GLuint shader, const char * logname ) const {
	GLint logLength = 0;
	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );

	if ( logLength > 0 ) {
		GLchar *log = (GLchar *)malloc( logLength );
		glGetShaderInfoLog( shader, logLength, &logLength, log );

		printf( "Shader %s log:\n%s\n", logname, log );
		free( log );
	}
}

/*
 ================================
 Shader::GetShader
	-Function either fetches an existing shader or loads a new one.
 ================================
 */
Shader * Shader::GetShader( const char *sprefix ) {
    // Search for preexisting shader and return if found
    resourceMap_s::iterator it = s_shaders.find( sprefix );
    if ( it != s_shaders.end() ) {
        return it->second;
    }

    // Couldn't find pre-loaded material, so load from file
	Shader * newShader = LoadShader( sprefix );
    if ( newShader != NULL ) {
		s_shaders[sprefix] = newShader;
		return newShader;
	}

	return NULL;
}

/*
 ================================
 Shader::LoadShader
	-Compile shaders from sprefix and create a new Shader object
 ================================
 */
Shader * Shader::LoadShader( const char * sprefix ) {
	std::string relative_vertex = "data\\shader\\" + std::string( sprefix ) + "_vshader.glsl";
	std::string relative_geometry = "data\\shader\\" + std::string( sprefix ) + "_gshader.glsl";
	std::string relative_fragment = "data\\shader\\" + std::string( sprefix ) + "_fshader.glsl";
	std::string relative_compute = "data\\shader\\" + std::string( sprefix ) + "_cshader.glsl";

	FILE * file = NULL;

	//if getting the absolute path of the compute shader returns false, then assume there isnt one
	char absolutePath_compute[ 2048 ];
	const bool c_bool = RelativePathToFullPath( relative_compute.c_str(), absolutePath_compute );	
	file = fopen( absolutePath_compute, "rb" );
	const bool c_exists = file != NULL;
	if ( c_exists ) {
		fclose( file );
	}

	//if getting the absolute path of the geometry shader returns false, then assume there isnt one
	char absolutePath_geometry[ 2048 ];
	const bool g_bool = RelativePathToFullPath( relative_geometry.c_str(), absolutePath_geometry );	
	file = fopen( absolutePath_geometry, "rb" );
	const bool g_exists = file != NULL;
	if ( g_exists ) {
		fclose( file );
	}
	
	Shader * shader = new Shader;
	if ( c_exists ) {
		if( shader->CompileShaderFromFile( relative_compute.c_str() ) ) {
			return shader;
		}
	} else {
		if ( g_exists ) {
			if( shader->CompileShaderFromFile( relative_vertex.c_str(), relative_geometry.c_str(), relative_fragment.c_str() ) ) {
				return shader;
			}
		} else {
			if( shader->CompileShaderFromFile( relative_vertex.c_str(), relative_fragment.c_str() ) ) {
				return shader;
			}
		}
	}
	delete shader;
	shader = nullptr;

	return NULL;
}

/*
================================
Shader::IDByName
================================
*/
const GLuint Shader::IDByName( const char * sprefix ) {
    // Search for preexisting shader and return if found
    resourceMap_s::iterator it = s_shaders.find( sprefix );
    if ( it != s_shaders.end() ) {
        return it->second->mShaderProgram;
    }
	return 0;
}

/*
================================
Shader::CompileVertexShader
	Loads and compiles geometry shader file.
================================
*/
unsigned int Shader::CompileVertexShader( const char *vshaderFile ) {
	GLchar *source_v = NULL;
	int success;
	char infoLog[512];
	unsigned int size( 0 );

	//load source of vertex shader	
	GetFileData( vshaderFile, ( unsigned char** )&source_v, size );
	if ( !source_v ) {
		printf( "Failed to load vertex shader file!!!\n" );
		return false;
	}
	const GLchar * source_const_v = ( const GLchar * )source_v;

	//create vertex shader
	unsigned int vertexShader;
	vertexShader = glCreateShader( GL_VERTEX_SHADER ); //create vertex shader object
	glShaderSource( vertexShader, 1, &source_const_v, NULL ); //load in source code
	glCompileShader( vertexShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::VERTEX::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return 0;
	}

	return vertexShader;
}

/*
================================
Shader::CompileGeometryShader
	Loads and compiles geometry shader file.
================================
*/
unsigned int Shader::CompileGeometryShader( const char *gshaderFile ) {
	GLchar *source_g = NULL;
	int success;
	char infoLog[512];
	unsigned int size( 0 );

	//load source of geometry shader	
	GetFileData( gshaderFile, ( unsigned char** )&source_g, size );
	if ( !source_g ) {
		printf( "Failed to load geometry shader file!!!\n" );
		return false;
	}
	const GLchar * source_const_g = ( const GLchar * )source_g;

	//create geometry shader
	unsigned int geometryShader;
	geometryShader = glCreateShader( GL_GEOMETRY_SHADER ); //create geometry shader object
	glShaderSource( geometryShader, 1, &source_const_g, NULL ); //load in source code
	glCompileShader( geometryShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( geometryShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( geometryShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return 0;
	}

	return geometryShader;
}

/*
================================
Shader::CompileFragmentShader
	Loads and compiles fragment shader file.
================================
*/
unsigned int Shader::CompileFragmentShader( const char *fshaderFile ) {
	GLchar *source_f = NULL;
	int success;
	char infoLog[512];
	unsigned int size( 0 );

	//load source of fragment shader
	GetFileData( fshaderFile, ( unsigned char** )&source_f, size );
	if ( !source_f ) {
		printf( "Failed to load fragment shader file!!!\n" );
		return false;
	}
	const GLchar * source_const_f = ( const GLchar * )source_f;

	//create fragment shader
	unsigned int fragmentShader;
	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER ); //create fragment shader object
	glShaderSource( fragmentShader, 1, &source_const_f, NULL ); //load in source code
	glCompileShader( fragmentShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( fragmentShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return 0;
	}

	return fragmentShader;
}

/*
================================
Shader::CompileComputeShader
	Loads and compiles compute shader file.
================================
*/
unsigned int Shader::CompileComputeShader( const char *cshaderFile ) {
	GLchar *source_c = NULL;
	int success;
	char infoLog[512];
	unsigned int size( 0 );

	//load source of compute shader
	GetFileData( cshaderFile, ( unsigned char** )&source_c, size );
	if ( !source_c ) {
		printf( "Failed to load compute shader file!!!\n" );
		return false;
	}
	const GLchar * source_const_c = ( const GLchar * )source_c;

	//create compute shader
	unsigned int computeShader;
	computeShader = glCreateShader( GL_COMPUTE_SHADER ); //create compute shader object
	glShaderSource( computeShader, 1, &source_const_c, NULL ); //load in source code
	glCompileShader( computeShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( computeShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( computeShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::COMPUTE::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return 0;
	}

	return computeShader;
}

/*
================================
Shader::CompileShaderFromFile
	Compiles a compute shader and links it to a shader program.
================================
*/
bool Shader::CompileShaderFromFile( const char *cshaderFile ) {
	int success;
	char infoLog[512];

	//load and compile shader code
	unsigned int computeShader = CompileComputeShader( cshaderFile );

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, computeShader );
	glLinkProgram( mShaderProgram );
	
	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( computeShader );

	return true;
}

/*
================================
Shader::CompileShaderFromFile
	Compiles a vertex and fragment shader and links them to a shader program.
================================
*/
bool Shader::CompileShaderFromFile( const char *vshaderFile, const char *fshaderFile ) {
	int success;
	char infoLog[512];

	//load and compile shader code
	unsigned int vertexShader = CompileVertexShader( vshaderFile );
	unsigned int fragmentShader = CompileFragmentShader( fshaderFile );

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, vertexShader );
	glAttachShader( mShaderProgram, fragmentShader );
	glLinkProgram( mShaderProgram );
	
	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	return true;
}

/*
================================
Shader::CompileShaderFromFile
	Compiles a vertex, geometry, and fragment shader and links them to a shader program.
================================
*/
bool Shader::CompileShaderFromFile( const char *vshaderFile, const char *gshaderFile, const char *fshaderFile ) {
	int success;
	char infoLog[512];

	//load and compile shader code
	unsigned int vertexShader = CompileVertexShader( vshaderFile );
	unsigned int geometryShader = CompileGeometryShader( gshaderFile );
	unsigned int fragmentShader = CompileFragmentShader( fshaderFile );

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, vertexShader );
	glAttachShader( mShaderProgram, geometryShader );
	glAttachShader( mShaderProgram, fragmentShader );
	glLinkProgram( mShaderProgram );
	
	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( vertexShader );
	glDeleteShader( geometryShader );
	glDeleteShader( fragmentShader );

	return true;
}

/*
================================
Shader::CompileShaderFromCSTR
	Compiles a compute shader and links it to a shader program.
================================
*/
bool Shader::CompileShaderFromCSTR( const char * cshaderSource ) {
	int success;
	char infoLog[512];

	//create vertex shader
	unsigned int computeShader;
	computeShader = glCreateShader( GL_COMPUTE_SHADER ); //create vertex shader object
	glShaderSource( computeShader, 1, &cshaderSource, NULL ); //load in source code
	glCompileShader( computeShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( computeShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( computeShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::COMPUTE::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, computeShader );
	glLinkProgram( mShaderProgram );

	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::CSTR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( computeShader );

	return true;
}


/*
================================
Shader::CompileShaderFromCSTR
	Compiles a vertex and fragment shader and links them to a shader program.
================================
*/
bool Shader::CompileShaderFromCSTR( const char * vshaderSource, const char * fshaderSource ) {
	int success;
	char infoLog[512];

	//create vertex shader
	unsigned int vertexShader;
	vertexShader = glCreateShader( GL_VERTEX_SHADER ); //create vertex shader object
	glShaderSource( vertexShader, 1, &vshaderSource, NULL ); //load in source code
	glCompileShader( vertexShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::VERTEX::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}
	
	//create fragment shader
	unsigned int fragmentShader;
	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER ); //create fragment shader object
	glShaderSource( fragmentShader, 1, &fshaderSource, NULL ); //load in source code
	glCompileShader( fragmentShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( fragmentShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, vertexShader );
	glAttachShader( mShaderProgram, fragmentShader );
	glLinkProgram( mShaderProgram );

	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::CSTR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	return true;
}

/*
================================
Shader::CompileShaderFromCSTR
	Compiles a vertex, geometry, and fragment shader and links them to a shader program.
================================
*/
bool Shader::CompileShaderFromCSTR( const char * vshaderSource, const char * gshaderSource, const char * fshaderSource ) {
	int success;
	char infoLog[512];

	//create vertex shader
	unsigned int vertexShader;
	vertexShader = glCreateShader( GL_VERTEX_SHADER ); //create vertex shader object
	glShaderSource( vertexShader, 1, &vshaderSource, NULL ); //load in source code
	glCompileShader( vertexShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::VERTEX::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//create geometry shader
	unsigned int geometryShader;
	geometryShader = glCreateShader( GL_GEOMETRY_SHADER ); //create geometry shader object
	glShaderSource( geometryShader, 1, &gshaderSource, NULL ); //load in source code
	glCompileShader( geometryShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( geometryShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( geometryShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}
	
	//create fragment shader
	unsigned int fragmentShader;
	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER ); //create fragment shader object
	glShaderSource( fragmentShader, 1, &fshaderSource, NULL ); //load in source code
	glCompileShader( fragmentShader ); //compile

	//check if shader compiled successfully
	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
	if ( !success ) {
		glGetShaderInfoLog( fragmentShader, 512, NULL, infoLog );
		printf( "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//create shader program and link together the vertex and fragment shader
	mShaderProgram = glCreateProgram();
	glAttachShader( mShaderProgram, vertexShader );
	glAttachShader( mShaderProgram, geometryShader );
	glAttachShader( mShaderProgram, fragmentShader );
	glLinkProgram( mShaderProgram );

	//check if linking failed
	glGetProgramiv( mShaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( mShaderProgram, 512, NULL, infoLog );
		printf( "ERROR::CSTR::SHADER::PROGRAM::LINKING_FAILED:: %s\n", ( const char * )infoLog );
		return false;
	}

	//once linked shader programs should be deleted
	glDeleteShader( vertexShader );
	glDeleteShader( geometryShader );
	glDeleteShader( fragmentShader );

	return true;
}

/*
 ================================
 Shader::GetUniform
 ================================
 */
GLuint Shader::GetUniform( const char * name ) {
	// didn't find this uniform... find it the slow way and store
	const int val = glGetUniformLocation( mShaderProgram, name );
	assert( val != -1 );
	return val;
}

/*
 ================================
 Shader::GetAttribute
 ================================
 */
int Shader::GetAttribute( const char * name ) {
	// didn't find this attribute... find it the slow way and store
	const int val = glGetAttribLocation( mShaderProgram, name );
	assert( val != -1 );
	return val;
}

/*
 ================================
 Shader::UseProgram
 ================================
 */
void Shader::UseProgram() {
	if ( gCurrentProgram != mShaderProgram ) {
		glUseProgram( mShaderProgram );
		gCurrentProgram = mShaderProgram;
	}
}

/*
 ================================
 Shader::IsCurrentProgram
 ================================
 */
bool Shader::IsCurrentProgram() const {
	return ( mShaderProgram == gCurrentProgram );
}

/*
 ================================
 Shader::DispatchCompute
 ================================
 */
void Shader::DispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) const {
	assert( IsCurrentProgram() );
	myglGetError();

	glDispatchCompute( num_groups_x, num_groups_y, num_groups_z );
}

/*
 ================================
 Shader::SetUniform1i
 ================================
 */
void Shader::SetUniform1i( const char * uniform, const int count, const int * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform1iv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform2i
 ================================
 */
void Shader::SetUniform2i( const char * uniform, const int count, const int * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform2iv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform3i
 ================================
 */
void Shader::SetUniform3i( const char * uniform, const int count, const int * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform3iv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform4i
 ================================
 */
void Shader::SetUniform4i( const char * uniform, const int count, const int * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform4iv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform1f
 ================================
 */
void Shader::SetUniform1f( const char * uniform, const int count, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID = GetUniform( uniform );
	glUniform1fv( uniformID, count, values );
	myglGetError();
}

/*
 ================================
 Shader::SetUniform2f
 ================================
 */
void Shader::SetUniform2f( const char * uniform, const int count, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform2fv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform3f
 ================================
 */
void Shader::SetUniform3f( const char * uniform, const int count, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform3fv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniform4f
 ================================
 */
void Shader::SetUniform4f( const char * uniform, const int count, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID   = GetUniform( uniform );
	glUniform4fv( uniformID, count, values );
}

/*
 ================================
 Shader::SetUniformMatrix2f
 ================================
 */
void Shader::SetUniformMatrix2f( const char * uniform, const int count, const bool transpose, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID = GetUniform( uniform );
	glUniformMatrix2fv( uniformID, count, transpose, values );
}

/*
 ================================
 Shader::SetUniformMatrix3f
 ================================
 */
void Shader::SetUniformMatrix3f( const char * uniform, const int count, const bool transpose, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID = GetUniform( uniform );
	glUniformMatrix3fv( uniformID, count, transpose, values );
}

/*
 ================================
 Shader::SetUniformMatrix4f
 ================================
 */
void Shader::SetUniformMatrix4f( const char * uniform, const int count, const bool transpose, const float * values ) {
	assert( IsCurrentProgram() );
	assert( uniform );
	assert( values );
	myglGetError();
	
	const int uniformID = GetUniform( uniform );
	glUniformMatrix4fv( uniformID, count, transpose, values );
}

/*
 ================================
 Shader::SetAndBindUniformTexture
 ================================
 */
void Shader::SetAndBindUniformTexture(    const char * uniform,
											const int textureSlot,
											const GLenum textureTarget,
											const GLuint textureID ) {
	assert( IsCurrentProgram() );
	assert( textureSlot >= 0 );
	assert( uniform );
	myglGetError();

	const int uniformID = GetUniform( uniform );
	glActiveTexture( GL_TEXTURE0 + textureSlot);
	glUniform1i( uniformID, textureSlot );
	glBindTexture( textureTarget, textureID );
}

/*
 ================================
 Shader::SetVertexAttribPointer
 ================================
 */
void Shader::SetVertexAttribPointer(  const char * attribute,
										GLint size,
										GLenum type,
										GLboolean normalized,
										GLsizei stride,
										const GLvoid * pointer ) {
	assert( IsCurrentProgram() );
	assert( attribute );
//    assert( pointer ); // It's okay if the pointer is null valued, it means we're using vbo's
	myglGetError();

	// Enable and set
	const int attributeID   = GetAttribute( attribute );
	glEnableVertexAttribArray( attributeID );
	glVertexAttribPointer( attributeID, size, type, normalized, stride, pointer );
	myglGetError();
}
