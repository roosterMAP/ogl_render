#include "Light.h"

unsigned int Light::s_lightCount = 0;
unsigned int Light::s_shadowCastingLightCount = 0;
Shader * Light::s_debugShader = new Shader();
Shader * Light::s_depthShader = new Shader();
Framebuffer * Light::s_depthBufferAtlas = new Framebuffer( "depthMap" );
unsigned int Light::s_partitionSize = s_depthBufferAtlasSize_min;

float Light::s_lightAttenuationBias = 0.005f;

Mesh * Light::s_debugModel = new Mesh();
Mesh * DirectionalLight::s_debugModel_directional = new Mesh();
Mesh * SpotLight::s_debugModel_spot = new Mesh();
Mesh * PointLight::s_debugModel_point = new Mesh();

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

	m_uniformBlock = LightStorage();	
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
	m_xfrm = Mat4();
	m_uniformBlock.position = Vec3( 0.0f );
	m_uniformBlock.color = Vec3( 1.0f );
	m_uniformBlock.direction = Vec3( 1.0f, 0.0f, 0.0f );
	m_uniformBlock.angle = cos( to_radians( 90.0f ) / 2.0f );
	m_uniformBlock.light_radius = 0.5f;	
	m_uniformBlock.brightness = 1.0f;
	m_uniformBlock.max_radius = MaxAttenuationDist();
	m_uniformBlock.dir_radius = m_uniformBlock.max_radius / 2.0f;
	m_uniformBlock.shadowIdx = -1;
	m_boundsUniformBlock = LightEffectStorage();
	lightEffectStorage_cached = false;
}

/*
================================
Light::InitShadowAtlas
	-Creates the FBO that stores depth maps for shadowmapping
	-Gets called from the Scene class once all lights have been loaded into the scene.
================================
*/
void Light::InitShadowAtlas() {
	if ( s_depthBufferAtlas->GetID() == 0 ) {
		unsigned int atlasSize = 4;
		if ( s_shadowCastingLightCount > 0 ) {
			const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
			atlasSize = mapsPerRow * s_depthBufferAtlasSize_min; //how big the atles is if shadowmap sizes were ideal
			if ( atlasSize > s_depthBufferAtlasSize_max ) {
				s_partitionSize = ( unsigned int )( ( float )s_depthBufferAtlasSize_max / mapsPerRow );
				atlasSize = s_depthBufferAtlasSize_max;
			}
		}

		s_depthBufferAtlas->CreateNewBuffer( atlasSize, atlasSize, "debug_quad" );

		//pin the depthbuffer shader so that it doesnt get removed when scene is reloaded.
		Shader * depthBufferShader = s_depthBufferAtlas->GetShader();
		depthBufferShader->PinShader();

		glDrawBuffer( GL_NONE ); //needed since there is no need for a color attachement
		glReadBuffer( GL_NONE ); //needed since there is no need for a color attachement
		s_depthBufferAtlas->AttachTextureBuffer( GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT );
		assert( s_depthBufferAtlas->Status() );
		s_depthBufferAtlas->Unbind();
		s_depthBufferAtlas->CreateScreen( 0, 0, 512, 512 );

		//set the boundary outside the texture border to a specific border color (white)
		glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas->m_attachements[0] );
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
void Light::DebugDraw( Camera * camera, const float * view, const float * projection, int mode ) {
	assert(  s_depthBufferAtlas->GetID() != 0 );

	//***********************
	//Render debug light model
	//***********************
	//calculate model matrix
	Mat4 translation = Mat4();
	translation.Translate( m_uniformBlock.position );

	Vec3 camToLight = ( camera->m_position - m_uniformBlock.position );
	float dist = camToLight.length() * 0.05f;

	Mat4 scale = Mat4();
	for ( int i = 0; i < 3; i++ ) {
		scale[i][i] = dist;
	}

	Vec3 axis = Vec3( 1.0, 0.0, 0.0 );
	Vec3 crossVec = axis.cross( m_uniformBlock.direction );
	if( crossVec.length() <= EPSILON ) {
		axis = Vec3( 1.0f, 1.0f, 0.0f );
		axis.normalize();
		crossVec = axis.cross( m_uniformBlock.direction );
		crossVec.normalize();
	}
	Mat4 rotation = Mat4();
	if ( TypeIndex() != 3 ) { //point light
		rotation[0] = Vec4( crossVec, 0.0 );
		rotation[1] = Vec4( m_uniformBlock.direction, 0.0 );
		rotation[2] = Vec4( m_uniformBlock.direction.cross( crossVec ).normal(), 0.0 );
		rotation[3] = Vec4( 0.0, 0.0, 0.0, 1.0 );
	}
		
	Mat4 model = translation * rotation * scale;

	//pass uniforms to shader
	s_debugShader->UseProgram();
	s_debugShader->SetUniformMatrix4f( "view", 1, false, view );
	s_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
	s_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );
	s_debugShader->SetUniform3f( "color", 1, m_uniformBlock.color.as_ptr() );
		
	//render debug light model
	Mesh * debugModel = GetDebugMesh();
	glBindVertexArray( debugModel->m_surfaces[0]->VAO );
	glDrawElements( GL_TRIANGLES, debugModel->m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );


	//***********************
	//Render light effect bounds
	//***********************	
	Mat4 ident = Mat4();
	s_debugShader->SetUniformMatrix4f( "model", 1, false, ident.as_ptr() ); //reuse the s_debugShader

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //draw wireframe
	glDisable( GL_CULL_FACE ); //disable backface culling
	glBindVertexArray( m_debugModel_VAO );
	glDrawElements( GL_TRIANGLES, m_boundsUniformBlock.tCount * 3, GL_UNSIGNED_INT, 0 );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); //draw regular
	glEnable( GL_CULL_FACE ); //enable backface culling


	//***********************
	//Render depth atlas
	//***********************
	//before drawing the texture to the buffer screen quad, the comparison mode used by shadow sampler needs to be disabled.
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas->m_attachements[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	if ( mode == 1 ) { //cvar takes arg. if set to 1 then draw the depthatlas
		//draw depth attachement to a quad on the screen
		s_depthBufferAtlas->Draw( s_depthBufferAtlas->m_attachements[0] );
	}
}

