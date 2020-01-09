#include "Light.h"

unsigned int Light::s_lightCount = 0;
unsigned int Light::s_shadowCastingLightCount = 0;
Shader * Light::s_debugShader = new Shader();
Shader * Light::s_depthShader = new Shader();
Framebuffer Light::s_depthBufferAtlas = Framebuffer( "depthMap" );
unsigned int Light::s_partitionSize = s_depthBufferAtlasSize_min;

Mesh Light::s_debugModel;
Mesh DirectionalLight::s_debugModel_directional;
Mesh SpotLight::s_debugModel_spot;
Mesh PointLight::s_debugModel_point;

Texture EnvProbe::s_brdfIntegrationMap = Texture();

/*
================================
Light::Light
================================
*/
Light::Light() {
	//ensure all static members are initialized
	if ( s_debugShader->GetShaderProgram() == 0 ) {
		//vertex shader prog for debug light object
		const char * debug_vshader_source =
			"#version 330 core\n"
			"layout ( location = 0 ) in vec3 aPos;"
			"uniform mat4 view;"
			"uniform mat4 model;"
			"uniform mat4 projection;"
			"void main() {"
			"	gl_Position = projection * view * model * vec4( aPos.x, aPos.y, aPos.z, 1.0 );"
			"}";

		//fragment shader prog for debug light object
		const char * debug_fshader_source =
			"#version 330 core\n"
			"uniform vec3 color;"
			"out vec4 FragColor;"
			"void main() {"
			"	FragColor = vec4( color, 1.0 );"
			"}";

		s_debugShader->CompileShaderFromCSTR( debug_vshader_source, debug_fshader_source );
	}

	if ( s_depthShader->GetShaderProgram() == 0 ) {
		//vertex shader prog for depth map shader
		const char * depth_vshader_source =
			"#version 330 core\n"
			"layout ( location = 0 ) in vec3 aPos;"
			"layout ( location = 5 ) in mat4 model;"
			"uniform mat4 lightSpaceMatrix;"
			"void main() {"
			"	gl_Position = lightSpaceMatrix * model * vec4( aPos, 1.0 );"
			"}";

		//fragment shader prog for depth map shader
		const char * depth_fshader_source =
			"#version 330 core\n"
			"void main() {"
			"	gl_FragDepth = gl_FragCoord.z;"
			"}";

		s_depthShader->CompileShaderFromCSTR( depth_vshader_source, depth_fshader_source );
	}

	m_shadowIndex = -1;
	m_PosInShadowAtlas = Vec2();
}

/*
================================
Light::Initialize
	-Initialize members and shadow stuff
================================
*/
void Light::Initialize() {
	s_lightCount += 1; //increment lightcounter

	//initialize members
	m_shadowCaster = false;
	m_near_plane = 0.1f;
	m_far_plane = 10.0f;
	m_position = Vec3( 0.0f );
	m_color = Vec3( 1.0f );
	m_direction = Vec3( 1.0f, 0.0f, 0.0f );	
	m_xfrm = Mat4();	
}

/*
================================
Light::InitShadowAtlas
	-Creates the FBO that stores depth maps for shadowmapping
	-Gets called from the Scene class once all lights have been loaded into the scene.
================================
*/
void Light::InitShadowAtlas() {
	if ( s_depthBufferAtlas.GetID() == 0 ) {
		unsigned int atlasSize = 4;
		if ( s_shadowCastingLightCount > 0 ) {
			const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
			atlasSize = mapsPerRow * s_depthBufferAtlasSize_min; //how big the atles is if shadowmap sizes were ideal
			if ( atlasSize > s_depthBufferAtlasSize_max ) {
				s_partitionSize = ( unsigned int )( ( float )s_depthBufferAtlasSize_max / mapsPerRow );
				atlasSize = s_depthBufferAtlasSize_max;
			}
		}

		s_depthBufferAtlas.CreateNewBuffer( atlasSize, atlasSize, "debug_quad" );
		glDrawBuffer( GL_NONE ); //needed since there is no need for a color attachement
		glReadBuffer( GL_NONE ); //needed since there is no need for a color attachement
		s_depthBufferAtlas.AttachTextureBuffer( GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT );
		assert( s_depthBufferAtlas.Status() );
		s_depthBufferAtlas.Unbind();
		s_depthBufferAtlas.CreateScreen( 0, 0, 512, 512 );

		//set the boundary outside the texture border to a specific border color (white)
		glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas.m_attachements[0] );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
}

