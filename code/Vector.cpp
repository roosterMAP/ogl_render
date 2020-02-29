#include "Vector.h"
#include "Matrix.h"
#include <math.h>

float to_degrees( const float rads ) {
	return ( float )( rads * 180.0 / PI );
}

float to_radians( const float deg ) {
	return ( float )( deg * PI / 180.0 );
}


Vec2::Vec2() {
	x = 0.0;
	y = 0.0;
}

Vec2::Vec2( float n ) {
	x = n;
	y = n;
}

Vec2::Vec2( float a, float b ) {
	x = a;
	y = b;
}

void Vec2::operator=(Vec2 other) {
	x = other.x;
	y = other.y;
}

bool Vec2::operator==(Vec2 other) {
	return x == other.x && y == other.y;
}

bool Vec2::operator!=(Vec2 other) {
	return x != other.x || y != other.y;
}

float Vec2::operator[]( int index ) const {
	if ( index == 0 ) {
		return x;
	} else {
		return y;
	}
}

float & Vec2::operator[]( int index ) {
	if ( index == 0 ) {
		return x;
	} else {
		return y;
	}
}

Vec2 Vec2::operator+( Vec2 other ) const {
	Vec2 returnVec;
	returnVec.x = x + other.x;
	returnVec.y = y + other.y;
	return returnVec;
}

void Vec2::operator+=( const Vec2 other ) {
	x += other.x;
	y += other.y;
}

Vec2 Vec2::operator+( float n ) const {
	Vec2 returnVec;
	returnVec.x = x + n;
	returnVec.y = y + n;
	return returnVec;
}

void Vec2::operator+=( float n ) {
	x += n;
	y += n;
}

Vec2 Vec2::operator-( Vec2 other ) const {
	Vec2 returnVec;
	returnVec.x = x - other.x;
	returnVec.y = y - other.y;
	return returnVec;
}

void Vec2::operator-=( const Vec2 other ) {
	x -= other.x;
	y -= other.y;
}

Vec2 Vec2::operator-( float n ) const {
	Vec2 returnVec;
	returnVec.x = x - n;
	returnVec.y = y - n;
	return returnVec;
}

void Vec2::operator-=( const float n ) {
	x -= n;
	y -= n;
}

Vec2 Vec2::operator*( float n ) const {
	Vec2 returnVec;
	returnVec.x = x * n;
	returnVec.y = y * n;
	return returnVec;
}

Vec2 Vec2::operator*( Mat2 mat ) const {
	Vec2 newVec;
	newVec.x = dot( Vec2( mat.m_col1.x, mat.m_col2.x ) );
	newVec.y = dot( Vec2( mat.m_col1.y, mat.m_col2.y ) );
	return newVec;
}

Vec2 Vec2::operator*( Mat3 mat ) const {
	Vec3 oldVec = Vec3( x, y, 1.0f );
	Vec2 newVec;
	newVec.x = oldVec.dot( Vec3( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x ) );
	newVec.y = oldVec.dot( Vec3( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y ) );
	return newVec;
}

Vec2 Vec2::operator*( Mat4 mat ) const {
	Vec4 oldVec = Vec4( x, y, 0.0f, 1.0f );
	Vec2 newVec;
	newVec.x = oldVec.dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	newVec.y = oldVec.dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
	return newVec;
}

void Vec2::operator*=( const float n ) {
	x *= n;
	y *= n;
}

void Vec2::operator*=( const Mat2 mat ) {
	x = dot( Vec2( mat.m_col1.x, mat.m_col2.x ) );
	y = dot( Vec2( mat.m_col1.y, mat.m_col2.y ) );
}

void Vec2::operator*=( const Mat3 mat ) {
	Vec3 oldVec = Vec3( x, y, 1.0f );
	x = oldVec.dot( Vec3( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x ) );
	y = oldVec.dot( Vec3( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y ) );
}

void Vec2::operator*=( const Mat4 mat ) {
	Vec4 oldVec = Vec4( x, y, 0.0f, 1.0f );
	x = oldVec.dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	y = oldVec.dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
}

Vec2 Vec2::operator/( float n ) const {
	Vec2 returnVec;
	returnVec.x = x / n;
	returnVec.y = y / n;
	return returnVec;
}

void Vec2::operator/=( const float n ) {
	x /= n;
	y /= n;
}

