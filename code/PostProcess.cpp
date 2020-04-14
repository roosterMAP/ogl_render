#include "PostProcess.h"

/*
===============================
PostProcessManager::PostProccessManager
	-Initialize internal framebuffer
===============================
*/
PostProcessManager::PostProcessManager( unsigned int width, unsigned int height, const char * shaderPrefix ) {
	m_bloomEnabled = false;
	m_lutEnabled = false;
	m_lutTexture = NULL;

	m_PostProcessFBO = Framebuffer( "screenTexture" );
	m_PostProcessFBO.CreateNewBuffer( width, height, shaderPrefix );
	m_PostProcessFBO.AttachTextureBuffer( GL_RGBA16F, GL_COLOR_ATTACHMENT0, GL_RGBA, GL_FLOAT );
	//m_PostProcessFBO.AttachRenderBuffer();
	m_PostProcessFBO.AttachTextureBuffer( GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT );
	m_PostProcessFBO.CreateScreen();
	m_PostProcessFBO.Unbind();

	Shader * postProcessQuad_shader = m_PostProcessFBO.GetShader();
	postProcessQuad_shader->PinShader();
}

/*
===============================
PostProcessManager::SetBlitParams
	-Set blit frames for next call of BlitFramebuffer().
===============================
*/
void PostProcessManager::SetBlitParams( Vec2 srcMin, Vec2 srcMax, Vec2 dstMin, Vec2 dstMax ) {
	//source frame
	m_blit_srcX = ( unsigned int )srcMin.x;
	m_blit_srcY = ( unsigned int )srcMin.y;
	m_blit_srcW = ( unsigned int )srcMax.x - ( unsigned int )srcMin.x;
	m_blit_srcH = ( unsigned int )srcMax.y - ( unsigned int )srcMin.y;

	//destination frame
	m_blit_dstX = ( unsigned int )dstMin.x;
	m_blit_dstY = ( unsigned int )dstMin.y;
	m_blit_dstW = ( unsigned int )dstMax.x - ( unsigned int )dstMin.x;
	m_blit_dstH = ( unsigned int )dstMax.y - ( unsigned int )dstMin.y;
}

/*
===============================
PostProcessManager::BlitFramebuffer
	-Copy the contents of the inputFBO to m_PostProcessFBO
===============================
*/
void PostProcessManager::BlitFramebuffer( Framebuffer * inputFBO ) {
	glBindFramebuffer( GL_READ_FRAMEBUFFER, inputFBO->GetID() ); //set source FBO
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_PostProcessFBO.GetID() ); //set destination FBO
	glBlitFramebuffer(	m_blit_srcX, m_blit_srcY, m_blit_srcW, m_blit_srcH,
						m_blit_dstX, m_blit_dstY, m_blit_dstW, m_blit_dstH,
						GL_COLOR_BUFFER_BIT, GL_NEAREST	);
	glBlitFramebuffer(	m_blit_srcX, m_blit_srcY, m_blit_srcW, m_blit_srcH,
						m_blit_dstX, m_blit_dstY, m_blit_dstW, m_blit_dstH,
						GL_DEPTH_BUFFER_BIT, GL_NEAREST	);
	glBindFramebuffer( GL_FRAMEBUFFER, 0 ); //unbind
}