/*
================================
Light::UpdateDepthBuffer
	-render the depth of the scene to lights piece of the depthbuffer
================================
*/
void Light::UpdateDepthBuffer( Scene * scene ) {
	assert(  s_depthBufferAtlas->GetID() != 0 );

	s_depthBufferAtlas->Bind();
	if ( m_uniformBlock.shadowIdx == 0 ) {
		glClear( GL_DEPTH_BUFFER_BIT ); //Clear previous frame values
	}

	s_depthShader->UseProgram();
	s_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, LightMatrix() );

	//set rendering for this light's portion of the depthBufferAtlas
	const float mapsPerRow = ceil( sqrt( ( float )s_shadowCastingLightCount ) );
	float currentRow = floor( ( float )m_uniformBlock.shadowIdx / mapsPerRow );
	unsigned int stepsUp = ( unsigned int )currentRow;
	unsigned int stepsRight = m_uniformBlock.shadowIdx - ( unsigned int )( mapsPerRow * currentRow );
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

	s_depthBufferAtlas->Unbind();
}

/*
================================
Light::SetRadius
	-radius of lightsource (size of the lightbulb)
================================
*/
void Light::SetRadius( float radius ) {
	m_uniformBlock.light_radius = radius;
	const float att = MaxAttenuationDist();
	m_far_plane = att;
	if ( m_uniformBlock.max_radius >= att ) {
		m_uniformBlock.max_radius = att;
	}
}

/*
================================
Light::SetRadius
	-artificially shortens the distance of a lights effect
================================
*/
void Light::SetMaxRadius( float radius ) {
	const float att = MaxAttenuationDist();
	if ( radius <= att ) {
		m_uniformBlock.max_radius = radius;
		m_far_plane = radius;
	} else {
		m_uniformBlock.max_radius = att;
	}
}