float Vec2::dot( Vec2 other ) const {
	return x * other.x + y * other.y;
}

float Vec2::length() const {
	return sqrt( x * x + y * y );
}

Vec2 Vec2::normal() const {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		return Vec2();
	}
	Vec2 returnVec = Vec2( x / len, y / len );
	return returnVec;
}

void Vec2::normalize() {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		x = 0.0f;
		y = 0.0f;
	}
	x /= len;
	y /= len;
}

Vec2 Vec2::proj( Vec2 other ) const {
	Vec2 returnVec = other;
	float other_len = other.length();
	float scalar = dot( other ) / other_len;
	return returnVec * scalar;
}

float Vec2::angle( Vec2 other ) const {
	Vec2 nml = normal();
	Vec2 other_nml = other.normal();
	float rad = nml.dot( other_nml );
	if ( fabsf( rad ) > 1.0f ) {
		return 0.0f;
	}
	return rad;
}

Vec3 Vec2::as_Vec3() const {
	Vec3 vec = Vec3();
	vec.x = x;
	vec.y = y;
	return vec;
}

Vec4 Vec2::as_Vec4() const {
	Vec4 vec = Vec4();
	vec.x = x;
	vec.y = y;
	return vec;
}

float Vec2::valByIdx( int idx ) const {
	if ( idx == 0 ) {
		return x;
	} else {
		return y;
	}
}

Vec3::Vec3() {
	x = 0.0;
	y = 0.0;
	z = 0.0;
}

Vec3::Vec3( float n ) {
	x = n;
	y = n;
	z = n;
}

Vec3::Vec3( float a, float b, float c ) {
	x = a;
	y = b;
	z = c;
}

void Vec3::operator=(Vec3 other) {
	x = other.x;
	y = other.y;
	z = other.z;
}

bool Vec3::operator==(Vec3 other) {
	return x == other.x && y == other.y && z == other.z;
}

bool Vec3::operator!=(Vec3 other) {
	return x != other.x || y != other.y || z != other.z;
}

float Vec3::operator[]( int index ) const {
	if ( index == 0 ) {
		return x;
	} else if ( index == 1 ) {
		return y;
	} else {
		return z;
	}
}

float & Vec3::operator[]( int index ) {
	if ( index == 0 ) {
		return x;
	} else if ( index == 1 ) {
		return y;
	} else {
		return z;
	}
}

Vec3 Vec3::operator+( Vec3 other ) const {
	Vec3 returnVec;
	returnVec.x = x + other.x;
	returnVec.y = y + other.y;
	returnVec.z = z + other.z;
	return returnVec;
}

void Vec3::operator+=( const Vec3 other ) {
	x += other.x;
	y += other.y;
	z += other.z;
}

Vec3 Vec3::operator+( float n ) const {
	Vec3 returnVec;
	returnVec.x = x + n;
	returnVec.y = y + n;
	returnVec.z = z + n;
	return returnVec;
}

void Vec3::operator+=( const float n ) {
	x += n;
	y += n;
	z += n;
}

Vec3 Vec3::operator-( Vec3 other ) const {
	Vec3 returnVec;
	returnVec.x = x - other.x;
	returnVec.y = y - other.y;
	returnVec.z = z - other.z;
	return returnVec;
}

void Vec3::operator-=( const Vec3 other ) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
}

Vec3 Vec3::operator-( float n ) const {
	Vec3 returnVec;
	returnVec.x = x - n;
	returnVec.y = y - n;
	returnVec.z = z - n;
	return returnVec;
}

void Vec3::operator-=( const float n ) {
	x -= n;
	y -= n;
	z -= n;
}

Vec3 Vec3::operator*( float n ) const {
	Vec3 returnVec;
	returnVec.x = x * n;
	returnVec.y = y * n;
	returnVec.z = z * n;
	return returnVec;
}

Vec3 Vec3::operator*( Mat3 mat ) const {
	Vec3 oldVec = Vec3( x, y, z );
	Vec3 newVec;
	newVec.x = oldVec.dot( Vec3( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x ) );
	newVec.y = oldVec.dot( Vec3( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y ) );
	newVec.z = oldVec.dot( Vec3( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z ) );
	return newVec;
}

