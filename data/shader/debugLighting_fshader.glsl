#version 430 core

out vec4 FragColor;

uniform sampler2D sampleTexture;
uniform int mode;

const int maxLightsPerTile = 32;
struct LightIDs {
	uint count;
	int ids[maxLightsPerTile];
};
layout ( std430 ) buffer light_LUT {
	LightIDs lightLists[];
};

struct Tri {
	uint vIdxs[3];
};

struct LightEffect {
	uint vCount;
	uint tCount;
	float vPos[ 8 * 3 ];
	uint tris[ 12 * 3 ];
};
layout( std430 ) buffer lightEffect_buffer {
	LightEffect lights[];
};

#define WORK_GROUP_SIZE 16
uniform int screenWidth;
uniform int screenHeight;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camPos;
uniform vec3 camLook;
uniform int lightCount;

in vec2 TexCoord;
in mat3 TBN;

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

float distanceToPlane( vec3 v, vec3 N, vec3 P ) {
	return dot( N, v - P );
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

//test if point P is within volume
bool WithinVolume( LightEffect volume, vec3 P ) {
	for ( uint i = 0; i < volume.tCount; i++ ) {
		Tri testTri;
		testTri.vIdxs[0] = volume.tris[ i * 3 + 0];
		testTri.vIdxs[1] = volume.tris[ i * 3 + 1];
		testTri.vIdxs[2] = volume.tris[ i * 3 + 2];

		vec3 p0_v3 = vec3( volume.vPos[ testTri.vIdxs[0] * 3 ], volume.vPos[ testTri.vIdxs[0] * 3 + 1 ], volume.vPos[ testTri.vIdxs[0] * 3 + 2 ] );
		vec3 p1_v3 = vec3( volume.vPos[ testTri.vIdxs[1] * 3 ], volume.vPos[ testTri.vIdxs[1] * 3 + 1 ], volume.vPos[ testTri.vIdxs[1] * 3 + 2 ] );
		vec3 p2_v3 = vec3( volume.vPos[ testTri.vIdxs[2] * 3 ], volume.vPos[ testTri.vIdxs[2] * 3 + 1 ], volume.vPos[ testTri.vIdxs[2] * 3 + 2 ] );

		vec3 p0p1 = p1_v3 - p0_v3;
		vec3 p0p2 = p2_v3 - p0_v3;

		vec3 plane_normal = normalize( cross( p0p1, p0p2 ) );

		if ( distanceToPlane( P, plane_normal, p2_v3 ) > 0.0 ) {
			return false;
		}
	}
	return true;
}

void main() {
	if ( mode == 3 ) {
		vec3 normal = texture( sampleTexture, TexCoord ).rgb;
		normal = normalize( normal * 2.0 - 1.0 );
		normal *= TBN;
		FragColor = vec4( normal, 1.0 );
	} else if ( mode == 5 ) {
		//render the light volume into the scene
		bool binFrag = false;

		mat4 MVP = projection * view;
		float frag_ndc_x = gl_FragCoord.x / screenWidth * 2.0 - 1.0;
		float frag_ndc_y = gl_FragCoord.y / screenHeight * 2.0 - 1.0;
		vec2 frag_ndc = vec2( frag_ndc_x, frag_ndc_y );

		vec3 clipPlanePos = camPos + ( camLook * 0.0001 ); //hardcode a tiny number instead of using near-plane

		float hfov_half = atan( 1.0 / projection[0][0] );
		float vfov_half = atan( 1.0 / projection[1][1] );
		vec3 left_plane_normal = transpose( mat3( view ) ) * vec3( cos( hfov_half ) * -1.0, 0.0, sin( hfov_half ) );
		vec3 right_plane_normal = transpose( mat3( view ) ) * vec3( cos( hfov_half ), 0.0, sin( hfov_half ) );
		vec3 top_plane_normal = transpose( mat3( view ) ) * vec3( 0.0, cos( vfov_half ), sin( vfov_half ) );
		vec3 bottom_plane_normal = transpose( mat3( view ) ) * vec3( 0.0, cos( vfov_half ) * -1.0, sin( vfov_half ) );

		bool inside = true;

		for ( uint i = 0; i < lightCount; i++ ) {
			LightEffect currentLightEffect = lights[i];
			
			bool withinLightEffect = WithinVolume( currentLightEffect, camPos );

			for ( uint j = 0; j < currentLightEffect.tCount; j++ ) {
				Tri currentTri; 
				currentTri.vIdxs[0] = currentLightEffect.tris[ j * 3 + 0 ];
				currentTri.vIdxs[1] = currentLightEffect.tris[ j * 3 + 1 ];
				currentTri.vIdxs[2] = currentLightEffect.tris[ j * 3 + 2 ];

				vec3 p0 = vec3( currentLightEffect.vPos[ currentTri.vIdxs[0] * 3 + 0 ], currentLightEffect.vPos[ currentTri.vIdxs[0] * 3 + 1 ], currentLightEffect.vPos[ currentTri.vIdxs[0] * 3 + 2 ] );
				vec3 p1 = vec3( currentLightEffect.vPos[ currentTri.vIdxs[1] * 3 + 0 ], currentLightEffect.vPos[ currentTri.vIdxs[1] * 3 + 1 ], currentLightEffect.vPos[ currentTri.vIdxs[1] * 3 + 2 ] );
				vec3 p2 = vec3( currentLightEffect.vPos[ currentTri.vIdxs[2] * 3 + 0 ], currentLightEffect.vPos[ currentTri.vIdxs[2] * 3 + 1 ], currentLightEffect.vPos[ currentTri.vIdxs[2] * 3 + 2 ] );

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

				//cull backfacing tris
				if ( withinLightEffect == false ) {
					vec3 triNml = normalize( cross( p0p1, p0p2 ) );
					vec3 lookV0 = p0 - camPos;
					vec3 lookV1 = p1 - camPos;
					vec3 lookV2 = p2 - camPos;
					if ( dot( triNml, lookV0 ) <= 0.0 && dot( triNml, lookV1 ) <= 0.0 && dot( triNml, lookV2 ) <= 0.0 ) {
						continue;
					}
				}

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

				//bin fragment
				vec3 b1 = Barycentric( frag_ndc, p0_v3.xy, p1_v3.xy, p2_v3.xy );
				if ( b1.x >= 0.0 && b1.y >= 0.0 && b1.z >= 0.0 ) {
					//depth test
					float b1_depth = getFragDepth( b1, vec3( p0_v3.z, p1_v3.z, p2_v3.z ) );
					if ( withinLightEffect ) {
						if ( b1_depth > gl_FragCoord.z ) {
							binFrag = true;
							break;
						}
					} else {
						if ( b1_depth < gl_FragCoord.z ) {
							binFrag = true;
							break;
						}
					}
				}
				if ( twoVertsInFront ) {
					vec3 b2 = Barycentric( frag_ndc, p1_v3.xy, p3_v3.xy, p2_v3.xy );
					if ( b2.x >= 0.0 && b2.y >= 0.0 && b2.z >= 0.0 ) {
						//depth test
						float b2_depth = getFragDepth( b2, vec3( p1_v3.z, p3_v3.z, p2_v3.z ) );
						if ( withinLightEffect ) {
							if ( b2_depth > gl_FragCoord.z ) {
								binFrag = true;
								break;
							}
						} else {
							if ( b2_depth < gl_FragCoord.z ) {
								binFrag = true;
								break;
							}
						}
					}
				}
			}
		}

		vec4 color = vec4( 0.0, 1.0, 1.0, 1.0 );
		if ( binFrag ) {
			color = vec4( 1.0, 1.0, 0.0, 1.0 );
		}
		FragColor = color;

	} else if ( mode == 6 ) {
		vec4 color = vec4( 0.0, 0.0, 0.0, 1.0 );

		//get the light count per tile
		ivec2 tiledCoord = ivec2( gl_FragCoord.xy ) / WORK_GROUP_SIZE;
		int workGroupID = tiledCoord.x + tiledCoord.y * screenWidth / WORK_GROUP_SIZE;
		int numLights = int( lightLists[ workGroupID ].count );

		float maxLightPerTile = 9.0;
		float step = 1.0 / ( maxLightPerTile / 3.0 );
		float val = step * numLights;
		if ( numLights <= maxLightPerTile / 3.0 ) { //blue channel
			color.b = val;
		} else if ( numLights <= maxLightPerTile / 3.0 * 2.0 ) { //green channel
			color.g = val;
		} else { //red channel
			color.r = val;
		}
		FragColor = color;

	} else {
		vec3 color = texture( sampleTexture, TexCoord ).rgb;
		FragColor = vec4( color, 1.0 );
	}
}