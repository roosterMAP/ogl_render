#pragma once

#include "Command.h"
#include "Console.h"
#include "Scene.h"

#include <assert.h>
#include <GL/freeglut.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

//declare cvars
CVar * g_cvar_debugLighting = new CVar();
CVar * g_cvar_renderLightModels = new CVar();
CVar * g_cvar_showVertTransform = new CVar();
CVar * g_cvar_showEdgeHighlights = new CVar();
CVar * g_cvar_brdfIntegrateLUT = new CVar();

CommandSys * g_cmdSys = CommandSys::getInstance(); //declare g_cmdSys singleton

CommandSys * CommandSys::inst_ = NULL; //Define the static Singleton pointer

/*
================================
Fn_TestLog
================================
*/
void Fn_TestLog( Str args ) {
	Console * console = Console::getInstance(); //retrieve console singleton
	console->AddInfo( "this is info test" );
	console->AddWarning( "this is warning test" );
	console->AddError( "this is error test" );
}

/*
================================
Fn_Quit
================================
*/
void Fn_Quit( Str args ) {
	int window = glutGetWindow();
	glutDestroyMenu( window );
	exit( 0 );
}

/*
================================
Fn_DebugLighting
================================
*/
void Fn_DebugLighting( Str args ) {
	const int argVal =  atoi( args.c_str() );
	if ( argVal == 0 ) {
		g_cvar_debugLighting->SetState( false );
	} else if ( argVal < 0 || argVal > 6 ) {
		Console * console = Console::getInstance();
		console->AddError( "debugLighting :: Invalid Arg!!!" );
	} else {
		g_cvar_debugLighting->SetState( true );
		g_cvar_debugLighting->SetArgs( args );
	}
}

/*
================================
Fn_ShowVertTransforms
================================
*/
void Fn_ShowVertTransforms( Str args ) {
	if ( atoi( args.c_str() ) == 0 ) {
		g_cvar_showVertTransform->SetState( false );
	} else if ( atoi( args.c_str() ) == 1 ) {
		g_cvar_showVertTransform->SetState( true );
	}
}

/*
================================
Fn_DebugLights
================================
*/
void Fn_RenderLightModels( Str args ) {
	if ( atoi( args.c_str() ) == 0 ) {
		g_cvar_renderLightModels->SetState( false );
	} else {
		g_cvar_renderLightModels->SetState( true );
		g_cvar_renderLightModels->SetArgs( args );
	}
}

/*
================================
Fn_ShowEdgeHighlights
================================
*/
void Fn_ShowEdgeHighlights( Str args ) {
	int arg_int = atoi( args.c_str() );
	if ( arg_int > 0 && arg_int < 3 ) {
		g_cvar_showEdgeHighlights->SetState( true );
		g_cvar_showEdgeHighlights->SetArgs( args );
	} else {
		g_cvar_showEdgeHighlights->SetState( false );
		g_cvar_showEdgeHighlights->SetArgs( Str( '0' ) );
	}
}

/*
================================
Fn_GenerateLUT
================================
*/
void Fn_GenerateLUT( Str args ) {
	args.Strip();

	//generate relative path
	Str relativePath = "data\\texture\\";
	if ( args.Length() <= 0 ) {
		relativePath.Append( "LUT_default" );
	} else {
		relativePath.Append( args );
	}
	relativePath.Append( ".tga" );

	//generate absolute path
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath.c_str(), imgPath );

	//generate default LUT data
	uint8_t data[256][16][3];
	for ( unsigned int i = 0; i < 16; i++ ) {
		for ( unsigned int j = 0; j < 16; j++ ) {
			for ( unsigned int k = 0; k < 16; k++ ) {
				data[ i * 16 + k ][ j ][ 0 ] = i * 16;
				data[ i * 16 + k ][ j ][ 1 ] = j * 16;
				data[ i * 16 + k ][ j ][ 2 ] = k * 16;
			}
		}
	}
	stbi_write_tga( imgPath, 256, 16, 3, data ); //save tga

	Console * console = Console::getInstance(); //retrieve console singleton
	Str info( "LUT image saved to: " );
	info.Append( Str( imgPath ) );
	console->AddInfo( info.c_str() );
}

