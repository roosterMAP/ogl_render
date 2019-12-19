#pragma once
#include "Mesh.h"
#include "Fileio.h"
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <cstddef>

#include "lx_geometry_triangulation_utilities.h"

int mikk_getNumFaces( const SMikkTSpaceContext * pContext ) {
	surface * data = ( surface * )pContext->m_pUserData;
	return data->triCount;
}

int mikk_getNumVerticesOfFace( const SMikkTSpaceContext * pContext, const int iFace ) {
	return 3;
}

void mikk_getPosition( const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert ) {
	surface * data = ( surface * )pContext->m_pUserData;
	const tri_t tri = data->tris[iFace];
	if ( iVert == 0 ) {
		fvPosOut[0] = data->verts[tri.a].pos[0];
		fvPosOut[1] = data->verts[tri.a].pos[1];
		fvPosOut[2] = data->verts[tri.a].pos[2];
	} else if ( iVert == 1 ) {
		fvPosOut[0] = data->verts[tri.b].pos[0];
		fvPosOut[1] = data->verts[tri.b].pos[1];
		fvPosOut[2] = data->verts[tri.b].pos[2];
	} else {
		fvPosOut[0] = data->verts[tri.c].pos[0];
		fvPosOut[1] = data->verts[tri.c].pos[1];
		fvPosOut[2] = data->verts[tri.c].pos[2];
	}
}

void mikk_getNormal( const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert ) {
	surface * data = ( surface * )pContext->m_pUserData;
	const tri_t tri = data->tris[iFace];
	if ( iVert == 0 ) {
		fvNormOut[0] = data->verts[tri.a].norm[0];
		fvNormOut[1] = data->verts[tri.a].norm[1];
		fvNormOut[2] = data->verts[tri.a].norm[2];
	} else if ( iVert == 1 ) {
		fvNormOut[0] = data->verts[tri.b].norm[0];
		fvNormOut[1] = data->verts[tri.b].norm[1];
		fvNormOut[2] = data->verts[tri.b].norm[2];
	} else {
		fvNormOut[0] = data->verts[tri.c].norm[0];
		fvNormOut[1] = data->verts[tri.c].norm[1];
		fvNormOut[2] = data->verts[tri.c].norm[2];
	}

}

void mikk_getTexCoord( const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert ) {
	surface * data = ( surface * )pContext->m_pUserData;
	const tri_t tri = data->tris[iFace];
	if ( iVert == 0 ) {
		fvTexcOut[0] = data->verts[tri.a].uv[0];
		fvTexcOut[1] = data->verts[tri.a].uv[1];
	} else if ( iVert == 1 ) {
		fvTexcOut[0] = data->verts[tri.b].uv[0];
		fvTexcOut[1] = data->verts[tri.b].uv[1];
	} else {
		fvTexcOut[0] = data->verts[tri.c].uv[0];
		fvTexcOut[1] = data->verts[tri.c].uv[1];
	}
}

void mikk_setTSpaceBasic( const SMikkTSpaceContext * pContext, const float fvTangent[], const float tSign, const int iFace, const int iVert ) {
	surface * data = ( surface * )pContext->m_pUserData;
	const tri_t tri = data->tris[iFace];

	glm::vec3 tangent = glm::vec3();
	tangent[0] = fvTangent[0];
	tangent[1] = fvTangent[1];
	tangent[2] = fvTangent[2];

	if ( iVert == 0 ) {
		data->verts[tri.a].tang = tangent;
		data->verts[tri.a].tSign = tSign;
	} else if ( iVert == 1 ) {
		data->verts[tri.b].tang = tangent;
		data->verts[tri.a].tSign = tSign;
	} else {
		data->verts[tri.c].tang = tangent;
		data->verts[tri.a].tSign = tSign;
	}
}

 /*
 ================================
 Transform::WorldXfrm
 ================================
 */