/*
================================
Light::DebugDraw
	-Draws debug light model and depth shadow atlas
================================
*/
void Light::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	assert(  s_depthBufferAtlas.GetID() != 0 );

	//***********************
	//Render debug light model
	//***********************
	//calculate model matrix
	Mat4 translation = Mat4();
	translation.Translate( m_position );

	Vec3 camToLight = ( camera->m_position - m_position );
	float dist = camToLight.length() * 0.05f;

	Mat4 scale = Mat4();
	for ( int i = 0; i < 3; i++ ) {
		scale[i][i] = dist;
	}

	Vec3 axis = Vec3( 1.0, 0.0, 0.0 );
	Vec3 crossVec = axis.cross( m_direction );
	if( crossVec.length() <= EPSILON ) {
		axis = Vec3( 1.0f, 1.0f, 0.0f );
		axis.normalize();
		crossVec = axis.cross( m_direction );
		crossVec.normalize();
	}
	Mat4 rotation = Mat4();
	if ( TypeIndex() != 3 ) { //point light
		rotation[0] = Vec4( crossVec, 0.0 );
		rotation[1] = Vec4( m_direction, 0.0 );
		rotation[2] = Vec4( m_direction.cross( crossVec ).normal(), 0.0 );
		rotation[3] = Vec4( 0.0, 0.0, 0.0, 1.0 );
	}
		
	const Mat4 model = translation * rotation * scale;

	//pass uniforms to shader
	s_debugShader->UseProgram();
	s_debugShader->SetUniformMatrix4f( "view", 1, false, view );
	s_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
	s_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );
	s_debugShader->SetUniform3f( "color", 1, m_color.as_ptr() );
		
	//render debug light model
	Mesh * debugModel = GetDebugMesh();
	glBindVertexArray( debugModel->m_surfaces[0]->VAO );
	glDrawElements( GL_TRIANGLES, debugModel->m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );


	//***********************
	//Render depth atlas
	//***********************
	//before drawing the texture to the buffer screen quad, the comparison mode used by shadow sampler needs to be disabled.
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas.m_attachements[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	//draw depth attachement to a quad on the screen
	s_depthBufferAtlas.Draw( s_depthBufferAtlas.m_attachements[0] );
}

/*
================================
Light::UpdateDepthBuffer
	-render the depth of the scene to lights piece of the depthbuffer
================================
*/
void Light::UpdateDepthBuffer( Scene * scene ) {
	assert(  s_depthBufferAtlas.GetID() != 0 );

	s_depthBufferAtlas.Bind();
	if ( m_shadowIndex == 0 ) {
		glClear( GL_DEPTH_BUFFER_BIT ); //Clear previous frame values
	}

	s_depthShader->UseProgram();
	s_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, LightMatrix() );

	//set rendering for this light's portion of the depthBufferAtlas
	const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
	float currentRow = floor( ( float )m_shadowIndex / mapsPerRow );
	unsigned int stepsUp = ( unsigned int )currentRow;
	unsigned int stepsRight = m_shadowIndex - ( unsigned int )( mapsPerRow * currentRow );
	glViewport( stepsRight * s_partitionSize, stepsUp * s_partitionSize, s_partitionSize, s_partitionSize );

	//store the position of this shadowMap in the shadowMapAtlas
	float x = ( float )stepsRight / ( float )mapsPerRow;
	float y = ( float )stepsUp / ( float )mapsPerRow;
	m_PosInShadowAtlas = Vec2( x, y );

	//iterate through each surface of each mesh in the scene and render it with m_depthShader active
	for ( int i = 0; i < scene->MeshCount(); i++ ) {			
		Mesh * mesh = NULL;
		scene->MeshByIndex( i, &mesh );
		for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
			mesh->DrawSurface( j );
		}
	}

	s_depthBufferAtlas.Unbind();
}

