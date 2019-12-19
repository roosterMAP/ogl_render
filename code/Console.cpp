#include "Console.h"
#include "Command.h"
#include "Fileio.h"

/*
================================
Console::getInstance
================================
*/
Console * Console::getInstance() {
   if ( inst_ == NULL ) {
      inst_ = new Console();
   }
   return( inst_ );
}

Console * Console::inst_ = NULL; //Define the static Singleton pointer

const char * Console::s_background_vshader_source =
	"#version 330 core\n"
	"layout ( location = 0 ) in vec3 aPos;"
	"void main() {"
	"	gl_Position = vec4( aPos, 1.0 );"
	"}";

const char * Console::s_background_fshader_source =
	"#version 330 core\n"
	"out vec4 FragColor;"
	"uniform vec3 color;"
	"void main() {"
	"	FragColor = vec4( color, 1.0 );"
	"}";

/*
================================
Console::Console
================================
*/
Console::Console() {
	m_isActive = false;
	m_maxLineCount = 10;
	m_currentString = "";
	m_fontHeight = 12;
	m_cursorPos = 0;
	m_currentHistory = 0;
	m_logHistoryOffset = 0;
}

/*
================================
Console::InitBackgroundGeo
================================
*/
void Console::InitBackgroundGeo( const unsigned int width, const unsigned int height ) {
	//create background plane geo
	const float boxHeight = ( float )m_fontHeight * ( float )m_maxLineCount;
	const float min_y = boxHeight / ( float )height * 0.5f + 0.5f;
	float vertices[] = {
		-1.0f,	min_y,	0.0f,
		1.0f,	min_y,	0.0f,
		1.0f,	1.0f,	0.0f,
		-1.0f,	1.0f,	0.0f
	};
	unsigned int VBO;

	//pass vertices
	glGenVertexArrays( 1, &m_bk_VAO );
	glGenBuffers( 1, &VBO );
	glBindVertexArray( m_bk_VAO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void * )0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
}

/*
================================
Console::InitTextGeo
	-inits m_VAO and m_VBO used to hold text geo.
	-this gets updated every time text is rendered in RenderText()
================================
*/
void Console::InitTextGeo() {
	glGenVertexArrays( 1, &m_VAO );
	glGenBuffers( 1, &m_VBO );
	glBindVertexArray( m_VAO );
	glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * 6 * 4, NULL, GL_DYNAMIC_DRAW );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
}

/*
================================
Console::Init
================================
*/
bool Console::Init( char * fontRelativePath ) {
	//get window properties
	m_gScreenWidth = glutGet( GLUT_WINDOW_WIDTH );
	m_gScreenHeight = glutGet( GLUT_WINDOW_HEIGHT );
	glm::mat4 projection = glm::ortho( 0.0f, ( float )m_gScreenWidth, 0.0f, ( float )m_gScreenHeight );

	//compile shader for background plane
	m_backgroundShader = new Shader();
	if( !m_backgroundShader->CompileShaderFromCSTR( s_background_vshader_source, s_background_fshader_source ) ){
		return false;
	}
	m_backgroundShader->UseProgram();
	glm::vec3 black = glm::vec3( 0.0, 0.0, 0.0 );
	m_backgroundShader->SetUniform3f( "color", 1, glm::value_ptr( black ) );

	//create background plane geo
	InitBackgroundGeo( m_gScreenWidth, m_gScreenHeight );

	//initialize freetext library and load font
	FT_Library ft;
	if ( FT_Init_FreeType( &ft ) ) {
		printf( "ERROR::FREETYPE: Could not init FreeType Library" );
	}

	char fontAbsolutePath[ 2048 ];
	RelativePathToFullPath( fontRelativePath, fontAbsolutePath );

	FT_Face face;
	if ( FT_New_Face( ft, fontAbsolutePath, 0, &face ) ) {
		printf( "ERROR::FREETYPE: Failed to load font" );
		return false;
	}
	
	if ( FT_Set_Pixel_Sizes( face, 0, m_fontHeight ) ) {
		printf( "ERROR::FREETYPE: Failed to set pixel size" );
		return false;
	}

	//init text shader
	m_shader = m_shader->GetShader( "text" );
	m_shader->UseProgram();
	m_shader->SetUniformMatrix4f( "projection", 1, false, glm::value_ptr( projection ) );

	//init VAO and VBO used to hold text geo. this gets updated every time text is rendered in RenderText()
	InitTextGeo();

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); //Disable byte-alignment restriction
	
	//load textures of each character
	for ( GLubyte i = 0; i < 255; i++ ) {
		// Load character glyph 
		if ( FT_Load_Char( face, i, FT_LOAD_RENDER ) ) {
			printf( "ERROR::FREETYTPE: Failed to load Glyph" );
			continue;
		}

		// Generate texture
		GLuint texture;
		glGenTextures( 1, &texture );
		glBindTexture( GL_TEXTURE_2D, texture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer );
		
		// Set texture options
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		// Now store character for later use
		Character character = {
			texture, 
			glm::ivec2( face->glyph->bitmap.width, face->glyph->bitmap.rows ),
			glm::ivec2( face->glyph->bitmap_left, face->glyph->bitmap_top ),
			face->glyph->advance.x
		};
		m_characterMap.insert( std::pair<GLchar, Character>( i, character ) );
	}
	FT_Done_Face( face );
	FT_Done_FreeType( ft );
	
	return true;
}