/*
================================
Fn_EquirectangularToCubeMap
================================
*/
void Fn_EquirectangularToCubeMap( Str args ) {
	Console * console = Console::getInstance(); //retrieve console singleton

	args.Strip();
	const Str equImg_relative = args;

	//ensure an arg is present
	if ( equImg_relative.Length() < 1 ) {
		console->AddError( "equirectangularToCubeMap :: RelativePath arg required!!!" );
		return;
	}	

	//generate absolute path for the input equirectangular map
	char imgPath[ 2048 ];
	if ( RelativePathToFullPath( equImg_relative.c_str(), imgPath ) == false ) {
		console->AddError( "equirectangularToCubeMap :: Invalid arg!!!" );
		return;
	}
	Str equImg_absolute = Str( imgPath );
	Str cubeMap_absolute;

	//read data from image file
	int width, height, nrChannels;
	const Vec3 x_axis = Vec3( 1.0, 0.0, 0.0 );
	const Vec3 y_axis = Vec3( 0.0, 1.0, 0.0 );
	const Vec3 z_axis = Vec3( 0.0, 0.0, 1.0 );

	if ( equImg_absolute.EndsWith( ".tga" ) ) {
		unsigned char *data = stbi_load( equImg_absolute.c_str(), &width, &height, &nrChannels, 0 );
		if ( data ) {
			const int cubemapSize = height / 2;
			const float halfSize = ( float )cubemapSize / 2.0f;
			uint8_t * output_data = new uint8_t[ cubemapSize * 6 * cubemapSize * 3 ];

			//generate cubemap image here
			for ( int i = 0; i < cubemapSize; i++ ) {
				for ( int j = 0; j < cubemapSize * 6; j++ ) {
					const int k = j % cubemapSize;
				
					Vec3 sampleVec;
					if ( j < cubemapSize ) { //front
						sampleVec = ( z_axis * -halfSize ) + ( Vec3( k, i, 0 ) - Vec3( halfSize, halfSize, 0.0 ) );
					} else if ( j < cubemapSize * 2 ) { //back
						sampleVec = ( z_axis * halfSize ) + ( Vec3( abs( cubemapSize - k ), i, 0 ) - Vec3( halfSize, halfSize, 0.0 ) );
					} else if ( j < cubemapSize * 3 ) { //up
						sampleVec = ( y_axis * -halfSize ) + ( Vec3( abs( cubemapSize - i ), 0, abs( cubemapSize - k ) ) - Vec3( halfSize, 0.0, halfSize ) );
					} else if ( j < cubemapSize * 4 ) { //down
						sampleVec = ( y_axis * halfSize ) + ( Vec3( i, 0, abs( cubemapSize - k ) ) - Vec3( halfSize, 0.0, halfSize ) );
					} else if ( j < cubemapSize * 5 ) { //right
						sampleVec = ( x_axis * -halfSize ) + ( Vec3( 0, i, abs( cubemapSize - k ) ) - Vec3( 0.0, halfSize, halfSize ) );
					} else if ( j < cubemapSize * 6 ) { //left
						sampleVec = ( x_axis * halfSize ) + ( Vec3( 0, i, k ) - Vec3( 0.0, halfSize, halfSize ) );
					}
					sampleVec.normalize();

					//convert to uv coords of spherical map
					const Vec2 invAtan = Vec2( 0.1591, 0.3183 );
					Vec2 uv = Vec2( atan2( sampleVec.z, sampleVec.x ), asin( sampleVec.y ) );
					uv[0] *= invAtan[0];
					uv[1] *= invAtan[1];
					uv += 0.5;

					//write to output
					unsigned int uv_x = ( unsigned int )( uv.x * ( float )width );
					unsigned int uv_y = ( unsigned int )( uv.y * ( float )height );
					unsigned int data_idx = ( width * uv_y + uv_x ) * 3;
					const unsigned int output_data_idx = ( cubemapSize * 6 * 3 * i ) + ( 3 * j );
					output_data[ output_data_idx + 0 ] = data[ data_idx + 0 ];
					output_data[ output_data_idx + 1 ] = data[ data_idx + 1 ];
					output_data[ output_data_idx + 2 ] = data[ data_idx + 2 ];
				}
			}

			//generate output image path
			cubeMap_absolute = equImg_absolute.Substring( 0, equImg_absolute.Length() - 4 );
			cubeMap_absolute += "_cube.tga";
			stbi_write_tga( cubeMap_absolute.c_str(), cubemapSize * 6, cubemapSize, 3, output_data ); //save image
			delete[] output_data;

		} else {
			console->AddError( "equirectangularToCubeMap :: Equirectangular image could not be found!!!" );
			return;
		}

	} else if ( equImg_absolute.EndsWith( ".hdr" ) ) {
		float *data = stbi_loadf( equImg_absolute.c_str(), &width, &height, &nrChannels, 0 );
		if ( data ) {
			const int cubemapSize = height / 2;
			const float halfSize = ( float )cubemapSize / 2.0f;
			float * output_data = new float[ cubemapSize * 6 * cubemapSize * 3 ];

			//generate cubemap image here
			for ( int i = 0; i < cubemapSize; i++ ) {
				for ( int j = 0; j < cubemapSize * 6; j++ ) {
					const int k = j % cubemapSize;
				
					Vec3 sampleVec;
					if ( j < cubemapSize ) { //front
						sampleVec = ( z_axis * -halfSize ) + ( Vec3( k, i, 0 ) - Vec3( halfSize, halfSize, 0.0 ) );
					} else if ( j < cubemapSize * 2 ) { //back
						sampleVec = ( z_axis * halfSize ) + ( Vec3( abs( cubemapSize - k ), i, 0 ) - Vec3( halfSize, halfSize, 0.0 ) );
					} else if ( j < cubemapSize * 3 ) { //up
						sampleVec = ( y_axis * -halfSize ) + ( Vec3( abs( cubemapSize - i ), 0, abs( cubemapSize - k ) ) - Vec3( halfSize, 0.0, halfSize ) );
					} else if ( j < cubemapSize * 4 ) { //down
						sampleVec = ( y_axis * halfSize ) + ( Vec3( i, 0, abs( cubemapSize - k ) ) - Vec3( halfSize, 0.0, halfSize ) );
					} else if ( j < cubemapSize * 5 ) { //right
						sampleVec = ( x_axis * -halfSize ) + ( Vec3( 0, i, abs( cubemapSize - k ) ) - Vec3( 0.0, halfSize, halfSize ) );
					} else if ( j < cubemapSize * 6 ) { //left
						sampleVec = ( x_axis * halfSize ) + ( Vec3( 0, i, k ) - Vec3( 0.0, halfSize, halfSize ) );
					}
					sampleVec.normalize();

					//convert to uv coords of spherical map
					const Vec2 invAtan = Vec2( 0.1591, 0.3183 );
					Vec2 uv = Vec2( atan2( sampleVec.z, sampleVec.x ), asin( sampleVec.y ) );
					uv[0] *= invAtan[0];
					uv[1] *= invAtan[1];
					uv += 0.5;

					//write to output
					unsigned int uv_x = ( unsigned int )( uv.x * ( float )width );
					unsigned int uv_y = ( unsigned int )( uv.y * ( float )height );
					unsigned int data_idx = ( width * uv_y + uv_x ) * 3;
					const unsigned int output_data_idx = ( cubemapSize * 6 * 3 * i ) + ( 3 * j );
					output_data[ output_data_idx + 0 ] = data[ data_idx + 0 ];
					output_data[ output_data_idx + 1 ] = data[ data_idx + 1 ];
					output_data[ output_data_idx + 2 ] = data[ data_idx + 2 ];
				}
			}

			//generate output image path
			cubeMap_absolute = equImg_absolute.Substring( 0, equImg_absolute.Length() - 4 );
			cubeMap_absolute += "_cube.hdr";
			stbi_write_hdr( cubeMap_absolute.c_str(), cubemapSize * 6, cubemapSize, 3, output_data ); //save image
			delete[] output_data;

		} else {
			console->AddError( "equirectangularToCubeMap :: Equirectangular image could not be found!!!" );
			return;
		}

	} else {
		console->AddError( "equirectangularToCubeMap :: Unsuported filetype!!!" );
		return;
	}

	//report on success
	Str info( "Cubemap image saved to: " );
	info.Append( cubeMap_absolute );
	console->AddInfo( info.c_str() );
}