/*
================================
Light::SetShadows
================================
*/
void Light::SetShadow( const bool shadowCasting ) {	
	if ( shadowCasting && !m_shadowCaster ) {
		//give shadowcasting lights an index in the atlas
		m_shadowIndex = ( int )s_shadowCastingLightCount;
		s_shadowCastingLightCount += 1;
	} else if ( !shadowCasting && m_shadowCaster ) {
		m_shadowIndex = -1;
		s_shadowCastingLightCount -= 1;
	}
	m_shadowCaster = shadowCasting;
}

/*
================================
Light::PassDepthAttribute
	-Pass depth buffer to arg shader
================================
*/
void Light::PassDepthAttribute( Shader* shader, const unsigned int slot ) const {
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas.m_attachements[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	shader->SetAndBindUniformTexture( "shadowAtlas", slot, GL_TEXTURE_2D, s_depthBufferAtlas.m_attachements[0] );
}

/*
================================
DirectionalLight::DirectionalLight
================================
*/
DirectionalLight::DirectionalLight() {
	if ( s_debugModel_directional.m_name.Initialized() ) {
		s_debugModel_directional.LoadOBJFromFile( "data\\model\\system\\light\\directionalLight.obj" );
		s_debugModel_directional.LoadVAO( 0 );
	}
}

/*
================================
DirectionalLight::LightMatrix
	-update light matrix member and return pointer to it
================================
*/
const float * DirectionalLight::LightMatrix() {
	//set up orthographic projection
	Mat4 projection = Mat4();
	projection.Orthographic( -1.0f, 1.0f, -1.0f, 1.0f, m_near_plane, m_far_plane );

	//set up view matrix
	Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
	Vec3 right = m_direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_direction.cross( up );
	}
	up = right.cross( m_direction );
	Vec3 eye = m_position + m_direction;
	Mat4 view = Mat4();
	view.LookAt( m_position, eye, up );

	//update member
	m_xfrm = projection * view;

	return m_xfrm.as_ptr();
}

/*
================================
DirectionalLight::PassUniforms
================================
*/
void DirectionalLight::PassUniforms( Shader * shader, int idx ) const {
	char str[16];
	sprintf( str, "lights[%d].", idx );
	Str structPrefix = Str( str );

	const int typeIndex = ( int )( TypeIndex() );
	const float temp = 0.0f;
	const int shadowIdx = GetShadowIndex();

	shader->SetUniform1i( ( structPrefix + Str( "typeIndex" ) ).c_str(), 1, &typeIndex );
	shader->SetUniform3f( ( structPrefix + Str( "position" ) ).c_str(), 1, m_position.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "color" ) ).c_str(), 1, m_color.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "direction" ) ).c_str(), 1, m_direction.as_ptr() );	
	shader->SetUniform1f( ( structPrefix + Str( "angle" ) ).c_str(), 1, &temp ); //not needed
	shader->SetUniform1f( ( structPrefix + Str( "radius" ) ).c_str(), 1, &m_radius );
	shader->SetUniform1i( ( structPrefix + Str( "shadowIdx" ) ).c_str(), 1, &shadowIdx );

	if ( m_shadowIndex > -1 ) {
		char str2[16];
		sprintf( str2, "shadows[%d].", m_shadowIndex );
		Str shadowStructPrefix = Str( str2 );
		shader->SetUniformMatrix4f( ( shadowStructPrefix + Str( "matrix" ) ).c_str(), 1, false, m_xfrm.as_ptr() );
		shader->SetUniform2f( ( shadowStructPrefix + Str( "loc" ) ).c_str(), 1, m_PosInShadowAtlas.as_ptr() );
	}
}

/*
================================
DirectionalLight::DirectionalLight
================================
*/
SpotLight::SpotLight() {
	if ( s_debugModel_spot.m_name.Initialized() ) {
		s_debugModel_spot.LoadOBJFromFile( "data\\model\\system\\light\\spotLight.obj" );
		s_debugModel_spot.LoadVAO( 0 );
	}
	m_radius = 4.0;
	m_angle = to_radians( 90.0f );
}

/*
================================
SpotLight::LightMatrix
	-update light matrix member and return pointer to it
================================
*/
const float * SpotLight::LightMatrix() {
	//set up perspective projection
	const float aspect = 1.0f;
	Mat4 projection = Mat4();
	projection.Perspective( m_angle, aspect, m_near_plane, m_far_plane );

	//set up view matrix
	Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
	Vec3 right = m_direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_direction.cross( up );
	}
	up = right.cross( m_direction );
	Vec3 eye = m_position + m_direction;
	Mat4 view = Mat4();
	view.LookAt( m_position, eye, up );

	//update member
	m_xfrm = projection * view;

	return m_xfrm.as_ptr();
}