bool Transform::WorldXfrm( glm::mat4x4* model ) {
	m_xfrm = glm::mat4x4( 1.0 );

	m_xfrm = glm::translate( m_xfrm, m_position );
	m_xfrm = m_xfrm * glm::mat4x4( m_rotation );
	m_xfrm = glm::scale( m_xfrm, m_scale );

	*model = m_xfrm;

	return true;
}

/*
================================
combine
	-concat a and b
================================
*/
unsigned int combine( unsigned int a, unsigned int b ) {
   int times = 1;
   while ( times <= b ) {
      times *= 10;
   }
   return a * times + b;
}

/*
================================
Mesh::LoadMSHFromFile
	-load vertex, face, and material name data from file
================================
*/
bool Mesh::LoadMSHFromFile( const char * msh_relative ) {
	//init m_bounds bbox
	m_bounds.min = glm::vec3( 99999999.9, 99999999.9, 99999999.9 );
	m_bounds.max = glm::vec3( -99999999.9, -99999999.9, -99999999.9 );

	m_name = new char[ 512 ];
	strcpy( m_name, msh_relative );

	char msh_absolute[ 2048 ];
	RelativePathToFullPath( m_name, msh_absolute );	
	FILE * fp;
	fopen_s( &fp, msh_absolute, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", msh_absolute );
		return false;
    }
	
	bool loadingSurface = false;
	unsigned int currentVertIndex = 0;
	unsigned int currentSurfaceIndex = 0;

	char buff[ 512 ] = { 0 };
	while ( !feof( fp ) ) {
		// Read whole line
		fgets( buff, sizeof( buff ), fp );
		Str line = Str( buff );
		line.Strip();

		if ( loadingSurface ) {
			if ( line.StartsWith( "}" ) ) {
				surface * currentSurface = m_surfaces[currentSurfaceIndex];

				//Generate Mikk Tangent Space and initialize tangents for verts in newSurface
				SMikkTSpaceInterface mikk_interface = { mikk_getNumFaces,
														mikk_getNumVerticesOfFace,
														mikk_getPosition,
														mikk_getNormal,
														mikk_getTexCoord,
														mikk_setTSpaceBasic };
				SMikkTSpaceContext pContext = { &mikk_interface, currentSurface };
				const bool tangentSpaceGenerated = ( bool )genTangSpaceDefault( &pContext );
				assert( tangentSpaceGenerated );

				currentSurfaceIndex += 1;
				currentVertIndex = 0;
				loadingSurface = false;

			} else if ( line.StartsWith( "v " ) ) {
				surface * currentSurface = m_surfaces[currentSurfaceIndex];
				std::vector<Str> splitLine = line.Split( ' ' );
				vert_t * currentVert = &( currentSurface->verts[currentVertIndex] );
				currentVertIndex += 1;
				currentVert->pos = glm::vec3( atof( splitLine[1].c_str() ), atof( splitLine[2].c_str() ), atof( splitLine[3].c_str() ) );
				currentVert->norm = glm::vec3( atof( splitLine[4].c_str() ), atof( splitLine[5].c_str() ), atof( splitLine[6].c_str() ) );
				currentVert->tang = glm::vec3();
				currentVert->uv = glm::vec2( atof( splitLine[7].c_str() ), atof( splitLine[8].c_str() ) );
				std::vector< tri_t * > surfaceTris;
				currentVert->tris = surfaceTris;

				//update m_bounds
				if ( currentVert->pos.x < m_bounds.min.x ) {
					m_bounds.min.x = currentVert->pos.x;
				}
				if ( currentVert->pos.y < m_bounds.min.y ) {
					m_bounds.min.y = currentVert->pos.y;
				}
				if ( currentVert->pos.z < m_bounds.min.z ) {
					m_bounds.min.z = currentVert->pos.z;
				}
				if ( currentVert->pos.x > m_bounds.max.x ) {
					m_bounds.max.x = currentVert->pos.x;
				}
				if ( currentVert->pos.y > m_bounds.max.y ) {
					m_bounds.max.y = currentVert->pos.y;
				}
				if ( currentVert->pos.z > m_bounds.max.z ) {
					m_bounds.max.z = currentVert->pos.z;
				}


			} else if ( line.StartsWith( "p " ) ) {
				surface * currentSurface = m_surfaces[currentSurfaceIndex];	
				std::vector<Str> splitLine = line.Split( ' ' );
				unsigned int polygon_vert_count = splitLine.size() - 1;
				unsigned int * polygon_vert_index_list = new unsigned int[polygon_vert_count];
				float * polygon_vert_pos_list = new float[polygon_vert_count * 3];
				for ( unsigned int i = 0; i < polygon_vert_count; i++ ) {
					unsigned int vIdx = atoi( splitLine[ i + 1 ].c_str() );
					polygon_vert_index_list[i] = vIdx;
					vert_t * tempVert = &( currentSurface->verts[vIdx] );
					polygon_vert_pos_list[ i * 3 + 0 ] = tempVert->pos.x;
					polygon_vert_pos_list[ i * 3 + 1 ] = tempVert->pos.y;
					polygon_vert_pos_list[ i * 3 + 2 ] = tempVert->pos.z;
				}

				std::vector<unsigned int> triIndexList = TriangulatePolygon( polygon_vert_index_list, polygon_vert_count, polygon_vert_pos_list );
				for ( unsigned int i = 0; i < triIndexList.size() / 3; i++ ) {
					//get vert indexes
					const unsigned int vidx1 = triIndexList[ i * 3 + 0 ];
					const unsigned int vidx2 = triIndexList[ i * 3 + 1 ];
					const unsigned int vidx3 = triIndexList[ i * 3 + 2 ];

					//add indexes to tri list (these three make up a triangle)
					tri_t newTri = { vidx1, vidx2, vidx3 };
					currentSurface->tris.push_back( newTri );
					currentSurface->triCount += 1;
				}

				//cleanup
				delete[] polygon_vert_index_list;
				delete[] polygon_vert_pos_list;
			}

		} else {
			if ( line.StartsWith( "h " ) ) {
				std::vector<Str> splitLine = line.Split( ' ' );
				const char * sourceFilePath = splitLine[1].c_str();
				const int surfaceCount =  atoi( splitLine[2].c_str() );
				for ( unsigned int i = 0; i < surfaceCount; i++ ) {
					m_surfaces.push_back( new surface );
				}

			} else if ( line.StartsWith( "s " ) ) {
				surface * currentSurface = m_surfaces[currentSurfaceIndex];				
				
				std::vector<Str> splitLine = line.Split( ' ' );
				currentSurface->VAO = 0;
				currentSurface->materialName = splitLine[1];
				std::vector< vert_t > surfaceVerts;
				currentSurface->verts = surfaceVerts;
				currentSurface->vCount = atoi( splitLine[2].c_str() );
				for ( unsigned int i = 0; i < currentSurface->vCount; i++ ) {
					currentSurface->verts.push_back( vert_t() );
				}
				std::vector< tri_t > surfaceTris;
				currentSurface->tris = surfaceTris;
				currentSurface->triCount = 0;

				loadingSurface = true;
			}
		}

		//clear buff before going to next line
		std::memset( buff, 0, strlen( buff ) );
	}
}

