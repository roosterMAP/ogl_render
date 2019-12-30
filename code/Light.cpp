#include "Light.h"
#include "Scene.h"

bool Light::s_drawEnable = false;

//vertex shader prog for debug light object
const char * Light::s_vshader_source =
	"#version 330 core\n"
	"layout ( location = 0 ) in vec3 aPos;"
	"uniform mat4 view;"
	"uniform mat4 model;"
	"uniform mat4 projection;"
	"void main() {"
	"	gl_Position = projection * view * model * vec4( aPos.x, aPos.y, aPos.z, 1.0 );"
	"}";

//fragment shader prog for debug light object
const char * Light::s_fshader_source =
	"#version 330 core\n"
	"uniform vec3 color;"
	"out vec4 FragColor;"
	"void main() {"
	"	FragColor = vec4( color, 1.0 );"
	"}";

//vertex shader prog for depth map shader
const char * Light::s_depth_vshader_source =
	"#version 330 core\n"
	"layout ( location = 0 ) in vec3 aPos;"
	"layout ( location = 5 ) in mat4 model;"
	"uniform mat4 lightSpaceMatrix;"
	"void main() {"
	"	gl_Position = lightSpaceMatrix * model * vec4( aPos, 1.0 );"
	"}";

//fragment shader prog for depth map shader
const char * Light::s_depth_fshader_source =
	"#version 330 core\n"
	"void main() {"
	"	gl_FragDepth = gl_FragCoord.z;"
	"}";

//texture ids of blank shadow maps to be passed to shader if light is not shadowcasting
unsigned int Light::s_blankShadowMap = 0;
unsigned int Light::s_blankShadowCubeMap = 0;

Shader * EnvProbe::s_ambientPass_shader = new Shader();
Texture EnvProbe::s_brdfIntegrationMap = Texture();

/*
================================
Light::Light
================================
*/
Light::Light() {
	m_position = Vec3( 0.0 );
	m_color = Vec3( 1.0 );
	m_shadowCaster = false;
	m_near_plane = 0.1f;
	m_far_plane = 7.5f;
	m_depthBuffer = Framebuffer( "depthMap" );
	m_xfrm = Mat4();

	if ( s_blankShadowMap == 0 ) {
		s_blankShadowMap = CreateBlankShadowMapTexture( 512, 512 );
		s_blankShadowCubeMap = CreateBlankShadowCubeMapTexture( 512, 512 );
	}
}

/*
================================
Light::Light
================================
*/
Light::Light( Vec3 pos ) {
	m_position = pos;
	m_color = Vec3( 1.0 );
	m_shadowCaster = false;
	m_near_plane = 0.1f;
	m_far_plane = 7.5f;
	m_depthBuffer = Framebuffer( "depthMap" );

	if ( s_blankShadowMap == 0 ) {
		s_blankShadowMap = CreateBlankShadowMapTexture( 512, 512 );
		s_blankShadowCubeMap = CreateBlankShadowCubeMapTexture( 512, 512 );
	}
}

/*
================================
Light::DrawDepthBuffer
================================
*/
void Light::DrawDepthBuffer( const float * view, const float * projection ) {
	//before drawing the texture to the buffer screen quad, the comparison mode used by shadow sampler needs to be disabled.
	glBindTexture( GL_TEXTURE_2D, m_depthBuffer.m_attachements[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );

	m_depthBuffer.Draw( m_depthBuffer.m_attachements[0] );
}

/*
================================
Light::EnableShadows
================================
*/
void Light::EnableShadows() {
	//compile shader
	m_depthShader = new Shader();
	if( !m_depthShader->CompileShaderFromCSTR( s_depth_vshader_source, s_depth_fshader_source ) ){
		delete m_depthShader;
		return;
	}
	m_shadowCaster = true;

	//configure depth map FBO
	m_depthBuffer.CreateNewBuffer( 512, 512, "debug_quad" );
    glDrawBuffer( GL_NONE ); //FBO cant be complete without color buffer. Since we dont need one, we use this command to override.
    glReadBuffer( GL_NONE ); //same as above line
	m_depthBuffer.AttachTextureBuffer( GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT ); //since attached first, the texture buffer id is the first element in m_attachements.

	//set the boundary outside the texture border to a specific border color
	glBindTexture( GL_TEXTURE_2D, m_depthBuffer.m_attachements[0] );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );

	glBindTexture( GL_TEXTURE_2D, 0 );

	assert( m_depthBuffer.Status() );
	
	m_depthBuffer.Unbind();
}

