#pragma once
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

void findAndReplaceAll( std::string & data, std::string toSearch, std::string replaceStr );
bool GetFileData( const char * fileName, unsigned char ** data, unsigned int & size );
bool RelativePathToFullPath( const char * relativePath, char fullPath[ 2048 ] );
std::string GetExtension( std::string path );
bool dirExists( const char * dirName_in );
void makeDir( const char * dirName );