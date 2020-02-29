#include "String.h"
#include <assert.h>
#include <string>

/*
================================
Str::Str
================================
*/
Str::Str() {
	m_len = 0;
	m_data = NULL;
}

/*
================================
Str::Str
================================
*/
Str::~Str() {
	delete[] m_data;
	m_data = NULL;
}

/*
================================
Str::Str
================================
*/
Str::Str( const Str & rhs ) {
	m_len = 0;
	m_data = NULL;
	*this = rhs;
}

/*
================================
Str::Str
================================
*/
Str::Str( const char * text ) {
	m_len = strlen( text );
	m_data = new char[ m_len + 1 ];
	strcpy( m_data, text );
	m_data[ m_len ] = '\0';
}

/*
================================
Str::Str
================================
*/
Str::Str( char c ) {
	m_len = 1;
	m_data = new char[ m_len + 1 ];
	m_data[ 0 ] = c;
	m_data[ 1 ] = '\0';
}

/*
================================
Str::Str
================================
*/
Str::Str( unsigned int len ) {
	m_len = len;
	m_data = new char[ m_len + 1 ];
	for ( unsigned int i = 0; i < len; i++ ) {
		m_data[i] = ' ';
	}
	m_data[ m_len ] = '\0';
}

/*
================================
Str::c_str
================================
*/
const char * Str::c_str() const {
	return m_data;
}

/*
================================
Str::operator[]
================================
*/
char Str::operator[]( int index ) const {
	assert( index < ( int )m_len );

	if ( index < 0 ) {
		index = m_len - index;
	}
	return m_data[index];
}

/*
================================
Str::operator[]
================================
*/
char & Str::operator[]( int index ) {
	assert( index < ( int )m_len );

	if ( index < 0 ) {
		index = m_len - index;
	}
	return m_data[index];
}

/*
================================
Str::operator+
================================
*/
Str Str::operator+( const Str & text ) {
	Str returnStr = Str( m_data );
	returnStr.Append( text );
	return returnStr;
}

/*
================================
Str::operator+
================================
*/
Str Str::operator+( const char * text ) {
	Str returnStr = Str( m_data );
	returnStr.Append( text );
	return returnStr;
}

/*
================================
Str::operator+=
================================
*/
void Str::operator+=( const Str & text ) {
	this->Append( text );
}

/*
================================
Str::operator+=
================================
*/
void Str::operator+=( const char * text ) {
	this->Append( text );
}

/*
================================
Str::operator=
================================
*/
void Str::operator=( const Str & text ) {
	m_len = text.m_len;
	delete[] m_data;
	m_data = new char[ m_len + 1 ];
	if ( text.m_data != NULL ) {
		strcpy( m_data, text.m_data );
	}
	m_data[m_len] = '\0';
}

/*
================================
Str::operator=
================================
*/
void Str::operator=( const char * text ) {
	m_len = strlen( text );
	delete[] m_data;
	m_data = new char[ m_len + 1 ];
	strcpy( m_data, text );
	m_data[m_len] = '\0';
}

/*
================================
Str::operator==
================================
*/
bool operator==( const Str &a, const Str &b ) {
	if ( strcmp( a.m_data, b.m_data ) != 0 ) {
		return false;
	} else {
		return true;
	}
}

/*
================================
Str::operator==
================================
*/
bool operator==( const Str &a, const char *b ) {
	assert( b );
	if ( strcmp( a.m_data, b ) != 0 ) {
		return false;
	} else {
		return true;
	}
}

/*
================================
Str::operator==
================================
*/
bool operator==( const char *a, const Str &b ) {
	assert( a );
	if ( strcmp( a, b.m_data ) != 0 ) {
		return false;
	} else {
		return true;
	}
}

/*
================================
Str::operator!=
================================
*/
bool operator!=( const Str &a, const Str &b ) {
	return !( a == b );
}

/*
================================
Str::operator!=
================================
*/
bool operator!=( const Str &a, const char *b ) {
	return !( a == b );
}

/*
================================
Str::operator!=
================================
*/
bool operator!=( const char *a, const Str &b ) {
	return !( a == b );
}

/*
================================
Str::Initialized
================================
*/
bool Str::Initialized() const {
	return m_data != NULL;
}

/*
================================
Str::Length
================================
*/
unsigned int Str::Length() const {
	return m_len;
}

/*
================================
Str::IsEmpty
================================
*/
bool Str::IsEmpty() const {
	return ( m_len == 0 && m_data[0] == '\0' );
}

/*
================================
Str::Clear
================================
*/
void Str::Clear() {
	m_len = 0;
	delete[] m_data;
	char * newStr = new char[ m_len + 1 ];
	newStr[0] = '\0';
	m_data = newStr;
}

