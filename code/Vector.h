#pragma once
#ifndef __VECTOR_H_INCLUDE__
#define __VECTOR_H_INCLUDE__

#define EPSILON 0.00000001
#define PI 3.14159265359

float to_degrees( const float rads );
float to_radians( const float deg );

class Mat2;
class Mat3;
class Mat4;

class Vec3;
class Vec4;

class Vec2 {
	public:
		float x;
		float y;

		Vec2();
		Vec2( float n );
		Vec2( float x, float y );

		void operator=( Vec2 other );
		bool operator==( Vec2 other );

		float operator[]( int index ) const;
		float & operator[]( int index );

		Vec2 operator+( Vec2 other ) const;
		Vec2 operator+( float n ) const;
		void operator+=( const Vec2 other );
		void operator+=( const float n );

		Vec2 operator-( Vec2 other ) const;
		Vec2 operator-( float n ) const;
		void operator-=( const Vec2 other );
		void operator-=( const float n );

		Vec2 operator*( const float n ) const;
		Vec2 operator*( Mat2 mat ) const;
		Vec2 operator*( Mat3 mat ) const;
		Vec2 operator*( Mat4 mat ) const;
		void operator*=( const float n );
		void operator*=( const Mat2 mat );
		void operator*=( const Mat3 mat );
		void operator*=( const Mat4 mat );

		Vec2 operator/( float n ) const;
		void operator/=( const float n );

		unsigned int size() const { return 2; }
		float dot( Vec2 other ) const;
		float length() const;
		Vec2 normal() const;
		void normalize();
		Vec2 proj( Vec2 other ) const;
		float angle( Vec2 other ) const;

		Vec3 as_Vec3() const;
		Vec4 as_Vec4() const;
		const float * as_ptr() const { return &x; }

	private:
		float valByIdx( int idx ) const;
};

class Vec3 {
	public:
		float x;
		float y;
		float z;

		Vec3();
		Vec3( float n );
		Vec3( float x, float y, float z );

		void operator=( Vec3 other );
		bool operator==( Vec3 other );

		float operator[]( int index ) const;
		float & operator[]( int index );

		Vec3 operator+( Vec3 other ) const;
		Vec3 operator+( float n ) const;
		void operator+=( const Vec3 other );
		void operator+=( const float n );

		Vec3 operator-( Vec3 other ) const;
		Vec3 operator-( float n ) const;
		void operator-=( const Vec3 other );
		void operator-=( const float n );

		Vec3 operator*( const float n ) const;
		Vec3 operator*( Mat3 mat ) const;
		Vec3 operator*( Mat4 mat ) const;
		void operator*=( const float n );
		void operator*=( const Mat3 mat );
		void operator*=( const Mat4 mat );

		Vec3 operator/( float n ) const;
		void operator/=( float n );

		unsigned int size() const { return 3; }
		float dot( Vec3 other ) const;
		Vec3 cross( Vec3 other ) const;
		float length() const;
		Vec3 normal() const;
		void normalize();
		Vec3 proj( Vec3 other ) const;
		float angle( Vec3 other ) const;

		Vec2 as_Vec2() const;
		Vec4 as_Vec4() const;
		const float * as_ptr() const { return &x; }

	private:
		float valByIdx( int idx ) const;
};

class Vec4 {
	public:
		float x;
		float y;
		float z;
		float w;

		Vec4();
		Vec4( float n );
		Vec4( float a, float b, float c, float d );
		Vec4( Vec3 vec, float val );

		void operator=( Vec4 other );
		bool operator==( Vec4 other );

		float operator[]( int index ) const;
		float & operator[]( int index );

		Vec4 operator+( Vec4 other ) const;
		Vec4 operator+( float n ) const;
		void operator+=( const Vec4 other );
		void operator+=( const float n );

		Vec4 operator-( Vec4 other ) const;
		Vec4 operator-( float n ) const;
		void operator-=( const Vec4 other );
		void operator-=( const float n );

		Vec4 operator*( const float n ) const;
		Vec4 operator*( Mat4 mat ) const;
		void operator*=( const float n );
		void operator*=( const Mat4 mat );

		Vec4 operator/( float n ) const;
		void operator/=( const float n );

		unsigned int size() const { return 4; }
		float dot( Vec4 other ) const;
		float length() const;
		Vec4 normal() const;
		void normalize();
		Vec4 proj( Vec4 other ) const;
		float angle( Vec4 other ) const;

		Vec2 as_Vec2() const;
		Vec3 as_Vec3() const;
		const float * as_ptr() const { return &x; }

	private:
		float valByIdx( int idx ) const;
};

#endif

/* SMOKE TEST FUNCTIONS
void vec2Test() {
	Vec2 vec = Vec2();
	vec = vec + Vec2( 1.0f );
	vec += Vec2( 0.0f, 1.0f );
	Vec2 otherVec = Vec2( 2.0f, 2.0f );
	vec = otherVec - vec;
	vec -= otherVec;
	if ( vec == otherVec ) {
		return;
	}
	float x = vec[0];
	otherVec = vec * x;
	otherVec *= x;
	vec = otherVec / 2.0f;
	vec /= 2.0f;
	float dotProd = vec.dot( otherVec );
	vec.normalize();
	otherVec = otherVec.normal();
	float angle = vec.angle( otherVec );
	otherVec = otherVec.proj( vec );
	
	Vec3 three = vec.as_Vec3();
	Vec4 four = vec.as_Vec4();
}

void vec3Test() {
	Vec3 vec = Vec3();
	vec = vec + Vec3( 1.0f );
	vec += Vec3( 0.0f, 1.0f, 2.0f );
	Vec3 otherVec = Vec3( 2.0f, 2.0f, 2.0f );
	vec = otherVec - vec;
	vec -= otherVec;
	if ( vec == otherVec ) {
		return;
	}
	float x = vec[2];
	otherVec = vec * x;
	otherVec *= x;
	otherVec[2] = 5.0f;
	vec = otherVec / 2.0f;
	vec /= 2.0f;
	float dotProd = vec.dot( otherVec );
	vec.normalize();
	otherVec = vec.cross( otherVec );
	otherVec = otherVec.normal();
	float angle = vec.angle( otherVec );
	otherVec = otherVec.proj( vec );
	
	Vec2 two = vec.as_Vec2();
	Vec4 four = vec.as_Vec4();
}

void vec4Test() {
	Vec4 vec = Vec4();
	vec = vec + Vec4( 1.0f );
	vec += Vec4( 0.0f, 1.0f, 2.0f, 3.0f );
	Vec4 otherVec = Vec4( 2.0f, 2.0f, 2.0f, 2.0f );
	vec = otherVec - vec;
	vec -= otherVec;
	if ( vec == otherVec ) {
		return;
	}
	float x = vec[2];
	otherVec = vec * x;
	otherVec *= x;
	otherVec[2] = 5.0f;
	vec = otherVec / 2.0f;
	vec /= 2.0f;
	float dotProd = vec.dot( otherVec );
	vec.normalize();
	otherVec = otherVec.normal();
	float angle = vec.angle( otherVec );
	otherVec = otherVec.proj( vec );
	
	Vec2 two = vec.as_Vec2();
	Vec3 three = vec.as_Vec3();
}*/