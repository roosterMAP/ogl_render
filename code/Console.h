#pragma once
#ifndef __CONSOLE_H_INCLUDE__
#define __CONSOLE_H_INCLUDE__

#include "Shader.h"
#include "String.h"
#include "Vector.h"

#include <ft2build.h>
#include FT_FREETYPE_H

struct IVec {
	signed int x;
	signed int y;
};

/*
==============================
Character
	-Data about an individual glyph used to draw characters to the screen.
==============================
*/
struct Character {
    GLuint TextureID;		// ID handle of the glyph texture
    IVec Size;				// Size of glyph
    IVec Bearing;			// Offset from baseline to left/top of glyph
    GLuint Advance;			// Offset to advance to next glyph
};

/*
==============================
Entry
	-An entry into the console command history. Valid inputs are converted
	-into Entry objects and stored in Console::m_commandHistory.
	-Entrys can also be creates as outputs and added to Console::m_logHistory.
==============================
*/
struct Entry {
	unsigned int type;
	Vec3 color;
    Str string;
};

/*
==============================
Console
	-Singleton class user uses to provide input or change the state of the program
==============================
*/
class Console {
	public:
		static Console* getInstance();
		~Console() { delete m_shader; delete m_backgroundShader; }

		bool Init( char * fontRelativePath );
		bool GetState() { return m_isActive; }
		void ToggleState() { m_isActive = !m_isActive; }

		void CursorPos_Right() { if ( m_cursorPos < m_currentString.Length() ) m_cursorPos++; }
		void CursorPos_Left() { if ( m_cursorPos > 0 ) { m_cursorPos--; } }
		void CursorPos_Up();
		void CursorPos_Down();
		void OffsetLogHistory( bool up );

		void AddInfo( Str text );	//print text in white
		void AddInfo( const char * text ) { Str strText = Str( text ); AddInfo( strText ); }

		void AddWarning( Str text );	//print text in orange
		void AddWarning( const char * text ) { Str strText = Str( text ); AddWarning( strText ); }

		void AddError( Str text );	//print text in red		
		void AddError( const char * text ) { Str strText = Str( text ); AddError( strText ); }

		void UpdateLog();	//draw the console to the screen
		void UpdateInputLine( char inputChar ); //update input line
		void UpdateInputLine_NewLine(); //add input line to command history
		void UpdateInputLine_Backspace(); //remove the character behind the cursor
		void UpdateInputLine_Tab();
		void UpdateInputLine_Delete(); //remove the character in front of the cursor

	private:
		static Console* inst_; //single instance
		Console();
        Console( const Console& ); //don't implement
        Console& operator=( const Console& ); //don't implement

		bool m_isActive;						//determines if the console is active an taking inputs from user
		unsigned int m_fontHeight;
		int m_maxLineCount;						//number of lines tall the console is when open
		std::vector<Entry> m_commandHistory;	//a history of past commands
		std::vector<Entry> m_logHistory;		//all the data currently in the log
		Str m_currentString;			//the current line where the use is inputting
		unsigned int m_VAO;
		unsigned int m_VBO;
		unsigned int m_bk_VAO;
		Shader * m_shader;
		Shader * m_backgroundShader;
		std::map<char, Character> m_characterMap;

		void InitBackgroundGeo( const unsigned int width, const unsigned int height );
		void InitTextGeo();
		void RenderText( Str text, GLfloat x, GLfloat y, GLfloat scale, const float * color, bool drawCursor=false );

		unsigned int m_cursorPos;
		unsigned int m_currentHistory;
		unsigned int m_logHistoryOffset;

		unsigned int m_gScreenWidth;
		unsigned int m_gScreenHeight;

		static const char * s_background_vshader_source;
		static const char * s_background_fshader_source;
};

#endif