/*
===============================
linearGaussianBlur
	-http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
===============================
*/
void linearGaussianBlur( unsigned int tapCount, float ** offsets_linearSampling, float ** weights_linearSampling ) {
	//build pascal triangle
	const unsigned int triHeight = 13;
	std::vector< std::vector< float > > pascalTri;
	std::vector<float> seed{ 1.0f };
	pascalTri.push_back( seed );
	for ( unsigned int i = 1; i < triHeight; i++ ) {
		std::vector<float> row;
		row.resize( i + 1 );
		row[0] = 1.0f;
		row[i] = 1.0f;
		for ( unsigned int j = 1; j < i; j++ ) {
			row[j] = pascalTri[i-1][j-1] + pascalTri[i-1][j];
		}
		pascalTri.push_back( row );
	}

	//get weight coefficients
	assert( tapCount % 2 != 0.0 ); //tapCount must be an odd number
	const unsigned int coefCount = ( tapCount + 1 ) / 2;
	float * weights = new float[coefCount];
	float coefSum = 0.0f;
	for ( unsigned int i = 0; i < coefCount - 1; i++ ) {
		//we use the 9+3th row to get a larger sample.
		//we skip the two smaller coefs cuz they too small to have an effect.
		float coef = pascalTri[tapCount+3][i+2];
		coefSum += coef * 2.0f;
		weights[coefCount - i - 1] = coef; //load in reverse order
	}
	weights[0] = pascalTri[tapCount+3][coefCount+1];
	coefSum += weights[0];
	//normalize the weights by dividing them by their sum
	for ( unsigned int i = 0; i < coefCount; i++ ) {
		weights[i] /= coefSum;
	}

	//get the weights for linear sampling optimization
	const unsigned int coefCount_linearSampling = ( coefCount + 1 ) / 2;
	*weights_linearSampling = new float[coefCount_linearSampling];
	( *weights_linearSampling )[0] = weights[0];
	for ( unsigned int i = 0; i < coefCount_linearSampling - 1; i++ ) {
		float sum = weights[ i * 2 + 1 ] + weights[ i * 2 + 2 ];
		( *weights_linearSampling )[ i + 1 ] = sum;
	}

	//calculate the offest for linear sampling optimization
	float * offsets = new float[coefCount];
	for ( unsigned int i = 0; i < coefCount; i++ ) {
		offsets[i] = ( float )i;
	}
	*offsets_linearSampling = new float[coefCount_linearSampling];
	( *offsets_linearSampling )[0] = 0.0f;
	for ( unsigned int i = 0; i < coefCount_linearSampling - 1; i++ ) {
		float o_d = (float)i * 2.0f + 1.0f; //discreet offset
		float offset = o_d * weights[ i * 2 + 1 ] + ( o_d + 1.0f ) * weights[ i * 2 + 2 ];
		( *offsets_linearSampling )[ i + 1 ] = offset / ( *weights_linearSampling )[ i + 1 ];
	}

	//cleanup
	delete[] weights;
	weights = nullptr;
	delete[] offsets;
	offsets = nullptr;
}

/*
===============================
PostProcessManager::BloomEnable
	-Adds a second rendertarget to m_PostProcessFBO
	-Inits up m_bloomFBOs
===============================
*/
bool PostProcessManager::BloomEnable( float resScale ) {
	m_bloomEnabled = true;

	//initialize the bloom threshold FBO and the associated member uniform
	m_bloomThresholdFBO = Framebuffer( "screenTexture" );
	m_bloomThresholdFBO.CreateNewBuffer( ( unsigned int )m_PostProcessFBO.GetWidth() * resScale, ( unsigned int )m_PostProcessFBO.GetHeight() * resScale, "bloom_threshold" );
	m_bloomThresholdFBO.AttachTextureBuffer( GL_RGBA16F, GL_COLOR_ATTACHMENT0, GL_RGBA, GL_FLOAT );
	if( !m_bloomThresholdFBO.Status() ) {
		return false;
	}
	m_bloomThresholdFBO.Unbind();
	m_bloomThresholdFBO.CreateScreen();
	m_bloomThreshold = 1.5f;
	
	//init bloom framebuffers
	for ( unsigned int i = 0; i < 2; i++ ){
		m_bloomFBOs[i].SetColorBufferUniform( "image" );
		m_bloomFBOs[i].CreateNewBuffer( ( unsigned int )m_PostProcessFBO.GetWidth() * resScale, ( unsigned int )m_PostProcessFBO.GetHeight() * resScale, "bloom_gaussian" );
		m_bloomFBOs[i].AttachTextureBuffer( GL_RGBA16F, GL_COLOR_ATTACHMENT0, GL_RGBA, GL_FLOAT );
		if ( !m_bloomFBOs[i].Status() ) {
			return false;
		}
		m_bloomFBOs[i].Unbind();
	}
	m_bloomFBOs[0].CreateScreen(); //create a screen for the first FBO because only one is needed (they use the same shader)

	//precompute weight and offset attributes for gaussian blur fragment shader and pass uniforms
	const unsigned int tapCount = 9;
	const unsigned int coefCount = ( tapCount + 1 ) / 2;
	float * weights;
	float * offsets;
	linearGaussianBlur( tapCount, &offsets, &weights );
	Shader * bloomShader = m_bloomFBOs[0].GetShader();
	bloomShader->UseProgram();
	bloomShader->SetUniform1f( "offsets", coefCount, offsets );
	bloomShader->SetUniform1f( "weights", coefCount, weights );
	delete[] weights;
	weights = nullptr;
	delete[] offsets;
	offsets = nullptr;

	//pin the bloom shaderprogs so they dont get sleared on scene load/reload
	Shader * bloomThresholdShader = m_bloomThresholdFBO.GetShader();
	bloomThresholdShader->PinShader();
	bloomShader->PinShader();

	return true;
}

