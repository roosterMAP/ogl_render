#pragma once
#ifndef __LIGHT_H_INCLUDE__
#define __LIGHT_H_INCLUDE__

#include "Scene.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"

class Scene;

struct Tri {
	unsigned int idx0, idx1, idx2;
};
struct LightEffectStorage {
	unsigned int vCount;
	unsigned int tCount;
	Vec3 vPos[8];
	Tri tris[12];
};

struct LightStorage {
	int typeIndex;
	Vec3 position;
	Vec3 color;
	Vec3 direction;
	float angle; //radians
	float light_radius;
	float max_radius;
	float dir_radius;
	float brightness;
	int shadowIdx;
};

struct ShadowStorage {
	Mat4 xfrm;
	Vec4 loc;
};

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

		void DebugDraw( Camera * camera, const float * view, const float * projection, int mode );

		virtual void UpdateDepthBuffer( Scene * scene );

		virtual const float * LightMatrix() { return m_xfrm.as_ptr(); }

		const unsigned int GetShadowIndex() const { return m_uniformBlock.shadowIdx; }

		const Vec3 GetPosition() const { return m_uniformBlock.position; }
		void SetPosition( Vec3 pos ) { m_uniformBlock.position = pos; }

		const Vec3 GetColor() const { return m_uniformBlock.color; }
		void SetColor( Vec3 col ) { m_uniformBlock.color = col.normal(); }

		const Vec3 GetBrightness() const { return m_uniformBlock.brightness; }
		void SetBrightness( float brt ) { m_uniformBlock.brightness = brt; }

		const Vec3 GetDirection() const { return m_uniformBlock.direction; }
		void SetDirection( Vec3 dir ) { m_uniformBlock.direction = dir; }

		const float GetRadius() const { return m_uniformBlock.light_radius; }
		void SetRadius( float radius );

		const float GetMaxRadius() const { return m_uniformBlock.max_radius; }
		void SetMaxRadius( float radius );

		const bool GetShadow() const { return m_shadowCaster; }
		virtual void SetShadow( const bool shadowCasting );

		float MaxAttenuationDist() const;

		void InitLightEffectStorage();

		virtual void PassUniforms( Shader* shader, int idx ) const;
		void PassDepthAttribute( Shader* shader, const unsigned int slot ) const;
		void PassPrepassUniforms( Shader* shader, int idx ) const;
		
		int m_idx;
		bool m_cachedShadows;
		bool m_firstFrameRendered;

		static float s_lightAttenuationBias;

		static unsigned int s_lightCount;
		static unsigned int s_shadowCastingLightCount;
		static Shader * s_debugShader;
		static Shader * s_depthShader;

		static void InitShadowAtlas();
		static Framebuffer * s_depthBufferAtlas;
		static unsigned int s_partitionSize;
		static const unsigned int s_depthBufferAtlasSize_min = 512;
		static const unsigned int s_depthBufferAtlasSize_max = 4096;

	protected:
		Light();

		LightStorage m_uniformBlock;
		LightEffectStorage m_boundsUniformBlock;
		bool m_shadowCaster;		
		float m_near_plane, m_far_plane;
		Mat4 m_xfrm;
		Vec2 m_PosInShadowAtlas;
		bool lightEffectStorage_cached;

		void InitBoundsVolume();

	private:		
		virtual Mesh * GetDebugMesh() const { return s_debugModel; }
		static Mesh * s_debugModel;
		unsigned int m_debugModel_VAO;

	friend Scene;
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

		unsigned int TypeIndex() const { return m_uniformBlock.typeIndex; }

		const float GetDirRadius() const { return m_uniformBlock.dir_radius; }
		void SetDirRadius( float radius ) { m_uniformBlock.dir_radius = radius; }

		const float * LightMatrix();

	private:
		Mesh * GetDebugMesh() const { return s_debugModel_directional; }

		static Mesh * s_debugModel_directional;

	friend Scene;
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

		unsigned int TypeIndex() const { return m_uniformBlock.typeIndex; }

		const float * LightMatrix();

		//cos(angle/2) is an optimization. rather than performing this in the frag shader
		const float GetAngle() const { return 2.0f * acos( m_uniformBlock.angle ); }
		void SetAngle( float radians ) { m_uniformBlock.angle = cos( radians / 2.0f ); }

	private:
		Mesh * GetDebugMesh() const { return s_debugModel_spot; }

		static Mesh * s_debugModel_spot;

	friend Scene;
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

		unsigned int TypeIndex() const { return m_uniformBlock.typeIndex; }

		void SetShadow( const bool shadowCasting );
		const Vec2 GetShadowMapLoc( unsigned int faceIdx ) const;

		const float * LightMatrix();

		void UpdateDepthBuffer( Scene * scene );

		void PassUniforms( Shader* shader, int idx ) const;

	private:
		Mesh * GetDebugMesh() const { return s_debugModel_point; }
		Mat4 m_xfrms[6];

		static Mesh * s_debugModel_point;

	friend Scene;
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
		void Delete();

		const Vec3 GetPosition() { return m_position; }
		void SetPosition( Vec3 pos ) { m_position = pos; }

		int MeshCount() { return m_meshCount; }
		bool MeshByIndex( unsigned int index, Mesh ** obj );
		Mesh * MeshByIndex( unsigned int index );
		void AddMesh( Mesh * mesh ) { m_meshes[m_meshCount] = mesh; m_meshCount += 1; }

		bool BuildProbe( unsigned int probeIdx );
		void PassUniforms( Shader* shader, unsigned int slot ) const;
		std::vector<unsigned int> RenderCubemaps( Shader * shader, const unsigned int cubemapSize );

	private:
		Vec3 m_position;

		CubemapTexture m_irradianceMap;
		CubemapTexture m_environmentMap;

		unsigned int m_meshCount;
		Mesh * m_meshes [256];

		static Texture s_brdfIntegrationMap;

	friend Scene;
};

#endif