/*
================================
Light::EnableCompareMode
	-shader uses sampler2DShadow where frag coord arg is a vec3 whose
	-z-value is the float to compare against.
================================
*/
void Light::EnableSamplerCompareMode() {
	glBindTexture( GL_TEXTURE_2D, m_depthBuffer.m_attachements[0] );

	//allows using sampler2DShadow as the sampler for the shadowmap texture
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
}

/*
================================
Light::DrawEnable
================================
*/
void Light::DebugDrawSetup( std::string obj ) {
	s_drawEnable = true; //enable debug drawing if everything went well

	//compile shader
	m_debugShader = new Shader();
	if( !m_debugShader->CompileShaderFromCSTR( s_vshader_source, s_fshader_source ) ){
		delete m_debugShader;
		return;
	}
	m_debugShader->UseProgram();
	m_debugShader->SetUniform3f( "color", 1, m_color.as_ptr() );
	
	//load light mesh
	if ( !m_mesh.LoadOBJFromFile( obj.c_str() ) ) {
		return;
	}
	m_mesh.LoadVAO( 0 ); //only load first surface since we assume these debug models only have one	
}

/*
================================
Light::DebugDrawEnable
	-Initializes debug model and quad to draw shadow depth buffer
================================
*/
void Light::DebugDrawEnable() {
	DebugDrawSetup( "data\\model\\light.obj" );
	m_depthBuffer.CreateScreen( 0, 0 );
}


/*
================================
Light::DebugDraw
	-Draws light at constant size by denamically scaling it.
================================
*/
void Light::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	if ( s_drawEnable ) {
		m_debugShader->UseProgram();

		//calculate model matrix
		Mat4 translation = Mat4();
		translation.Translate( m_position );

		Vec3 camToLight = ( camera->m_position - m_position );
		float dist = camToLight.length() * 0.05f;

		Mat4 scale = Mat4();
		for ( int i = 0; i < 3; i++ ) {
			scale[i][i] = dist;
		}

		Mat4 model = translation * scale;

		//pass uniforms to shader
		m_debugShader->SetUniformMatrix4f( "view", 1, false, view );
		m_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
		m_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );		
		
		//render debug light model
		glBindVertexArray( m_mesh.m_surfaces[0]->VAO );
		glDrawElements( GL_TRIANGLES, m_mesh.m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );
	}
}

/*
================================
Light::BindDepthBuffer
================================
*/
void Light::BindDepthBuffer() {
	m_depthBuffer.Bind();
	glClear( GL_DEPTH_BUFFER_BIT );
}

/*
================================
Light::PassUniforms
================================
*/
void Light::PassUniforms( Shader * shader ) {
	const int typeIndex = 0;
	shader->SetUniform1i( "light.typeIndex", 1, &typeIndex );
	shader->SetUniform3f( "light.position", 1, m_position.as_ptr() );
	shader->SetUniform3f( "light.color", 1, m_color.as_ptr() );
	const int shadow = 0;
	shader->SetUniform1i( "light.shadow", 1, &shadow );
}

/*
================================
Light::CreateBlankShadowMapTexture
================================
*/
unsigned int Light::CreateBlankShadowMapTexture( unsigned int width, unsigned int height ) {
	unsigned int texBuffer;
	glGenTextures( 1, &texBuffer );
	glBindTexture( GL_TEXTURE_2D, texBuffer );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );

	glBindTexture( GL_TEXTURE_2D, 0 );

	return texBuffer;
}