/*
================================
SpotLight::PassUniforms
================================
*/
void SpotLight::PassUniforms( Shader * shader, int idx ) const {
	char str[16];
	sprintf( str, "lights[%d].", idx );
	Str structPrefix = Str( str );

	const int typeIndex = ( int )( TypeIndex() );
	const float lightAngle_cosine = cos( m_angle / 2.0f ); //passing in cos so we dont have to do it in the fragment shader
	const int shadowIdx = GetShadowIndex();

	shader->SetUniform1i( ( structPrefix + Str( "typeIndex" ) ).c_str(), 1, &typeIndex );
	shader->SetUniform3f( ( structPrefix + Str( "position" ) ).c_str(), 1, m_position.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "color" ) ).c_str(), 1, m_color.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "direction" ) ).c_str(), 1, m_direction.as_ptr() );
	shader->SetUniform1f( ( structPrefix + Str( "angle" ) ).c_str(), 1, &lightAngle_cosine );
	shader->SetUniform1f( ( structPrefix + Str( "radius" ) ).c_str(), 1, &m_radius );	
	shader->SetUniform1i( ( structPrefix + Str( "shadowIdx" ) ).c_str(), 1, &shadowIdx );

	if ( m_shadowIndex > -1 ) {
		char str2[16];
		sprintf( str2, "shadows[%d].", m_shadowIndex );
		Str shadowStructPrefix = Str( str2 );
		shader->SetUniformMatrix4f( ( shadowStructPrefix + Str( "matrix" ) ).c_str(), 1, false, m_xfrm.as_ptr() );
		shader->SetUniform2f( ( shadowStructPrefix + Str( "loc" ) ).c_str(), 1, m_PosInShadowAtlas.as_ptr() );
	}
}

/*
================================
PointLight::PointLight
================================
*/
PointLight::PointLight() {
	if ( s_debugModel_point.m_name.Initialized() ) {
		s_debugModel_point.LoadOBJFromFile( "data\\model\\system\\light\\light.obj" );
		s_debugModel_point.LoadVAO( 0 );
	}
	m_radius = 4.0;
}

/*
================================
PointLight::SetShadow
================================
*/
void PointLight::SetShadow( const bool shadowCasting ) {	
	if ( shadowCasting && !m_shadowCaster ) {
		//give shadowcasting lights an index in the atlas
		m_shadowIndex = ( int )s_shadowCastingLightCount;
		s_shadowCastingLightCount += 6;
	} else if ( !shadowCasting && m_shadowCaster ) {
		m_shadowIndex = -1;
		s_shadowCastingLightCount -= 6;
	}
	m_shadowCaster = shadowCasting;
}

/*
================================
PointLight::LightMatrix
================================
*/
const float * PointLight::LightMatrix() {
	//set up perspective projection
	const float aspect = 1.0f;
	Mat4 projection = Mat4();
	projection.Perspective( to_radians( 91.0f ), aspect, m_near_plane, m_far_plane );

	Vec3 dirs[6];
	dirs[0] = Vec3( 1.0f, 0.0f, 0.0f );
	dirs[1] = Vec3( 0.0f, 0.0f, 1.0f );
	dirs[2] = Vec3( -1.0f, 0.0f, 0.0f );
	dirs[3] = Vec3( 0.0f, 0.0f, -1.0f );
	dirs[4] = Vec3( 0.0f, 1.0f, 0.0f );
	dirs[5] = Vec3( 0.0f, -1.0f, 0.0f );

	for ( unsigned int i = 0; i < 6; i++ ) {
		//set up view matrix
		Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
		Vec3 right = dirs[i].cross( up );
		if ( right.length() <= EPSILON ) {
			up = Vec3( 1.0f, 0.0f, 0.0f );
			right = dirs[i].cross( up );
		}
		up = right.cross( dirs[i] );
		Vec3 eye = m_position + dirs[i];
		Mat4 view = Mat4();
		view.LookAt( m_position, eye, up );

		//update member
		m_xfrms[i] = projection * view;
	}

	return m_xfrms[0].as_ptr();
}

