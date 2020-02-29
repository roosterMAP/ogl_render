#include "Scene.h"
#include "Fileio.h"

/*
================================
Console::getInstance
================================
*/
Scene * Scene::getInstance() {
   if ( inst_ == NULL ) {
      inst_ = new Scene();
   }
   return( inst_ );
}

Scene * Scene::inst_ = NULL; //Define the static Singleton pointer

/*
================================
Scene::MeshByIndex
================================
*/
Mesh * Scene::MeshByIndex( unsigned int index ) {
	if ( index >= m_meshCount ) {
		return NULL;
	}

	return m_meshes[ index ];
}

/*
================================
Scene::MeshByIndex
================================
*/
bool Scene::MeshByIndex( unsigned int index, Mesh ** obj ) {
	if ( index >= m_meshCount ) {
		return false;
	}

	*obj = ( m_meshes[index] );
	return true;
}

/*
================================
Scene::AddLight
================================
*/
const unsigned int Scene::AddLight( Light * light ) {
	m_lightCount += 1;
	m_lights[ m_lightCount - 1 ] = light;
	return m_lightCount - 1;
}

/*
================================
Scene::LightByIndex
================================
*/
bool Scene::LightByIndex( unsigned int index, Light ** obj ) {
	if ( index >= m_lightCount ) {
		return false;
	}

	*obj = m_lights[index];
	return true;
}

/*
================================
Scene::EnvProbeByIndex
================================
*/
bool Scene::EnvProbeByIndex( unsigned int index, EnvProbe ** obj ) {
	if ( index >= m_envProbeCount ) {
		return false;
	}

	*obj = m_envProbes[index];
	return true;
}