/*
================================
Fn_ComputeIrradiancceMap
================================
*/
void Fn_ComputeIrradianceMap( Str args ) {
	Console * console = Console::getInstance(); //retrieve console singleton

	args.Strip();
	const Str cubemap_relative = args;
	if ( cubemap_relative.Length() < 1 ) {
		console->AddError( "computeIrradianceMap :: RelativePath arg required!!!" );
		return;
	}
	//generate absolute path for the input equirectangular map
	char imgPath[ 2048 ];
	if ( RelativePathToFullPath( cubemap_relative.c_str(), imgPath ) == false ) {
		console->AddError( "equirectangularToCubeMap :: Invalid arg!!!" );
		return;
	}
	Str cubeMap_absolute = Str( imgPath );

	CubemapTexture cubemapTexture = CubemapTexture();
	if ( cubemapTexture.InitFromFile( cubemap_relative.c_str() ) ) {		
		const int height = 128;
		const int width = 128 * 6;
		const int chanCount = cubemapTexture.GetChanCount();

		//create framebuffer whose color attachement we are drawing to
		Framebuffer irradianceFBO( "cubemap" );
		irradianceFBO.CreateNewBuffer( height, height, "cubemap" );
		irradianceFBO.AttachCubeMapTextureBuffer( GL_RGB16F, GL_COLOR_ATTACHMENT0, GL_RGB, GL_FLOAT );
		if ( !irradianceFBO.Status() ) {
			assert( false );
		}

		//compile irradiance convolution shader and pass uniforms
		Shader * shaderProg = new Shader();
		shaderProg = shaderProg->GetShader( "irradiance_convolution" );
		shaderProg->UseProgram();
		shaderProg->SetAndBindUniformTexture( "environmentMap", 0, cubemapTexture.GetTarget(), cubemapTexture.GetName() );
		Mat4 projection = Mat4();
		projection.Perspective( to_radians( 90.0f ), 1.0f, 0.1f, 10.0f );
		shaderProg->SetUniformMatrix4f( "projection", 1, false, projection.as_ptr() );
		Mat4 views[] = { Mat4(), Mat4(), Mat4(), Mat4(), Mat4(), Mat4() };
		views[0].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
		views[1].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( -1.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
		views[2].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 1.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ) );
		views[3].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ) );
		views[4].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 0.0f, 1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );
		views[5].LookAt( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 0.0f, -1.0f ), Vec3( 0.0f, -1.0f, 0.0f ) );

		//draw to irradianceFBO	by convoluting each face of the environment cubemap
		glViewport( 0, 0, height, height );
		irradianceFBO.Bind();
		Cube renderBox = Cube( "" );
		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			shaderProg->SetUniformMatrix4f( "view", 1, false, views[faceIdx].as_ptr() );
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, irradianceFBO.m_attachements[0], 0 );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			renderBox.DrawSurface( true );
		}
		irradianceFBO.Unbind();
		glViewport( 0, 0, glutGet( GLUT_WINDOW_WIDTH ), glutGet( GLUT_WINDOW_HEIGHT ) );

		//copy irradiance texture data from gpu to cpu
		float * output_data = new float[ width * height * chanCount ];
		float * cubemapFace_data = new float[ height * height * chanCount ];
		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			//copy from gpu to cubemapFace_data
			glBindTexture( GL_TEXTURE_CUBE_MAP, irradianceFBO.m_attachements[0] );
			glGetTexImage( GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, 0, GL_RGB,  GL_FLOAT, cubemapFace_data );

			//write from cubemapFace_data to output_data
			for ( int i = 0; i < height; i++ ) {
				for ( int j = 0; j < height; j++ ) {
					const int k = faceIdx * height + j;
					const int output_idx = ( i * width * chanCount ) + ( k * chanCount );
					const int cubemapFace_idx = ( i * height * chanCount ) + ( j * chanCount );
					output_data[ output_idx + 0 ] = cubemapFace_data[ cubemapFace_idx + 0 ];
					output_data[ output_idx + 1 ] = cubemapFace_data[ cubemapFace_idx + 1 ];
					output_data[ output_idx + 2 ] = cubemapFace_data[ cubemapFace_idx + 2 ];
				}
			}
		}

		//generate output image path
		Str irradianceMap_absolute = cubeMap_absolute.Substring( 0, cubeMap_absolute.Length() - 4 );
		irradianceMap_absolute += "_irr.hdr";
		stbi_write_hdr( irradianceMap_absolute.c_str(), width, height, chanCount, output_data ); //save image
		delete[] cubemapFace_data;
		delete[] output_data;

		//report on success
		Str info( "IrradianceMap image saved to: " );
		info.Append( irradianceMap_absolute );
		console->AddInfo( info.c_str() );
	}
}

