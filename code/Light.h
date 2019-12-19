#pragma once
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define EPSILON 0.000001f

/*
==============================
Light
==============================
*/
class Light {
	public:
		Light();
		Light( glm::vec3 pos );
		~Light() { delete m_depthShader; }

		const glm::vec3 GetPosition() { return m_position; }
		void SetPosition( glm::vec3 pos ) { m_position = pos; }

		const glm::vec3 GetColor() { return m_color; }
		void SetColor( glm::vec3 col ) { m_color = col; }

		virtual const float * LightMatrix() { return glm::value_ptr( glm::mat4( 1 ) ); } //return identity matrix

		virtual void DebugDrawEnable();
		virtual void DebugDraw( Camera * camera, const float * view, const float * projection );

		virtual void PassUniforms( Shader* shader );

		virtual void EnableShadows();
		virtual void EnableSamplerCompareMode();
		virtual void BindDepthBuffer();
		virtual void PassDepthShaderUniforms() { m_depthShader->SetUniformMatrix4f( "lightSpaceMatrix", 1, false, LightMatrix() ); }
		virtual void DrawDepthBuffer( const float * view, const float * projection );
		bool m_shadowCaster;
		Shader * m_depthShader;
		Framebuffer m_depthBuffer;
		Framebuffer m_depthCubeView;

	protected:
		void DebugDrawSetup( std::string obj );
		
		float m_near_plane, m_far_plane;
		glm::vec3 m_position, m_color;
		glm::mat4 m_xfrm;

		Mesh m_mesh;
		Shader * m_debugShader;
		static bool s_drawEnable;

		static const char * s_vshader_source;
		static const char * s_fshader_source;

		static const char * s_depth_vshader_source;
		static const char * s_depth_fshader_source;

		unsigned int CreateBlankShadowMapTexture( unsigned int width, unsigned int height );
		unsigned int CreateBlankShadowCubeMapTexture( unsigned int width, unsigned int height );
		static unsigned int s_blankShadowMap;
		static unsigned int s_blankShadowCubeMap;
};


/*
==============================
DirectionalLight
==============================
*/
class DirectionalLight : public Light {
	public:
		DirectionalLight() { m_direction = glm::vec3( 0.0 ); }
		DirectionalLight( glm::vec3 dir ) { m_direction = glm::normalize( dir ); }
		~DirectionalLight() {};

		const glm::vec3 GetDirection() { return m_direction; }
		void SetDirection( glm::vec3 dir ) { m_direction = dir; }

		const float * LightMatrix();

		void DebugDrawEnable() override;
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		glm::vec3 m_direction;

};

/*
==============================
SpotLight
==============================
*/
class SpotLight : public Light {
	public:
		SpotLight();
		SpotLight( glm::vec3 pos, glm::vec3 dir, float degrees, float radius );
		~SpotLight() {};

		const float GetRadius() { return m_radius; }
		void SetRadius( float radius ) { m_radius = radius; m_far_plane = radius; }

		const glm::vec3 GetDirection() { return m_direction; }
		void SetDirection( glm::vec3 dir ) { m_direction = dir; }

		void SetAngle( float degrees ) { m_angle = degrees; }

		const float * LightMatrix();

		void DebugDrawEnable() override;
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		float m_radius; //radius of the light source used for attenuation
		float m_angle; //radians
		glm::vec3 m_direction;
};

/*
==============================
PointLight
==============================
*/
class PointLight : public Light {
	public:
		PointLight();
		PointLight( glm::vec3 pos, float radius );
		~PointLight() {};

		const float GetRadius() { return m_radius; }
		void SetRadius( float radius ) { m_radius = radius; m_far_plane = radius; }

		void PassDepthShaderUniforms();
		const float * LightMatrix();
		void EnableShadows();
		void EnableSamplerCompareMode();
		void BindDepthBuffer();
		void DrawDepthBuffer( const float * view, const float * projection );

		void DebugDrawEnable() override;
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		float m_radius;
		Cube m_depthCube;
		unsigned int m_shadowFaceIdx; //tells LightMatrix() which face of cubemap we are drawing this frame
};

/*
==============================
AmbientLight
==============================
*/
class AmbientLight : public Light {
	public:
		AmbientLight() { m_strength = 1.0; }
		AmbientLight( float strength ) { m_strength = strength; }
		~AmbientLight() {};

		const float GetStrength() { return m_strength; }
		void SetStrength( float strength ) { m_strength = strength; }

		void DebugDrawEnable() override { DebugDrawSetup( "data\\model\\ambientLight.obj" ); }
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		float m_strength;
};

/*
==============================
EnvProbe
	-cubemaps are generated via cvar
	-the scene populates the m_meshes array and calls BuildProbe member function
==============================
*/
class EnvProbe {
	public:
		EnvProbe();
		EnvProbe( glm::vec3 pos );
		~EnvProbe() {};

		const glm::vec3 GetPosition() { return m_position; }
		void SetPosition( glm::vec3 pos ) { m_position = pos; }

		int MeshCount() { return m_meshCount; }
		bool MeshByIndex( unsigned int index, Mesh ** obj );
		Mesh * MeshByIndex( unsigned int index );
		void AddMesh( Mesh * ) { m_meshes[ m_meshCount ]; m_meshCount += 1; }

		bool BuildProbe( unsigned int probeIdx );
		bool PassUniforms( Str materialDeclNames );
		unsigned int RenderCubemaps( const unsigned int cubemapSize );

		static Shader * s_ambientPass_shader;

	private:
		glm::vec3 m_position;

		CubemapTexture m_irradianceMap;
		CubemapTexture m_environmentMap;

		unsigned int m_meshCount;
		Mesh * m_meshes [256];

		static Texture s_brdfIntegrationMap;
};