/*
================================
Mesh::LoadOBJFromFile
	-load vertex, face, and material name data from file
================================
*/
bool Mesh::LoadOBJFromFile( const char * obj_relative ) {
	//init m_bounds bbox
	m_bounds.min = glm::vec3( 99999999.9, 99999999.9, 99999999.9 );
	m_bounds.max = glm::vec3( -99999999.9, -99999999.9, -99999999.9 );

	m_name = new char[ 512 ];
	strcpy( m_name, obj_relative );

	char obj_absolute[ 2048 ];
	RelativePathToFullPath( m_name, obj_absolute );	
	FILE * fp;
	fopen_s( &fp, obj_absolute, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", obj_absolute );
		return false;
    }
	
	char buff[ 512 ] = { 0 };

	std::vector< glm::vec3 > pointList;
	std::vector< glm::vec2 > uvList;
	std::vector< glm::vec3 > normalList;

	glm::vec3 point;
	glm::vec3 norm;
	glm::vec2 uv;

	while ( !feof( fp ) ) {
		// Read whole line
		fgets( buff, sizeof( buff ), fp );
		
		if ( sscanf_s( buff, "v %f %f %f", &point.x, &point.y, &point.z ) == 3 ) { //load vertex
			// Add this point to the list of positions
			glm::vec3 tempPos = glm::vec3( point.x, point.y, point.z );
			pointList.push_back( tempPos );

			//update m_bounds
			if ( tempPos.x < m_bounds.min.x ) {
				m_bounds.min.x = tempPos.x;
			}
			if ( tempPos.y < m_bounds.min.y ) {
				m_bounds.min.y = tempPos.y;
			}
			if ( tempPos.z < m_bounds.min.z ) {
				m_bounds.min.z = tempPos.z;
			}
			if ( tempPos.x > m_bounds.max.x ) {
				m_bounds.max.x = tempPos.x;
			}
			if ( tempPos.y > m_bounds.max.y ) {
				m_bounds.max.y = tempPos.y;
			}
			if ( tempPos.z > m_bounds.max.z ) {
				m_bounds.max.z = tempPos.z;
			}

		} else if ( sscanf_s( buff, "vn %f %f %f", &norm.x, &norm.y, &norm.z ) == 3 ) { //load normal
			// Add this norm to the list of normals
			glm::vec3 tempNorm = glm::normalize( norm );
			normalList.push_back( tempNorm );

		} else if ( sscanf_s( buff, "vt %f %f", &uv.x, &uv.y ) == 2 ) { //load uv coord
			// Add this texture coordinate to the list of texture coordinates
			glm::vec2 tempUV = glm::vec2( uv.x, uv.y );
			uvList.push_back( tempUV );

		} else if ( buff[0] == 'f' ) { //load face
			//break up buffer string into an array of strings where each element is the data for a single vertex
			Str faceStr( buff );
			faceStr = faceStr.Substring( 2, faceStr.Length() );
			std::vector<Str> vertStrings;
			faceStr.Strip();
			vertStrings = faceStr.Split( ' ' );

			//create a blank array to store the unique vert indexes that make up the polygon.
			//the index is for the m_surfaceVerts member;
			unsigned int polygon_vert_count = vertStrings.size();
			unsigned int * polygon_vert_index_list = new unsigned int[polygon_vert_count];
			unsigned int * polygon_vID_list = new unsigned int[polygon_vert_count];
			float * polygon_vert_pos_list = new float[polygon_vert_count * 3];

			//retrieve the index of pre-existing verts, or create new verts and add them to m_surfaceVerts member
			//these indexes are stored in polygon_vert_index_list for triangulation and storage of generated m_surfaceTris.
			unsigned int m, n, o;
			for ( unsigned int i = 0; i < polygon_vert_count; i++ ) {
				if ( sscanf_s( vertStrings[i].c_str(), "%i/%i/%i", &m, &n, &o ) == 3 ) {
					
					//create a new vert by combining the vert component data
					const unsigned int vID = combine( combine( m, n ), o );
					std::vector< tri_t * > triPtrList;
					vert_t tempVert = {
						pointList[m-1],	//position
						normalList[o-1],//normal
						glm::vec3(),	//tangent
						uvList[n-1],	//uv
						1.0f,			//tSign
						triPtrList		//tris
					};

					//check if vID already exists within the current surface
					int existingVertIndex = -1;
					for ( unsigned int j = 0; j < m_surfaceVIDs.size(); j++ ) {
						if ( m_surfaceVIDs[j] == vID ) {
							existingVertIndex = ( int )j;
							break;
						}
					}					

					if ( existingVertIndex > -1 ) {
						//If a duplicate vert is detected, ensure its still unique within this current polygon being loaded
						bool dupeFound = false;
						for ( unsigned int j = 0; j < i; j++ ) {
							if ( vID == polygon_vID_list[j] ) {
								dupeFound = true;
								break;
							}
						}
						if ( dupeFound ) {
							//If a duplicate vert is detected within the list of verts for the polygon, then the polygon loops in on itself.
							//Example idx list: 0,1,2,3,4,5,3,2: a poly in the shape of a triangle with a hole cut out of it.
							//Modo's TriangulatePolygon method does not support this. So a new vert need to be created.
							//Just take the tempVert, and move its position a tiny bit in any direction
							const float low_range = -0.0001;
							const float high_range = 0.0001;
							const float rand_x = low_range + static_cast<float>( rand() ) / ( static_cast<float>( RAND_MAX / ( high_range - low_range ) ) );
							const float rand_y = low_range + static_cast<float>( rand() ) / ( static_cast<float>( RAND_MAX / ( high_range - low_range ) ) );
							const float rand_z = low_range + static_cast<float>( rand() ) / ( static_cast<float>( RAND_MAX / ( high_range - low_range ) ) );
							glm::vec3 rand_offset = glm::vec3( rand_x, rand_y, rand_z );
							tempVert.pos += rand_offset;
							
							//add new vert data to polygon and master list
							polygon_vert_index_list[i] = m_surfaceVerts.size();
							polygon_vID_list[i] = vID + 1;
							m_surfaceVIDs.push_back( vID + 1 ); //add the new vID to the list
							m_surfaceVerts.push_back( tempVert ); //add vert to master list
						} else {
							//add existing vert data to polygon
							polygon_vert_index_list[i] = existingVertIndex;
							polygon_vID_list[i] = vID;
						}

					} else {
						//add new vert data to polygon and master list
						polygon_vID_list[i] = vID;
						polygon_vert_index_list[i] = m_surfaceVerts.size();
						m_surfaceVIDs.push_back( vID ); //add the new vID to the list
						m_surfaceVerts.push_back( tempVert ); //add vert to master list
					}

				} else {
					printf( "MODEL LOADING ERROR :: INVALID FACE FORMAT! Should be pos/uv/norm!" );
					return false;
				}
			}

			//generate triangles
			for ( unsigned int i = 0; i < polygon_vert_count; i++ ) {
				for ( unsigned int j = 0; j < 3; j++ ) {
					polygon_vert_pos_list[i*3+j] = m_surfaceVerts[polygon_vert_index_list[i]].pos[j];
				}
			}
			std::vector<unsigned int> triIndexList;
			triIndexList = TriangulatePolygon( polygon_vert_index_list, polygon_vert_count, polygon_vert_pos_list );
			for ( unsigned int i = 0; i < triIndexList.size() / 3; i++ ) {
				//get vert indexes
				const unsigned int vidx1 = triIndexList[ i * 3 + 0 ];
				const unsigned int vidx2 = triIndexList[ i * 3 + 1 ];
				const unsigned int vidx3 = triIndexList[ i * 3 + 2 ];

				//add indexes to tri list (these three make up a triangle)
				tri_t newTri = { vidx1, vidx2, vidx3 };
				m_surfaceTris.push_back( newTri );
			}

			//cleanup
			delete[] polygon_vert_index_list;
			delete[] polygon_vert_pos_list;
			delete[] polygon_vID_list;

		} else if ( sscanf( buff, "usemtl  %s", &buff ) == 1 ) { //load material
			if ( m_materials.size() > 0 ) {
				AddSurface();
			}

			//get the material name and add it to the member list
			Str matName( buff );
			matName.ReplaceChar( '\r', '\0' );
			matName.ReplaceChar( '\n', '\0' );
			matName.Replace( "_092", "\\", true );
			matName.Replace( "_045", "-", true );
			m_materials.push_back( matName );
		}

		//clear buff before going to next line
		std::memset( buff, 0, strlen( buff ) );
	}

	//we try to add another surface before closing the file
	//this is because new m_surfaces only get created once the NEXT material is found.
	AddSurface();

	fclose( fp );

	if ( m_materials.size() < 1 ) {
		fprintf( stderr, "Error: \"%s\"!\n No m_materials present!", obj_absolute );
		return false;
	}

	return true;
}