/*
================================
Console::CursorPos_Up
================================
*/
void Console::CursorPos_Up() {
	if ( m_commandHistory.size() > 0 && m_currentHistory > 0 ) {
		m_currentHistory--;
		m_currentString = m_commandHistory[m_currentHistory].string;
		m_cursorPos = m_currentString.Length();
	}
}

/*
================================
Console::CursorPos_Down
================================
*/
void Console::CursorPos_Down() {
	if (  m_commandHistory.size() > 0 && m_currentHistory < m_commandHistory.size() - 1 ) {
		m_currentHistory++;
		m_currentString = m_commandHistory[m_currentHistory].string;
		m_cursorPos = m_currentString.Length();
	}
}

/*
================================
Console::OffsetLogHistory
================================
*/
void Console::OffsetLogHistory( bool up ) {
	int newOffset = ( int )m_logHistoryOffset;

	//increment proposed new log offset
	if ( up ) {
		newOffset += 1;
	} else {
		newOffset -= 1;
	}

	//check if we can scroll DOWN furthur
	if ( newOffset < 0 ) {
		return;
	}

	//check if we can scroll UP furthur
	int logSize = m_logHistory.size();
	if ( logSize - newOffset < m_maxLineCount ) {
		return;
	}

	//update offset now that we've confirmed its valid
	m_logHistoryOffset = ( unsigned int )newOffset;
}

/*
================================
Console::UpdateLog
	-Draw last 10 lines of log history to the screen
================================
*/
void Console::UpdateLog() {
	//only run if state is active
	if ( !m_isActive ) {
		return;
	}

	//draw background plane first
	m_backgroundShader->UseProgram();
	glBindVertexArray( m_bk_VAO );
	glDrawArrays( GL_QUADS, 0, 4 );
	glBindVertexArray( 0 );	

	//draw lines of text from log history
	unsigned int inputLine_y = m_fontHeight * ( m_maxLineCount + 2 );
	unsigned int logSize = m_logHistory.size();
	for ( unsigned int i = 0; i < logSize; i++ ) {
		if ( ( int )i >= m_maxLineCount ) {
			break;
		}
		unsigned int entryIdx = logSize - i - 1 - m_logHistoryOffset;
		Entry currentEntry = m_logHistory[entryIdx];

		unsigned int logLine_y = m_gScreenHeight - ( inputLine_y - ( m_fontHeight * ( i + 1 ) ) );
		RenderText( currentEntry.string, 10.0f, ( float )logLine_y, 1.0f, glm::value_ptr( currentEntry.color ) );
	}

	//draw input line of text with a text cursor
	const float x_offset = 10.0f;
	const float textCursorPos_y = ( float )( m_gScreenHeight - inputLine_y );
	glm::vec3 white = glm::vec3( 1.0, 1.0, 1.0 );
	RenderText( m_currentString, x_offset, ( float )( m_gScreenHeight - inputLine_y ), 1.0f, glm::value_ptr( white ), true );
}