/*
================================
Fn_BuildScene
================================
*/
void Fn_BuildScene( Str args ) {
	Console * console = Console::getInstance();
	Scene * scene = Scene::getInstance();

	const Str scene_relativePath = scene->GetName();
	unsigned int idx = scene_relativePath.Find( "scenes" );
	Str probeName = Str( scene_relativePath.c_str() );
	probeName.Replace( "data\\scenes\\", "data\\generated\\envprobes\\", false );
	probeName = probeName.Substring( 0, probeName.Length() - 4 );

	const unsigned int chanCount = 3;
	const unsigned int cubemapSize = 128;
	for ( unsigned int i = 0; i < scene->EnvProbeCount(); i++ ) {
		//create probe img filename
		char idx_str[5];
		itoa( ( int )i, idx_str, 10 );
		Str probe_suffix = Str( idx_str );

		Str environment_relativePath = Str( probeName.c_str() );
		if ( dirExists( environment_relativePath.c_str() ) == false ) {
			makeDir( environment_relativePath.c_str() );
		}
		environment_relativePath.Append( "\\env_" );
		environment_relativePath.Append( probe_suffix );
		environment_relativePath.Append( ".hdr" );
		
		//render the probe
		EnvProbe * probe = NULL;
		scene->EnvProbeByIndex( i, &probe );
		std::vector< unsigned int > envCubemapTextureIDs = probe->RenderCubemaps( cubemapSize );

		//copy env texture data from gpu to cpu
		float * output_data = new float[ cubemapSize * 6 * cubemapSize * chanCount ];
		float * cubemapFace_data = new float[ cubemapSize * 6 * cubemapSize * chanCount ];
		for ( int faceIdx = 0; faceIdx < 6; faceIdx++ ) {
			//copy from gpu to cubemapFace_data
			const unsigned int textureID = envCubemapTextureIDs[faceIdx];
			glBindTexture( GL_TEXTURE_2D, textureID );
			glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB,  GL_FLOAT, cubemapFace_data );

			//write from cubemapFace_data to output_data
			for ( int i = 0; i < cubemapSize; i++ ) {
				for ( int j = 0; j < cubemapSize; j++ ) {
					const int k = faceIdx * cubemapSize + j;
					const int output_idx = ( i * cubemapSize * 6 * chanCount ) + ( k * chanCount );
					const int cubemapFace_idx = ( i * cubemapSize * chanCount ) + ( j * chanCount );
					output_data[ output_idx + 0 ] = cubemapFace_data[ cubemapFace_idx + 0 ];
					output_data[ output_idx + 1 ] = cubemapFace_data[ cubemapFace_idx + 1 ];
					output_data[ output_idx + 2 ] = cubemapFace_data[ cubemapFace_idx + 2 ];
				}
			}
		}

		//generate absolute path for the input equirectangular map
		char imgPath[ 2048 ];
		if ( RelativePathToFullPath( environment_relativePath.c_str(), imgPath ) == false ) {
			console->AddError( "equirectangularToCubeMap :: Invalid arg!!!" );
			return;
		}
		Str environment_absolutePath = Str( imgPath );
		stbi_write_hdr( environment_absolutePath.c_str(), cubemapSize * 6, cubemapSize, chanCount, output_data ); //save image
		delete[] cubemapFace_data;
		delete[] output_data;

		//report on success
		Str info( "Envmap image saved to: " );
		info.Append( environment_absolutePath );
		console->AddInfo( info.c_str() );

		Fn_ComputeIrradianceMap( environment_relativePath );
	}
	console->AddInfo( "Scene build successfull." );
}

