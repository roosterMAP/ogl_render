/*
 *  Fileio.cpp
 *
 */

#include "Fileio.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <windows.h>


#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

static char gApplicationDirectory[ FILENAME_MAX ];	// needs to be CWD
static bool gWasInitialized = false;


/*
================================
findAndReplaceAll
================================
*/
void findAndReplaceAll( std::string & data, std::string toSearch, std::string replaceStr ) {
	// Get the first occurrence
	size_t pos = data.find( toSearch );
 
	// Repeat till end is reached
	while( pos != std::string::npos ) {
		// Replace this occurrence of Sub String
		data.replace( pos, toSearch.size(), replaceStr );
		// Get the next occurrence from the current position
		pos = data.find( toSearch, pos + replaceStr.size() );
	}
}

/*
 =================================
 Initialize
 Grabs the application's directory
 =================================
 */
static void Initialize() {
	if ( gWasInitialized ) {
		return;
	}
	gWasInitialized = true;
	
	bool result = GetCurrentDir( gApplicationDirectory, sizeof( gApplicationDirectory ) );
	assert( result );
	if ( result ) {
		printf( "ApplicationDirectory: %s\n", gApplicationDirectory );
	} else {
		printf( "ERROR: Unable to get current working directory!\n");
	}
}

bool RelativePathToFullPath( const char * relativePath, char fullPath[ 2048 ] ) {
	Initialize();

	const int rv = sprintf( fullPath, "%s\\%s", gApplicationDirectory, relativePath );
	return rv > 0;
}

/*
 =================================
 GetFileData
 Opens the file and stores it in data
 =================================
 */
bool GetFileData( const char * fileName, unsigned char ** data, unsigned int & size ) {
	Initialize();

	char newFileName[ 2048 ];
	RelativePathToFullPath( fileName, newFileName );
	
	// open file for reading
	printf( "opening file: %s\n", newFileName );
	FILE * file = fopen( newFileName, "rb" );
	
	// handle any errors
	if ( file == NULL ) {
		printf("ERROR: open file failed: %s\n", newFileName );
		return false;
	}
	
	// get file size
	fseek( file, 0, SEEK_END );
	fflush( file );
	size = ftell( file );
	fflush( file );
	rewind( file );
	fflush( file );
	
	// output file size
	printf( "file size: %u\n", size );
	
	// create the data buffer
	*data = (unsigned char*)malloc( ( size + 1 ) * sizeof( unsigned char ) );
    
	// handle any errors
	if ( *data == NULL ) {
		printf( "ERROR: Could not allocate memory!\n" );
		fclose( file );
		return false;
	}

	// zero out the memory
	memset( *data, 0, ( size + 1 ) * sizeof( unsigned char ) );
	
	// read the data
	unsigned int bytesRead = fread( *data, sizeof( unsigned char ), size, file );
    fflush( file );
	printf( "total bytes read: %u\n", bytesRead );
    
    assert( bytesRead == size );
	
	// handle any errors
	if ( bytesRead != size ) {
		printf( "ERROR: reading file went wrong\n" );
		fclose( file );
		if ( *data != NULL ) {
			free( *data );
		}
		return false;
	}
	
	fclose( file );
	printf( "Read file was success\n");
	return true;	
}

/*
=================================
GetExtension
=================================
*/
std::string GetExtension( std::string path ) {
	if ( path.substr( path.find_last_of( "." ) + 1 ) == "tga" ) {
		return "tga";
	} else if  ( path.substr( path.find_last_of( "." ) + 1 ) == "hdr" ) {
		return "hdr";
	} else {
		return NULL;
	}
}

/*
=================================
dirExists
=================================
*/
bool dirExists( const char * dirName_in ) {
	DWORD ftyp = GetFileAttributesA( dirName_in );
	if ( ftyp == INVALID_FILE_ATTRIBUTES ) {
		return false; //something is wrong with your path!
	}

	if ( ftyp & FILE_ATTRIBUTE_DIRECTORY ) {
		return true; //this is a directory!
	}

	return false; //this is not a directory!
}

/*
=================================
makeDir
=================================
*/
void makeDir( const char * dirName ) {
	unsigned int len = strlen( dirName );
	char * buff;
	if ( dirName[len-1] == '\\' || dirName[len-1] == '/' ) {
		buff = new char[len];
	} else {
		len += 1;
		buff = new char[len];
		buff[len] = '\\';
	}
	for ( unsigned int i = 0; i < len; i++ ) {
		buff[i] = dirName[i];
	}

	unsigned int i = 0;
	while( i <= len - 1 ){
		if ( buff[i] == '\\' || buff[i] == '/' ) {
			if ( dirExists( buff ) == false ) {
				buff[i] = '\0';
				mkdir( buff );
			}
		}
		buff[i] = dirName[i];
		i += 1;
	}

	delete[] buff;
	buff = nullptr;
}