/*
================================
Light::CreateBlankShadowCubeMapTexture
================================
*/
unsigned int Light::CreateBlankShadowCubeMapTexture( unsigned int width, unsigned int height ) {
	unsigned int texBuffer;
	glGenTextures( 1, &texBuffer );
	glBindTexture( GL_TEXTURE_CUBE_MAP, texBuffer );

	for ( unsigned int i = 0; i < 6; i++ ) {
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
	}

	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

	return texBuffer;
}

/*
================================
DirectionalLight::LightMatrix
	-returns a light space transformation matrix that transforms each
	-world-space vector into the space as visible from the light source
================================
*/
const float * DirectionalLight::LightMatrix() {
	Mat4 lightProjection = Mat4();
	lightProjection.Orthographic( -1.0f, 1.0f, -1.0f, 1.0f, m_near_plane, m_far_plane );

	Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
	Vec3 right = m_direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_direction.cross( up );
	}
	up = right.cross( m_direction );
	Vec3 eye = m_position + m_direction;
	Mat4 lightView = Mat4();
	lightView.LookAt( m_position, eye, up );

	m_xfrm = lightProjection * lightView;

	return m_xfrm.as_ptr();
}

/*
================================
DirectionalLight::DebugDrawEnable
	-Initializes debug model and quad to draw shadow depth buffer
================================
*/
void DirectionalLight::DebugDrawEnable() {
	DebugDrawSetup( "data\\model\\directionalLight.obj" );
	m_depthBuffer.CreateScreen( 0, 0 );
}

/*
================================
DirectionalLight::DirectionalLight
	-Draws light at constant size by dynamically scaling it.
================================
*/
void DirectionalLight::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	if ( s_drawEnable ) {
		m_debugShader->UseProgram();

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
		Mat4 rotation = Mat4( 1.0 );
		rotation[0] = Vec4( crossVec, 0.0 );
		rotation[1] = Vec4( m_direction, 0.0 );
		rotation[2] = Vec4( m_direction.cross( crossVec ).normal(), 0.0 );
		rotation[3] = Vec4( 0.0, 0.0, 0.0, 1.0 );
		
		Mat4 model = translation * rotation * scale;

		//pass uniforms to shader
		m_debugShader->SetUniformMatrix4f( "view", 1, false, view );
		m_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
		m_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );		
		
		//render debug light model
		glBindVertexArray( m_mesh.m_surfaces[0]->VAO );
		glDrawElements( GL_TRIANGLES, m_mesh.m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );
	}
}

/*
================================
DirectionalLight::PassUniforms
================================
*/
void DirectionalLight::PassUniforms( Shader * shader ) {
	const int typeIndex = 1;
	shader->SetUniform1i( "light.typeIndex", 1, &typeIndex );
	shader->SetUniform3f( "light.direction", 1, m_direction.as_ptr() );
	shader->SetUniform3f( "light.color", 1, m_color.as_ptr() );
	const int shadow = ( int )m_shadowCaster;
	shader->SetUniform1i( "light.shadow", 1, &shadow );
	if ( m_shadowCaster ) {
		shader->SetUniformMatrix4f( "light.matrix", 1, false, m_xfrm.as_ptr() ); //pass light view matrix
		shader->SetAndBindUniformTexture( "light.shadowMap", 4, GL_TEXTURE_2D, m_depthBuffer.m_attachements[0] );
	} else {
		shader->SetAndBindUniformTexture( "light.shadowMap", 4, GL_TEXTURE_2D, s_blankShadowMap );
	}
	shader->SetAndBindUniformTexture( "light.shadowCubeMap", 5, GL_TEXTURE_CUBE_MAP, s_blankShadowCubeMap );
}

/*
================================
SpotLight::SpotLight
================================
*/
SpotLight::SpotLight() {
	m_direction = Vec3( 1.0, 0.0, 0.0 );	
	m_radius = 4.0;
	m_angle = to_radians( 90.0f );

}

/*
================================
SpotLight::SpotLight
================================
*/
SpotLight::SpotLight( Vec3 pos, Vec3 dir, float degrees, float radius ) {
	m_position = pos;
	m_direction = dir;
	m_radius = radius;

	if( degrees < 170.0 ) {
		m_angle = to_radians( degrees );
	} else {
		m_angle = to_radians( 169.0f );
	}
}

