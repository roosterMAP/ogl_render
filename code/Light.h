#pragma once
#ifndef __LIGHT_H_INCLUDE__
#define __LIGHT_H_INCLUDE__

#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"

/*
==============================
Light
==============================
*/
class Light {
	public:
		Light();
		Light( Vec3 pos );
		~Light() { delete m_depthShader; }

		const Vec3 GetPosition() { return m_position; }
		void SetPosition( Vec3 pos ) { m_position = pos; }

		const Vec3 GetColor() { return m_color; }
		void SetColor( Vec3 col ) { m_color = col; }

		virtual const float * LightMatrix() { return m_xfrm.as_ptr(); } //return identity matrix

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
		Vec3 m_position, m_color;
		Mat4 m_xfrm;

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
		DirectionalLight() { m_direction = Vec3( 0.0 ); }
		DirectionalLight( Vec3 dir ) { m_direction = dir.normal(); }
		~DirectionalLight() {};

		const Vec3 GetDirection() { return m_direction; }
		void SetDirection( Vec3 dir ) { m_direction = dir; }

		const float * LightMatrix();

		void DebugDrawEnable() override;
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		Vec3 m_direction;

};

/*
==============================
SpotLight
==============================
*/
class SpotLight : public Light {
	public:
		SpotLight();
		SpotLight( Vec3 pos, Vec3 dir, float degrees, float radius );
		~SpotLight() {};

		const float GetRadius() { return m_radius; }
		void SetRadius( float radius ) { m_radius = radius; m_far_plane = radius; }

		const Vec3 GetDirection() { return m_direction; }
		void SetDirection( Vec3 dir ) { m_direction = dir; }

		void SetAngle( float degrees ) { m_angle = degrees; }

		const float * LightMatrix();

		void DebugDrawEnable() override;
		void DebugDraw( Camera * camera, const float * view, const float * projection ) override;

		void PassUniforms( Shader* shader ) override;

	private:
		float m_radius; //radius of the light source used for attenuation
		float m_angle; //radians
		Vec3 m_direction;
};

/*
==============================
PointLight
==============================
*/
class PointLight : public Light {
	public:
		PointLight();
		PointLight( Vec3 pos, float radius );
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
		EnvProbe( Vec3 pos );
		~EnvProbe() {};

		const Vec3 GetPosition() { return m_position; }
		void SetPosition( Vec3 pos ) { m_position = pos; }

		int MeshCount() { return m_meshCount; }
		bool MeshByIndex( unsigned int index, Mesh ** obj );
		Mesh * MeshByIndex( unsigned int index );
		void AddMesh( Mesh * mesh ) { m_meshes[m_meshCount] = mesh; m_meshCount += 1; }

		bool BuildProbe( unsigned int probeIdx );
		bool PassUniforms( Str materialDeclNames );
		std::vector<unsigned int> RenderCubemaps( const unsigned int cubemapSize );

		static Shader * s_ambientPass_shader;

	private:
		Vec3 m_position;

		CubemapTexture m_irradianceMap;
		CubemapTexture m_environmentMap;

		unsigned int m_meshCount;
		Mesh * m_meshes [256];

		static Texture s_brdfIntegrationMap;
};

#endif