/*
================================
PointLight::UpdateDepthBuffer
	-render the depth of the scene to lights piece of the depthbuffer
================================
*/
void PointLight::UpdateDepthBuffer( Scene * scene ) {
	assert(  s_depthBufferAtlas.GetID() != 0 );

	s_depthBufferAtlas.Bind();
	if ( m_shadowIndex == 0 ) {
		glClear( GL_DEPTH_BUFFER_BIT ); //Clear previous frame values
	}

	LightMatrix(); //update m_xfrms
	for ( unsigned int i = 0; i < 6; i++ ) {
		s_depthShader->UseProgram();
		s_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, m_xfrms[i].as_ptr() );

		//set rendering for this light's portion of the depthBufferAtlas
		const unsigned int shadowIdx = m_shadowIndex + i;
		const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
		float currentRow = floor( ( float )shadowIdx / mapsPerRow );
		unsigned int stepsDown = ( unsigned int )currentRow;
		unsigned int stepsRight = shadowIdx - ( ( unsigned int )mapsPerRow * stepsDown );
		glViewport( stepsRight * s_partitionSize, stepsDown * s_partitionSize, s_partitionSize, s_partitionSize );

		//iterate through each surface of each mesh in the scene and render it with m_depthShader active
		for ( int i = 0; i < scene->MeshCount(); i++ ) {			
			Mesh * mesh = NULL;
			scene->MeshByIndex( i, &mesh );
			for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
				mesh->DrawSurface( j );
			}
		}
	}

	s_depthBufferAtlas.Unbind();
}

/*
================================
PointLight::GetShadowMapLoc
================================
*/
const Vec2 PointLight::GetShadowMapLoc( unsigned int faceIdx ) const {
	const unsigned int shadowIdx = m_shadowIndex + faceIdx;
	const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
	float currentRow = floor( ( float )shadowIdx / mapsPerRow );
	unsigned int stepsUp = ( unsigned int )currentRow;
	unsigned int stepsRight = shadowIdx - ( unsigned int )( mapsPerRow * currentRow );

	float x = ( float )stepsRight / ( float )mapsPerRow;
	float y = ( float )stepsUp / ( float )mapsPerRow;
	return Vec2( x, y );
}

/*
================================
PointLight::PassUniforms
================================
*/
void PointLight::PassUniforms( Shader * shader, int idx ) const {
	char str[16];
	sprintf( str, "lights[%d].", idx );
	Str structPrefix = Str( str );

	const int typeIndex = ( int )( TypeIndex() );
	const float temp = 0.0f;
	const int shadowIdx = GetShadowIndex();

	shader->SetUniform1i( ( structPrefix + Str( "typeIndex" ) ).c_str(), 1, &typeIndex );
	shader->SetUniform3f( ( structPrefix + Str( "position" ) ).c_str(), 1, m_position.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "color" ) ).c_str(), 1, m_color.as_ptr() );
	shader->SetUniform3f( ( structPrefix + Str( "direction" ) ).c_str(), 1, m_direction.as_ptr() );
	shader->SetUniform1f( ( structPrefix + Str( "angle" ) ).c_str(), 1, &temp ); //not needed
	shader->SetUniform1f( ( structPrefix + Str( "radius" ) ).c_str(), 1, &m_radius );	
	shader->SetUniform1i( ( structPrefix + Str( "shadowIdx" ) ).c_str(), 1, &shadowIdx );

	if ( m_shadowIndex > -1 ) {
		for ( unsigned int i = 0; i < 6; i++ ) {
			char str2[16];
			sprintf( str2, "shadows[%d].", m_shadowIndex + i );
			Str shadowStructPrefix = Str( str2 );
			shader->SetUniformMatrix4f( ( shadowStructPrefix + Str( "matrix" ) ).c_str(), 1, false, m_xfrms[i].as_ptr() );
			const Vec2 loc = GetShadowMapLoc( i );
			shader->SetUniform2f( ( shadowStructPrefix + Str( "loc" ) ).c_str(), 1, loc.as_ptr() );
		}
	}
}