/*
================================
SpotLight::LightMatrix
	-returns a light space transformation matrix that transforms each
	-world-space vector into the space as visible from the light source
================================
*/
const float * SpotLight::LightMatrix() {
	const float aspect = 1.0f;
	Mat4 lightProjection = Mat4();
	lightProjection.Perspective( m_angle, aspect, m_near_plane, m_far_plane );

	Vec3 up = Vec3( 0.0f, 1.0f, 0.0f );
	Vec3 right = m_direction.cross( up );
	if ( right.length() <= EPSILON ) {
		up = Vec3( 1.0f, 0.0f, 0.0f );
		right = m_direction.cross( up );
	}
	up = right.cross( m_direction );
	Vec3 eye = m_position + m_direction;
	Mat4 lightView = Mat4();
	lightView.LookAt( m_position, eye, up );

	m_xfrm = lightProjection * lightView;

	return m_xfrm.as_ptr();
}

/*
================================
SpotLight::DebugDrawEnable
	-Initializes debug model and quad to draw shadow depth buffer
================================
*/
void SpotLight::DebugDrawEnable() {
	DebugDrawSetup( "data\\model\\spotLight.obj" );
	m_depthBuffer.CreateScreen( 0, 0 );
}

/*
================================
SpotLight::DebugDraw
================================
*/
void SpotLight::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	if ( s_drawEnable ) {
		m_debugShader->UseProgram();

		//calculate model matrix
		Mat4 translation = Mat4();
		translation.Translate( m_position );

		Vec3 camToLight = ( camera->m_position - m_position );
		float dist = camToLight.length() * 0.05f;


		float obj_angle = to_radians( 45.0f );
		float scaleFactor = m_angle / obj_angle;
		Vec3 scaleVec = Vec3( scaleFactor * dist, dist, scaleFactor * dist );
		Mat4 scale = Mat4();
		for ( int i = 0; i < 3; i++ ) {
			scale[i][i] = scaleVec[i];
		}

		Vec3 axis = Vec3( 1.0, 0.0, 0.0 );
		Vec3 crossVec = axis.cross( m_direction );
		if( crossVec.length() <= EPSILON ) {
			axis = Vec3( 1.0f, 1.0f, 0.0f );
			axis.normalize();
			crossVec = axis.cross( m_direction );
			crossVec.normalize();
		}
		Mat4 rotation = Mat4( 1.0 );
		rotation[0] = Vec4( crossVec, 0.0 );
		rotation[1] = Vec4( m_direction, 0.0 );
		rotation[2] = Vec4( m_direction.cross( crossVec ).normal(), 0.0 );
		rotation[3] = Vec4( 0.0, 0.0, 0.0, 1.0 );

		Mat4 model = translation * rotation * scale;

		//pass uniforms to shader
		m_debugShader->SetUniformMatrix4f( "view", 1, false, view );
		m_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
		m_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );		
		
		//render debug light model
		glBindVertexArray( m_mesh.m_surfaces[0]->VAO );
		glDrawElements( GL_TRIANGLES, m_mesh.m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );
	}
}

/*
================================
SpotLight::PassUniforms
================================
*/
void SpotLight::PassUniforms( Shader * shader ) {
	const int typeIndex = 2;
	const float lightAngle_cosine = cos( m_angle / 2.0f );
	shader->SetUniform1i( "light.typeIndex", 1, &typeIndex );
	shader->SetUniform3f( "light.position", 1, m_position.as_ptr() );
	shader->SetUniform3f( "light.direction", 1, m_direction.as_ptr() );
	shader->SetUniform3f( "light.color", 1, m_color.as_ptr() );
	shader->SetUniform1f( "light.angle", 1, &lightAngle_cosine ); //passing in cosine so we dont have to do it in the fragment shader
	shader->SetUniform1f( "light.radius", 1, &m_radius );
	const int shadow = ( int )m_shadowCaster;
	shader->SetUniform1i( "light.shadow", 1, &shadow );
	if ( m_shadowCaster ) {
		shader->SetUniformMatrix4f( "light.matrix", 1, false, m_xfrm.as_ptr() ); //pass light view matrix
		shader->SetAndBindUniformTexture( "light.shadowMap", 4, GL_TEXTURE_2D, m_depthBuffer.m_attachements[0] );
	} else {
		shader->SetAndBindUniformTexture( "light.shadowMap", 4, GL_TEXTURE_2D, s_blankShadowMap );
	}
	shader->SetAndBindUniformTexture( "light.shadowCubeMap", 5, GL_TEXTURE_CUBE_MAP, s_blankShadowCubeMap );
}

