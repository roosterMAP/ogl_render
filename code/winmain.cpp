#include <string>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/matrix_access.hpp>
#include <gtc/type_ptr.hpp>

#include "Scene.h"
#include "Fileio.h"
#include "Texture.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "PostProcess.h"
#include "Command.h"
#include "Console.h"
#include "String.h"

//Global storage of the window size
int gScreenWidth  = 1200;
int gScreenHeight = 720;

//user input state variables
int window;
bool lmb_down = false;
bool* keyStates = new bool[256]; // Create an array of boolean values of length 256 (0-255)
unsigned char currentKey;

//function declarations cuz i want the defs way at the bottom
void specialInput( int key, int x, int y );
void keyDown( unsigned char key, int x, int y );
void keyUp( unsigned char key, int x, int y );
void keyOperations( void );
void mouseButton( int button, int state, int x, int y );
void mouseMovement( int x, int y );

Camera camera( 45.0, glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
Framebuffer mainFBO( "screenTexture" );
PostProcessManager postProcessManager;
const unsigned int MSAA_sampleCount = 0;

Console * g_console = Console::getInstance(); //declare g_console singleton

extern CVar * g_cvar_debugLighting;
extern CVar * g_cvar_renderLightModels;
extern CVar * g_cvar_showVertTransform;
extern CVar * g_cvar_showEdgeHighlights;

Scene * g_scene = Scene::getInstance(); //declare g_scene singleton

void RenderDebug( const float * view, const float * perspective, int mode ) {
	//Draw the scene to the main buffer
	mainFBO.Bind(); //bind the framebuffer so all subsequent drawing is to it.

	//configure opengl to draw the scene for this light
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer
	glViewport( 0, 0, gScreenWidth, gScreenHeight ); //Set the OpenGL viewport to be the entire size of the window
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); //Clear previous frame values

	Shader * debugLightingShader = new Shader();
	debugLightingShader = debugLightingShader->GetShader( "debugLighting" );
	debugLightingShader->UseProgram();
	int renderNormal_uniform = 0;

	MaterialDecl* matDecl;
	for ( int n = 0; n < g_scene->MeshCount(); n++ ) {
		Mesh * mesh = NULL;
		g_scene->MeshByIndex( n, &mesh );

		for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
			matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[j]->materialName.c_str() );
			
			//get the target uniform name bassed off the debug mode and shader type
			Str uniformName;
			if ( mode == 1 ) {
				if ( matDecl->m_shaderProg == "cook-torrance" ) {
					uniformName = "albedoTexture";
				} else {
					uniformName = "diffuseTexture";
				}				
			} else if ( mode == 2 ) {
				uniformName = "specularTexture";
			} else if ( mode == 3 ) {
				uniformName = "normalTexture";
				renderNormal_uniform = 1;
			} else if ( mode == 4 ) {
				if ( matDecl->m_shaderProg == "cook-torrance" ) {
					uniformName = "glossTexture";
				} else {
					uniformName = "powerTexture";
				}
			}

			if ( matDecl->m_shaderProg == "error" ) {
				uniformName = "errorTexture";
			}

			//get and bind texture
			textureMap::const_iterator it = matDecl->m_textures.find( uniformName.c_str() );
			Texture * texture;
			if ( it != matDecl->m_textures.end() ) {
				texture = matDecl->m_textures[uniformName.c_str()];
			} else {
				assert( false );
			}
			debugLightingShader->SetAndBindUniformTexture( "sampleTexture", 0, texture->GetTarget(), texture->GetName() );

			debugLightingShader->SetUniform1i( "debugNormal", 1, &renderNormal_uniform );

			//pass in camera matrices
			debugLightingShader->SetUniformMatrix4f( "view", 1, false, view );		
			debugLightingShader->SetUniformMatrix4f( "projection", 1, false, perspective );
	
			//draw surface
			mesh->DrawSurface( j );
		}
	}
	mainFBO.Unbind();
}

void RenderVertexTransforms( const float * view, const float * perspective ) {
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer
	glViewport( 0, 0, gScreenWidth, gScreenHeight ); //Set the OpenGL viewport to be the entire size of the window

	Shader * debugLightingShader = new Shader();
	debugLightingShader = debugLightingShader->GetShader( "vertTransform" );
	debugLightingShader->UseProgram();

	for ( int n = 0; n < g_scene->MeshCount(); n++ ) {
		Mesh * mesh = NULL;
		g_scene->MeshByIndex( n, &mesh );

		for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
			//pass in camera matrices
			debugLightingShader->SetUniformMatrix4f( "view", 1, false, view );		
			debugLightingShader->SetUniformMatrix4f( "projection", 1, false, perspective );
	
			//draw surface
			mesh->DrawSurface( j );
		}
	}
}