/*
================================
Fn_BRDFIntegrationLUT
================================
*/
void Fn_BRDFIntegrationLUT( Str args ) {
	//toggle the state of the cvar
	if ( g_cvar_brdfIntegrateLUT->GetState() ) {
		g_cvar_brdfIntegrateLUT->SetState( false );
	} else {
		g_cvar_brdfIntegrateLUT->SetState( true );
	}

	//build the BRDFIntegration
	Shader * brdfIntegration_shader = new Shader();
	brdfIntegration_shader = brdfIntegration_shader->GetShader( "brdfIntegration" );

	//create Framebuffer whose color attachement is the LUT texture
	const unsigned int lutSize = 512;
	Framebuffer brdfIntegrationFBO = Framebuffer( "screenTexture" );
	brdfIntegrationFBO.CreateNewBuffer( lutSize, lutSize, "framebuffer" );
	brdfIntegrationFBO.AttachTextureBuffer( GL_RGB16F, GL_COLOR_ATTACHMENT0, GL_RGB, GL_FLOAT );
	brdfIntegrationFBO.CreateScreen();

	//draw the screen with brdfIntegration_shader active to generate the LUT on the bound FBO
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	brdfIntegrationFBO.Bind();
	brdfIntegration_shader->UseProgram();
	glBindVertexArray( brdfIntegrationFBO.GetScreenVAO() );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
	brdfIntegrationFBO.Unbind();
	glEnable( GL_DEPTH_TEST );

	//copy LUT texture data from gpu to cpu
	float * output_data = new float[ lutSize * lutSize * 3 ];
	glBindTexture( GL_TEXTURE_2D, brdfIntegrationFBO.m_attachements[0] );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB,  GL_FLOAT, output_data );

	//save the image
	Str relativePath = "data\\texture\\system\\brdfIntegrationLUT.hdr";
	char imgPath[ 2048 ];
	RelativePathToFullPath( relativePath.c_str(), imgPath );
	Str absolutePath = Str( imgPath );
	stbi_write_hdr( absolutePath.c_str(), lutSize, lutSize, 3, output_data ); //save image

	Console * console = Console::getInstance(); //retrieve console singleton
	Str info( "BRDF Integration LUT image saved to: " );
	info.Append( Str( imgPath ) );
	console->AddInfo( info.c_str() );
}