/*
================================
PointLight::PointLight
================================
*/
PointLight::PointLight() {
	m_radius = 1.0;
	m_shadowFaceIdx = 0;
	m_depthBuffer.SetColorBufferUniform( "cubemap" ); //change depthmap uniform name to cubemap
}

/*
================================
PointLight::PointLight
================================
*/
PointLight::PointLight( Vec3 pos, float radius ) {
	m_position = pos;
	m_radius = radius;
	m_shadowFaceIdx = 0;
	m_depthBuffer.SetColorBufferUniform( "cubemap" ); //change depthmap uniform name to cubemap
}

/*
================================
PointLight::DebugDrawEnable
	-Initializes debug model and quad to draw shadow depth buffer
================================
*/
void PointLight::DebugDrawEnable() {
	DebugDrawSetup( "data\\model\\light.obj" );
	m_depthCubeView.CreateScreen( 0, 0 );
}

/*
================================
PointLight::PassDepthShaderUniforms
================================
*/
void PointLight::PassDepthShaderUniforms() {
	m_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, LightMatrix() );
	m_depthShader->SetUniform1f( "far_plane", 1, &m_far_plane );
	m_depthShader->SetUniform3f( "lightPos", 1, m_position.as_ptr() );
}

/*
================================
PointLight::DebugDraw
	-Draws light at constant size by denamically scaling it.
================================
*/
void PointLight::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	if ( s_drawEnable ) {
		m_debugShader->UseProgram();

		//calculate model matrix
		Mat4 translation = Mat4();
		translation.Translate( m_position );

		Vec3 camToLight = ( camera->m_position - m_position );
		float dist = camToLight.length() * 0.05f;
		Mat4 scale = Mat4();
		for ( int i = 0; i < 3; i++ ) {
			scale[i][i] = dist;
		}

		Mat4 model = translation * scale;

		//pass uniforms to shader
		m_debugShader->SetUniformMatrix4f( "view", 1, false, view );
		m_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
		m_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );		
		
		//render debug light model
		glBindVertexArray( m_mesh.m_surfaces[0]->VAO );
		glDrawElements( GL_TRIANGLES, m_mesh.m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );
	}
}

/*
================================
PointLight::PassUniforms
================================
*/
void PointLight::PassUniforms( Shader * shader ) {
	const int typeIndex = 3;
	shader->SetUniform1i( "light.typeIndex", 1, &typeIndex );
	shader->SetUniform3f( "light.position", 1, m_position.as_ptr() );
	shader->SetUniform3f( "light.color", 1, m_color.as_ptr() );
	shader->SetUniform1f( "light.radius", 1, &m_radius );
	const int shadow = ( int )m_shadowCaster;
	shader->SetUniform1i( "light.shadow", 1, &shadow );
	shader->SetUniform1f( "light.far_plane", 1, &m_far_plane );
	const Mat4 ident = Mat4( 1.0 );
	shader->SetUniformMatrix4f( "light.matrix", 1, false, ident.as_ptr() );
	shader->SetAndBindUniformTexture( "light.shadowMap", 4, GL_TEXTURE_2D, s_blankShadowMap );
	if ( m_shadowCaster ) {
		shader->SetAndBindUniformTexture( "light.shadowCubeMap", 5, GL_TEXTURE_CUBE_MAP, m_depthBuffer.m_attachements[0] );
	} else {
		shader->SetAndBindUniformTexture( "light.shadowCubeMap", 5, GL_TEXTURE_CUBE_MAP, s_blankShadowCubeMap );
	}
}