/*
================================
EnvProbe::EnvProbe
================================
*/
EnvProbe::EnvProbe() {
	m_position = Vec3();
	m_irradianceMap = CubemapTexture();
	m_environmentMap = CubemapTexture();
	m_meshCount = 0;

	if ( s_brdfIntegrationMap.m_empty ) {
		if ( !s_brdfIntegrationMap.InitFromFile( "data\\texture\\system\\brdfIntegrationLUT.hdr" ) ) {
			assert( false );
		}
	}
}

/*
================================
EnvProbe::EnvProbe
================================
*/
EnvProbe::EnvProbe( Vec3 pos ) {
	m_position = pos;
	m_irradianceMap = CubemapTexture();
	m_environmentMap = CubemapTexture();
	m_meshCount = 0;

	if ( s_brdfIntegrationMap.m_empty ) {
		if ( !s_brdfIntegrationMap.InitFromFile( "data\\texture\\system\\brdfIntegrationLUT.hdr" ) ) {
			assert( false );
		}
	}
}

/*
================================
EnvProbe::MeshByIndex
================================
*/
Mesh * EnvProbe::MeshByIndex( unsigned int index ) {
	if ( index >= m_meshCount ) {
		return NULL;
	}

	return m_meshes[ index ];
}

/*
================================
EnvProbe::MeshByIndex
================================
*/
bool EnvProbe::MeshByIndex( unsigned int index, Mesh ** obj ) {
	if ( index >= m_meshCount ) {
		return false;
	}

	*obj = ( m_meshes[index] );
	return true;
}

/*
================================
EnvProbe::BuildProbe
	-load cubemap texture files for this probe
================================
*/
bool EnvProbe::BuildProbe( unsigned int probeIdx ) {
	Scene * scene = Scene::getInstance();

	const Str scene_relativePath = scene->GetName();
	unsigned int idx = scene_relativePath.Find( "scenes" );

	char idx_str[5];
	itoa( probeIdx, idx_str, 10 );
	Str probe_suffix = Str( idx_str );

	Str probeName = Str( scene_relativePath.c_str() );
	probeName.Replace( "data\\scenes\\", "data\\generated\\envprobes\\", false );
	probeName = probeName.Substring( 0, probeName.Length() - 4 );

	//load env map and generate mips
	Str environment_relativePath = Str( probeName.c_str() );
	environment_relativePath.Append( "\\env_" );
	environment_relativePath.Append( probe_suffix );
	environment_relativePath.Append( ".hdr" );
	if ( m_environmentMap.InitFromFile( environment_relativePath.c_str() ) ) {
		m_environmentMap.PrefilterSpeculateProbe();
	} else {
		assert( m_irradianceMap.InitFromFile( "data\\texture\\system\\error_cube.tga" ) );
	}

	//load irradiance map
	Str irradiance_relativePath = environment_relativePath.Substring( 0, environment_relativePath.Length() - 4 );
	irradiance_relativePath.Append( "_irr.hdr" );
	if ( !m_irradianceMap.InitFromFile( irradiance_relativePath.c_str() ) ) {
		assert( m_irradianceMap.InitFromFile( "data\\texture\\system\\error_cube.tga" ) );
	}

	return true;
}

/*
================================
EnvProbe::PassUniforms
================================
*/
void EnvProbe::PassUniforms( Shader* shader, unsigned int slot ) const {
	shader->SetAndBindUniformTexture( "brdfLUT", slot, s_brdfIntegrationMap.GetTarget(), s_brdfIntegrationMap.GetName() );
	shader->SetAndBindUniformTexture( "irradianceMap", slot + 1, m_irradianceMap.GetTarget(), m_irradianceMap.GetName() );
	shader->SetAndBindUniformTexture( "prefilteredEnvMap", slot + 2, m_environmentMap.GetTarget(), m_environmentMap.GetName() );
}