Vec3 Vec3::operator*( Mat4 mat ) const {
	Vec4 oldVec = Vec4( x, y, z, 1.0f );
	Vec3 newVec;
	newVec.x = oldVec.dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	newVec.y = oldVec.dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
	newVec.z = oldVec.dot( Vec4( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z, mat.m_col4.z ) );
	return newVec;
}

void Vec3::operator*=( const float n ) {
	x *= n;
	y *= n;
	z *= n;
}

void Vec3::operator*=( const Mat3 mat ) {
	Vec3 newVec = Vec3( x, y, z );
	x = newVec.dot( Vec3( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x ) );
	y = newVec.dot( Vec3( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y ) );
	z = newVec.dot( Vec3( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z ) );
}

void Vec3::operator*=( const Mat4 mat ) {
	Vec4 newVec = Vec4( x, y, z, 1.0f );
	x = newVec.dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	y = newVec.dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
	z = newVec.dot( Vec4( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z, mat.m_col4.z ) );
}

Vec3 Vec3::operator/( const float n ) const {
	Vec3 returnVec;
	returnVec.x = x / n;
	returnVec.y = y / n;
	returnVec.z = z / n;
	return returnVec;
}

void Vec3::operator/=( const float n ) {
	x /= n;
	y /= n;
	z /= n;
}

float Vec3::dot( Vec3 other ) const {
	return x * other.x + y * other.y + z * other.z;
}

Vec3 Vec3::cross( Vec3 other ) const {
	Vec3 returnVec;
	returnVec.x = y * other.z - z * other.y;
	returnVec.y = z * other.x - x * other.z;
	returnVec.z = x * other.y - y * other.x;
	return returnVec;
}

float Vec3::length() const {
	return sqrt( x * x + y * y + z * z );
}

Vec3 Vec3::normal() const {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		return Vec3();
	}
	Vec3 returnVec = Vec3( x / len, y / len, z / len );
	return returnVec;
}

void Vec3::normalize() {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	x /= len;
	y /= len;
	z /= len;
}

Vec3 Vec3::proj( Vec3 other ) const {
	Vec3 returnVec = other;
	float other_len = other.length();
	float scalar = dot( other ) / other_len;
	return returnVec * scalar;
}

float Vec3::angle( Vec3 other ) const {
	Vec3 nml = normal();
	Vec3 other_nml = other.normal();
	return acos( nml.dot( other_nml ) );
}

Vec2 Vec3::as_Vec2() const{
	Vec2 vec = Vec2();
	vec.x = x;
	vec.y = y;
	return vec;
}

Vec4 Vec3::as_Vec4() const {
	Vec4 vec = Vec4();
	vec.x = x;
	vec.y = y;
	vec.z = z;
	return vec;
}

float Vec3::valByIdx( int idx ) const {
	if ( idx == 0 ) {
		return x;
	} else if ( idx == 1 ) {
		return y;
	} else {
		return z;
	}
}

Vec4::Vec4() {
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 0.0f;
}

Vec4::Vec4( float n ) {
	x = n;
	y = n;
	z = n;
	w = n;
}

Vec4::Vec4( float a, float b, float c, float d ) {
	x = a;
	y = b;
	z = c;
	w = d;
}

Vec4::Vec4( Vec3 vec, float val ) {
	x = vec.x;
	y = vec.y;
	z = vec.z;
	w = val;
}

void Vec4::operator=( Vec4 other ) {
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
}

bool Vec4::operator==( Vec4 other ) {
	return x == other.x && y == other.y && z == other.z && w == other.w;
}

bool Vec4::operator!=( Vec4 other ) {
	return x != other.x || y != other.y || z != other.z || w != other.w;
}

float Vec4::operator[]( int index ) const {
	if ( index == 0 ) {
		return x;
	} else if ( index == 1 ) {
		return y;
	} else if ( index == 2 ) {
		return z;
	} else {
		return w;
	}
}

float & Vec4::operator[]( int index ) {
	if ( index == 0 ) {
		return x;
	} else if ( index == 1 ) {
		return y;
	} else if ( index == 2 ) {
		return z;
	} else {
		return w;
	}
}

Vec4 Vec4::operator+( Vec4 other ) const {
	Vec4 returnVec;
	returnVec.x = x + other.x;
	returnVec.y = y + other.y;
	returnVec.z = z + other.z;
	returnVec.w = w + other.w;
	return returnVec;
}