void RenderWireframe( const float * view, const float * perspective, const float * color, const unsigned int mode ) {
	mainFBO.Bind(); //bind the framebuffer so all subsequent drawing is to it.

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //draw wireframe on both sides of poly
	if ( mode == 2 ) {
		glDisable( GL_CULL_FACE ); //disable backface culling
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); //clear buffers
	}
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer
	glViewport( 0, 0, gScreenWidth, gScreenHeight ); //Set the OpenGL viewport to be the entire size of the window

	Shader * edgeHighlightShader = new Shader();
	edgeHighlightShader = edgeHighlightShader->GetShader( "edgeHighlight" );
	edgeHighlightShader->UseProgram();

	for ( int n = 0; n < g_scene->MeshCount(); n++ ) {
		Mesh * mesh = NULL;
		g_scene->MeshByIndex( n, &mesh );

		for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
			edgeHighlightShader->SetUniformMatrix4f( "view", 1, false, view );		
			edgeHighlightShader->SetUniformMatrix4f( "projection", 1, false, perspective );
			edgeHighlightShader->SetUniform3f( "color", 1, color );	
			
			mesh->DrawSurface( j ); //draw surface
		}
	}
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); //draw regular
	glEnable( GL_CULL_FACE ); //enable backface culling
	mainFBO.Unbind();
}

void RenderScene( const float * view, const float * projection ) {
	//configure opengl to draw the scene for this light
	mainFBO.Bind(); //bind the framebuffer so all subsequent drawing is to it.
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); //Enabling color writing to the frame buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); //Clear previous frame values

	//debulLighting cvar
	int debugRenderMode = 0;
	if ( g_cvar_debugLighting->GetState() ) {
		debugRenderMode = atoi( g_cvar_debugLighting->GetArgs().c_str() );
	}

	if ( debugRenderMode != 6 ) {	
		//render the scene
		for ( int i = 0; i < g_scene->LightCount(); i++ ) {
			Light * light = NULL;
			g_scene->LightByIndex( i, &light );

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
				for ( int n = 0; n < g_scene->MeshCount(); n++ ) {			
					Mesh * mesh = NULL;
					g_scene->MeshByIndex( n, &mesh );
					for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
						mesh->DrawSurface( j ); //draw surface
					}
				}
				light->m_depthBuffer.Unbind();
			}

			//Draw the scene to the main buffer
			mainFBO.Bind(); //bind the framebuffer so all subsequent drawing is to it.
			glViewport( 0, 0, gScreenWidth, gScreenHeight ); //Set the OpenGL viewport to be the entire size of the window
			if ( i > 0 ) {
				//subsequent lighting passes add their contributions now that the first one has set all initial depth values.
				glBlendFunc( GL_ONE, GL_ONE );
			}

			MaterialDecl* matDecl;
			for ( int n = 0; n < g_scene->MeshCount(); n++ ) {
				Mesh * mesh = NULL;
				g_scene->MeshByIndex( n, &mesh );

				for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
					matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[j]->materialName.c_str() );
					matDecl->BindTextures();

					//pass in camera matrices
					matDecl->shader->SetUniformMatrix4f( "view", 1, false, view );		
					matDecl->shader->SetUniformMatrix4f( "projection", 1, false, projection );

					//exit early if this material is errored out
					if ( matDecl->m_shaderProg == "error" ) {
						mesh->DrawSurface( j ); //draw surface
						continue;
					}

					//pass camera position
					matDecl->shader->SetUniform3f( "camPos", 1, glm::value_ptr( camera.position ) );

					//pass lights data
					light->PassUniforms( matDecl->shader );
	
					//draw surface
					mesh->DrawSurface( j );
				}
			}
			mainFBO.Unbind();
		}
	}

	//ambient pass
	/*if ( debugRenderMode != 5 ) {		
		mainFBO.Bind();
		if ( debugRenderMode == 6 ) {
			glBlendFunc( GL_ONE, GL_ZERO );
		} else {
			glBlendFunc( GL_ONE, GL_ONE );
		}

		for ( int i = 0; i < g_scene->EnvProbeCount(); i++ ) {
			EnvProbe * probe = NULL;
			g_scene->EnvProbeByIndex( i, &probe );

			MaterialDecl* matDecl;
			for ( int n = 0; n < probe->MeshCount(); n++ ) {
				Mesh * mesh = NULL;
				probe->MeshByIndex( n, &mesh );
				for ( unsigned int j = 0; j < mesh->m_surfaces.size(); j++ ) {
					Str matDeclName = mesh->m_surfaces[j]->materialName;
					if ( probe->PassUniforms( matDeclName ) == false ) {
						continue;
					}

					//pass in uniforms
					probe->s_ambientPass_shader->SetUniformMatrix4f( "view", 1, false, view );		
					probe->s_ambientPass_shader->SetUniformMatrix4f( "projection", 1, false, projection );
					probe->s_ambientPass_shader->SetUniform3f( "camPos", 1, glm::value_ptr( camera.position ) );
	
					//draw surface
					mesh->DrawSurface( j );
				}
			}
		}
	}*/
}