/*
================================
Light::BindDepthBuffer
================================
*/
void PointLight::BindDepthBuffer() {
	//bind depth buffer
	m_depthBuffer.Bind();

	//increment which fact of the cubemap we are drawing to
	m_shadowFaceIdx += 1;
	if ( m_shadowFaceIdx > 5 ) {
		m_shadowFaceIdx = 0;
	}

	//activate the cubemap face we are about to draw to	
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + m_shadowFaceIdx, m_depthBuffer.m_attachements[0], 0 );
	glClear( GL_DEPTH_BUFFER_BIT ); //clear depth buffer
}

/*
================================
PointLight::LightMatrix
	-return the view matrix of a facet of the depthcube for the pointlight.
================================
*/
const float * PointLight::LightMatrix() {
	//initialize projection matrix
	Mat4 shadowProj = Mat4();
	shadowProj.Perspective( to_radians( 90.0f ), 1.0f, m_near_plane, m_far_plane );

	Mat4 view = Mat4();
	if ( m_shadowFaceIdx == 0 ) {
		view.LookAt( m_position, m_position + Vec3( 1.0, 0.0, 0.0 ), Vec3( 0.0, -1.0, 0.0 ) );
	} else if ( m_shadowFaceIdx == 1 ) {
		view.LookAt( m_position, m_position + Vec3( -1.0, 0.0, 0.0 ), Vec3( 0.0, -1.0, 0.0 ) );		
	} else if ( m_shadowFaceIdx == 2 ) {
		view.LookAt( m_position, m_position + Vec3( 0.0, 1.0, 0.0 ), Vec3( 0.0, 0.0, 1.0 ) );
	} else if ( m_shadowFaceIdx == 3 ) {
		view.LookAt( m_position, m_position + Vec3( 0.0, -1.0, 0.0 ), Vec3( 0.0, 0.0, -1.0 ) );
	} else if ( m_shadowFaceIdx == 4 ) {
		view.LookAt( m_position, m_position + Vec3( 0.0, 0.0, 1.0 ), Vec3( 0.0, -1.0, 0.0 ) );
	} else {
		view.LookAt( m_position, m_position + Vec3( 0.0, 0.0, -1.0 ), Vec3( 0.0, -1.0, 0.0 ) );
	}
	m_xfrm = shadowProj * view;

	return m_xfrm.as_ptr();
}

/*
================================
PointLight::DrawDepthBuffer
	-Bind the depth cubemap and draw it on the debug skybox cube
================================
*/
void PointLight::DrawDepthBuffer( const float * view, const float * projection ) {
	//must remove translation from view matrix to keep it bound to camera
	float skyBox_view[16] = { 0.0f };
	for ( int i = 0; i < 16; ++i ) {
		skyBox_view[i] = view[i];
	}
	skyBox_view[12] = 0.0f;
	skyBox_view[13] = 0.0f;
	skyBox_view[14] = 0.0f;
	skyBox_view[15] = 1.0f;

	//before drawing the texture to the buffer screen quad, the comparison mode used by shadow sampler needs to be disabled.
	glBindTexture( GL_TEXTURE_CUBE_MAP, m_depthBuffer.m_attachements[0] );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE );

	//draw the depth cubemap skybox to the m_depthCubeViewFBO render target using the skybox shader
	glViewport( 0, 0, m_depthCubeView.GetWidth(), m_depthCubeView.GetHeight() );
	m_depthCubeView.Bind();
	Shader * depthCubeShader = m_depthBuffer.GetShader();
	depthCubeShader->UseProgram();
	depthCubeShader->SetUniformMatrix4f( "view", 1, false, skyBox_view );
	depthCubeShader->SetUniformMatrix4f( "projection", 1, false, projection );
	depthCubeShader->SetAndBindUniformTexture( "cubemap", 0, GL_TEXTURE_CUBE_MAP, m_depthBuffer.m_attachements[0] );
	m_depthCube.DrawSurface( true ); //draw cube
	m_depthCubeView.Unbind();

	//draw the m_depthCubeView texture to a quad for rendering
	glViewport( 0, 0, glutGet( GLUT_WINDOW_WIDTH ), glutGet( GLUT_WINDOW_HEIGHT ) );
	m_depthCubeView.Draw( m_depthCubeView.m_attachements[0] );
}