/*
================================
Console::UpdateInputLine
================================
*/
void Console::UpdateInputLine( char inputChar ) {
	//only run if state is active
	if ( m_isActive ) {
		m_currentString.Insert( inputChar, m_cursorPos );
		m_cursorPos += 1;
	}
}

/*
================================
Console::UpdateInputLine_NewLine
	-Sets the states of commands
	-Clears input line
	-Updates log
================================
*/
void Console::UpdateInputLine_NewLine() {
	//only run if state is active
	if ( m_isActive ) {

		if ( g_cmdSys->CallByName( Str( m_currentString ) ) ) {
			//if new string is a newline character, then user has committed a new entry.
			//it needs to be passed to the history and the input line must be cleared.
			Entry newEntry = {
				0,
				glm::vec3( 1.0, 1.0, 1.0 ),
				m_currentString
			};
			m_commandHistory.push_back( newEntry );
			m_currentString = "";
			UpdateLog();
			m_cursorPos = 0; //reset cursor position to begining
			m_currentHistory = m_commandHistory.size(); //reset cursorHistory to the size of the log
		}
	}
}

/*
================================
Console::UpdateInputLine_Backspace
================================
*/
void Console::UpdateInputLine_Backspace() {
	if ( m_isActive ) {
		if ( m_cursorPos > 0 && m_currentString.Length() > 0 ) {
			m_currentString.Remove( m_cursorPos - 1 );
			m_cursorPos -= 1;
		}
	}
}

/*
================================
Console::UpdateInputLine_Tab
================================
*/
void Console::UpdateInputLine_Tab() {
	if ( m_isActive && m_currentString.Length() > 0 ) {
		Str tabCmdName = m_currentString;
		const int firstSpace_idx = m_currentString.Find( ' ', 0 );
		if ( firstSpace_idx >= 0 ) {
			tabCmdName = m_currentString.Substring( 0, firstSpace_idx );
		}

		//get list of all cmd names that start with m_currentString
		std::vector< Str > tabCmds;
		std::vector< unsigned int > tabCmdIdx;
		for ( unsigned int i = 0; i < g_cmdSys->CommandCount(); i++ ) {
			const Cmd * cmd = g_cmdSys->CommandByIndex( i );
			const Str cmdName = cmd->name;
			if ( cmdName.StartsWith( tabCmdName ) ) {
				tabCmds.push_back( cmdName );
				tabCmdIdx.push_back( i );
			}
		}

		//if no matches found, then return early
		if ( tabCmds.size() == 0 ) {
			return;
		}

		//if only one found, then autocomplete and return early
		if ( tabCmds.size() == 1 ) {
			//if m_currentString is already a complete cmd name, then print its description
			if ( tabCmdName.Length() == tabCmds[0].Length() ) {
				const Cmd * cmd = g_cmdSys->CommandByIndex( tabCmdIdx[0] );
				AddInfo( cmd->description );
			}
			if ( tabCmds[0].Length() > m_currentString.Length() ) {
				m_currentString = tabCmds[0];
			}
			m_cursorPos = m_currentString.Length();
			return;
		}

		//find the longest starting substring within all tabCmds list
		bool matchingChars = true;
		unsigned int charIdx = 0;
		for ( unsigned int i = 0; i < tabCmds.size(); i++ ) {			
			while ( matchingChars ) {
				if ( charIdx >= tabCmds[i].Length() ) {
					matchingChars = false;
					charIdx -= 1;
					break;
				}

				for ( unsigned int j = 0; j <tabCmds.size(); j++  ) {
					if ( i == j ) {
						continue;
					}
					if ( charIdx >= tabCmds[j].Length() || tabCmds[i][charIdx] != tabCmds[j][charIdx] ) {
						matchingChars = false;
						charIdx -= 1;
						break;
					}
				}				
				charIdx += 1;
			}

			if ( matchingChars == false ) {
				break;
			}
		}

		//if user presses tab a second time, then we enter this if statement
		if ( charIdx == m_currentString.Length() ) {
			//print contents of tabCmds to let user know which are available
			for ( unsigned int i = 0; i < tabCmds.size(); i++ ) {
				AddInfo( tabCmds[i] );
			}
		}

		//set the current string, and move cursor to end of line
		m_currentString = tabCmds[0].Substring( 0, charIdx );
		m_cursorPos = m_currentString.Length();
	}
}

