#pragma once
#ifndef __SCENE_H_INCLUDE__
#define __SCENE_H_INCLUDE__

#include "Light.h"
#include "String.h"
#include "Mesh.h"

class Light;
class PointLight;
class EnvProbe;

/*
================================
Scene
================================
*/

class Scene {
	public:
		static Scene* getInstance();
		~Scene() {};

		bool LoadFromFile( const char * scn_relative );

		const Str& GetName() { return m_name; }

		int MeshCount() const { return m_meshCount; }
		bool MeshByIndex( unsigned int index, Mesh ** obj );
		Mesh * MeshByIndex( unsigned int index );

		const unsigned int AddLight( Light * newLight );
		int LightCount() { return m_lightCount; }
		bool LightByIndex( unsigned int index, Light ** obj );

		int EnvProbeCount() { return m_envProbeCount; }
		bool EnvProbeByIndex( unsigned int index, EnvProbe ** obj );

		const Cube& GetSkybox() { return m_skybox; }
		void SetSkybox( Str declName ) { m_skybox = Cube( declName ); }

	private:
		static Scene* inst_; //single instance
		Scene() { m_name = Str(); m_meshCount = 0; m_lightCount = 0; m_envProbeCount = 0; }
        Scene( const Scene& ); //don't implement
        Scene& operator=( const Scene& ); //don't implement

		Str m_name; //also the relative path to the scene file

		Cube m_skybox;

		void LoadVAOs();
		void BuildProbes();

		unsigned int m_meshCount;
		unsigned int m_lightCount;
		unsigned int m_envProbeCount;

		Mesh* m_meshes [256];
		Light* m_lights [256];
		EnvProbe* m_envProbes [256];
};

#endif