/*
 ================================
 twoVertsShared
	-returns true if exactly two verts are shared between tri_a and tri_b
 ================================
 */
bool twoVertsShared( tri_t * tri_a, tri_t * tri_b ) {
	unsigned int sharedVertCount = 0;
	if ( tri_a->a == tri_b->a ) {
		sharedVertCount += 1;
	}
	if ( tri_a->a == tri_b->b ) {
		sharedVertCount += 1;
	}
	if ( tri_a->a == tri_b->c ) {
		sharedVertCount += 1;
	}
	if ( tri_a->b == tri_b->a ) {
		sharedVertCount += 1;
	}
	if ( tri_a->b == tri_b->b ) {
		sharedVertCount += 1;
	}
	if ( tri_a->b == tri_b->c ) {
		sharedVertCount += 1;
	}
	if ( tri_a->c == tri_b->a ) {
		sharedVertCount += 1;
	}
	if ( tri_a->c == tri_b->b ) {
		sharedVertCount += 1;
	}
	if ( tri_a->c == tri_b->c ) {
		sharedVertCount += 1;
	}
	return sharedVertCount == 2;
}

/*
 ================================
 triangleUVSign
	-returns <0.0 if tri has flipped uvs
 ================================
 */
float triangleUVSign( vert_t * m_surfaceVerts, tri_t * tri ) {
	glm::vec2 t_a_1 = m_surfaceVerts[ tri->a ].uv;
	glm::vec2 t_a_2 = m_surfaceVerts[ tri->b ].uv;
	glm::vec2 t_a_3 = m_surfaceVerts[ tri->c ].uv;
	float sign = ( ( t_a_2.x - t_a_1.x ) * ( t_a_3.y - t_a_1.y ) ) - ( ( t_a_2.y - t_a_1.y ) * ( t_a_3.x - t_a_1.x ) );
	return sign;
}