void Vec4::operator+=( const Vec4 other ) {
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
}

Vec4 Vec4::operator+( float n ) const {
	Vec4 returnVec;
	returnVec.x = x + n;
	returnVec.y = y + n;
	returnVec.z = z + n;
	returnVec.w = w + n;
	return returnVec;
}

void Vec4::operator+=( const float n ) {
	x += n;
	y += n;
	z += n;
	w += n;
}

Vec4 Vec4::operator-( Vec4 other ) const {
	Vec4 returnVec;
	returnVec.x = x - other.x;
	returnVec.y = y - other.y;
	returnVec.z = z - other.z;
	returnVec.w = w - other.w;
	return returnVec;
}

void Vec4::operator-=( const Vec4 other ) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
}

Vec4 Vec4::operator-( float n ) const {
	Vec4 returnVec;
	returnVec.x = x - n;
	returnVec.y = y - n;
	returnVec.z = z - n;
	returnVec.w = w - n;
	return returnVec;
}

void Vec4::operator-=( const float n ) {
	x -= n;
	y -= n;
	z -= n;
	w -= n;
}

Vec4 Vec4::operator*( float n ) const {
	Vec4 returnVec;
	returnVec.x = x * n;
	returnVec.y = y * n;
	returnVec.z = z * n;
	returnVec.w = w * n;
	return returnVec;
}

Vec4 Vec4::operator*( Mat4 mat ) const {
	Vec4 newVec;
	newVec.x = dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	newVec.y = dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
	newVec.z = dot( Vec4( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z, mat.m_col4.z ) );
	newVec.w = dot( Vec4( mat.m_col1.w, mat.m_col2.w, mat.m_col3.w, mat.m_col4.w ) );
	return newVec;
}

void Vec4::operator*=( const float n ) {
	x *= n;
	y *= n;
	z *= n;
	w *= n;
}

void Vec4::operator*=( const Mat4 mat ) {
	x = dot( Vec4( mat.m_col1.x, mat.m_col2.x, mat.m_col3.x, mat.m_col4.x ) );
	y = dot( Vec4( mat.m_col1.y, mat.m_col2.y, mat.m_col3.y, mat.m_col4.y ) );
	z = dot( Vec4( mat.m_col1.z, mat.m_col2.z, mat.m_col3.z, mat.m_col4.z ) );
	w = dot( Vec4( mat.m_col1.w, mat.m_col2.w, mat.m_col3.w, mat.m_col4.w ) );
}

Vec4 Vec4::operator/( float n ) const {
	Vec4 returnVec;
	returnVec.x = x / n;
	returnVec.y = y / n;
	returnVec.z = z / n;
	returnVec.w = w / n;
	return returnVec;
}

void Vec4::operator/=( const float n ) {
	x /= n;
	y /= n;
	z /= n;
	w /= n;
}

float Vec4::dot( Vec4 other ) const {
	return x * other.x + y * other.y + z * other.z + w * other.w;
}

float Vec4::length() const {
	return sqrt( x * x + y * y + z * z + w * w );
}

Vec4 Vec4::normal() const {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		return Vec4();
	}
	Vec4 returnVec = Vec4( x / len, y / len, z / len, w / len );
	return returnVec;
}

void Vec4::normalize() {
	float len = length();
	if ( fabsf( len ) < EPSILON ) {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	}
	x /= len;
	y /= len;
	z /= len;
	w /= len;
}

Vec4 Vec4::proj( Vec4 other ) const {
	Vec4 returnVec = other;
	float other_len = other.length();
	float scalar = dot( other ) / other_len;
	return returnVec * scalar;
}

float Vec4::angle( Vec4 other ) const {
	Vec4 nml = normal();
	Vec4 other_nml = other.normal();
	return acos( nml.dot( other_nml ) );
}

Vec2 Vec4::as_Vec2() const {
	Vec2 vec = Vec2();
	vec.x = x;
	vec.y = y;
	return vec;
}

Vec3 Vec4::as_Vec3() const {
	Vec3 vec = Vec3();
	vec.x = x;
	vec.y = y;
	vec.z = z;
	return vec;
}

float Vec4::valByIdx( int idx ) const {
	if ( idx == 0 ) {
		return x;
	} else if ( idx == 1 ) {
		return y;
	} else if ( idx == 2 ) {
		return z;
	} else {
		return w;
	}
}