/*
================================
CommandSys::getInstance
================================
*/
CommandSys * CommandSys::getInstance() {
   if ( inst_ == NULL ) {
      inst_ = new CommandSys();
   }
   return( inst_ );
}

/*
================================
CommandSys::BuildCommands
================================
*/
void CommandSys::BuildCommands() {
	Cmd * testLogCommand = new Cmd;
	testLogCommand->name = Str( "testLog" );
	testLogCommand->description = Str( "Prints stuff to the log." );
	testLogCommand->fn = Fn_TestLog;
	m_commands.push_back( testLogCommand );

	Cmd * quitCommand = new Cmd;
	quitCommand->name = Str( "quit" );
	quitCommand->description = Str( "closes application." );
	quitCommand->fn = Fn_Quit;
	m_commands.push_back( quitCommand );

	Cmd * debugLightingCommand = new Cmd;
	debugLightingCommand->name = Str( "debugLighting" );
	debugLightingCommand->description = Str( "Sets a rendering mode." );
	debugLightingCommand->fn = Fn_DebugLighting;
	m_commands.push_back( debugLightingCommand );

	Cmd * showVertTransformsCommand = new Cmd;
	showVertTransformsCommand->name = Str( "showVertTransforms" );
	showVertTransformsCommand->description = Str( "Renders normals, tangents, and bitangents of all rendered vertices." );
	showVertTransformsCommand->fn = Fn_ShowVertTransforms;
	m_commands.push_back( showVertTransformsCommand );

	Cmd * renderLightModelsCommand = new Cmd;
	renderLightModelsCommand->name = Str( "renderLightModels" );
	renderLightModelsCommand->description = Str( "Render debug models for lights in the scene." );
	renderLightModelsCommand->fn = Fn_RenderLightModels;
	m_commands.push_back( renderLightModelsCommand );

	Cmd * createDefaultLUTCommand = new Cmd;
	createDefaultLUTCommand->name = Str( "createLUT" );
	createDefaultLUTCommand->description = Str( "Generate the default LUT tga image. Optional arg is filename without file extension." );
	createDefaultLUTCommand->fn = Fn_GenerateLUT;
	m_commands.push_back( createDefaultLUTCommand );

	Cmd * showEdgeHighlightsCommand = new Cmd;
	showEdgeHighlightsCommand->name = Str( "showEdgeHighlights" );
	showEdgeHighlightsCommand->description = Str( "Render geo as wireframe. Args: 0->off, 1->overlayed, 2->on" );
	showEdgeHighlightsCommand->fn = Fn_ShowEdgeHighlights;
	m_commands.push_back( showEdgeHighlightsCommand );

	Cmd * convertEquirectangularToCubemapCommand = new Cmd;
	convertEquirectangularToCubemapCommand->name = Str( "equirectangularToCubeMap" );
	convertEquirectangularToCubemapCommand->description = Str( "Converts equirectangular map provided by arg (relative path) to a cubemap texture." );
	convertEquirectangularToCubemapCommand->fn = Fn_EquirectangularToCubeMap;
	m_commands.push_back( convertEquirectangularToCubemapCommand );

	Cmd * computeIrradianceMapCommand = new Cmd;
	computeIrradianceMapCommand->name = Str( "computeIrradianceMap" );
	computeIrradianceMapCommand->description = Str( "Convolutes input cubemap and outputs an irradiance map." );
	computeIrradianceMapCommand->fn = Fn_ComputeIrradianceMap;
	m_commands.push_back( computeIrradianceMapCommand );

	Cmd * buildSceneCommand = new Cmd;
	buildSceneCommand->name = Str( "buildScene" );
	buildSceneCommand->description = Str( "Pre-generate all required data for the scene." );
	buildSceneCommand->fn = Fn_BuildScene;
	m_commands.push_back( buildSceneCommand );

	Cmd * brdfIntegrationLUTCommand = new Cmd;
	brdfIntegrationLUTCommand->name = Str( "brdfIntegrationLUT" );
	brdfIntegrationLUTCommand->description = Str( "Pre-compute the BRDF as a LUT." );
	brdfIntegrationLUTCommand->fn = Fn_BRDFIntegrationLUT;
	m_commands.push_back( brdfIntegrationLUTCommand );
}

