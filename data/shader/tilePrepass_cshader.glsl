#version 430 core

layout( binding = 0 ) uniform sampler2D depthTexture;

uniform int screenWidth;
uniform int screenHeight;
uniform int maxLightCount;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camPos;
uniform vec3 camLook;

const int maxLightsPerTile = 32;
struct LightIDs {
	uint count;
	int ids[maxLightsPerTile];
};
layout ( std430 ) buffer light_LUT {
	LightIDs lightLists[];
};

struct Position {
	float x, y, z;
};
struct Tri {
	uint vIdxs[3];
};
struct LightEffect {
	uint vCount;
	uint tCount;
	Position vPos[8];
	Tri tris[12];
};
layout( std430 ) buffer lightEffect_buffer {
	LightEffect lights[];
};


#define WORK_GROUP_SIZE 16
layout ( local_size_x=WORK_GROUP_SIZE, local_size_y=WORK_GROUP_SIZE, local_size_z = 1 ) in;

#define MAX_UNSIGNED_INT 0x7FFFFFFF

// This shares memory between the threads in the current workgroup
shared uint minDepth = MAX_UNSIGNED_INT;
shared uint maxDepth = 0;

vec3 Barycentric( vec2 P, vec2 A, vec2 B, vec2 C ) {
	// precompute the affine transform from fragment coordinates to barycentric coordinates
	float denom = 1.0 / ( ( A.x - C.x ) * ( B.y - A.y ) - ( A.x - B.x ) * ( C.y - A.y ) );
	vec3 barycentric_d0 = denom * vec3( B.y - C.y, C.y - A.y, A.y - B.y );
	vec3 barycentric_d1 = denom * vec3( C.x - B.x, A.x - C.x, B.x - A.x );
	vec3 barycentric_0 = denom * vec3( B.x * C.y - C.x * B.y, C.x * A.y - A.x * C.y, A.x * B.y - B.x * A.y );

	return P.x * barycentric_d0 + P.y * barycentric_d1 + barycentric_0;
}

bool PointInTriangle( vec2 P, vec2 A, vec2 B, vec2 C ) {
	//Compute vectors
	vec2 v0 = C - A;
	vec2 v1 = B - A;
	vec2 v2 = P - A;

	//Compute dot products
	float dot00 = dot( v0, v0 );
	float dot01 = dot( v0, v1 );
	float dot02 = dot( v0, v2 );
	float dot11 = dot( v1, v1 );
	float dot12 = dot( v1, v2 );

	//Compute barycentric coordinates
	float invDenom = 1.0 / ( dot00 * dot11 - dot01 * dot01 );
	float u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
	float v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

	// Check if point is in triangle
	return ( u >= 0.0 ) && ( v >= 0.0 ) && ( u + v < 1.0 );
}

//returns signed distance from point v to plane at position P with normal N
float distanceToPlane( vec3 v, vec3 N, vec3 P ) {
	return dot( N, v - P );
}

//test if point P is within volume
bool WithinVolume( LightEffect volume, vec3 P ) {
	for ( uint i = 0; i < volume.tCount; i++ ) {
		Tri testTri = volume.tris[i];

		Position p0 = volume.vPos[ testTri.vIdxs[0] ];
		Position p1 = volume.vPos[ testTri.vIdxs[1] ];
		Position p2 = volume.vPos[ testTri.vIdxs[2] ];

		vec3 p0_v3 = vec3( p0.x, p0.y, p0.z );
		vec3 p1_v3 = vec3( p1.x, p1.y, p1.z );
		vec3 p2_v3 = vec3( p2.x, p2.y, p2.z );

		vec3 p0p1 = p1_v3 - p0_v3;
		vec3 p0p2 = p2_v3 - p0_v3;

		vec3 plane_normal = normalize( cross( p0p1, p0p2 ) );

		if ( distanceToPlane( P, plane_normal, p2_v3 ) > 0.0 ) {
			return false;
		}
	}
	return true;
}