/*
================================
PointLight::EnableShadows
================================
*/
void PointLight::EnableShadows(){
	//compile depth shader that will be applied to the entire scene
	m_depthShader = m_depthShader->GetShader( "linearDepth" );
	if ( m_depthShader == NULL ) {
		return;
	}
	m_shadowCaster = true;

	//configure depth map FBO that contains the depth cubemap
	//it also contains the shader used render the cubemap to a texture in m_depthCubeView FBO given camera matrices
	m_depthBuffer.CreateNewBuffer( 512, 512, "cubemap" );
	m_depthBuffer.AttachCubeMapTextureBuffer( GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT );
    glDrawBuffer( GL_NONE ); //FBO cant be complete without color buffer. Since we dont need one, we use this command to override.
    glReadBuffer( GL_NONE ); //FBO cant be complete without color buffer. Since we dont need one, we use this command to override.
	assert( m_depthBuffer.Status() );
	m_depthBuffer.Unbind();

	//configure the FBO used to display the depth cubemap
	//m_depthBuffer FBO will write to m_depthCubeView FBO once bound.
	m_depthCubeView = Framebuffer( "screenTexture" );
	m_depthCubeView.CreateDefaultBuffer( 512, 512 );
	m_depthCubeView.AttachTextureBuffer();
	assert( m_depthCubeView.Status() );
	m_depthCubeView.CreateScreen();
	m_depthCubeView.Unbind();

	//compile shader used to display cubemap onto a 2d texture
	m_depthCube = Cube( "" ); //create skybox geo that will get said shader
}

/*
================================
Light::PointLight
	-shader uses sampler2DShadow where frag coord arg is a vec3 whose
	-z-value is the float to compare against.
================================
*/
void PointLight::EnableSamplerCompareMode() {
	glBindTexture( GL_TEXTURE_CUBE_MAP, m_depthBuffer.m_attachements[0] );

	//allows using shadowCubeMap as the sampler for the shadowmap texture
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
}

/*
================================
AmbientLight::DebugDraw
	-Draws light at constant size by denamically scaling it.
================================
*/
void AmbientLight::DebugDraw( Camera * camera, const float * view, const float * projection ) {
	if ( s_drawEnable ) {
		m_debugShader->UseProgram();

		//calculate model matrix
		Mat4 translation = Mat4();
		translation.Translate( m_position );

		Vec3 camToLight = ( camera->m_position - m_position );
		float dist = camToLight.length() * 0.05f;
		Mat4 scale = Mat4();
		for ( int i = 0; i < 3; i++ ) {
			scale[i][i] = dist;
		}

		Mat4 model = translation * scale;

		//pass uniforms to shader
		m_debugShader->SetUniformMatrix4f( "view", 1, false, view );
		m_debugShader->SetUniformMatrix4f( "model", 1, false, model.as_ptr() );
		m_debugShader->SetUniformMatrix4f( "projection", 1, false, projection );		
		
		//render debug light model
		glBindVertexArray( m_mesh.m_surfaces[0]->VAO );
		glDrawElements( GL_TRIANGLES, m_mesh.m_surfaces[0]->triCount * 3, GL_UNSIGNED_INT, 0 );
	}
}