/*
 ================================
 triangleUVSign
	-returns true if both tris have flipped uvs or both have non-flipped uvs.
 ================================
 */
bool compareTriangleUVSign( vert_t * m_surfaceVerts, tri_t * tri_a, tri_t * tri_b ) {
	float sign_a = triangleUVSign( m_surfaceVerts, tri_a );
	float sign_b = triangleUVSign( m_surfaceVerts, tri_a );

	if ( sign_a > 0.0 && sign_b > 0.0 ) {
		return true;
	} else if ( sign_a < 0.0 && sign_b < 0.0 ) {
		return true;
	} else if ( sign_a == 0.0 && sign_b == 0.0 ) {
		return true;
	} else {
		return false;
	}
}

/*
 ================================
 Mesh::AddSurface
	-A surface is a group of triangles withint a mesh that all share the same material.
	-A surface can be drawn in a single draw call.
	-This function adds a new surface to the m_surfaces member and clears intermediary members.
 ================================
 */
void Mesh::AddSurface() {
	if( m_surfaceVerts.size() >= 3 ) {
		//create and init new surface
		surface * newSurface = new surface;
		newSurface->materialName = m_materials[ m_materials.size() - 1 ];
		newSurface->verts = m_surfaceVerts;
		newSurface->vCount = m_surfaceVerts.size();
		newSurface->tris = m_surfaceTris;
		newSurface->triCount = m_surfaceTris.size();

		//Generate Mikk Tangent Space and initialize tangents for verts in newSurface
		SMikkTSpaceInterface mikk_interface = { mikk_getNumFaces,
												mikk_getNumVerticesOfFace,
												mikk_getPosition,
												mikk_getNormal,
												mikk_getTexCoord,
												mikk_setTSpaceBasic };
		SMikkTSpaceContext pContext = { &mikk_interface, newSurface };
		const bool tangentSpaceGenerated = ( bool )genTangSpaceDefault( &pContext );
		assert( tangentSpaceGenerated );

		//clear intermediary member
		m_surfaceVIDs.clear();
		m_surfaceVerts.clear();
		m_surfaceTris.clear();

		//add new surface
		m_surfaces.push_back( newSurface );
	}
}

