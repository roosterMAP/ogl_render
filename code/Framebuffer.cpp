#include "Framebuffer.h"
#include "Mesh.h"


//vertex shader prog for debug light object
const char * Framebuffer::s_vshader_source =
	"#version 330 core\n"
	"layout ( location = 0 ) in vec3 aPos;"
	"layout ( location = 1 ) in vec2 aTexCoords;"
	"out vec2 TexCoords;"
	"void main() {"
	"	gl_Position = vec4( aPos.x, aPos.y, 0.0, 1.0 ); "
	"	TexCoords = aTexCoords;"
	"}";

//fragment shader prog for debug light object
const char * Framebuffer::s_fshader_source =
	"#version 330 core\n"
	"out vec4 FragColor;"
	"in vec2 TexCoords;"
	"uniform sampler2D screenTexture;"
	"void main() {"
	"	FragColor = vec4( texture( screenTexture, TexCoords ).rgb, 1.0 );"
	"}";

//fragment shader prog for debug light object
const char * Framebuffer::s_fshader_source_shadow =
	"#version 330 core\n"
	"out vec4 FragColor;"
	"in vec2 TexCoords;"
	"uniform sampler2DShadow screenTexture;"
	"void main() {"
	"	FragColor = vec4( texture( screenTexture, TexCoords ).rgb, 1.0 );"
	"}";

/*
================================
Framebuffer::Framebuffer
================================
*/
Framebuffer::Framebuffer( std::string colorbufferUniform ) {
	m_id = 0;
	m_colorbufferUniform = colorbufferUniform;
	m_colorBufferCount = 0;
	m_multisample = 0;
	m_screenVAO = 0;
}

/*
================================
Framebuffer::Delete
================================
*/
void Framebuffer::Delete() {
	for ( unsigned int i = 0; i < m_attachements.size(); i++ ) {
		glDeleteTextures( 1, &m_attachements[i] );		
	}
	m_attachements.clear();
	if ( m_id > 0 ) {
		glDeleteFramebuffers( 1, &m_id );
		m_id = 0;
	}
	if ( m_screenVAO > 0 ) {
		glDeleteVertexArrays( 1, &m_screenVAO );
		m_screenVAO = 0;
	}
}

/*
================================
Framebuffer::CreateDefaultBuffer
	-Create a new framebuffer and compiles shader from static member strings
================================
*/
bool Framebuffer::CreateDefaultBuffer( unsigned int width, unsigned int height ) {
	m_width = width;
	m_height = height;

	//compile shader
	m_shader = new Shader();
	if( !m_shader->CompileShaderFromCSTR( s_vshader_source, s_fshader_source ) ){
		delete m_shader;
		m_shader = nullptr;
		return false;
	}

	glGenFramebuffers( 1, &m_id ); //create frame buffer object
	glBindFramebuffer( GL_FRAMEBUFFER, m_id ); //bind both read and write buffers

	return true;
}

/*
================================
Framebuffer::CreateNewBuffer
	-Create a new framebuffer and compiles shader program for said buffer
================================
*/
bool Framebuffer::CreateNewBuffer( unsigned int width, unsigned int height, const char *shaderPrefix ) {
	m_width = width;
	m_height = height;

	//load and compile shaders
	m_shader = m_shader->GetShader( shaderPrefix );
	if ( m_shader == NULL ) {
		return false;
	}

	glGenFramebuffers( 1, &m_id ); //create frame buffer object
	glBindFramebuffer( GL_FRAMEBUFFER, m_id ); //bind both read and write buffers

	return true;
}

/*
================================
Framebuffer::AttachTextureBuffer
	-Creates a colorbuffer as a texture and attaches it to the framebuffer.
================================
*/
void Framebuffer::AttachTextureBuffer( GLenum internalformat, GLenum attachment, GLenum format, GLenum pixelFormat ) {
	assert( !m_colorbufferUniform.empty() );

	//increment color buffer counter
	if ( attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15 ) {
		m_colorBufferCount += 1;
	}
	
	//create framebuffer texture to read/write to.
	unsigned int texBuffer;
	glGenTextures( 1, &texBuffer );
	glBindTexture( GL_TEXTURE_2D, texBuffer );
	glTexImage2D( GL_TEXTURE_2D, 0, internalformat, m_width, m_height, 0, format, pixelFormat, NULL );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texBuffer, 0 ); //attach to frame buffer

	m_attachements.push_back( texBuffer );

	m_shader->UseProgram();
	m_shader->SetAndBindUniformTexture( m_colorbufferUniform.c_str(), 0, GL_TEXTURE_2D, texBuffer ); //pass texture uniform to shader

	glBindTexture( GL_TEXTURE_2D, 0 );
}

/*
================================
Framebuffer::AttachCubeMapTextureBuffer
	-Creates a colorbuffer as a cubemap texture and attaches it to the framebuffer.
================================
*/
void Framebuffer::AttachCubeMapTextureBuffer( GLenum internalformat, GLenum attachment,  GLenum format, GLenum pixelFormat ) {
	assert( !m_colorbufferUniform.empty() );
	
	//increment color buffer counter
	if ( attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15 ) {
		m_colorBufferCount += 1;
	}

	//create framebuffer texture to read/write to.
	unsigned int texBuffer;
	glGenTextures( 1, &texBuffer );
	glBindTexture( GL_TEXTURE_CUBE_MAP, texBuffer );

	for ( unsigned int i = 0; i < 6; i++ ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalformat, m_width, m_height, 0, format, pixelFormat, NULL );
	}
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
	
	m_attachements.push_back( texBuffer );
	glFramebufferTexture( GL_FRAMEBUFFER, attachment, texBuffer, 0 ); //attach to frame buffer

	m_shader->UseProgram();
	m_shader->SetAndBindUniformTexture( m_colorbufferUniform.c_str(), 0, GL_TEXTURE_CUBE_MAP, texBuffer ); //pass texture uniform to shader
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
}