/*
================================
Light::MaxAttenuationDist
	-init returns the farthest the light can illuminate by using light attenuation
================================
*/
float Light::MaxAttenuationDist() const {
	float maxDist = sqrt( m_uniformBlock.brightness / s_lightAttenuationBias ) - 1.0f;
	maxDist *= GetRadius();
	return maxDist;
}

/*
================================
Light::SetShadows
================================
*/
void Light::SetShadow( const bool shadowCasting ) {	
	if ( shadowCasting && !m_shadowCaster ) {
		//give shadowcasting lights an index in the atlas
		m_uniformBlock.shadowIdx = ( int )s_shadowCastingLightCount;
		s_shadowCastingLightCount += 1;
	} else if ( !shadowCasting && m_shadowCaster ) {
		m_uniformBlock.shadowIdx = -1;
		s_shadowCastingLightCount -= 1;
	}

	m_shadowCaster = shadowCasting;
}

/*
================================
Light::PassUniforms
================================
*/
void Light::PassUniforms( Shader * shader, int idx ) const {
	//fetch the buffer object from the shader. If its not present, the create and add it.
	{
		const GLsizeiptr size = sizeof( LightStorage );
		const int block_index = shader->BufferBlockIndexByName( "light_buffer" );
		Buffer * ssbo = NULL;
		if ( block_index == GL_INVALID_INDEX ) {
			//create it if it doesnt exist
			const GLsizeiptr totalSize = s_lightCount * size;
			ssbo = ssbo->GetBuffer( "light_buffer" );
			ssbo->Initialize( totalSize, NULL, GL_DYNAMIC_READ );
			shader->AddBuffer( ssbo );
		} else {
			ssbo = shader->BufferByBlockIndex( block_index );
		}

		//copy uniform data into this lights region of the buffer
		const GLintptr offset = idx * size;	
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, ssbo->GetBindingPoint(), ssbo->GetID() );
		glBufferSubData( GL_SHADER_STORAGE_BUFFER, offset, size, &m_uniformBlock );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
	}

	//if shadowcasting copy shadow data
	if ( m_uniformBlock.shadowIdx > -1 ) {
		ShadowStorage shadowUniformBlock;
		shadowUniformBlock.xfrm = m_xfrm;
		shadowUniformBlock.loc = m_PosInShadowAtlas.as_Vec4();

		const GLsizeiptr size = sizeof( ShadowStorage );
		const int block_index = shader->BufferBlockIndexByName( "shadow_buffer" );
		Buffer * ssbo = NULL;
		if ( block_index == GL_INVALID_INDEX ) {
			//create it if it doesnt exist
			const GLsizeiptr totalSize = s_shadowCastingLightCount * size;
			ssbo = ssbo->GetBuffer( "shadow_buffer" );
			ssbo->Initialize( totalSize, NULL, GL_DYNAMIC_READ );
			shader->AddBuffer( ssbo );
		} else {
			ssbo = shader->BufferByBlockIndex( block_index );
		}
		const GLintptr offset = m_uniformBlock.shadowIdx * size;
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, ssbo->GetBindingPoint(), ssbo->GetID() );
		glBufferSubData( GL_SHADER_STORAGE_BUFFER, offset, size, &shadowUniformBlock );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

		//read the ssbo that was just sent to GPU
		/*
		ShadowStorage shader_data;
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		GLvoid* p = glMapBufferRange( GL_SHADER_STORAGE_BUFFER, offset, size, GL_MAP_READ_BIT );
		memcpy( &shader_data, p, size );
		glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
		*/
	}
}