/*
================================
DrawFrame
================================
*/
void drawFrame( void ) {
	keyOperations();

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClearDepth( 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); //Clear previous frame values
	
	//depth buffer
	glDepthMask( GL_TRUE ); //Enable writing to the depth buffer	
	glDepthFunc( GL_LEQUAL ); //Set the depth test function to less than or equal
	glEnable( GL_DEPTH_TEST ); //Enable testing against the depth buffer

	//enable additive blending for the framebuffer we are rendering lights to
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ZERO );
	glBlendEquation( GL_FUNC_ADD );	
	
	//----------------------------------------------
	//Draw the scene to the post process framebuffer
	//----------------------------------------------
	
	const float aspect = ( float )gScreenWidth / ( float )gScreenHeight;

	const glm::mat4 view = camera.viewMatrix();
	const glm::mat4 projection = camera.projectionMatrix( aspect );

	//get polygon rendermode from g_cvar_showEdgeHighlights.
	//0 -> no highlights, 1 -> with highlights, 2 -> only highlights
	unsigned int edgeHighlights_renderMode = 0;
	if ( g_cvar_showEdgeHighlights->GetState() ) {
		const int cvar_showEdgeHighlights_arg = atoi( g_cvar_showEdgeHighlights->GetArgs().c_str() );
		if ( cvar_showEdgeHighlights_arg > 0 && cvar_showEdgeHighlights_arg < 3 ) {
			edgeHighlights_renderMode = ( unsigned int )cvar_showEdgeHighlights_arg;
		}
	}

	//debulLighting cvar
	int debugRenderMode = 0;
	if (  g_cvar_debugLighting->GetState() ) {
		debugRenderMode = atoi( g_cvar_debugLighting->GetArgs().c_str() );
	}

	if ( debugRenderMode > 0 && debugRenderMode <= 4 ) {		
		RenderDebug( glm::value_ptr( view ), glm::value_ptr( projection ), debugRenderMode );
	} else {
		if ( edgeHighlights_renderMode == 0 ) { //regular
			RenderScene( glm::value_ptr( view ), glm::value_ptr( projection ) );
		} else if ( edgeHighlights_renderMode == 1 ) { //wireframe overlayed
			RenderScene( glm::value_ptr( view ), glm::value_ptr( projection ) );
			const glm::vec3 edgeColor = glm::vec3( 1.0, 0.0, 0.0 );
			RenderWireframe( glm::value_ptr( view ), glm::value_ptr( projection ), glm::value_ptr( edgeColor ), edgeHighlights_renderMode );
		} else if ( edgeHighlights_renderMode == 2 ) { //wireframe only
			const glm::vec3 edgeColor = glm::vec3( 1.0, 0.0, 0.0 );
			RenderWireframe( glm::value_ptr( view ), glm::value_ptr( projection ), glm::value_ptr( edgeColor ), edgeHighlights_renderMode );
		}
	}

	glDisable( GL_BLEND ); //since models and lights are finished drawing, turn off blending

	mainFBO.Bind();

	if ( g_cvar_renderLightModels->GetState() ) {
		int debugLightRenderMode = atoi( g_cvar_renderLightModels->GetArgs().c_str() );
		if ( debugLightRenderMode > 0 ) {
			//draw debug light models
			for ( int i = 0; i < g_scene->LightCount(); i++ ) {
				Light * light = NULL;
				g_scene->LightByIndex( i, &light );
				light->DebugDraw( &camera, glm::value_ptr( view ), glm::value_ptr( projection ) );
			}
		}
	}
	
	//draw skybox
	if ( debugRenderMode == 0 ) {
		MaterialDecl* skyMat;
		Cube sceneSkybox = g_scene->GetSkybox();
		skyMat = MaterialDecl::GetMaterialDecl( sceneSkybox.m_surface->materialName.c_str() );
		skyMat->BindTextures();
		const glm::mat4 skyBox_viewMat = glm::mat4( glm::mat3( view ) );
		skyMat->shader->SetUniformMatrix4f( "projection", 1, false, glm::value_ptr( projection ) );
		skyMat->shader->SetUniformMatrix4f( "view", 1, false, glm::value_ptr( skyBox_viewMat ) );
		sceneSkybox.DrawSurface( true );
	}

	//if cvar is active, draw normal, tangent, and bitangent of all vertices in scene
	if ( g_cvar_showVertTransform->GetState() ) {		
		RenderVertexTransforms( glm::value_ptr( view ), glm::value_ptr( projection ) );
	}

	//if cvar debug render mode on, skip post process pass
	if ( debugRenderMode > 0 && debugRenderMode <= 4 ) {
		//Draw mainFBO to the default buffer directly
		glBindFramebuffer( GL_READ_FRAMEBUFFER, mainFBO.GetID() );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
		glBlitFramebuffer( 0, 0, gScreenWidth, gScreenHeight, 0, 0,  gScreenWidth, gScreenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST );
	} else {
		postProcessManager.BlitFramebuffer( &mainFBO );
		postProcessManager.Bloom();
		postProcessManager.Draw( 1.0 );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	}
	
	//---------------------
	//Display Shadowmap texture
	//---------------------	
	if ( g_cvar_renderLightModels->GetState() ) {
		int debugLightRenderMode = atoi( g_cvar_renderLightModels->GetArgs().c_str() );
		if ( debugLightRenderMode > 0 ) {
			Light * light = NULL;
			g_scene->LightByIndex( debugLightRenderMode - 1, &light );
			if ( g_scene->LightByIndex( debugLightRenderMode - 1, &light ) ) {
				if ( light->m_shadowCaster ) {
					//render the depth buffer to a quad on the screen
					light->DrawDepthBuffer( glm::value_ptr( view ), glm::value_ptr( projection ) );
				}
			} else {
				g_console->AddError( "ERROR: There is no light at this index." );
				g_cvar_renderLightModels->SetArgs( Str( "0" ) );
			}
		}
	}

	//---------------------
	//Draw g_console
	//---------------------
	g_console->UpdateLog();
	
	glFinish(); //Tell OpenGL to finish all the previous OpenGL commands before continuing
	glutSwapBuffers(); //Swap the back buffer to the front buffer
}

