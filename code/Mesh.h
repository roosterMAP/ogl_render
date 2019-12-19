#pragma once
#include <vector>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "Decl.h"
#include "String.h"

#include "mikktspace.h"

struct tri_t {
	unsigned int a;
	unsigned int b;
	unsigned int c;
};

struct vert_t {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 tang;
	glm::vec2 uv;
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
	glm::vec3 min;
	glm::vec3 max;
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

		void SetPosition( glm::vec3 pos ) { m_position = pos; }
		void SetRotation( glm::mat3x3 rot ) { m_rotation = rot; }
		void SetScale( glm::vec3 scl ) { m_scale = scl; }
		bool WorldXfrm( glm::mat4x4* model );

	private:
		glm::vec3 m_position;
		glm::mat3x3 m_rotation;
		glm::vec3 m_scale;
		glm::mat4x4 m_xfrm;
};

/*
================================
Mesh
================================
*/
class Mesh {
	public:
		Mesh() {};
		~Mesh() { delete m_name; };
		bool LoadMSHFromFile( const char * msh_relative );
		bool LoadOBJFromFile( const char * obj_relative );
		void DrawSurface( unsigned int surfaceIdx );
		unsigned int LoadVAO( const unsigned int surfaceIdx );
				
		char * m_name;

		const bbox& GetBounds() { return m_bounds; }

		std::vector< surface* > m_surfaces; //geometry data for mesh.
		std::vector< Str > m_materials; //list of materials used in mesh
		std::vector< Transform * > m_transforms; //each entry is an instance of this mesh with unique transforms

	private:
		void AddSurface();

		bbox m_bounds;

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