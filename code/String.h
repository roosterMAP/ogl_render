#pragma once
#ifndef __STRING_H_INCLUDE__
#define __STRING_H_INCLUDE__

#include <vector>

/*
================================
Str
================================
*/
class Str {
	public:
		Str();
		Str( const char * text );
		Str( char c );
		Str( unsigned int len );
		Str( const Str & rhs );
		~Str();

		const char * c_str() const;

		char operator[]( int index ) const;
		char & operator[]( int index );

		Str operator+( const Str & text );
		Str operator+( const char * text );

		void operator+=( const Str & text );
		void operator+=( const char * text );

		void operator=( const Str & text );
		void operator=( const char * text );

		friend bool operator==( const Str &a, const Str &b );
		friend bool operator==( const Str &a, const char *b );
		friend bool operator==( const char *a, const Str &b );

		friend bool operator!=( const Str &a, const Str &b );
		friend bool operator!=( const Str &a, const char *b );
		friend bool operator!=( const char *a, const Str &b );

		unsigned int Length() const;
		bool IsEmpty() const;
		void Clear();

		void Append( const char * text );
		void Append( const Str & text );

		void Insert( const char c, unsigned int idx );
		void Insert( const char * text, unsigned int idx );
		void Insert( const Str & text, unsigned int idx );

		void Remove( unsigned int idx );
		void Remove( unsigned int startIdx, unsigned int endIdx );

		Str Substring( unsigned int startIdx, unsigned int endIdx );

		int Find( const char * searchStr, const unsigned int startIdx=0 ) const;
		int Find( const Str & text, const unsigned int startIdx=0 ) const;
		void ReplaceChar( const char oldChar, const char newChar );
		void Replace( const char * oldText, const char * newText, const bool allOccurances );
		void Replace( const Str & oldText, const char * newText, const bool allOccurances );
		void Replace( const Str & oldText, const Str & newText, const bool allOccurances );
		void Replace( const char * oldText, const Str & newText, const bool allOccurances );

		void RStrip();
		void LStrip();
		void Strip();

		std::vector<Str> Split( char c );

		bool StartsWith( const char * text ) const;
		bool StartsWith( const Str & text ) const;
		bool EndsWith( const char * text ) const;
		bool EndsWith( const Str & text ) const;

	protected:
		unsigned int m_len; //length of m_data minus the '\0' char
		char * m_data; //ALWAYS ends with '\0'
};

#endif

//SMOKE TEST
/*
	Str str_a = Str();
	Str str_b = Str( "pAop" );
	Str str_c = Str( 'o' );
	printf( str_b.c_str() );
	str_b[1] = str_c[0];
	if ( str_b == str_a ) {
		printf( "wtf?" );
	}
	if ( str_a != str_b ) {
		str_a = "fuck";
	}
	unsigned int len = str_a.Length();
	if ( !str_a.IsEmpty() ) {
		str_a.Clear();
	}
	str_a = "fuck";

	str_b.Append( " in my butt" );
	str_b.Append( str_a );

	str_b.Insert( " and on", 7 );
	Str str_d = Str( "ing" );
	str_b.Insert( str_d, 4 );

	int idx1 = str_b.Find( "ing", 3 );
	int idx2 = str_b.Find( str_d );

	Str str_e = Str( "  \n \tpoopy\n\t " );
	str_e.Strip();

	if ( str_b.StartsWith( "poop" ) ) {
		printf( str_b.c_str() );
	}

	str_b.Replace( "in", "on", false );
*/