/*
================================
Light::PassDepthAttribute
	-Pass depth buffer to arg shader
================================
*/
void Light::PassDepthAttribute( Shader* shader, const unsigned int slot ) const {
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, s_depthBufferAtlas->m_attachements[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	shader->SetAndBindUniformTexture( "shadowAtlas", slot, GL_TEXTURE_2D, s_depthBufferAtlas->m_attachements[0] );
}

float distanceToPlane( Vec3 v, Vec3 N, Vec3 P ) {
	return N.dot( v - P );
}

void Light::InitBoundsVolume() {
	//create the boundMVPs
	m_boundsUniformBlock.vCount = 8;
	m_boundsUniformBlock.tCount = 12;	
	
	m_boundsUniformBlock.vPos[0] = Vec3( -1.0f, -1.0f, -1.0f );
	m_boundsUniformBlock.vPos[1] = Vec3( -1.0f, 1.0f, -1.0f );
	m_boundsUniformBlock.vPos[2] = Vec3( 1.0f, 1.0f, -1.0f );
	m_boundsUniformBlock.vPos[3] = Vec3( 1.0f, -1.0f, -1.0f );
	m_boundsUniformBlock.vPos[4] = Vec3( -1.0f, -1.0f, 1.0f );
	m_boundsUniformBlock.vPos[5] = Vec3( -1.0f, 1.0f, 1.0f );
	m_boundsUniformBlock.vPos[6] = Vec3( 1.0f, 1.0f, 1.0f );
	m_boundsUniformBlock.vPos[7] = Vec3( 1.0f, -1.0f, 1.0f );	

	m_boundsUniformBlock.tris[0] = Tri{ 4, 7, 5 };
	m_boundsUniformBlock.tris[1] = Tri{ 6, 5, 7 };
	m_boundsUniformBlock.tris[2] = Tri{ 7, 4, 3 };
	m_boundsUniformBlock.tris[3] = Tri{ 0, 3, 4 };
	m_boundsUniformBlock.tris[4] = Tri{ 6, 7, 2 };
	m_boundsUniformBlock.tris[5] = Tri{ 3, 2, 7 };
	m_boundsUniformBlock.tris[6] = Tri{ 5, 6, 1 };
	m_boundsUniformBlock.tris[7] = Tri{ 2, 1, 6 };
	m_boundsUniformBlock.tris[8] = Tri{ 4, 5, 0 };
	m_boundsUniformBlock.tris[9] = Tri{ 1, 0, 5 };
	m_boundsUniformBlock.tris[10] = Tri{ 0, 1, 3 };
	m_boundsUniformBlock.tris[11] = Tri{ 2, 3, 1 };
}

/*
================================
Light::InitLightEffectStorage
	-calculate the max bounds of light effect
================================
*/
void Light::InitLightEffectStorage() {
	//makes sure bounds are calculated only once.
	if ( lightEffectStorage_cached ) {
		return;
	}
	lightEffectStorage_cached = true;

	//build normalized volume
	InitBoundsVolume();
	if ( TypeIndex() == 1 ) { //directional light
		m_boundsUniformBlock.vPos[0].y = 0.0f;
		m_boundsUniformBlock.vPos[3].y = 0.0f;
		m_boundsUniformBlock.vPos[4].y = 0.0f;
		m_boundsUniformBlock.vPos[7].y = 0.0f;
	} else if ( TypeIndex() == 2 ) { //spot light
		m_boundsUniformBlock.vPos[0] = Vec3( -0.001f, 0.0f, -0.001f );
		m_boundsUniformBlock.vPos[3] = Vec3( 0.001f, 0.0f, -0.001f );
		m_boundsUniformBlock.vPos[4] = Vec3( -0.001f, 0.0f, 0.001f );
		m_boundsUniformBlock.vPos[7] = Vec3( 0.001f, 0.0f, 0.001f );

		const float normalizedWidth = atanf( m_uniformBlock.angle );
		m_boundsUniformBlock.vPos[1] = Vec3( -normalizedWidth, 1.0f, -normalizedWidth );
		m_boundsUniformBlock.vPos[2] = Vec3( normalizedWidth, 1.0f, -normalizedWidth );
		m_boundsUniformBlock.vPos[5] = Vec3( -normalizedWidth, 1.0f, normalizedWidth );
		m_boundsUniformBlock.vPos[6] = Vec3( normalizedWidth, 1.0f, normalizedWidth );	
	}

	//transform it to light position, orientation, and size
	Mat4 translation = Mat4();
	translation.Translate( m_uniformBlock.position );

	Mat4 scale = Mat4();
	const float maxDist = GetMaxRadius();
	for ( int i = 0; i < 3; i++ ) {
		scale[i][i] = maxDist;
	}

	Vec3 axis = Vec3( 1.0, 0.0, 0.0 );
	Vec3 crossVec = m_uniformBlock.direction.cross( axis );
	if( crossVec.length() <= EPSILON ) {
		axis = Vec3( 1.0f, 1.0f, 0.0f );
		axis.normalize();
		crossVec = axis.cross( m_uniformBlock.direction );
		crossVec.normalize();
	}
	Mat4 rotation = Mat4();
	if ( TypeIndex() != 3 ) { //if not a point light
		rotation[0] = Vec4( m_uniformBlock.direction.cross( crossVec ).normal(), 0.0 );
		rotation[1] = Vec4( m_uniformBlock.direction.normal(), 0.0 );
		rotation[2] = Vec4( crossVec, 0.0 );
		rotation[3] = Vec4( 0.0, 0.0, 0.0, 1.0 );
	}
		
	Mat4 lightMat = translation * rotation * scale;
	for ( unsigned int i = 0; i < m_boundsUniformBlock.vCount; i++ ) {
		m_boundsUniformBlock.vPos[i] *= lightMat;
	}

	//create VAO to render light bounds as geo in debug view
    unsigned int VBO, EBO;
    glGenVertexArrays( 1, &m_debugModel_VAO );
    glGenBuffers( 1, &VBO );
    glGenBuffers( 1, &EBO );

    glBindVertexArray( m_debugModel_VAO );
    glBindBuffer( GL_ARRAY_BUFFER, VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( m_boundsUniformBlock.vPos ), m_boundsUniformBlock.vPos, GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( m_boundsUniformBlock.tris ), m_boundsUniformBlock.tris, GL_STATIC_DRAW );

    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void * )0 );
    glEnableVertexAttribArray( 0 );
}