/*
 ================================
 Mesh::LoadVAO
 ================================
 */
unsigned int Mesh::LoadVAO( const unsigned int surfaceIdx ) {
	//create VAO to bind/configure the corresponding VBO(s) and attribute pointer(s)
	surface * currentSurface = m_surfaces[surfaceIdx];	

	//create buffers
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &VBO );
	glGenBuffers( 1, &EBO );	
	
	//put openGL in the state to bind/configure the VAO FIRST
	glBindVertexArray( VAO );

	//create BVO
	glBindBuffer( GL_ARRAY_BUFFER, VBO ); //bind it to GL_ARRAY_BUFFER target. This effectively sets the buffer type.
	glBufferData( GL_ARRAY_BUFFER, currentSurface->vCount * sizeof( vert_t ), &currentSurface->verts[0], GL_STATIC_DRAW ); //load vert data into it as static data (wont change)

	//create EBO for indexed drawing of m_surfaceTris
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, currentSurface->triCount * sizeof( tri_t ), &currentSurface->tris[0], GL_STATIC_DRAW );

	//Each vertex attribute takes its data from memory managed by a VBO.
	//Since the previously defined VBO is still bound before calling glVertexAttribPointer vertex attribute 0 is now associated with its(the VBOs) vertex data.
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )0 ); //position
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, norm ) ); //normal
	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, uv ) ); //uvs

	//unbind VBO and VAO
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ); remember: do NOT unbind the EBO while a VAO is active because the bound element buffer object IS stored in the VAO. keep the EBO bound.
	glBindVertexArray( 0 );

	currentSurface->VAO = VAO;

	return VAO;
}

