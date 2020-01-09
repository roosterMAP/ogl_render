#pragma once
#ifndef __LIGHT_H_INCLUDE__
#define __LIGHT_H_INCLUDE__

#include "Scene.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"

class Scene;

/*
==============================
Light
==============================
*/
class Light {
	public:
		~Light() {};

		virtual unsigned int TypeIndex() const { return 0; }

		void Initialize();		

		void DebugDraw( Camera * camera, const float * view, const float * projection );

		virtual void UpdateDepthBuffer( Scene * scene );

		virtual const float * LightMatrix() { return m_xfrm.as_ptr(); }

		const unsigned int GetShadowIndex() const { return m_shadowIndex; }

		const Vec3 GetPosition() const { return m_position; }
		void SetPosition( Vec3 pos ) { m_position = pos; }

		const Vec3 GetColor() const { return m_color; }
		void SetColor( Vec3 col ) { m_color = col; }

		const Vec3 GetDirection() const { return m_direction; }
		void SetDirection( Vec3 dir ) { m_direction = dir; }

		const float GetRadius() const { return m_radius; }
		void SetRadius( float radius ) { m_radius = radius; m_far_plane = radius; }

		const bool GetShadow() const { return m_shadowCaster; }
		virtual void SetShadow( const bool shadowCasting );

		virtual void PassUniforms( Shader* shader, int idx ) const {};
		void PassDepthAttribute( Shader* shader, const unsigned int slot ) const;
		
		int m_idx;

		static unsigned int s_lightCount;
		static unsigned int s_shadowCastingLightCount;
		static Shader * s_debugShader;
		static Shader * s_depthShader;

		static void InitShadowAtlas();
		static Framebuffer s_depthBufferAtlas;
		static unsigned int s_partitionSize;
		static const unsigned int s_depthBufferAtlasSize_min = 512;
		static const unsigned int s_depthBufferAtlasSize_max = 4096;

	protected:
		Light();		
		
		int m_shadowIndex;
		bool m_shadowCaster;
		float m_near_plane, m_far_plane, m_radius;
		Vec3 m_position, m_color, m_direction;
		Mat4 m_xfrm;
		Vec2 m_PosInShadowAtlas;

	private:		
		virtual Mesh * GetDebugMesh() const { return &s_debugModel; }
		static Mesh s_debugModel;
};


/*
==============================
DirectionalLight
==============================
*/
class DirectionalLight : public Light {
	public:
		DirectionalLight();
		~DirectionalLight() {};

		unsigned int TypeIndex() const { return 1; }

		const float * LightMatrix();

		void PassUniforms( Shader* shader, int idx ) const;

	private:
		Mesh * GetDebugMesh() const { return &s_debugModel_directional; }

		static Mesh s_debugModel_directional;
};

/*
==============================
SpotLight
==============================
*/
class SpotLight : public Light {
	public:
		SpotLight();
		~SpotLight() {};

		unsigned int TypeIndex() const { return 2; }

		const float * LightMatrix();

		const float GetAngle() const { return m_angle; }
		void SetAngle( float radians ) { m_angle = radians; }

		void PassUniforms( Shader* shader, int idx ) const;

	private:
		Mesh * GetDebugMesh() const { return &s_debugModel_spot; }
		float m_angle; //radians

		static Mesh s_debugModel_spot;
};

/*
==============================
PointLight
==============================
*/
class PointLight : public Light {
	public:
		PointLight();
		~PointLight() {};

		unsigned int TypeIndex() const { return 3; }

		void SetShadow( const bool shadowCasting );
		const Vec2 GetShadowMapLoc( unsigned int faceIdx ) const;

		const float * LightMatrix();

		void UpdateDepthBuffer( Scene * scene );

		void PassUniforms( Shader* shader, int idx ) const;

	private:
		Mesh * GetDebugMesh() const { return &s_debugModel_point; }
		Mat4 m_xfrms[6];

		static Mesh s_debugModel_point;
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
		void PassUniforms( Shader* shader, unsigned int slot ) const;
		std::vector<unsigned int> RenderCubemaps( const unsigned int cubemapSize );

	private:
		Vec3 m_position;

		CubemapTexture m_irradianceMap;
		CubemapTexture m_environmentMap;

		unsigned int m_meshCount;
		Mesh * m_meshes [256];

		static Texture s_brdfIntegrationMap;
};

#endif