/*
================================
Light::PassPrepassUniforms
	-Pass oriented light bbox to tile binning compute shader
================================
*/
void Light::PassPrepassUniforms( Shader* shader, int idx ) const {
	//fetch the buffer object from the shader. If its not present, the create and add it.
	const int block_index = shader->BufferBlockIndexByName( "lightEffect_buffer" );
	Buffer * ssbo = NULL;
	const GLsizeiptr size = sizeof( LightEffectStorage );
	const GLsizeiptr totalSize = s_lightCount * size;
	if ( block_index == GL_INVALID_INDEX ) {
		//create it if it doesnt exist		
		ssbo = ssbo->GetBuffer( "lightEffect_buffer" );
		ssbo->Initialize( totalSize, NULL, GL_DYNAMIC_READ );
		shader->AddBuffer( ssbo );
	} else {
		ssbo = shader->BufferByBlockIndex( block_index );
	}

	//copy uniform data into this lights region of the buffer
	const GLintptr offset = idx * size;	
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, ssbo->GetBindingPoint(), ssbo->GetID() );
	glBufferSubData( GL_SHADER_STORAGE_BUFFER, offset, size, &m_boundsUniformBlock );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
}

/*
================================
DirectionalLight::DirectionalLight
================================
*/
DirectionalLight::DirectionalLight() {
	m_uniformBlock.typeIndex = 1;
	if ( s_debugModel_directional->m_name.Initialized() == false ) {
		s_debugModel_directional->LoadOBJFromFile( "data\\model\\system\\light\\directionalLight.obj" );
		s_debugModel_directional->LoadVAO( 0 );
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
	Vec3 right = m_uniformBlock.direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_uniformBlock.direction.cross( up );
	}
	up = right.cross( m_uniformBlock.direction );
	Vec3 eye = m_uniformBlock.position + m_uniformBlock.direction;
	Mat4 view = Mat4();
	view.LookAt( m_uniformBlock.position, eye, up );

	//update member
	m_xfrm = projection * view;

	return m_xfrm.as_ptr();
}