/*
 ================================
 Mesh::DrawSurface
 ================================
 */
void Mesh::DrawSurface( unsigned int surfaceIdx ) {
	assert( surfaceIdx < m_surfaces.size() );
	glBindVertexArray( m_surfaces[surfaceIdx]->VAO );
	glDrawElementsInstanced( GL_TRIANGLES, m_surfaces[surfaceIdx]->triCount * 3, GL_UNSIGNED_INT, 0, m_transforms.size() );
	glBindVertexArray( 0 );
}


/*
 ================================
 Cube::Cube
 ================================
 */
Cube::Cube( Str matName ) {
	m_surface = new surface;
	m_surface->VAO = LoadVAO();
	m_surface->triCount = 6; //is case of Cube, triCount is more of a quadCount
	m_surface->materialName = matName;
	if( !matName.IsEmpty() ) { //allow cubes with no materials
		LoadDecl();
	}
}

/*
 ================================
 Cube::LoadDecl
 ================================
 */
bool Cube::LoadDecl() {
	MaterialDecl * matDecl = MaterialDecl::GetMaterialDecl( m_surface->materialName.c_str() );
	if ( matDecl == NULL ) {
		return false;
	}
	if( matDecl->CompileShader() ) {
		matDecl->BindTextures();
	} else {
		return false;
	}
	return true;
}

/*
 ================================
 Cube::LoadVAO
 ================================
 */
unsigned int Cube::LoadVAO() {
	float verts[] = {
		//positions
		 1.0f,  1.0f, -1.0f, //back
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f,  1.0f,  1.0f, //left
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f,  1.0f, -1.0f, //right
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,

		 1.0f, -1.0f,  1.0f, //front
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f,  1.0f, //up
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		
		-1.0f, -1.0f, -1.0f, //down
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f
	};

	//pass to GPU
	unsigned int VAO, VBO;
	glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &VBO );
	glBindVertexArray( VAO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( verts ), &verts, GL_STATIC_DRAW );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void * )0 );
	return VAO;
}

/*
 ================================
 Cube::DrawSurface
 ================================
 */
void Cube::DrawSurface( bool flip ) {
	if ( flip ) {
		glFrontFace( GL_CW );
	}
	glBindVertexArray( m_surface->VAO );
	glDrawArrays( GL_QUADS, 0, m_surface->triCount * 4 );
	glBindVertexArray( 0 );
	glFrontFace( GL_CCW );
}