/*
================================
AmbientLight::PassUniforms
================================
*/
void AmbientLight::PassUniforms( Shader * shader ) {
	const int typeIndex = 4;
	shader->SetUniform1i( "light.typeIndex", 1, &typeIndex );
	shader->SetUniform1f( "light.ambient", 1, &m_strength );
	shader->SetUniform3f( "light.color", 1, m_color.as_ptr() );
	const int shadow = 0;
	shader->SetUniform1i( "light.shadow", 1, &shadow );
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
		s_ambientPass_shader = s_ambientPass_shader->GetShader( "ambientPass" );
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
		s_ambientPass_shader = s_ambientPass_shader->GetShader( "ambientPass" );
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
bool EnvProbe::PassUniforms( Str materialDeclName ) {
	MaterialDecl * matDecl = MaterialDecl::GetMaterialDecl( materialDeclName.c_str() );
	if ( matDecl->m_shaderProg == "error" ) {
		return false;
	}

	s_ambientPass_shader->UseProgram();
	unsigned int slotCount = 0;

	//bind model textures
	textureMap::iterator it = matDecl->m_textures.begin();
	while ( it != matDecl->m_textures.end() ) {
		std::string uniformName = it->first;
		Texture* texture = it->second;
		s_ambientPass_shader->SetAndBindUniformTexture( uniformName.c_str(), slotCount, texture->GetTarget(), texture->GetName() );
		slotCount++;
		it++;
	}
	s_ambientPass_shader->SetAndBindUniformTexture( "irradianceMap", slotCount, m_irradianceMap.GetTarget(), m_irradianceMap.GetName() );
	s_ambientPass_shader->SetAndBindUniformTexture( "prefilteredEnvMap", slotCount + 1, m_environmentMap.GetTarget(), m_environmentMap.GetName() );
	s_ambientPass_shader->SetAndBindUniformTexture( "brdfLUT", slotCount + 2, s_brdfIntegrationMap.GetTarget(), s_brdfIntegrationMap.GetName() );

	return true;
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
	for ( int i = 0; i < scene->LightCount(); i++ ) {
		Light * light = NULL;
		scene->LightByIndex( i, &light );

		//Create shadow map for shadowcasting lights
		if ( light->m_shadowCaster ) {
			//configure opengl to render the shadowmap
			glViewport( 0, 0, light->m_depthBuffer.GetWidth(), light->m_depthBuffer.GetHeight() );
			light->BindDepthBuffer();

			//render depth of scene to texture (from this lights perspective)
			light->m_depthShader->UseProgram(); //use depth shader
			light->EnableSamplerCompareMode();
			light->PassDepthShaderUniforms(); //pass light matrix to depth shader and activate a face of the cubmap

			//iterate through each surface of each mesh in the scene and render it with m_depthShader active
			for ( int n = 0; n < scene->MeshCount(); n++ ) {			
				Mesh * mesh = NULL;
				scene->MeshByIndex( n, &mesh );
				for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
					mesh->DrawSurface( j ); //draw surface
				}
			}
			light->m_depthBuffer.Unbind();
		}

		//Draw the scene to the main buffer
		
		glViewport( 0, 0, cubemapSize, cubemapSize ); //Set the OpenGL viewport to be the entire size of the window
		if ( i > 0 ) {
			//subsequent lighting passes add their contributions now that the first one has set all initial depth values.
			glBlendFunc( GL_ONE, GL_ONE );
		}

		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			fbos[faceIdx].Bind(); //bind the framebuffer so all subsequent drawing is to it.
			glClear( GL_DEPTH_BUFFER_BIT ); //clear only depth because we wanna keep skybox color

			MaterialDecl* matDecl;
			for ( int n = 0; n < scene->MeshCount(); n++ ) {
				Mesh * mesh = NULL;
				scene->MeshByIndex( n, &mesh );

				for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
					matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[j]->materialName.c_str() );
					matDecl->BindTextures();

					//pass in camera matrices
					matDecl->shader->SetUniformMatrix4f( "view", 1, false, views[faceIdx].as_ptr() );		
					matDecl->shader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );

					//exit early if this material is errored out
					if ( matDecl->m_shaderProg == "error" ) {
						mesh->DrawSurface( j ); //draw surface
						continue;
					}

					//pass camera position
					matDecl->shader->SetUniform3f( "camPos", 1, m_position.as_ptr() );

					//pass lights data
					light->PassUniforms( matDecl->shader );
	
					//draw surface
					mesh->DrawSurface( j );
				}
			}
			fbos[faceIdx].Unbind();
		}		
	}

	//gather all texture ids from color attachments of the fbos
	std::vector< unsigned int > textureIDs;
	for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {	
		textureIDs.push_back( fbos[faceIdx].m_attachements[0] );
	}
	return textureIDs;
}