/*
================================
SpotLight::SpotLight
================================
*/
SpotLight::SpotLight() {
	m_uniformBlock.typeIndex = 2;
	if ( s_debugModel_spot->m_name.Initialized() == false ) {
		s_debugModel_spot->LoadOBJFromFile( "data\\model\\system\\light\\spotLight.obj" );
		s_debugModel_spot->LoadVAO( 0 );
	}
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
	projection.Perspective( GetAngle(), aspect, m_near_plane, m_far_plane );

	//set up view matrix
	Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
	Vec3 right = m_uniformBlock.direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_uniformBlock.direction.cross( up );
	}
	up = right.cross( m_uniformBlock.direction );
	Vec3 eye = m_uniformBlock.position + m_uniformBlock.direction;
	Mat4 view = Mat4();
	view.LookAt( m_uniformBlock.position, eye, up );

	//update member
	m_xfrm = projection * view;

	return m_xfrm.as_ptr();
}

/*
================================
PointLight::PointLight
================================
*/
PointLight::PointLight() {
	m_uniformBlock.typeIndex = 3;
	if ( s_debugModel_point->m_name.Initialized() == false ) {
		s_debugModel_point->LoadOBJFromFile( "data\\model\\system\\light\\light.obj" );
		s_debugModel_point->LoadVAO( 0 );
	}
}

/*
================================
PointLight::SetShadow
================================
*/
void PointLight::SetShadow( const bool shadowCasting ) {	
	if ( shadowCasting && !m_shadowCaster ) {
		//give shadowcasting lights an index in the atlas
		m_uniformBlock.shadowIdx = ( int )s_shadowCastingLightCount;
		s_shadowCastingLightCount += 6;
	} else if ( !shadowCasting && m_shadowCaster ) {
		m_uniformBlock.shadowIdx = -1;
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
		Vec3 eye = m_uniformBlock.position + dirs[i];
		Mat4 view = Mat4();
		view.LookAt( m_uniformBlock.position, eye, up );

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
	assert(  s_depthBufferAtlas->GetID() != 0 );

	s_depthBufferAtlas->Bind();
	if ( m_uniformBlock.shadowIdx == 0 ) {
		glClear( GL_DEPTH_BUFFER_BIT ); //Clear previous frame values
	}

	LightMatrix(); //update m_xfrms
	for ( unsigned int i = 0; i < 6; i++ ) {
		s_depthShader->UseProgram();
		s_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, m_xfrms[i].as_ptr() );

		//set rendering for this light's portion of the depthBufferAtlas
		const unsigned int shadowIdx = m_uniformBlock.shadowIdx + i;
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

	s_depthBufferAtlas->Unbind();
}

/*
================================
PointLight::GetShadowMapLoc
================================
*/
const Vec2 PointLight::GetShadowMapLoc( unsigned int faceIdx ) const {
	const unsigned int shadowIdx = m_uniformBlock.shadowIdx + faceIdx;
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
	//fetch the buffer object from the shader. If its not present, the create and add it.
	{
		const GLsizeiptr size = sizeof( LightStorage );
		const int block_index = shader->BufferBlockIndexByName( "light_buffer" );
		Buffer * ssbo = NULL;
		if ( block_index == GL_INVALID_INDEX ) {
			//create it if it doesnt exist
			const GLsizeiptr totalSize = s_lightCount * size;
			ssbo = ssbo->GetBuffer( "light_buffer" );
			ssbo->Initialize( totalSize, NULL, GL_DYNAMIC_READ );
			shader->AddBuffer( ssbo );
		} else {
			ssbo = shader->BufferByBlockIndex( block_index );
		}
		const GLintptr offset = idx * size;	
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, ssbo->GetBindingPoint(), ssbo->GetID() );
		glBufferSubData( GL_SHADER_STORAGE_BUFFER, offset, size, &m_uniformBlock );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
	}

	//if shadowcasting send shadow data
	if ( m_uniformBlock.shadowIdx > -1 ) {
		ShadowStorage shadowUniformBlock[6];
		for ( unsigned int i = 0; i < 6; i++ ) {
			shadowUniformBlock[i].xfrm = m_xfrms[i];
			shadowUniformBlock[i].loc = GetShadowMapLoc( i ).as_Vec4();
		}

		const GLsizeiptr size = sizeof( ShadowStorage );
		const int block_index = shader->BufferBlockIndexByName( "shadow_buffer" );
		Buffer * ssbo = NULL;
		if ( block_index == GL_INVALID_INDEX ) {
			//create it if it doesnt exist
			const GLsizeiptr totalSize = s_shadowCastingLightCount * size;
			ssbo = ssbo->GetBuffer( "shadow_buffer" );
			ssbo->Initialize( totalSize, NULL, GL_DYNAMIC_READ );
			shader->AddBuffer( ssbo );
		} else {
			ssbo = shader->BufferByBlockIndex( block_index );
		}
		const GLintptr offset = m_uniformBlock.shadowIdx * size;
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, ssbo->GetBindingPoint(), ssbo->GetID() );
		glBufferSubData( GL_SHADER_STORAGE_BUFFER, offset, size * 6, &shadowUniformBlock );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

		/*
		//read the ssbo that was just sent to GPU
		ShadowStorage shader_data[6];
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo->GetID() );
		GLvoid* p = glMapBufferRange( GL_SHADER_STORAGE_BUFFER, offset, size * 6, GL_MAP_READ_BIT );
		memcpy( shader_data, p, size * 6 );
		glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
		*/
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
		s_brdfIntegrationMap.InitFromFile_Uncompressed( "data\\texture\\system\\brdfIntegrationLUT.hdr" );
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
		s_brdfIntegrationMap.InitFromFile_Uncompressed( "data\\texture\\system\\brdfIntegrationLUT.hdr" );
	}
}

