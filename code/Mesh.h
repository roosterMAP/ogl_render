#pragma once
#ifndef __MESH_H_INCLUDE__
#define __MESH_H_INCLUDE__

#include <vector>
#include "Vector.h"
#include "Matrix.h"
#include "Decl.h"

class EnvProbe;


struct tri_t {
	unsigned int a;
	unsigned int b;
	unsigned int c;
};

struct vert_t {
	Vec3 pos;
	Vec3 norm;
	Vec3 tang;
	Vec2 uv;
	float tSign;
	std::vector< tri_t * > tris;
};

struct surface {
	unsigned int VAO;
	Str materialName;
	std::vector< vert_t > verts;
	unsigned int vCount;
	std::vector< tri_t > tris;
	unsigned int triCount;
};

struct bbox {
	Vec3 min;
	Vec3 max;
};

/*
================================
Transform
================================
*/
class Transform {
	public:
		Transform() {};
		~Transform() {};

		void SetPosition( Vec3 pos ) { m_position = pos; }
		void SetRotation( Mat3 rot ) { m_rotation = rot; }
		void SetScale( Vec3 scl ) { m_scale = scl; }
		bool WorldXfrm( Mat4* model );

	private:
		Vec3 m_position;
		Mat3 m_rotation;
		Vec3 m_scale;
		Mat4 m_xfrm;
};

/*
================================
Mesh
================================
*/
class Mesh {
	public:
		Mesh() { m_probe = NULL; };
		~Mesh() {};
		bool LoadMSHFromFile( const char * msh_relative );
		bool LoadOBJFromFile( const char * obj_relative );
		void DrawSurface( unsigned int surfaceIdx );
		unsigned int LoadVAO( const unsigned int surfaceIdx );
				
		Str m_name;

		const bbox& GetBounds() { return m_bounds; }
		const Vec3 GetCenter() { return ( m_bounds.max + m_bounds.min ) / 2.0f; }

		const EnvProbe * GetProbe() const { return m_probe; }
		void SetProbe( EnvProbe * probe ) { m_probe = probe; }

		std::vector< surface* > m_surfaces; //geometry data for mesh.
		std::vector< Str > m_materials; //list of materials used in mesh
		std::vector< Transform * > m_transforms; //each entry is an instance of this mesh with unique transforms

	private:
		void AddSurface();

		bbox m_bounds;

		EnvProbe * m_probe;

		std::vector< vert_t > m_surfaceVerts; //the vertices of the surface being loaded
		std::vector< tri_t > m_surfaceTris; //vert indexes (every 3 represents a triangle) of the surface being loaded
		std::vector< unsigned int > m_surfaceVIDs; //unique id numbers for each vertex for the currently loading surface
};

/*
================================
Cube
================================
*/
class Cube {
	public:
		Cube() {};
		Cube( Str matName );
		~Cube() {}; //SHOULD BE DELETING M_SURFACE... RIGHT?
		bool LoadDecl();
		void DrawSurface( bool flip );
		surface * m_surface;

	private:
		unsigned int LoadVAO();
};

#endif