vec3 planeLineIntersection( vec3 v0, vec3 v1, vec3 N, vec3 P ) {
	vec3 Pv0 = v0 - P;
	vec3 v0v1 = v1 - v0;
	return v0 - v0v1 * ( dot( Pv0, N ) / dot( v0v1, N ) );
}

float getFragDepth( vec3 barycentric, vec3 vDepths ) {
	float ndc_depth = dot( barycentric, vDepths );
	return ( ( ( gl_DepthRange.far - gl_DepthRange.near ) * ndc_depth ) + gl_DepthRange.near + gl_DepthRange.far ) / 2.0;
}

void main () {
	//calculate the min/max for this tile (workgroup)
	//sample depth texture
	ivec2 storePos = ivec2( gl_GlobalInvocationID.xy );
	vec2 uv = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5 ) ) / vec2( screenWidth, screenHeight );	
	float depth = texture( depthTexture, uv ).r;

	//use atomics to update min/max for current group
	uint uDepth = uint( depth * MAX_UNSIGNED_INT );
	atomicMin( minDepth, uDepth );
	atomicMax( maxDepth, uDepth );

	barrier(); //Block until all threads finish the above code
	
	//map the integer coordinates to -1,1 normalized device coordinates
	const float numGroupsX	= float( screenWidth ) / float( WORK_GROUP_SIZE );
	const float xWidth		= float( WORK_GROUP_SIZE ) / float( screenWidth );
	float x					= 2.0 * ( float( gl_WorkGroupID.x ) + 0.5 ) / numGroupsX - 1.0;	// map from 0,1 to -1,1
	float xmin				= x - xWidth;
	float xmax				= x + xWidth;
	
	//map the integer coordinates to -1,1 normalized device coordinates
	const float numGroupsY	= float( screenHeight ) / float( WORK_GROUP_SIZE );
	const float yHeight		= float( WORK_GROUP_SIZE ) / float( screenHeight );
	float y					= 2.0 * ( float( gl_WorkGroupID.y ) + 0.5 ) / numGroupsY - 1.0;	// map from 0,1 to -1,1
	float ymin				= y - yHeight;
	float ymax				= y + yHeight;
	
	//calculate screen tile bounds in ndc space
	vec2 tileBounds[4];
	tileBounds[0] = vec2( xmin, ymin );
	tileBounds[1] = vec2( xmax, ymin );
	tileBounds[2] = vec2( xmax, ymax );
	tileBounds[3] = vec2( xmin, ymax );

	//bring min/max depths into range
	float minDepthZ = float( minDepth ) / float( MAX_UNSIGNED_INT );
	float maxDepthZ = float( maxDepth ) / float( MAX_UNSIGNED_INT );

	//populate group with the ids of the lights that affect it	
	int threadID = int( gl_LocalInvocationID.x + gl_LocalInvocationID.y * WORK_GROUP_SIZE );
	int tileID = int( gl_WorkGroupID.x + gl_WorkGroupID.y * screenWidth / WORK_GROUP_SIZE );
	const int threadsPerWorkGroup = WORK_GROUP_SIZE * WORK_GROUP_SIZE;

	//light binning
	mat4 MVP = projection * view;

	vec3 clipPlanePos = camPos + ( camLook * 0.0001 ); //hardcode a tiny number instead of using near-plane

	float hfov_half = atan( 1.0 / projection[0][0] );
	float vfov_half = atan( 1.0 / projection[1][1] );
	vec3 left_plane_normal = transpose( mat3( view ) ) * vec3( cos( hfov_half ) * -1.0, 0.0, sin( hfov_half ) );
	vec3 right_plane_normal = transpose( mat3( view ) ) * vec3( cos( hfov_half ), 0.0, sin( hfov_half ) );
	vec3 top_plane_normal = transpose( mat3( view ) ) * vec3( 0.0, cos( vfov_half ), sin( vfov_half ) );
	vec3 bottom_plane_normal = transpose( mat3( view ) ) * vec3( 0.0, cos( vfov_half ) * -1.0, sin( vfov_half ) );

	for ( int i = threadID; i < maxLightCount; i += threadsPerWorkGroup ) {
		LightEffect currentLightEffect = lights[i];

		bool withinLightEffect = WithinVolume( currentLightEffect, camPos );

		bool binLight = false;
		float light_minDepth = 100.0;
		float light_maxDepth = -100.0;

		for ( uint j = 0; j < currentLightEffect.tCount; j++ ) {
			Tri currentTri = currentLightEffect.tris[j];

			Position p0_pos = currentLightEffect.vPos[ currentTri.vIdxs[0] ];
			Position p1_pos = currentLightEffect.vPos[ currentTri.vIdxs[1] ];
			Position p2_pos = currentLightEffect.vPos[ currentTri.vIdxs[2] ];

			vec3 p0 = vec3( p0_pos.x, p0_pos.y, p0_pos.z );
			vec3 p1 = vec3( p1_pos.x, p1_pos.y, p1_pos.z );
			vec3 p2 = vec3( p2_pos.x, p2_pos.y, p2_pos.z );

			vec3 center = ( p0 + p1 + p2 ) / 3.0;
			vec3 p0p1 = p1 - p0;
			vec3 p1p2 = p2 - p1;
			vec3 p0p2 = p2 - p0;
			float radius = max( max( length( p0p1 ), length( p1p2 ) ), length( p0p2 ) ) / 2.0; 

			//test if the entire triangle is within the frustrum planes
			float left_plane_dist = distanceToPlane( center, normalize( left_plane_normal ) * -1.0, camPos );
			float right_plane_dist = distanceToPlane( center, normalize( right_plane_normal ) * -1.0, camPos );
			float top_plane_dist = distanceToPlane( center, normalize( top_plane_normal ) * -1.0, camPos );
			float bottom_plane_dist = distanceToPlane( center, normalize( bottom_plane_normal ) * -1.0, camPos );
			if ( left_plane_dist <= -radius || right_plane_dist <= -radius || top_plane_dist <= -radius || bottom_plane_dist <= -radius ) {
				continue;
			}

			//test if the entire triangle is behind the camera
			float tri_dist = distanceToPlane( center, camLook, clipPlanePos );
			if ( tri_dist <= -radius ) {
				continue;
			}

			/*
			//cull backfacing tris
			vec3 triNml = normalize( cross( p0p1, p0p2 ) );
			vec3 lookV0 = p0 - camPos;
			vec3 lookV1 = p1 - camPos;
			vec3 lookV2 = p2 - camPos;
			if ( dot( triNml, lookV0 ) <= 0.0 && dot( triNml, lookV1 ) <= 0.0 && dot( triNml, lookV2 ) <= 0.0 ) {
				continue;
			}
			*/

			//get distance of each vert to clip plane
			float p0_dis = distanceToPlane( p0, camLook, clipPlanePos );
			float p1_dis = distanceToPlane( p1, camLook, clipPlanePos );
			float p2_dis = distanceToPlane( p2, camLook, clipPlanePos );

			//in cases where only one vert is clipped, an extra tri needs to be created
			bool twoVertsInFront = false;
			vec3 p3 = vec3( 0.0, 0.0, 0.0 );

			//clip triangle
			if ( p0_dis < 0.0 && p1_dis < 0.0 && p2_dis < 0.0 ) { //discard triangle
				continue;
			} else if ( p0_dis < 0.0 && p1_dis < 0.0 ) { //clip p0 and p1			
				p0 = planeLineIntersection( p2, p0, camLook, clipPlanePos );
				p1 = planeLineIntersection( p2, p1, camLook, clipPlanePos );
			} else if ( p1_dis < 0.0 && p2_dis < 0.0 ) { //clip p1 and p2			
				p1 = planeLineIntersection( p0, p1, camLook, clipPlanePos );
				p2 = planeLineIntersection( p0, p2, camLook, clipPlanePos );
			} else if ( p2_dis < 0.0 && p0_dis < 0.0 ) { //clip p2 and p0			
				p2 = planeLineIntersection( p1, p2, camLook, clipPlanePos );
				p0 = planeLineIntersection( p1, p0, camLook, clipPlanePos );
			} else if ( p0_dis < 0.0 ) { //clip p0			
				twoVertsInFront = true;
				vec3 temp = p2;
				p2 = planeLineIntersection( p0, p1, camLook, clipPlanePos );
				p3 = planeLineIntersection( p0, temp, camLook, clipPlanePos );
				p0 = p1;
				p1 = temp;
			} else if ( p1_dis < 0.0 ) { //clip p1			
				twoVertsInFront = true;
				vec3 temp = p2;
				p2 = planeLineIntersection( p1, p2, camLook, clipPlanePos );
				p3 = planeLineIntersection( p1, p0, camLook, clipPlanePos );
				p1 = p0;
				p0 = temp;			
			} else if ( p2_dis < 0.0 ) { //clip p2			
				twoVertsInFront = true;
				vec3 temp = p2;
				p2 = planeLineIntersection( p2, p0, camLook, clipPlanePos );
				p3 = planeLineIntersection( temp, p1, camLook, clipPlanePos );
			}

			//bring tris into NDC space
			vec4 p0_v4 = MVP * vec4( p0, 1.0 );
			vec3 p0_v3 = p0_v4.xyz / p0_v4.w;

			vec4 p1_v4 = MVP * vec4( p1, 1.0 );
			vec3 p1_v3 = p1_v4.xyz / p1_v4.w;

			vec4 p2_v4 = MVP * vec4( p2, 1.0 );
			vec3 p2_v3 = p2_v4.xyz / p2_v4.w;

			vec4 p3_v4 = MVP * vec4( p3, 1.0 );
			vec3 p3_v3 = p3_v4.xyz / p3_v4.w;

			//light binning
			for ( uint k = 0; k < 4; k++ ) {
				vec2 frag_ndc = tileBounds[k];
				//test if frag_ndc is in the screenspace bounds of the triangle
				vec3 b1 = Barycentric( frag_ndc, p0_v3.xy, p1_v3.xy, p2_v3.xy );
				if ( b1.x >= 0.0 && b1.y >= 0.0 && b1.z >= 0.0 ) {
					binLight = true;

					//depth test
					float depth1 = getFragDepth( b1, vec3( p0_v3.z, p1_v3.z, p2_v3.z ) );
					if ( depth1 <= light_minDepth ){
						light_minDepth = depth1;
					}
					if ( depth1 >= light_maxDepth ){
						light_maxDepth = depth1;
					}
				}
				if ( twoVertsInFront ) {
					vec3 b2 = Barycentric( frag_ndc, p1_v3.xy, p3_v3.xy, p2_v3.xy );

					//test if frag_ndc is in the screenspace bounds of the triangle
					if ( b2.x >= 0.0 && b2.y >= 0.0 && b2.z >= 0.0 ) {
						binLight = true;

						//depth test
						float depth2 = getFragDepth( b2, vec3( p1_v3.z, p3_v3.z, p2_v3.z ) );
						if ( depth2 <= light_minDepth ){
							light_minDepth = depth2;
						}
						if ( depth2 >= light_maxDepth ){
							light_maxDepth = depth2;
						}
					}
				}
			}
			if ( binLight ) {
				if ( withinLightEffect ) {
					if ( light_minDepth >= minDepthZ ) {
						int id = int( atomicAdd( lightLists[tileID].count, 1 ) );
						lightLists[tileID].ids[id] = i;
						break;
					}
				} else {
					if ( light_minDepth <= maxDepthZ && light_maxDepth >= minDepthZ ) {
						int id = int( atomicAdd( lightLists[tileID].count, 1 ) );
						lightLists[tileID].ids[id] = i;
						break;
					}
				}
			}
		}
	}
}