/*
================================
Console::UpdateInputLine_Delete
================================
*/
void Console::UpdateInputLine_Delete() {
	if ( m_isActive ) {
		if ( m_cursorPos < m_currentString.Length() && m_currentString.Length() > 0 ) {
			m_currentString.Remove( m_cursorPos );
		}
	}
}

/*
================================
Console::RenderText
================================
*/
void Console::RenderText( Str text, GLfloat x, GLfloat y, GLfloat scale, const float * color, bool drawCursor ) {
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	//activate shader and pass color uniform
    m_shader->UseProgram();
	m_shader->SetUniform3f( "color", 1, color );
	glActiveTexture( GL_TEXTURE0 );
    glBindVertexArray( m_VAO ); //bind VAO

	GLfloat cursorPos_x = 0.0f; //x pos of the cursor on the screen (in screen space)

    //Iterate through all characters
	unsigned int i = 0;
	for ( unsigned int i = 0; i < text.Length(); i++ ) {
		char c = text[i];
        Character ch = m_characterMap[c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - ( ch.Size.y - ch.Bearing.y ) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        //Render glyph texture over quad
		glBindTexture( GL_TEXTURE_2D, ch.TextureID );

        // Update content of VBO memory
        glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
        glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof( vertices ), vertices ); 
        glBindBuffer( GL_ARRAY_BUFFER, 0 );        
        glDrawArrays( GL_TRIANGLES, 0, 6 ); //Render quad

		//get the x position of the cursor
		if ( i == m_cursorPos - 1 ) {
			cursorPos_x = xpos + ( w / 2.0f );
		}

        //advance cursor for next glyph (advance is number of 1/64 pixels)
        x += ( ch.Advance >> 6 ) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray( 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glDisable( GL_BLEND );

	//draw the text cursor
	if ( drawCursor ) {
		if ( text.Length() == 0 ) {
			cursorPos_x = 5.0f;
		}
		glm::vec3 green = glm::vec3( 0.0, 1.0, 0.0 );
		RenderText( "|", cursorPos_x, y, scale, glm::value_ptr( green ), false );
	}
}

/*
================================
Console::AddInfo
================================
*/
void Console::AddInfo( Str text ) {
	Entry newEntry = {
		0,
		glm::vec3( 0.8, 0.8, 0.8 ), //light grey
		text
	};

	m_logHistory.push_back( newEntry );
	UpdateLog();
}

/*
================================
Console::AddInfo
================================
*/
void Console::AddWarning( Str text ) {
	Entry newEntry = {
		0,
		glm::vec3( 1.0, 0.64, 0.0 ), //orange
		text
	};

	m_logHistory.push_back( newEntry );
	UpdateLog();
}

/*
================================
Console::AddInfo
================================
*/
void Console::AddError( Str text ) {
	Entry newEntry = {
		0,
		glm::vec3( 1.0, 0.0, 0.0 ), //red
		text
	};

	m_logHistory.push_back( newEntry );
	UpdateLog();
}