/*
================================
glSetup
	Init GLUT and GLEW
================================
*/
bool glSetup( int argc, char ** argv ) {
	glutInit( &argc, argv ); //Initialize GLUT

	const bool msaa = true;
	if ( MSAA_sampleCount > 0 ) {
		glutSetOption( GLUT_MULTISAMPLE, MSAA_sampleCount );
		glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE );
        glEnable( GL_MULTISAMPLE );
        glHint( GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST );

        // detect current settings
        GLint iMultiSample = 0;
        GLint iNumSamples = 0;
        glGetIntegerv( GL_SAMPLE_BUFFERS, &iMultiSample );
        glGetIntegerv( GL_SAMPLES, &iNumSamples );
        printf( "MSAA on, GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d\n", iMultiSample, iNumSamples );
	} else {
		glDisable(GL_MULTISAMPLE);
		glutInitDisplayMode( GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH ); //Tell GLUT to create a single display with Red Green Blue (RGB) color.
		printf( "MSAA off" );
	}
	
	glutInitWindowSize( gScreenWidth, gScreenHeight ); //Tell GLUT the size of the desired window
	glutInitWindowPosition( 50, 50 ); //Set the intial window position	
	window = glutCreateWindow( "OpenGL program" ); //Create the window

	glEnable( GL_CULL_FACE ); //enable backface culling
	glCullFace( GL_BACK ); //cull backfaces (default)
	glFrontFace( GL_CCW ); //frontface is the normal from a counter-clock wise winding of triangles (default)

	//Register input callbacks:
	//glutReshapeFunc( reshape );
	glutSetCursor( GLUT_CURSOR_NONE );
	glutKeyboardFunc( keyDown );
	glutSpecialFunc( specialInput );
	glutKeyboardUpFunc( keyUp );
	glutMouseFunc( mouseButton );
	glutPassiveMotionFunc( mouseMovement );
	glutIdleFunc( drawFrame ); //Setting the idle function to point to the drawFrame function tells GLUT to call this function in GLUT's infinite loop
	
	GLenum err = glewInit(); //Initialize GLEW

	//Check for any errors that may have occured during the initialization of GLEW
	if ( GLEW_OK != err ) {
		fprintf( stderr, "Error: %s\n", glewGetErrorString( err ) );
		return false;
	}
	printf( "GL_VERSION:  %s\n", ( const char * )glGetString( GL_VERSION ) ); //Print out the installed version of OpenGL on this system
	return true;
}