/*
================================
CommandSys::CallByName
================================
*/
const bool CommandSys::CallByName( Str name ) const {
	//separate the cmdName from the args
	Str cmdName;
	Str args = "";
	bool loadArgs = false;
	for ( unsigned int i = 0; i < name.Length(); i++ ){
		if ( name[i] == ' ' ) {
			loadArgs = true;
			continue;
		}
		if ( loadArgs ) {
			args.Append( name[i] );
		} else {
			cmdName.Append( name[i] );
		}
	}

	//search for the command and call its fn member and pass args
	for ( unsigned int i = 0; i < m_commands.size(); i++ ) {
		if ( m_commands[i]->name == cmdName ) {
			m_commands[i]->fn( args );
			return true;
		}
	}
	return false;
}

/*
================================
CommandSys::CommandByIndex
================================
*/
const Cmd* CommandSys::CommandByIndex( unsigned int i ) const {
	assert( i >= 0 && i < CommandCount() );
	const Cmd* command = m_commands[i];
	return command;
}

/*
================================
CommandSys::CommandByIndex
================================
*/
const Cmd* CommandSys::CommandByName( Str name ) const {
	for ( unsigned int i = 0; i < CommandCount(); i++ ) {
		const Cmd * cmd = CommandByIndex( i );
		if ( cmd->name == name ) {
			return cmd;
		}
	}
	return NULL;
}