/*
===============================
PostProcessManager::Bloom
	-Performs bloom operation on m_PostProcessFBO
===============================
*/
void PostProcessManager::Bloom() {
	if ( !m_bloomEnabled ) {
		return;
	}

	//clamp the postprocess framebuffer and write to the render target of m_bloomThresholdFBO
	Shader * bloomThresholdShader = m_bloomThresholdFBO.GetShader();
	bloomThresholdShader->UseProgram();
	bloomThresholdShader->SetUniform1f( "threshold", 1, &m_bloomThreshold );
	m_bloomThresholdFBO.Bind();	
	m_bloomThresholdFBO.Draw( m_PostProcessFBO.m_attachements[0] );
	m_bloomThresholdFBO.Unbind();

	//reads from the bloom render target from m_PostProcessFBO and blurs
	//horizontally, writing to the first bloomFBO target.
	//then reads the render target of the first bloomFBO and blurs vertically,
	//writing to the second bloomFBO target.
	bool horizontalSwitch = true;
	bool first_iteration = true;
	Shader * bloomShader = m_bloomFBOs[horizontalSwitch].GetShader();
	bloomShader->UseProgram();
	for ( int i = 0; i < 6; i++ ) {
		const int horizontalUniform = ( int )horizontalSwitch;
		m_bloomFBOs[horizontalSwitch].Bind();
		bloomShader->SetUniform1i( "horizontal", 1, &horizontalUniform );

		if ( first_iteration ) {
			m_bloomFBOs[0].Draw( m_bloomThresholdFBO.m_attachements[0] );
		} else {
			m_bloomFBOs[0].Draw( m_bloomFBOs[!horizontalSwitch].m_attachements[0] );
		}

		horizontalSwitch = !horizontalSwitch;
		if ( first_iteration ) {
			first_iteration = false;
		}
	}
	m_bloomFBOs[0].Unbind();

	//pass in second bloomFBO render target to be additively blended with m_PostProcessFBO target.
	Shader * postProcessQuad_shader = m_PostProcessFBO.GetShader();
	postProcessQuad_shader->UseProgram();
	postProcessQuad_shader->SetAndBindUniformTexture( "bloomTexture", 1, GL_TEXTURE_2D, m_bloomFBOs[0].m_attachements[0] );
}

/*
===============================
PostProcessManager::SSAOEnable
	-create framebuffer for SSAO
===============================
*/
bool PostProcessManager::SSAOEnable( const Camera * camera ) {
	m_ssaoEnabled = true;
	m_camera = camera;

	//init ssao framebuffer
	m_ssaoFBO = Framebuffer( "depthTexture" );

	m_ssaoFBO.CreateNewBuffer( ( unsigned int )m_PostProcessFBO.GetWidth(), ( unsigned int )m_PostProcessFBO.GetHeight(), "ssao" );
	m_ssaoFBO.AttachTextureBuffer( GL_RGB, GL_COLOR_ATTACHMENT0, GL_RGB, GL_UNSIGNED_BYTE );
	if ( !m_ssaoFBO.Status() ) {
		return false;
	}
	m_ssaoFBO.Unbind();
	m_ssaoFBO.CreateScreen();

	//pin ssao shader
	Shader * ssaoShader = m_ssaoFBO.GetShader();
	ssaoShader->PinShader();

	//pass uniforms that can be pre-calculated

	//max dist for sampler fragment to be able to occlude current fragment
	const float sampleRadius = 0.85f;
	ssaoShader->SetUniform1f( "sampleRadius", 1, &sampleRadius );

	//intensity of occlusion :: ao = pow( ao, strenth );
	const float strength = 0.9f;
	ssaoShader->SetUniform1f( "strength", 1, &strength );

	//generate samples between (-1.0, 1.0) and store in karnel array
	Vec3 kernel[SSAO_KERNEL_SIZE];
	for ( int i = 0 ; i < SSAO_KERNEL_SIZE ; i++ ) {
		float scale = ( float )i / ( float )( SSAO_KERNEL_SIZE );
		Vec3 v;
		v.x = 2.0f * ( float )rand() / RAND_MAX - 1.0f;
		v.y = 2.0f * ( float )rand() / RAND_MAX - 1.0f;
		v.z = 2.0f * ( float )rand() / RAND_MAX - 1.0f;

		// Use an acceleration function so more points are located closer to the origin
		v *= ( 0.1f + 0.9f * scale * scale );		
		kernel[i] = v;
	}
	ssaoShader->SetUniform3f( "kernel", SSAO_KERNEL_SIZE, kernel[0].as_ptr() );

	return true;
}