/*
================================
main
================================
*/
int main( int argc, char ** argv ) {
	if( !glSetup( argc, argv ) ) {
		return 0;
	}

	//initialize g_console
	g_console->Init( "data\\fonts\\consolab.ttf" );

	//create cubemap for background
	g_scene->SetSkybox( "material\\hdr_skyboxTest" );

	//build debuglighting shader
	Shader * debugLighting_shader = new Shader();
	debugLighting_shader = debugLighting_shader->GetShader( "debugLighting" );

	//build edgeHightlights shader
	Shader * edgeHighlightShader = new Shader();
	edgeHighlightShader = edgeHighlightShader->GetShader( "edgeHighlight" );

	//build vert transform shader
	Shader * vertTransform_shader = new Shader();
	vertTransform_shader = vertTransform_shader->GetShader( "vertTransform" );

	MaterialDecl * matDecl = NULL;
	if ( g_scene->LoadFromFile( "data\\scenes\\ibl_test.SCN" ) ) {
		for ( int n = 0; n < g_scene->MeshCount(); n++ ) {
			Mesh * mesh = g_scene->MeshByIndex( n );
			g_scene->MeshByIndex( n, &mesh );
			if ( NULL == mesh ) {
				continue;
			}

			for ( unsigned int i = 0; i < mesh->m_surfaces.size(); i++ ) {
				matDecl = MaterialDecl::GetMaterialDecl( mesh->m_surfaces[ i ]->materialName.c_str() );
				if ( matDecl == NULL ) {
					//load a material that renders pink
					printf( "Material decl %s failed to load!!!\n", mesh->m_surfaces[i]->materialName.c_str() );

					Str errorMaterialName( "material\\error" );
					mesh->m_surfaces[ i ]->materialName = errorMaterialName;
					matDecl = MaterialDecl::GetMaterialDecl( errorMaterialName.c_str() );
				}
				if( matDecl->CompileShader() ) {
					matDecl->BindTextures(); //pass in texture
				} else {
					return 0;
				}
			}
		}
	}

	//enable drawing of debug light data (model and shadow depthbuffer)
	for ( int i = 0; i < g_scene->LightCount(); i++ ) {
		Light * light = NULL;
		g_scene->LightByIndex( i, &light );
		light->DebugDrawEnable();
	}

	//create post processing frame buffer
	mainFBO.CreateNewBuffer( gScreenWidth, gScreenHeight, "framebuffer" );
	mainFBO.EnableMultisampledAttachments( MSAA_sampleCount );
	mainFBO.AttachRenderBuffer( GL_RGBA16F, GL_COLOR_ATTACHMENT0 );
	mainFBO.AttachRenderBuffer( GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT );
	if( !mainFBO.Status() ) {
		return 0;
	}
	mainFBO.CreateScreen();
	mainFBO.Unbind();

	postProcessManager = PostProcessManager( gScreenWidth, gScreenHeight, "postProcess" );
	postProcessManager.SetBlitParams( glm::vec2( 0.0, 0.0 ), glm::vec2( gScreenWidth, gScreenHeight ), glm::vec2( 0.0, 0.0 ), glm::vec2( gScreenWidth, gScreenHeight ) );
	//postProcessManager.BloomEnable( 0.5 );
	postProcessManager.LUTEnable( "data\\texture\\LUT_default.tga" );

	glutMainLoop(); //Do the infinite loop. This starts glut's inifinite loop.

	//Cleanup
	Shader::DeleteAllPrograms();

	return 0;
}