/*
================================
EnvProbe::Delete
	-Delete textures from gpu
================================
*/
void EnvProbe::Delete() {
	const unsigned int id1 = m_irradianceMap.GetName();
	if ( id1 != CubemapTexture::s_errorCube ) {
		glDeleteTextures( 1, &id1 );
	}

	const unsigned int id2 = m_environmentMap.GetName();
	if ( id1 != CubemapTexture::s_errorCube ) {
		glDeleteTextures( 1, &id2 );
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
	m_environmentMap.InitFromFile( environment_relativePath.c_str() );
	m_environmentMap.PrefilterSpeculateProbe();

	//load irradiance map
	Str irradiance_relativePath = environment_relativePath.Substring( 0, environment_relativePath.Length() - 4 );
	irradiance_relativePath.Append( "_irr.hdr" );
	m_irradianceMap.InitFromFile( irradiance_relativePath.c_str() );

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
std::vector< unsigned int > EnvProbe::RenderCubemaps( Shader * shader, const unsigned int cubemapSize ) {
	glViewport( 0, 0, cubemapSize, cubemapSize ); //Set the OpenGL viewport to be the entire size of the window
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer

	Scene * scene = Scene::getInstance();

	//create list of 6 FBOs. Once for each face of cube
	std::vector< Framebuffer > fbos;
	for ( unsigned int i = 0; i < 6; i++ ) {
		Framebuffer newFBO = Framebuffer( "screenTexture" );
		newFBO.CreateNewBuffer( cubemapSize, cubemapSize, "framebuffer" );
		newFBO.AttachTextureBuffer( GL_RGB16F, GL_COLOR_ATTACHMENT0, GL_RGB, GL_FLOAT );
		newFBO.AttachRenderBuffer( GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT );
		newFBO.CreateScreen( 0, 0, cubemapSize, cubemapSize );
		if ( !newFBO.Status() ) {
			assert( false );
		}
		fbos.push_back( newFBO );
	}

	//set projections for cubemap render
	Mat4 projection = Mat4();
	projection.Perspective( to_radians( 90.0f ), 1.0f, 0.01f, 100.0f );
	Mat4 views[] = { Mat4(), Mat4(), Mat4(), Mat4(), Mat4(), Mat4() };
	views[0].LookAt2( Vec3( 1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ), m_position );
	views[1].LookAt2( Vec3( -1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ), m_position );
	views[2].LookAt2( Vec3( 0.0f, 1.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ), m_position );
	views[3].LookAt2( Vec3( 0.0f, -1.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ), m_position );
	views[4].LookAt2( Vec3( 0.0f, 0.0f, 1.0f ), Vec3( 0.0f, -1.0f, 0.0f ), m_position );
	views[5].LookAt2( Vec3( 0.0f, 0.0f, -1.0f ), Vec3( 0.0f, -1.0f, 0.0f ), m_position );

	//draw skybox
	glDisable( GL_BLEND );
	MaterialDecl* skyMat;
	const Cube * sceneSkybox = scene->GetSkybox();
	skyMat = MaterialDecl::GetMaterialDecl( sceneSkybox->m_surface->materialName.c_str() );
	skyMat->BindTextures();
	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
		fbos[faceIdx].Bind();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		const Mat4 skyBox_viewMat = views[faceIdx].as_Mat3().as_Mat4();
		skyMat->shader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );
		skyMat->shader->SetUniformMatrix4f( "view", 1, false, skyBox_viewMat.as_ptr() );
		sceneSkybox->DrawSurface( true );
		fbos[faceIdx].Unbind();
	}
	glEnable( GL_BLEND );

	//render the scene
	Light * light = NULL;
	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
		fbos[faceIdx].Bind(); //bind the framebuffer so all subsequent drawing is to it.
		glClear( GL_DEPTH_BUFFER_BIT ); //clear only depth because we wanna keep skybox color

		//draw scene to cubemap face
		MaterialDecl* matDecl;
		for ( unsigned int i = 0; i < scene->MeshCount(); i++ ) {
			Mesh * mesh = NULL;
			scene->MeshByIndex( i, &mesh );

			for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
				matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[j]->materialName.c_str() );

				//exit early if this material is errored out
				if ( matDecl->m_shaderProg == "error" ) {
					matDecl->BindTextures();
					mesh->DrawSurface( j ); //draw surface
					continue;
				}

				//bind textures
				shader->UseProgram();
				unsigned int slotCount = 0;
				textureMap::iterator it_t = matDecl->m_textures.begin();
				while ( it_t != matDecl->m_textures.end() ) {
					std::string uniformName = it_t->first;
					Texture* texture = it_t->second;
					shader->SetAndBindUniformTexture( uniformName.c_str(), slotCount, texture->GetTarget(), texture->GetName() );

					slotCount++;
					it_t++;
				}

				//bind float decl uniforms
				vec3Map::iterator it_f = matDecl->m_vec3s.begin();
				while ( it_f != matDecl->m_vec3s.end() ) {
					const std::string uniformName = it_f->first;
					const Vec3 val = it_f->second;
					shader->SetUniform3f( uniformName.c_str(), 1, val.as_ptr() );
					it_f++;
				}

				//pass in camera data
				shader->SetUniformMatrix4f( "view", 1, false, views[faceIdx].as_ptr() );		
				shader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );
				shader->SetUniform3f( "camPos", 1, m_position.as_ptr() );
				const int lightCount = ( int )( scene->LightCount() );
				shader->SetUniform1i( "lightCount", 1, &lightCount );

				//pass lights data
				for ( int k = 0; k < scene->LightCount(); k++ ) {
					scene->LightByIndex( k, &light );
					if ( k == 0 ) {
						light->PassDepthAttribute( shader, 4 );
						const int shadowMapPartitionSize = ( unsigned int )( light->s_partitionSize );
						shader->SetUniform1i( "shadowMapPartitionSize", 1, &shadowMapPartitionSize );
					}					
					light->PassUniforms( shader, k );
				}
	
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