/*
================================
Framebuffer::AttachRenderBuffer
	-Creates a write only renderbuffer and attaches it to the framebuffer.
================================
*/
void Framebuffer::AttachRenderBuffer( GLenum internalformat, GLenum attachment ) {
	//In order to be able to do depth/stencil testing, we can add a depth/stencil attachment to the frame buffer.
	//Since we only sample color in the shader, and not the depth data, we can use a render buffer object instead.
	unsigned int RBO;
	glGenRenderbuffers( 1, &RBO );
	glBindRenderbuffer( GL_RENDERBUFFER, RBO );

	if ( m_multisample == 0 ) {
		glRenderbufferStorage( GL_RENDERBUFFER, internalformat, m_width, m_height );
	} else { //MSAA
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, m_multisample, internalformat, m_width, m_height );
	}

	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	glFramebufferRenderbuffer( GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, RBO ); //attach the renderbuffer object to the depth and stencil attachment of the framebuffer

	m_attachements.push_back( RBO );
}
/*
================================
FrameBuffer::DrawToMultipleBuffers
	-Tells opengl that when we draw to this FrameBuffer we are drawing
	-to ALL attached color buffers. Only needed when more than one color
	-buffer is present.
================================
*/
void Framebuffer::DrawToMultipleBuffers() {
	unsigned int * attachements = new unsigned int[m_colorBufferCount];
	for ( unsigned int i = 0; i < m_colorBufferCount; i++ ) {
		attachements[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	glDrawBuffers( m_colorBufferCount, attachements );
	delete[] attachements;
	attachements = nullptr;
}

/*
================================
Framebuffer::Status
	-Returns true if framebuffer is complete
================================
*/
bool Framebuffer::Status() {
	glBindFramebuffer( GL_FRAMEBUFFER, m_id ); //bind both read and write buffers

	//check if framebuffer is properly hooked up and ready for rendering
	bool completeFrameBuffer = false;
	GLenum buff_status;
	buff_status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if ( buff_status == GL_FRAMEBUFFER_COMPLETE ) {
		completeFrameBuffer = true;
		//All subsequent rendering operations will now render to the attachments of the currently bound framebuffer.
	} else if ( buff_status == GL_FRAMEBUFFER_UNDEFINED ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_UNDEFINED: specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist." );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: at lease one of the framebuffer attachment points are framebuffer incomplete." );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: framebuffer does not have at least one image attached to it." );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi." );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER." );
	} else if ( buff_status == GL_FRAMEBUFFER_UNSUPPORTED ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_UNSUPPORTED: the combination of internal formats of the attached images violates an implementation-dependent set of restrictions." );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" );
	} else if ( buff_status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS ) {
		printf( "ERROR::FRAMEBUFFER::GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" );
	} else {
		printf( "ERROR::FRAMEBUFFER:: STATUS GAVE A STRANGE ERROR!!!" );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 ); //unbind framebuffer

	return completeFrameBuffer;
}

/*
================================
Framebuffer::CreateScreen
	-Return vertex arraw object of quad that will be hold the resulting framebuffer 
================================
*/
void Framebuffer::CreateScreen( int x_offset, int y_offset, int width, int height ) {
	if ( width < 0 ) {
		width = m_width;
	}
	if ( height < 0 ) {
		height = m_height;
	}

	//fetch the current render window dimensions from glut
	const int gScreenWidth = glutGet( GLUT_WINDOW_WIDTH );
	const int gScreenHeight = glutGet( GLUT_WINDOW_HEIGHT );

	const float width_normalized = ( float )width / ( float )gScreenWidth * 2.0f;
	const float height_normalized = ( float )height / ( float )gScreenHeight * 2.0f;

	const float x_normalized = ( float )x_offset / ( float )gScreenWidth - 1.0f;
	const float y_normalized = ( float )y_offset / ( float )gScreenHeight - 1.0f;

	//verts coords of the trianlge
	float vertices[] = {
		// positions
		x_normalized,						y_normalized,						0.0f,
		x_normalized + width_normalized,	y_normalized,						0.0f,
		x_normalized + width_normalized,	y_normalized + height_normalized,	0.0f,
		x_normalized,						y_normalized + height_normalized,	0.0f,

		//uvs
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	unsigned int triangles[] = {
		0, 1, 2,
		0, 2, 3
	};

	//pass to GPU
	unsigned int VBO;
	glGenVertexArrays( 1, &m_screenVAO );
	glGenBuffers( 1, &VBO );
	glBindVertexArray( m_screenVAO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void * )0 ); //position
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( float ), ( void* )( 12 * sizeof( float ) ) ); //uvs
}

/*
================================
Framebuffer::Draw
	-Draw the contents of the buffer onto the screen quad defined by m_screenVAO.
================================
*/
void Framebuffer::Draw( unsigned int colorBuffer ) {
	glDisable( GL_DEPTH_TEST ); //disable depth test so screen-space quad isn't discarded due to depth test.

	m_shader->UseProgram();
	m_shader->SetAndBindUniformTexture( m_colorbufferUniform.c_str(), 0, GL_TEXTURE_2D, colorBuffer );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, colorBuffer );

	glBindVertexArray( m_screenVAO );
	glDrawArrays( GL_QUADS, 0, 4 );	

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArray( 0 );

	glEnable( GL_DEPTH_TEST );
}