/*
================================
specialInput
================================
*/
void specialInput( int key, int x, int y ) {
	if ( g_console->GetState() ) {	
		if ( key == GLUT_KEY_LEFT ) {
			g_console->CursorPos_Left();
		} else if ( key == GLUT_KEY_RIGHT ) {
			g_console->CursorPos_Right();
		} else if ( key == GLUT_KEY_UP) {
			g_console->CursorPos_Up();
		} else if ( key == GLUT_KEY_DOWN ) {
			g_console->CursorPos_Down();
		}
	}
}

/*
================================
keyDown
================================
*/
void keyDown( unsigned char key, int x, int y ) {
	keyStates[key] = true; // Set the state of the current key to pressed
	currentKey = key;
}  
  
/*
================================
keyUp
================================
*/
void keyUp( unsigned char key, int x, int y ) {
	keyStates[key] = false; // Set the state of the current key to not pressed
}

/*
================================
keyOperations
================================
*/
void keyOperations() {
	const float camSpeed = 0.001f;

	//global keys
	if ( keyStates['`'] == true ) { //g_console toggle
		keyStates['`'] = false;
		g_console->ToggleState();

		//toggle cursor behavior
		if ( g_console->GetState() ) {
			glutSetCursor( GLUT_CURSOR_RIGHT_ARROW );
		} else {
			glutSetCursor( GLUT_CURSOR_NONE );
		}
	}

	if ( g_console->GetState() ) {		
		if ( currentKey != NULL ) {
			if ( currentKey == 13 ) { //ENTER key
				g_console->UpdateInputLine_NewLine();
			} else if ( currentKey == 8 ) { //BACKSPACE key
				g_console->UpdateInputLine_Backspace();
			} else if ( currentKey == 9 ) { //TAB key
				g_console->UpdateInputLine_Tab();
			} else if ( currentKey == 127 ) { //DELETE key
				g_console->UpdateInputLine_Delete();
			} else {
				if ( currentKey != 96 ) { //is NOT tilda key (keyStates[96] == '~')
					//if the g_console is active, then all inputs feed into the g_console
					g_console->UpdateInputLine( ( char )currentKey );
				}
			}
			currentKey = NULL;
		}
	} else {
		if ( keyStates[32] == true ) { //SPACE key
			const glm::vec3 up = glm::vec3( 0.0, 1.0, 0.0 );
			camera.position += up * camSpeed;
		}

		if ( keyStates['c'] == true || keyStates['C'] == true ) {
			const glm::vec3 up = glm::vec3( 0.0, 1.0, 0.0 );
			camera.position -= up * camSpeed;
		}

		if ( keyStates['w'] == true || keyStates['W'] == true ) {
			glutDestroyMenu( window );
			camera.position += camera.look * camSpeed;
		}

		if ( keyStates['a'] == true || keyStates['A'] == true ) {
			camera.position += camera.right * camSpeed;
		}
	
		if ( keyStates['s'] == true || keyStates['S'] == true ) {
			camera.position -= camera.look * camSpeed;
		}
	
		if ( keyStates['d'] == true || keyStates['D'] == true ) {
			camera.position -= camera.right * camSpeed;
		}
	}
}

/*
================================
mouseButton
================================
*/
void mouseButton( int button, int state, int x, int y ) {
	if ( button == 3 ) { //wheel UP
		if ( state == GLUT_UP ) {
			return; //Disregard redundant GLUT_UP events
		}
		if ( g_console->GetState() ) {
			g_console->OffsetLogHistory( true );
		} else {
			camera.fov -= 1.0f;
		}
	} else if ( button == 4 ) { //wheel DOWN
		if ( state == GLUT_UP ) {
			return; //Disregard redundant GLUT_UP events
		}
		if ( g_console->GetState() ) {
			g_console->OffsetLogHistory( false );
		} else {
			camera.fov += 1.0f;
		}
	}
}

/*
================================
mouseMovement
================================
*/
void mouseMovement( int x, int y ) {
	if ( g_console->GetState() ) {
		return;
	}

	//get offset from center of screen
	float center_x = ( float )gScreenWidth / 2.0f;
	float center_y = ( float )gScreenHeight / 2.0f;
	float offset_x = ( float )( x - center_x );
	float offset_y = ( float )( y - center_y );

	//update camera orientation from cursor pos offset
	camera.FPLookOffset( offset_x, offset_y, 0.05f );

	//set the cursor position to the center of the window
	glutWarpPointer( gScreenWidth / 2, gScreenHeight / 2 );
}