/*
================================
EnvProbe::RenderCubemaps
================================
*/
std::vector< unsigned int > EnvProbe::RenderCubemaps( const unsigned int cubemapSize ) {
	glViewport( 0, 0, cubemapSize, cubemapSize ); //Set the OpenGL viewport to be the entire size of the window

	Scene * scene = Scene::getInstance();

	//create list of 6 FBOs. Once for each face of cube
	std::vector< Framebuffer > fbos;
	for ( unsigned int i = 0; i < 6; i++ ) {
		Framebuffer newFBO = Framebuffer( "screenTexture" );
		newFBO.CreateNewBuffer( cubemapSize, cubemapSize, "framebuffer" );
		newFBO.AttachTextureBuffer( GL_RGB16F, GL_COLOR_ATTACHMENT0, GL_RGB, GL_FLOAT );
		newFBO.AttachRenderBuffer( GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT );
		if ( !newFBO.Status() ) {
			assert( false );
		}
		fbos.push_back( newFBO );
	}

	//set projections for cubemap render
	Mat4 projection = Mat4();
	projection.Perspective( to_radians( 90.0f ), 1.0f, 0.01f, 100.0f );
	Mat4 views[] = { Mat4(), Mat4(), Mat4(), Mat4(), Mat4(), Mat4() };
	views[0].LookAt( m_position, Vec3( 1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[1].LookAt( m_position, Vec3( -1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[2].LookAt( m_position, Vec3( 0.0f, 1.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ) );
	views[3].LookAt( m_position, Vec3( 0.0f, -1.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ) );
	views[4].LookAt( m_position, Vec3( 0.0f, 0.0f, 1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
	views[5].LookAt( m_position, Vec3( 0.0f, 0.0f, -1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );

	//draw skybox
	glDisable( GL_BLEND );
	MaterialDecl* skyMat;
	Cube sceneSkybox = scene->GetSkybox();
	skyMat = MaterialDecl::GetMaterialDecl( sceneSkybox.m_surface->materialName.c_str() );
	skyMat->BindTextures();
	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
		fbos[faceIdx].Bind();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		const Mat4 skyBox_viewMat = views[faceIdx].as_Mat3().as_Mat4();
		skyMat->shader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );
		skyMat->shader->SetUniformMatrix4f( "view", 1, false, skyBox_viewMat.as_ptr() );
		sceneSkybox.DrawSurface( true );
		fbos[faceIdx].Unbind();
	}
	glEnable( GL_BLEND );
	
	//render the scene
	Light * light = NULL;
	for ( unsigned int i = 0; i < scene->LightCount(); i++ ) {
		scene->LightByIndex( i, &light );
		if ( light->GetShadow() ) {
			light->UpdateDepthBuffer( scene );
		}
	}

	//Draw the scene to the main buffer		
	glViewport( 0, 0, cubemapSize, cubemapSize ); //Set the OpenGL viewport to be the entire size of the window
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer

	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
		fbos[faceIdx].Bind(); //bind the framebuffer so all subsequent drawing is to it.
		glClear( GL_DEPTH_BUFFER_BIT ); //clear only depth because we wanna keep skybox color

		MaterialDecl* matDecl;
		for ( unsigned int i = 0; i < scene->MeshCount(); i++ ) {
			Mesh * mesh = NULL;
			scene->MeshByIndex( i, &mesh );

			for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
				matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[j]->materialName.c_str() );
				matDecl->BindTextures();

				//pass in camera data
				matDecl->shader->SetUniformMatrix4f( "view", 1, false, views[faceIdx].as_ptr() );		
				matDecl->shader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );
				matDecl->shader->SetUniform3f( "camPos", 1, m_position.as_ptr() );

				//exit early if this material is errored out
				if ( matDecl->m_shaderProg == "error" ) {
					mesh->DrawSurface( j ); //draw surface
					continue;
				}				

				//pass lights data
				for ( int k = 0; k < scene->LightCount(); k++ ) {
					if ( k == 0 ) {
						light->PassDepthAttribute( matDecl->shader, 4 );
						const int shadowMapPartitionSize = ( unsigned int )( light->s_partitionSize );
						matDecl->shader->SetUniform1i( "shadowMapPartitionSize", 1, &shadowMapPartitionSize );
					}
					scene->LightByIndex( k, &light );
					light->PassUniforms( matDecl->shader, k );
				}
				const int lightCount = ( int )( scene->LightCount() );
				matDecl->shader->SetUniform1i( "lightCount", 1, &lightCount );				
	
				//draw surface
				mesh->DrawSurface( j );
			}
		}
		fbos[faceIdx].Unbind();
	}

	//gather all texture ids from color attachments of the fbos
	std::vector< unsigned int > textureIDs;
	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {	
		textureIDs.push_back( fbos[faceIdx].m_attachements[0] );
	}
	return textureIDs;
}