/*
================================
Scene::LoadFromFile
	-Function is responsible for loading the contents of a scene into memory
	-Light object are created and their attributes initialized.
	-Each unique mesh is loaded into a Mesh object. Instances are stored as transforms as a mesh member.
	-Once everything is loaded, VAOs are created and geometry is passed to the GPU.
================================
*/
bool Scene::LoadFromFile( const char * scn_relative ) {
	bool sceneLoadSuccess = true;

	char scn_absolute[ 2048 ];
	RelativePathToFullPath( scn_relative, scn_absolute );	
	
	FILE * fp;
	fopen_s( &fp, scn_absolute, "rb" );
	if ( !fp ) {
		fprintf( stderr, "Error: couldn't open \"%s\"!\n", scn_absolute );
		return false;
    }
	m_name = Str( scn_relative );
	
	char buff[ 512 ] = { 0 };

	bool loadingMeshEntity = false;
	bool loadingSpotLightEntity = false;
	bool loadingDirectionalLightEntity = false;
	bool loadingPointLightEntity = false;
	bool loadingAmbientLightEntity = false;
	bool loadingEnvProbeEntity = false;

	Transform * currentTransform;
	Mesh* currentMesh;
	Light* currentLight;
	EnvProbe* currentEnvProbe;

	int intVal;
	float val1;
	Vec3 val3;
	Mat3 val9;

	while ( !feof( fp ) ) {
		// Read whole line
		fgets( buff, sizeof( buff ), fp );

		if ( loadingMeshEntity ) { //load mesh data
			if ( strncmp( buff, "}", 1 ) == 0 ) {
				loadingMeshEntity = false;
			} else if ( sscanf_s( buff, "\tpos %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) {
				currentTransform->SetPosition( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\trot %f %f %f %f %f %f %f %f %f", &val9[0][0], &val9[0][1], &val9[0][2], &val9[1][0], &val9[1][1], &val9[1][2], &val9[2][0], &val9[2][1], &val9[2][2] ) == 9 ) {
				currentTransform->SetRotation( Mat3( val9 ) );
			} else if ( sscanf_s( buff, "\tscl %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) {
				currentTransform->SetScale( Vec3( val3 ) );
			}
		} else if ( loadingSpotLightEntity ) { //load spotlight data
			if ( strncmp( buff, "}", 1 ) == 0 ) {
				loadingSpotLightEntity = false;
			} else if ( sscanf_s( buff, "\tcol %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //color
				currentLight->SetColor( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tpos %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //position
				currentLight->SetPosition( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tdir %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //direction
				currentLight->SetDirection( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tang %f", &val1 ) == 1 ) { //inner angle
				SpotLight* tempSpotLight = ( SpotLight* )currentLight;
				tempSpotLight->SetAngle( val1 );
			} else if ( sscanf_s( buff, "\tsiz %f", &val1 ) == 1 ) { //radius
				currentLight->SetRadius( val1 );
			} else if ( sscanf_s( buff, "\tmxr %f", &val1 ) == 1 ) { //max_radius
				currentLight->SetMaxRadius( val1 );
			} else if ( sscanf_s( buff, "\tbrt %f", &val1 ) == 1 ) { //brightness
				currentLight->SetBrightness( val1 );
			} else if ( sscanf_s( buff, "\tsha %d", &intVal ) == 1 ) { //shadow
				if ( intVal == 1 ) {
					currentLight->SetShadow( true );
				}
			}
		} else if ( loadingDirectionalLightEntity ) { //load directionallight data
			if ( strncmp( buff, "}", 1 ) == 0 ) {
				loadingDirectionalLightEntity = false;
			} else if ( sscanf_s( buff, "\tcol %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //color
				currentLight->SetColor( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tpos %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //position
				currentLight->SetPosition( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tdir %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //direction
				currentLight->SetDirection( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tsiz %f", &val1 ) == 1 ) { //radius
				currentLight->SetRadius( val1 );
			} else if ( sscanf_s( buff, "\tmxr %f", &val1 ) == 1 ) { //max_radius
				currentLight->SetMaxRadius( val1 );
			} else if ( sscanf_s( buff, "\tdrr %f", &val1 ) == 1 ) { //dir_radius
				DirectionalLight* tempDirectionalLight = ( DirectionalLight* )currentLight;
				tempDirectionalLight->SetDirRadius( val1 );
			} else if ( sscanf_s( buff, "\tbrt %f", &val1 ) == 1 ) { //brightness
				currentLight->SetBrightness( val1 );
			} else if ( sscanf_s( buff, "\tsha %d", &intVal ) == 1 ) { //shadow
				if ( intVal == 1 ) {
					currentLight->SetShadow( true );
				}
			}
		} else if ( loadingPointLightEntity ) { //load pointlight data
			if ( strncmp( buff, "}", 1 ) == 0 ) {
				loadingPointLightEntity = false;
			} else if ( sscanf_s( buff, "\tcol %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //color
				currentLight->SetColor( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tpos %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //position
				currentLight->SetPosition( Vec3( val3 ) );
			} else if ( sscanf_s( buff, "\tsiz %f", &val1 ) == 1 ) { //radius
				currentLight->SetRadius( val1 );
			} else if ( sscanf_s( buff, "\tmxr %f", &val1 ) == 1 ) { //max_radius
				currentLight->SetMaxRadius( val1 );
			} else if ( sscanf_s( buff, "\tbrt %f", &val1 ) == 1 ) { //brightness
				currentLight->SetBrightness( val1 );
			} else if ( sscanf_s( buff, "\tsha %d", &intVal ) == 1 ) { //shadow
				if ( intVal == 1 ) {
					currentLight->SetShadow( true );
				}
			}
		} else if ( loadingEnvProbeEntity ) { //load environment probe data
			if ( strncmp( buff, "}", 1 ) == 0 ) {
				loadingEnvProbeEntity = false;
			} else if ( sscanf_s( buff, "\tpos %f %f %f", &val3.x, &val3.y, &val3.z ) == 3 ) { //position
				currentEnvProbe->SetPosition( Vec3( val3 ) );
			}
		}

		if ( sscanf( buff, "mesh  %s.obj {", &buff ) == 1 ) { //mesh entity header
			//check to see if mesh already exists
			bool alreadyExists = false;
			for( unsigned int i = 0; i < m_meshCount; i++ ) {
				currentMesh = m_meshes[i];
				if ( strcmp( currentMesh->m_name.c_str(), buff ) == 0 ) {
					alreadyExists = true;
					break;
				}
			}

			//if it doesnt exist, then create a new one
			if ( alreadyExists == false ) {
				Str line = Str( buff );
				line.Strip();
				currentMesh = new Mesh();
				bool meshLoaded = false;
				if ( line.EndsWith( ".obj" ) || line.EndsWith( ".OBJ" ) ) {				
					meshLoaded = currentMesh->LoadOBJFromFile( buff );
				} else if ( line.EndsWith( ".msh" ) || line.EndsWith( ".MSH" ) ) {
					meshLoaded = currentMesh->LoadMSHFromFile( buff );
				}
				assert( meshLoaded );
				m_meshes[ m_meshCount ] = currentMesh;
				m_meshCount += 1;
			}

			//create transform for instance and add to mesh resource
			currentTransform = new Transform();
			currentMesh->m_transforms.push_back( currentTransform );			
			
			loadingMeshEntity = true;

		} else if ( strncmp( buff, "spotlight {", 11 ) == 0 ) { //spoptlight entity header
			m_lightCount += 1;
			m_lights[ m_lightCount - 1 ] = new SpotLight();
			currentLight = m_lights[ m_lightCount - 1 ];
			currentLight->m_idx = m_lightCount - 1;
			currentLight->Initialize();
			loadingSpotLightEntity = true;
		} else if ( strncmp( buff, "directionallight {", 18 ) == 0 ) { //directionalight entity header
			m_lightCount += 1;
			m_lights[ m_lightCount - 1 ] = new DirectionalLight();
			currentLight = m_lights[ m_lightCount - 1 ];
			currentLight->m_idx = m_lightCount - 1;
			currentLight->Initialize();
			loadingDirectionalLightEntity = true;
		} else if ( strncmp( buff, "pointlight {", 12 ) == 0 ) { //pointlight entity header
			m_lightCount += 1;
			m_lights[ m_lightCount - 1 ] = new PointLight();
			currentLight = m_lights[ m_lightCount - 1 ];
			currentLight->Initialize();
			loadingPointLightEntity = true;
		} else if ( strncmp( buff, "envProbe {", 10 ) == 0 ) { //envProbe entity header
			m_envProbeCount += 1;
			m_envProbes[ m_envProbeCount - 1 ] = new EnvProbe();
			currentEnvProbe = m_envProbes[ m_envProbeCount - 1 ];
			loadingEnvProbeEntity = true;
		}

		//clear buff before going to next line
		std::memset( buff, 0, strlen( buff ) );
	}
	fclose( fp );

	//pass models and instances to the GPU
	LoadVAOs();

	//build shadowmap atlas for shadowcasting lights
	Light::InitShadowAtlas();

	//build env probes
	BuildProbes();

	return true;
}

/*
================================
Scene::LoadVAOs
================================
*/
void Scene::LoadVAOs() {
	for ( int i = 0; i < MeshCount(); i++ ) {
		Mesh * currentMesh = MeshByIndex( i );

		//build list of transforms for each instance
		const unsigned int instanceCount = currentMesh->m_transforms.size();
		Mat4* instanceXfrms;
		instanceXfrms = new Mat4[instanceCount];
		for ( unsigned int j = 0; j < instanceCount; j++ ) {
			Transform * currentTransform = currentMesh->m_transforms[j];
			Mat4 newXfrm;
			currentTransform->WorldXfrm( &newXfrm );
			instanceXfrms[j] = newXfrm;
		}

		//pass each surface (one instance per transform) to the GPU
		for ( unsigned int j = 0; j < currentMesh->m_surfaces.size(); j++ ) {
			surface * currentSurface = currentMesh->m_surfaces[j];		

			//create VAO to bind/configure the corresponding VBO(s) and attribute pointer(s)
			unsigned int VAO, VBO, EBO;
			glGenVertexArrays( 1, &VAO );
			glGenBuffers( 1, &VBO );
			glGenBuffers( 1, &EBO );	
	
			//put openGL in the state to bind/configure the VAO FIRST
			glBindVertexArray( VAO );

			//create BVO
			glBindBuffer( GL_ARRAY_BUFFER, VBO ); //bind it to GL_ARRAY_BUFFER target. This effectively sets the buffer type.
			glBufferData( GL_ARRAY_BUFFER, currentSurface->vCount * sizeof( vert_t ), &currentSurface->verts[0], GL_STATIC_DRAW ); //load vert data into it as static data (wont change)

			//create EBO for indexed drawing of tris
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, currentSurface->triCount * sizeof( tri_t ), &currentSurface->tris[0], GL_STATIC_DRAW );

			//Each vertex attribute takes its data from memory managed by a VBO.
			//Since the previously defined VBO is still bound before calling glVertexAttribPointer vertex attribute 0 is now associated with its(the VBOs) vertex data.
			glEnableVertexAttribArray( 0 );
			glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )0 ); //position
			glEnableVertexAttribArray( 1 );
			glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, norm ) ); //normal
			glEnableVertexAttribArray( 2 );
			glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, tang ) ); //tangent
			glEnableVertexAttribArray( 3 );
			glVertexAttribPointer( 3, 2, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, uv ) ); //uvs
			glEnableVertexAttribArray( 4 );
			glVertexAttribPointer( 4, 1, GL_FLOAT, GL_FALSE, sizeof( vert_t ), ( void* )offsetof( vert_t, tSign ) ); //fSign

			//configure instanced array
			unsigned int transfomrBuffer;
			glGenBuffers( 1, &transfomrBuffer );
			glBindBuffer( GL_ARRAY_BUFFER, transfomrBuffer );
			glBufferData( GL_ARRAY_BUFFER, instanceCount * sizeof( Mat4 ), &instanceXfrms[0], GL_STATIC_DRAW );

			//set transformation matrices as an instance vertex attribute for the currently bound VAO
			glEnableVertexAttribArray( 5 );
			glVertexAttribPointer( 5, 4, GL_FLOAT, GL_FALSE, sizeof( Mat4 ), ( void* )0 );
			glEnableVertexAttribArray( 6 );
			glVertexAttribPointer( 6, 4, GL_FLOAT, GL_FALSE, sizeof( Mat4 ), ( void* )( sizeof( Vec4 ) ) );
			glEnableVertexAttribArray( 7 );
			glVertexAttribPointer( 7, 4, GL_FLOAT, GL_FALSE, sizeof( Mat4 ), ( void* )( 2 * sizeof( Vec4 ) ) );
			glEnableVertexAttribArray( 8 );
			glVertexAttribPointer( 8, 4, GL_FLOAT, GL_FALSE, sizeof( Mat4 ), ( void* )( 3 * sizeof( Vec4 ) ) );

			//use divisor 1 cuz we want to update the content of the vertex attribute when we start to render a new instance
			glVertexAttribDivisor( 5, 1 );
			glVertexAttribDivisor( 6, 1 );
			glVertexAttribDivisor( 7, 1 );
			glVertexAttribDivisor( 8, 1 );

			//unbind VBO and VAO
			glBindBuffer( GL_ARRAY_BUFFER, 0 );
			//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ); remember: do NOT unbind the EBO while a VAO is active because the bound element buffer object IS stored in the VAO. keep the EBO bound.
			glBindVertexArray( 0 );

			currentSurface->VAO = VAO; //init surface VAO member
		}
	}
}

/*
================================
Scene::BuildProbes
	-associate all meshes with one envProbe via the EnvProbe::AddMesh function.
	-call EnvProbe::BuildProbe function.
================================
*/
void Scene::BuildProbes() {
	if ( m_envProbeCount < 1 ) {
		m_envProbeCount += 1;
		m_envProbes[ m_envProbeCount - 1 ] = new EnvProbe();
	}

	//associate all meshes with one envProbe
	for ( unsigned int i = 0; i < m_meshCount; i++ ) {
		Mesh * mesh = m_meshes[i];

		const bbox meshBounds = mesh->GetBounds();
		const Vec3 meshCenter = ( meshBounds.min + meshBounds.max ) / 2.0f;

		unsigned int nearestProbeIdx = 0;
		float minDist = 99999999.0;
		for ( unsigned int j = nearestProbeIdx; j < m_envProbeCount; j++ ) {
			EnvProbe * probe = m_envProbes[j];
			const float dist = ( meshCenter - probe->GetPosition() ).length();
			if ( dist < minDist ) {
				minDist = dist;
				nearestProbeIdx = j;
			}
		}
		EnvProbe * nearestProbe = m_envProbes[nearestProbeIdx];
		nearestProbe->AddMesh( mesh );
		mesh->SetProbe( nearestProbe );
	}

	//call EnvProbe::BuildProbe function which fetches env and irradiance cubemap images and generates specular mips.
	for ( unsigned int i = 0; i < m_envProbeCount; i++ ) {
		EnvProbe * probe = m_envProbes[i];
		probe->BuildProbe( i );
	}
}