/*
================================
Str::Append
================================
*/
void Str::Append( const char * text ) {
	const unsigned int text_len = strlen( text );
	const unsigned int newLen = m_len + text_len;
	char * newStr = new char[ newLen + 1 ];
	
	for ( unsigned int i = 0; i < m_len; i++ ) {
		newStr[i] = m_data[i];
	}

	for ( unsigned int i = 0; i < text_len; i++ ) {
		newStr[ m_len + i ] = text[i];
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Append
================================
*/
void Str::Append( const Str & text ) {
	Append( text.m_data );
}

/*
================================
Str::Insert
================================
*/
void Str::Insert( const char c, unsigned int idx ) {
	assert( idx <= m_len );
	const unsigned int newLen = m_len + 1;
	char * newStr = new char[ newLen + 1 ];

	unsigned int currentIdx = 0;
	for ( unsigned int i = 0; i <= m_len; i++ ) {
		if ( i == idx ) {
			newStr[currentIdx] = c;
			currentIdx += 1;
		}
		newStr[currentIdx] = m_data[i];
		currentIdx += 1;
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Insert
================================
*/
void Str::Insert( const char * text, unsigned int idx ) {
	assert( idx < m_len );
	const unsigned int text_len = strlen( text );
	const unsigned int newLen = m_len + text_len;
	char * newStr = new char[ newLen + 1 ];

	for ( unsigned int i = 0; i < idx; i++ ) {
		newStr[i] = m_data[i];
	}

	for ( unsigned int i = 0; i < text_len; i++ ) {
		newStr[ idx + i ] = text[i];
	}

	for ( unsigned int i = idx + text_len; i < newLen; i++ ) {
		newStr[i] = m_data[ i - text_len ];
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Insert
================================
*/
void Str::Insert( const Str & text, unsigned int idx ) {
	Insert( text.m_data, idx );
}

/*
================================
Str::Remove
================================
*/
void Str::Remove( unsigned int idx ) {
	assert( idx < m_len );

	unsigned int newLen = m_len - 1;
	char * newStr = new char[ newLen + 1 ];

	unsigned int currentIdx = 0;
	for ( unsigned int i = 0; i < m_len; i++ ) {
		if ( idx != i ) {
			newStr[currentIdx] = m_data[i];		
			currentIdx += 1;
		}
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Remove
	-Deletes character at startIdx all the way to the char BEFORE endIdx.
================================
*/
void Str::Remove( unsigned int startIdx, unsigned int endIdx ) {
	assert( endIdx > startIdx );
	assert( endIdx - startIdx <= m_len );

	unsigned int newLen = m_len - ( endIdx - startIdx );
	char * newStr = new char[ newLen + 1 ];

	unsigned int currentIdx = 0;
	for ( unsigned int i = 0; i < m_len; i++ ) {
		if ( i < startIdx || i >= endIdx ) {
			newStr[currentIdx] = m_data[i];
			currentIdx += 1;
		}
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Substring
================================
*/
Str Str::Substring( unsigned int startIdx, unsigned int endIdx ) {
	assert( endIdx > startIdx );
	assert( endIdx - startIdx <= m_len );

	Str substring( endIdx - startIdx );
	for ( unsigned int i = startIdx; i < endIdx; i++ ) {
		substring[ i - startIdx ] = m_data[i];
	}

	return substring;
}

/*
================================
Str::Find
	-returns index of searchStr
	-returns -1 if search failed
================================
*/
int Str::Find( const char * searchStr, const unsigned int startIdx ) const {
	assert( startIdx < m_len );
	const unsigned int search_len = strlen( searchStr );
	int returnIdx = -1;

	unsigned int comparisonIdx = 0;
	for ( unsigned int i = startIdx; i < m_len; i++ ) {
		if ( m_data[i] == searchStr[comparisonIdx] ) {
			comparisonIdx += 1;
		} else {
			comparisonIdx = 0;
		}

		if ( comparisonIdx >= search_len ) {
			returnIdx = i + 1 - search_len;
			break;
		}
	}

	return returnIdx;
}

/*
================================
Str::Find
================================
*/
int Str::Find( const Str & text, const unsigned int startIdx ) const {
	return Find( text.m_data, startIdx );
}

/*
================================
Str::ReplaceChar
	-If newChar == '/0', then occurances of oldChar are removed
================================
*/
void Str::ReplaceChar( const char oldChar, const char newChar ) {
	if ( newChar == '\0' ) {
		unsigned int occurances = 0;
		for ( unsigned int i = 0; i < m_len; i++ ) {
			if ( m_data[i] == oldChar ) {
				occurances += 1;
			}
		}
		unsigned int newLen = m_len - occurances;
		char * newStr = new char[ newLen + 1 ];
		unsigned int currentIdx = 0;
		for ( unsigned int i = 0; i < m_len; i++ ) {
			if ( m_data[i] != oldChar ) {
				newStr[currentIdx] = m_data[i];
				currentIdx += 1;
			}
		}
		m_len = newLen;
		delete[] m_data;
		m_data = newStr;
		m_data[m_len] = '\0';
	} else {
		for ( unsigned int i = 0; i < m_len; i++ ) {
			if ( m_data[i] == oldChar ) {
				m_data[i] = newChar;
			}
		}
	}
}

/*
================================
Str::Replace
================================
*/
void Str::Replace( const char * oldText, const char * newText, const bool allOccurances ) {
	const unsigned int oldText_len = strlen( oldText );
	const unsigned int newText_len = strlen( newText );

	std::vector< unsigned int > occurances;
	unsigned int startIdx = 0;
	while ( startIdx < m_len ) {
		int idx = Find( oldText, startIdx );
		if ( idx < 0 ) {
			break;
		}
		occurances.push_back( idx );
		startIdx = idx + oldText_len;
		if ( !allOccurances ) {
			break;
		}
	}

	unsigned int newLen = m_len - ( oldText_len * occurances.size() ) +  ( newText_len * occurances.size() );
	char * newStr = new char[ newLen + 1 ];

	unsigned int newStrProgress = 0;
	unsigned int oldStrProgress = 0;
	for ( unsigned int i = 0; i < occurances.size(); i++ ) {
		for ( unsigned int j = oldStrProgress; j < occurances[i]; j++ ) {
			newStr[newStrProgress] = m_data[j];
			newStrProgress += 1;
		}
		oldStrProgress = occurances[i] + oldText_len;
		for ( unsigned int j = 0; j < newText_len; j++ ) {
			newStr[newStrProgress] = newText[j];
			newStrProgress += 1;
		}
	}

	for ( unsigned int i = oldStrProgress; i < m_len; i++ ) {
		newStr[newStrProgress] = m_data[i];
		newStrProgress += 1;
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Replace
================================
*/
void Str::Replace( const Str & oldText, const char * newText, const bool allOccurances ) {
	Replace( oldText.m_data, newText, allOccurances );
}

/*
================================
Str::Replace
================================
*/
void Str::Replace( const Str & oldText, const Str & newText, const bool allOccurances ) {
	Replace( oldText.m_data, newText.m_data, allOccurances );
}

/*
================================
Str::Replace
================================
*/
void Str::Replace( const char * oldText, const Str & newText, const bool allOccurances ) {
	Replace( oldText, newText.m_data, allOccurances );
}

/*
================================
Str::RStrip
================================
*/
void Str::RStrip() {
	unsigned int count = 0;
	while ( count < m_len ) {
		if ( !isspace( m_data[ m_len - count - 1 ] ) ) { //-1 becasue /0 isnt considered a white-space character. So we skip.
			break;
		}
		count += 1;
	}

	unsigned int newLen = m_len - count;
	char * newStr = new char[ newLen + 1 ];

	for ( unsigned int i = 0; i < newLen; i++ ) {
		newStr[i] = m_data[ i ];
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::LStrip
================================
*/
void Str::LStrip() {
	unsigned int count = 0;
	while ( count < m_len ) {
		if ( !isspace( m_data[count] ) ) {
			break;
		}
		count += 1;
	}

	unsigned int newLen = m_len - count;
	char * newStr = new char[ newLen + 1 ];

	for ( unsigned int i = 0; i < newLen; i++ ) {
		newStr[i] = m_data[ i + count ];
	}

	m_len = newLen;
	delete[] m_data;
	m_data = newStr;
	m_data[m_len] = '\0';
}

/*
================================
Str::Strip
================================
*/
void Str::Strip() {
	RStrip();
	LStrip();
}

/*
================================
Str::Split
================================
*/
std::vector<Str> Str::Split( char c ) {
	std::vector<Str> strList;
	char * buff = new char[m_len];
	unsigned int buffLen = 0;
	for ( unsigned int i = 0; i < m_len; i++ ) {
		if ( m_data[i] != c ) {
			buff[buffLen] = m_data[i];
			buffLen += 1;
		} else {
			buff[buffLen] = '\0';
			strList.push_back(  Str( buff ) );

			buffLen = 0;
			delete[] buff;
			buff = new char[m_len];
		}
	}
	if ( buffLen > 0 ) {
		buff[buffLen] = '\0';
		strList.push_back(  Str( buff ) );
		delete[] buff;
	}

	return strList;
}

/*
================================
Str::StartsWith
================================
*/
bool Str::StartsWith( const char * text ) const {
	const unsigned int text_len = strlen( text );

	bool match = false;
	for ( unsigned int i = 0; i < m_len; i++ ) {
		if ( m_data[i] != text[i] ) {
			break;
		}
		if ( i == text_len - 1 ) {
			match = true;
			break;
		}
	}

	return match;
}

/*
================================
Str::StartsWith
================================
*/
bool Str::StartsWith( const Str & text ) const {
	return StartsWith( text.m_data );
}

/*
================================
Str::EndsWith
================================
*/
bool Str::EndsWith( const char * text ) const {
	const unsigned int text_len = strlen( text );
	if ( text_len > m_len ) {
		return false;
	}

	bool match = false;
	for ( unsigned int i = 0; i < text_len; i++ ) {
		if ( m_data[ m_len - i ] != text[ text_len - i ] ) {
			break;
		}
		if ( i == text_len - 1 ) {
			match = true;
			break;
		}
	}

	return match;
}

/*
================================
Str::EndsWith
================================
*/
bool Str::EndsWith( const Str & text ) const {
	return StartsWith( text.m_data );
}