/*
===============================
PostProcessManager::SSAO
	-updates the color attachement to the ssao fbo texture attachement
===============================
*/
void PostProcessManager::SSAO() {
	if ( !m_ssaoEnabled ) {
		return;
	}

	Shader * ssaoShader = m_ssaoFBO.GetShader();
	ssaoShader->UseProgram();

	//for some reason, projection matrix must be adjusted
	const Mat4 projection = m_camera->GetProjection();
	Mat4 aoProjection = projection;
	aoProjection[2][3] = 1.0f;
	ssaoShader->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );

	const float aspect = ( float )m_PostProcessFBO.GetWidth() / ( float )m_PostProcessFBO.GetHeight();
	ssaoShader->SetUniform1f( "aspect", 1, &aspect );

	const float tanHalfFOV = tanf( to_radians( m_camera->m_fov / 2.0f ) );
	ssaoShader->SetUniform1f( "tanHalfFOV", 1, &tanHalfFOV );

	ssaoShader->SetUniform1f( "near", 1, &m_camera->m_near );
	ssaoShader->SetUniform1f( "far", 1, &m_camera->m_far );
	
	m_ssaoFBO.Bind();
	m_ssaoFBO.Draw( m_PostProcessFBO.m_attachements[1] );
	m_ssaoFBO.Unbind();

	//pass ssaoFBO render target to post process shader.
	Shader * postProcessQuad_shader = m_PostProcessFBO.GetShader();
	postProcessQuad_shader->UseProgram();
	postProcessQuad_shader->SetAndBindUniformTexture( "ssaoTexture", 2, GL_TEXTURE_2D, m_ssaoFBO.m_attachements[0] );
}

/*
===============================
PostProcessManager::LUTEnable
===============================
*/
void PostProcessManager::LUTEnable( Str lut_image ) {
	m_lutTexture = new LUTTexture();
	if ( !m_lutTexture->InitFromFile( lut_image.c_str() ) ) {
		return;
	}

	//pass in second bloomFBO render target to bne additively blended with m_PostProcessFBO target.
	Shader * postProcessQuad_shader = m_PostProcessFBO.GetShader();
	postProcessQuad_shader->UseProgram();
	postProcessQuad_shader->SetAndBindUniformTexture( "lookup", 3, GL_TEXTURE_3D, m_lutTexture->GetName() );
}

/*
===============================
PostProcessManager::Draw
===============================
*/
void PostProcessManager::Draw( const float exposure ) {
	Shader * postProcessQuad_shader = m_PostProcessFBO.GetShader();
	postProcessQuad_shader->UseProgram();

	const int bloomEnabled = ( int )m_bloomEnabled;
	postProcessQuad_shader->SetUniform1i( "bloomEnabled", 1, &bloomEnabled );
	postProcessQuad_shader->SetUniform1f( "exposure", 1, &exposure );

	const int ssaoEnabled = ( int )m_ssaoEnabled;
	postProcessQuad_shader->SetUniform1i( "ssaoEnabled", 1, &ssaoEnabled );

	m_PostProcessFBO.Draw( m_PostProcessFBO.m_attachements[0] );
}

/*
===============================
PostProcessManager::DrawBloomOnly
	-draws the post process fbo with the first bloomFBO attachement as the passed in input texture
===============================
*/
void PostProcessManager::DrawBloomOnly( const float exposure ) {
	Shader * framebuffer_shader = NULL;
	framebuffer_shader = framebuffer_shader->GetShader( "framebuffer" );
	framebuffer_shader->UseProgram();

	glDisable( GL_DEPTH_TEST );

	framebuffer_shader->UseProgram();
	framebuffer_shader->SetAndBindUniformTexture( "screenTexture", 0, GL_TEXTURE_2D, m_bloomFBOs[0].m_attachements[0] );

	glBindVertexArray( m_PostProcessFBO.GetScreenVAO() );
	glDrawArrays( GL_QUADS, 0, 4 );	

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArray( 0 );

	glEnable( GL_DEPTH_TEST );
}

/*
===============================
PostProcessManager::DrawSSAOOnly
	-draws the post process fbo with the ssao attachement as the passed in input texture
===============================
*/
void PostProcessManager::DrawSSAOOnly( const float exposure ) {
	Shader * framebuffer_shader = NULL;
	framebuffer_shader = framebuffer_shader->GetShader( "framebuffer" );
	framebuffer_shader->UseProgram();

	glDisable( GL_DEPTH_TEST );

	framebuffer_shader->UseProgram();
	framebuffer_shader->SetAndBindUniformTexture( "screenTexture", 0, GL_TEXTURE_2D, m_ssaoFBO.m_attachements[0] );

	glBindVertexArray( m_PostProcessFBO.GetScreenVAO() );
	glDrawArrays( GL_QUADS, 0, 4 );	

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArray( 0 );

	glEnable( GL_DEPTH_TEST );
}