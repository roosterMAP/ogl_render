#include "Vector.h"
#include "Matrix.h"

#include <assert.h>
#include <math.h>

Mat2::Mat2() {
	m_col1 = Vec2( 1.0f, 0.0f );
	m_col2 = Vec2( 0.0f, 1.0f );
}

Mat2::Mat2( float n ) {
	m_col1 = Vec2( n );
	m_col2 = Vec2( n );
}

Mat2::Mat2( float * data ) {
	m_col1 = Vec2( data[0], data[1] );
	m_col2 = Vec2( data[2], data[3] );
}

void Mat2::operator=( Mat2 other ) {
	m_col1 = other.m_col1;
	m_col2 = other.m_col2;
}

bool Mat2::operator==( Mat2 other ) {
	return m_col1 == other.m_col1 && m_col2 == other.m_col2;
}

Vec2 Mat2::operator[]( int i ) const {
	assert( i < 2 );
	if ( i == 0 ){
		return m_col1;
	} else {
		return m_col2;
	}
}

Vec2 & Mat2::operator[]( int i ) {
	assert( i < 2 );
	if ( i == 0 ){
		return m_col1;
	} else {
		return m_col2;
	}
}

Mat2 Mat2::operator+( Mat2 other ) const {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1 = m_col1 + other.m_col1;
	newMat.m_col2 = m_col2 + other.m_col2;
	return newMat;
}

void Mat2::operator+=( Mat2 other ) {
	m_col1 += other.m_col1;
	m_col2 += other.m_col2;
}

Mat2 Mat2::operator-( Mat2 other ) const {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1 = m_col1 - other.m_col1;
	newMat.m_col2 = m_col2 - other.m_col2;
	return newMat;
}

void Mat2::operator-=( Mat2 other ) {
	m_col1 -= other.m_col1;
	m_col2 -= other.m_col2;
}

Mat2 Mat2::operator*( float scalar ) const {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1 = m_col1 * scalar;
	newMat.m_col2 = m_col2 * scalar;
	return newMat;
}

void Mat2::operator*=( float scalar ) {
	m_col1 *= scalar;
	m_col2 *= scalar;
}

Mat2 Mat2::operator*( Vec2 vec ) const {
	Mat2 newMat = Mat2();

	newMat.m_col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0] );
	newMat.m_col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1] );

	newMat.m_col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0] );
	newMat.m_col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1] );

	return newMat;
}

void Mat2::operator*=( Vec2 vec ) {
	Vec2 col1 = Vec2();
	col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0] );
	col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1] );

	Vec2 col2 = Vec2();
	col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0] );
	col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1] );

	m_col1 = col1;
	m_col2 = col2;
}

Mat2 Mat2::operator*( Mat2 other ) const {
	Mat2 newMat = Mat2();

	newMat.m_col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0] );
	newMat.m_col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1] );

	newMat.m_col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0] );
	newMat.m_col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1] );

	return newMat;
}

void Mat2::operator*=( Mat2 other ) {
	Vec2 col1 = Vec2();
	col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0] );
	col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1] );

	Vec2 col2 = Vec2();
	col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0] );
	col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1] );

	m_col1 = col1;
	m_col2 = col2;
}

Mat2 Mat2::operator/( float scalar ) const {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1 = m_col1 / scalar;
	newMat.m_col2 = m_col2 / scalar;
	return newMat;
}

void Mat2::operator/=( float scalar ) {
	m_col1 /= scalar;
	m_col2 /= scalar;
}

void Mat2::transposed() {
	float temp;
	temp = m_col1[1];
	m_col1[1] = m_col2[0];
	m_col2[0] = temp;
}

Mat2 Mat2::transpose() {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1[0] = m_col1[0];
	newMat.m_col1[1] = m_col2[0];
	newMat.m_col2[0] = m_col1[1];
	newMat.m_col2[1] = m_col2[1];
	return newMat;
}

float Mat2::determinant() const {
	return m_col1[0] * m_col2[1] - m_col1[1] * m_col2[0];
}

Mat2 Mat2::inverse() const {
	const float det = determinant();
	assert( fabsf( det ) > EPSILON );
	const float invDet = 1.0f/ det;

	Mat2 inv = Mat2( 0.0f );

	inv.m_col1[0] = m_col2[1] * invDet;
	inv.m_col1[1] = -m_col1[1] * invDet;
	inv.m_col2[0] = -m_col2[0] * invDet;
	inv.m_col2[1] = m_col1[0] * invDet;

	return inv;
}

Mat3 Mat2::as_Mat3() {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1.as_Vec3();
	newMat.m_col2 = m_col2.as_Vec3();
	return newMat;
}

Mat4 Mat2::as_Mat4() {
	Mat4 newMat = Mat4( 0.0f );
	newMat.m_col1 = m_col1.as_Vec4();
	newMat.m_col2 = m_col2.as_Vec4();
	return newMat;
}

float Mat2::dot( const float * colVec, const float row_0, const float row_1 ) const {
	return colVec[0] * row_0 + colVec[1] * row_1;
}

Mat3::Mat3() {
	m_col1 = Vec3( 1.0f, 0.0f, 0.0f );
	m_col2 = Vec3( 0.0f, 1.0f, 0.0f );
	m_col3 = Vec3( 0.0f, 0.0f, 1.0f );
}

Mat3::Mat3( float n ) {
	m_col1 = Vec3( n );
	m_col2 = Vec3( n );
	m_col3 = Vec3( n );
}

Mat3::Mat3( float * data ) {
	m_col1 = Vec3( data[0], data[1], data[2] );
	m_col2 = Vec3( data[3], data[4], data[5] );
	m_col3 = Vec3( data[6], data[7], data[8] );
}

void Mat3::operator=( Mat3 other ) {
	m_col1 = other.m_col1;
	m_col2 = other.m_col2;
	m_col3 = other.m_col3;
}

bool Mat3::operator==( Mat3 other ) {
	return m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3;
}

Vec3 Mat3::operator[]( int i ) const {
	assert( i < 3 );
	if ( i == 0 ){
		return m_col1;
	} else if ( i == 1 ) {
		return m_col2;
	} else {
		return m_col3;
	}
}

Vec3 & Mat3::operator[]( int i ) {
	assert( i < 3 );
	if ( i == 0 ){
		return m_col1;
	} else if ( i == 1 ) {
		return m_col2;
	} else {
		return m_col3;
	}
}

Mat3 Mat3::operator+( Mat3 other ) const {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1 + other.m_col1;
	newMat.m_col2 = m_col2 + other.m_col2;
	newMat.m_col3 = m_col3 + other.m_col3;
	return newMat;
}

void Mat3::operator+=( Mat3 other ) {
	m_col1 += other.m_col1;
	m_col2 += other.m_col2;
	m_col3 += other.m_col3;
}

Mat3 Mat3::operator-( Mat3 other ) const {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1 - other.m_col1;
	newMat.m_col2 = m_col2 - other.m_col2;
	newMat.m_col3 = m_col3 - other.m_col3;
	return newMat;
}

void Mat3::operator-=( Mat3 other ) {
	m_col1 -= other.m_col1;
	m_col2 -= other.m_col2;
	m_col3 -= other.m_col3;
}

Mat3 Mat3::operator*( float scalar ) const {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1 * scalar;
	newMat.m_col2 = m_col2 * scalar;
	newMat.m_col3 = m_col3 * scalar;
	return newMat;
}

void Mat3::operator*=( float scalar ) {
	m_col1 *= scalar;
	m_col2 *= scalar;
	m_col3 *= scalar;
}

Mat3 Mat3::operator*( Vec3 vec ) const {
	Mat3 newMat = Mat3();

	newMat.m_col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col1[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	newMat.m_col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col2[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	newMat.m_col3[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col3[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col3[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	return newMat;
}

void Mat3::operator*=( Vec3 vec ) {
	Vec3 col1 = Vec3();
	col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col1[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	Vec3 col2 = Vec3();
	col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col2[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	Vec3 col3 = Vec3();
	col3[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col3[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col3[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	m_col1 = col1;
	m_col2 = col2;
	m_col3 = col3;
}

Mat3 Mat3::operator*( Mat3 other ) const {
	Mat3 newMat = Mat3();

	newMat.m_col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col1[2] = dot( other.m_col1.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	newMat.m_col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col2[2] = dot( other.m_col2.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	newMat.m_col3[0] = dot( other.m_col3.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	newMat.m_col3[1] = dot( other.m_col3.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	newMat.m_col3[2] = dot( other.m_col3.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	return newMat;
}

void Mat3::operator*=( Mat3 other ) {
	Vec3 col1 = Vec3();
	col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col1[2] = dot( other.m_col1.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	Vec3 col2 = Vec3();
	col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col2[2] = dot( other.m_col2.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	Vec3 col3 = Vec3();
	col3[0] = dot( other.m_col3.as_ptr(), m_col1[0], m_col2[0], m_col3[0] );
	col3[1] = dot( other.m_col3.as_ptr(), m_col1[1], m_col2[1], m_col3[1] );
	col3[2] = dot( other.m_col3.as_ptr(), m_col1[2], m_col2[2], m_col3[2] );

	m_col1 = col1;
	m_col2 = col2;
	m_col3 = col3;
}

Mat3 Mat3::operator/( float scalar ) const {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1 / scalar;
	newMat.m_col2 = m_col2 / scalar;
	newMat.m_col3 = m_col3 / scalar;
	return newMat;
}

void Mat3::operator/=( float scalar ) {
	m_col1 /= scalar;
	m_col2 /= scalar;
	m_col3 /= scalar;
}

const bool Mat3::isRotationMatrix() const {
	const float det = determinant();
	if ( 1.0f - det < EPSILON && det >= 0.0f ) {
		return true;
	}
	return false;
}

void Mat3::transposed() {
	float temp;

	temp = m_col1[1];
	m_col1[1] = m_col2[0];
	m_col2[0] = temp;

	temp = m_col1[2];
	m_col1[2] = m_col3[0];
	m_col3[0] = temp;

	temp = m_col2[2];
	m_col2[2] = m_col3[1];
	m_col3[1] = temp;
}

Mat3 Mat3::transpose() {
	Mat3 newMat = Mat3( 0.0f );
	float temp;

	newMat.m_col1[0] = m_col1[0];
	newMat.m_col1[1] = m_col2[0];
	newMat.m_col1[2] = m_col3[0];

	newMat.m_col2[0] = m_col1[1];
	newMat.m_col2[1] = m_col2[1];
	newMat.m_col2[2] = m_col3[1];

	newMat.m_col3[0] = m_col1[2];
	newMat.m_col3[1] = m_col2[2];
	newMat.m_col3[2] = m_col3[2];

	return newMat;
}

float Mat3::determinant() const {
	float det2_12_01 = m_col2[0] * m_col3[1] - m_col2[1] * m_col3[0];
	float det2_12_02 = m_col2[0] * m_col3[2] - m_col2[2] * m_col2[0];
	float det2_12_12 = m_col2[1] * m_col3[2] - m_col2[2] * m_col3[1];

	return m_col1[0] * det2_12_12 - m_col1[1] * det2_12_02 + m_col1[2] * det2_12_01;
}

Mat3 Mat3::inverse() const {
	const float det = determinant();
	assert( fabsf( det ) > EPSILON );
	const float invDet = 1.0 / det;

	Mat3 inv = Mat3( 0.0f );

	inv.m_col1[1] = m_col1[2] * m_col3[1] - m_col1[1] * m_col3[2];
	inv.m_col1[2] = m_col1[1] * m_col2[2] - m_col1[2] * m_col2[1];
	inv.m_col2[1] = m_col1[0] * m_col3[2] - m_col1[2] * m_col3[0];
	inv.m_col2[2] = m_col1[2] * m_col2[0] - m_col1[0] * m_col2[2];
	inv.m_col3[1] = m_col1[1] * m_col3[0] - m_col1[0] * m_col3[1];
	inv.m_col3[2] = m_col1[0] * m_col2[1] - m_col1[1] * m_col2[0];

	inv.m_col1[0] *= invDet;
	inv.m_col1[1] *= invDet;
	inv.m_col1[2] *= invDet;

	inv.m_col2[0] *= invDet;
	inv.m_col2[1] *= invDet;
	inv.m_col2[2] *= invDet;

	inv.m_col3[0] *= invDet;
	inv.m_col3[1] *= invDet;
	inv.m_col3[2] *= invDet;

	return inv;
}

Mat2 Mat3::as_Mat2() {
	Mat2 returnMat = Mat2( 0.0f );
	returnMat.m_col1 = m_col1.as_Vec2();
	returnMat.m_col2 = m_col2.as_Vec2();
	return returnMat;
}

Mat4 Mat3::as_Mat4() {
	Mat4 returnMat = Mat4( 0.0f );
	returnMat.m_col1 = m_col1.as_Vec4();
	returnMat.m_col2 = m_col2.as_Vec4();
	returnMat.m_col3 = m_col3.as_Vec4();
	returnMat.m_col4 = Vec4( 0.0f, 0.0f, 0.0f, 1.0f );
	return returnMat;
}

float Mat3::dot( const float * colVec, const float row_0, const float row_1, const float row_2 ) const {
	return colVec[0] * row_0 + colVec[1] * row_1 + colVec[2] * row_2;
}

Mat4::Mat4() {
	m_col1 = Vec4( 1.0f, 0.0f, 0.0f, 0.0f );
	m_col2 = Vec4( 0.0f, 1.0f, 0.0f, 0.0f );
	m_col3 = Vec4( 0.0f, 0.0f, 1.0f, 0.0f );
	m_col4 = Vec4( 0.0f, 0.0f, 0.0f, 1.0f );
}

Mat4::Mat4( float n ) {
	m_col1 = Vec4( n );
	m_col2 = Vec4( n );
	m_col3 = Vec4( n );
	m_col4 = Vec4( n );
}

Mat4::Mat4( const float * data ) {
	m_col1 = Vec4( data[0], data[1], data[2], data[3] );
	m_col2 = Vec4( data[4], data[5], data[6], data[7] );
	m_col3 = Vec4( data[8], data[9], data[10], data[11] );
	m_col4 = Vec4( data[12], data[13], data[14], data[15] );
}

void Mat4::LookAt( const Vec3 eye, const Vec3 center, const Vec3 up ) {	
	Vec3 z = center - eye;
	Vec3 x = z.cross( up );
	Vec3 y = x.cross( z );

	x.normalize();
	y.normalize();
	z.normalize();

	m_col1[0] = x[0];
	m_col2[0] = x[1];
	m_col3[0] = x[2];
	m_col4[0] = -x.dot( eye );

	m_col1[1] = y[0];
	m_col2[1] = y[1];
	m_col3[1] = y[2];
	m_col4[1] = -y.dot( eye );

	m_col1[2] = -z[0];
	m_col2[2] = -z[1];
	m_col3[2] = -z[2];
	m_col4[2] = z.dot( eye );

	m_col1[3] = 0.0f;
	m_col2[3] = 0.0f;
	m_col3[3] = 0.0f;
	m_col4[3] = 1.0f;
}

void Mat4::LookAt2( const Vec3 look, const Vec3 up, const Vec3 pos ) {	
	Vec3 r = look.cross( up ).normal();
	Vec3 d = look.normal();
	Vec3 u = up.normal();

	m_col1[0] = r[0];
	m_col2[0] = r[1];
	m_col3[0] = r[2];
	m_col4[0] = -r.dot( pos );

	m_col1[1] = u[0];
	m_col2[1] = u[1];
	m_col3[1] = u[2];
	m_col4[1] = -u.dot( pos );

	m_col1[2] = -d[0];
	m_col2[2] = -d[1];
	m_col3[2] = -d[2];
	m_col4[2] = d.dot( pos );

	m_col1[3] = 0.0f;
	m_col2[3] = 0.0f;
	m_col3[3] = 0.0f;
	m_col4[3] = 1.0f;
}

void Mat4::Perspective( const float verticalFOV, const float aspect, const float near, const float far ) {
	const float tanHalfFOV = tanf( verticalFOV / 2.0f );

	m_col1[0] = 1.0f / ( tanHalfFOV * aspect );
	m_col1[1] = 0.0f;
	m_col1[2] = 0.0f;
	m_col1[3] = 0.0f;

	m_col2[0] = 0.0f;
	m_col2[1] = 1.0f / tanHalfFOV;
	m_col2[2] = 0.0f;
	m_col2[3] = 0.0f;

	m_col3[0] = 0.0f;
	m_col3[1] = 0.0f;
	m_col3[2] = ( -far - near ) / ( far - near );
	m_col3[3] = -1.0f;

	m_col4[0] = 0.0f;
	m_col4[1] = 0.0f;
	m_col4[2] = ( -2.0f * far * near ) / ( far - near );
	m_col4[3] = 0.0f;
}

void Mat4::Orthographic( const float left, const float right, const float bottom, const float top ) {
	const float near = -1.0f;
	const float far = 1.0f;
	Orthographic( left, right, bottom, top, near, far );
}

void Mat4::Orthographic( const float left, const float right, const float bottom, const float top, const float near, const float far ) {
	m_col1[0] = 2.0f / ( right - left );
	m_col1[1] = 0.0f;
	m_col1[2] = 0.0f;
	m_col1[3] = 0.0f;

	m_col2[0] = 0.0f;
	m_col2[1] = 2.0f / ( top - bottom );
	m_col2[2] = 0.0f;
	m_col2[3] = 0.0f;

	m_col3[0] = 0.0f;
	m_col3[1] = 0.0f;
	m_col3[2] = -2.0f / ( far - near );
	m_col3[3] = 0.0f;

	m_col4[0] = ( right + left ) / ( right - left ) * -1.0f;
	m_col4[1] = ( top + bottom ) / ( top - bottom ) * -1.0f;
	m_col4[2] = ( far + near ) / ( far - near ) * -1.0f;
	m_col4[3] = 1.0f;
}

void Mat4::Translate( const Vec3 offset ) {
	m_col4[0] = offset[0];
	m_col4[1] = offset[1];
	m_col4[2] = offset[2];
	m_col4[3] = 1.0f;
}

void Mat4::operator=( Mat4 other ) {
	m_col1 = other.m_col1;
	m_col2 = other.m_col2;
	m_col3 = other.m_col3;
	m_col4 = other.m_col4;
}

bool Mat4::operator==( Mat4 other ) {
	return m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3 && m_col4 == other.m_col4;
}

Vec4 Mat4::operator[]( int i ) const {
	assert( i < 4 );
	if ( i == 0 ){
		return m_col1;
	} else if ( i == 1 ) {
		return m_col2;
	} else if ( i == 2 ) {
		return m_col3;
	} else {
		return m_col4;
	}
}

Vec4 & Mat4::operator[]( int i ) {
	assert( i < 4 );
	if ( i == 0 ){
		return m_col1;
	} else if ( i == 1 ) {
		return m_col2;
	} else if ( i == 2 ) {
		return m_col3;
	} else {
		return m_col4;
	}
}

Mat4 Mat4::operator+( Mat4 other ) const {
	Mat4 newMat = Mat4( 0.0f );
	newMat.m_col1 = m_col1 + other.m_col1;
	newMat.m_col2 = m_col2 + other.m_col2;
	newMat.m_col3 = m_col3 + other.m_col3;
	newMat.m_col4 = m_col4 + other.m_col4;
	return newMat;
}

void Mat4::operator+=( Mat4 other ) {
	m_col1 += other.m_col1;
	m_col2 += other.m_col2;
	m_col3 += other.m_col3;
	m_col4 += other.m_col4;
}

Mat4 Mat4::operator-( Mat4 other ) const {
	Mat4 newMat = Mat4( 0.0f );
	newMat.m_col1 = m_col1 - other.m_col1;
	newMat.m_col2 = m_col2 - other.m_col2;
	newMat.m_col3 = m_col3 - other.m_col3;
	newMat.m_col4 = m_col4 - other.m_col4;
	return newMat;
}

void Mat4::operator-=( Mat4 other ) {
	m_col1 -= other.m_col1;
	m_col2 -= other.m_col2;
	m_col3 -= other.m_col3;
	m_col4 -= other.m_col4;
}

Mat4 Mat4::operator*( float scalar ) const {
	Mat4 newMat = Mat4( 0.0f );
	newMat.m_col1 = m_col1 * scalar;
	newMat.m_col2 = m_col2 * scalar;
	newMat.m_col3 = m_col3 * scalar;
	newMat.m_col4 = m_col4 * scalar;
	return newMat;
}

void Mat4::operator*=( float scalar ) {
	m_col1 *= scalar;
	m_col2 *= scalar;
	m_col3 *= scalar;
	m_col4 *= scalar;
}

Mat4 Mat4::operator*( Vec4 vec ) const {
	Mat4 newMat = Mat4();

	newMat.m_col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col1[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col1[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col2[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col2[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col3[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col3[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col3[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col3[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col4[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col4[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col4[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col4[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	return newMat;
}

void Mat4::operator*=( Vec4 vec ) {
	Vec4 col1 = Vec4();
	col1[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col1[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col1[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col1[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	Vec4 col2 = Vec4();
	col2[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col2[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col2[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col2[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	Vec4 col3 = Vec4();
	col3[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col3[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col3[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col3[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	Vec4 col4 = Vec4();
	col4[0] = dot( vec.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col4[1] = dot( vec.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col4[2] = dot( vec.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col4[3] = dot( vec.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	m_col1 = col1;
	m_col2 = col2;
	m_col3 = col3;
	m_col4 = col4;
}

Mat4 Mat4::operator*( Mat4 other ) const {
	Mat4 newMat = Mat4();

	newMat.m_col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col1[2] = dot( other.m_col1.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col1[3] = dot( other.m_col1.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col2[2] = dot( other.m_col2.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col2[3] = dot( other.m_col2.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col3[0] = dot( other.m_col3.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col3[1] = dot( other.m_col3.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col3[2] = dot( other.m_col3.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col3[3] = dot( other.m_col3.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	newMat.m_col4[0] = dot( other.m_col4.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	newMat.m_col4[1] = dot( other.m_col4.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	newMat.m_col4[2] = dot( other.m_col4.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	newMat.m_col4[3] = dot( other.m_col4.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	return newMat;
}

void Mat4::operator*=( Mat4 other ) {
	Vec4 col1 = Vec4();
	col1[0] = dot( other.m_col1.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col1[1] = dot( other.m_col1.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col1[2] = dot( other.m_col1.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col1[3] = dot( other.m_col1.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );	

	Vec4 col2 = Vec4();
	col2[0] = dot( other.m_col2.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col2[1] = dot( other.m_col2.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col2[2] = dot( other.m_col2.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col2[3] = dot( other.m_col2.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );	

	Vec4 col3 = Vec4();
	col3[0] = dot( other.m_col3.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col3[1] = dot( other.m_col3.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col3[2] = dot( other.m_col3.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col3[3] = dot( other.m_col3.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );	

	Vec4 col4 = Vec4();
	col4[0] = dot( other.m_col4.as_ptr(), m_col1[0], m_col2[0], m_col3[0], m_col4[0] );
	col4[1] = dot( other.m_col4.as_ptr(), m_col1[1], m_col2[1], m_col3[1], m_col4[1] );
	col4[2] = dot( other.m_col4.as_ptr(), m_col1[2], m_col2[2], m_col3[2], m_col4[2] );
	col4[3] = dot( other.m_col4.as_ptr(), m_col1[3], m_col2[3], m_col3[3], m_col4[3] );

	m_col1 = col1;
	m_col2 = col2;
	m_col3 = col3;
	m_col4 = col4;
}

Mat4 Mat4::operator/( float scalar ) const {
	Mat4 newMat = Mat4( 0.0f );
	newMat.m_col1 = m_col1 / scalar;
	newMat.m_col2 = m_col2 / scalar;
	newMat.m_col3 = m_col3 / scalar;
	newMat.m_col4 = m_col4 / scalar;
	return newMat;
}

void Mat4::operator/=( float scalar ) {
	m_col1 /= scalar;
	m_col2 /= scalar;
	m_col3 /= scalar;
	m_col4 /= scalar;
}

void Mat4::transposed() {
	float temp;

	temp = m_col1[1];
	m_col1[1] = m_col2[0];
	m_col2[0] = temp;

	temp = m_col1[2];
	m_col1[2] = m_col3[0];
	m_col3[0] = temp;

	temp = m_col1[3];
	m_col1[3] = m_col4[0];
	m_col4[0] = temp;

	temp = m_col2[2];
	m_col2[2] = m_col3[1];
	m_col3[1] = temp;

	temp = m_col2[3];
	m_col2[3] = m_col4[1];
	m_col4[1] = temp;

	temp = m_col3[3];
	m_col3[3] = m_col4[2];
	m_col4[2] = temp;
}

Mat4 Mat4::transpose() {
	Mat4 trans = Mat4( 0.0f );
	float temp;

	trans.m_col1[0] = m_col1[0];
	trans.m_col2[1] = m_col2[1];
	trans.m_col3[2] = m_col3[2];
	trans.m_col4[3] = m_col4[3];

	temp = m_col1[1];
	trans.m_col1[1] = m_col2[0];
	trans.m_col2[0] = temp;

	temp = m_col1[2];
	trans.m_col1[2] = m_col3[0];
	trans.m_col3[0] = temp;

	temp = m_col1[3];
	trans.m_col1[3] = m_col4[0];
	trans.m_col4[0] = temp;

	temp = m_col2[2];
	trans.m_col2[2] = m_col3[1];
	trans.m_col3[1] = temp;

	temp = m_col2[3];
	trans.m_col2[3] = m_col4[1];
	trans.m_col4[1] = temp;

	temp = m_col3[3];
	trans.m_col3[3] = m_col4[2];
	trans.m_col4[2] = temp;

	return trans;
}

float Mat4::determinant() const {
		//2x2 sub-determinants
		const float det2_01_01 = m_col1[0] * m_col2[1] - m_col1[1] * m_col2[0];
		const float det2_01_02 = m_col1[0] * m_col2[2] - m_col1[2] * m_col2[0];
		const float det2_01_03 = m_col1[0] * m_col2[3] - m_col1[3] * m_col2[0];
		const float det2_01_12 = m_col1[1] * m_col2[2] - m_col1[2] * m_col2[1];
		const float det2_01_13 = m_col1[1] * m_col2[3] - m_col1[3] * m_col2[1];
		const float det2_01_23 = m_col1[2] * m_col2[3] - m_col1[3] * m_col2[2];

		//3x3 sub-determinants
		const float det3_201_012 = m_col3[0] * det2_01_12 - m_col3[1] * det2_01_02 + m_col3[2] * det2_01_01;
		const float det3_201_013 = m_col3[0] * det2_01_13 - m_col3[1] * det2_01_03 + m_col3[3] * det2_01_01;
		const float det3_201_023 = m_col3[0] * det2_01_23 - m_col3[2] * det2_01_03 + m_col3[3] * det2_01_02;
		const float det3_201_123 = m_col3[1] * det2_01_23 - m_col3[2] * det2_01_13 + m_col3[3] * det2_01_12;

		return ( -det3_201_123 * m_col4[0] + det3_201_023 * m_col4[1] - det3_201_013 * m_col4[2] + det3_201_012 * m_col4[3] );
}

Mat4 Mat4::inverse() const {
	//2x2 sub-determinants required to calculate 4x4 determinant
	float det2_01_01 = m_col1[0] * m_col2[1] - m_col1[1] * m_col2[0];
	float det2_01_02 = m_col1[0] * m_col2[2] - m_col1[2] * m_col2[0];
	float det2_01_03 = m_col1[0] * m_col2[3] - m_col1[3] * m_col2[0];
	float det2_01_12 = m_col1[1] * m_col2[2] - m_col1[2] * m_col2[1];
	float det2_01_13 = m_col1[1] * m_col2[3] - m_col1[3] * m_col2[1];
	float det2_01_23 = m_col1[2] * m_col2[3] - m_col1[3] * m_col2[2];

	//3x3 sub-determinants required to calculate 4x4 determinant
	float det3_201_012 = m_col3[0] * det2_01_12 - m_col3[1] * det2_01_02 + m_col3[2] * det2_01_01;
	float det3_201_013 = m_col3[0] * det2_01_13 - m_col3[1] * det2_01_03 + m_col3[3] * det2_01_01;
	float det3_201_023 = m_col3[0] * det2_01_23 - m_col3[2] * det2_01_03 + m_col3[3] * det2_01_02;
	float det3_201_123 = m_col3[1] * det2_01_23 - m_col3[2] * det2_01_13 + m_col3[3] * det2_01_12;
	
	const float det = ( -det3_201_123 * m_col4[0] + det3_201_023 * m_col4[1] - det3_201_013 * m_col4[2] + det3_201_012 * m_col4[3] );

	assert( fabsf( det ) > EPSILON );
	const float invDet = 1.0 / det;
	
	//remaining 2x2 sub-determinants
	float det2_03_01 = m_col1[0] * m_col4[1] - m_col1[1] * m_col4[0];
	float det2_03_02 = m_col1[0] * m_col4[2] - m_col1[2] * m_col4[0];
	float det2_03_03 = m_col1[0] * m_col4[3] - m_col1[3] * m_col4[0];
	float det2_03_12 = m_col1[1] * m_col4[2] - m_col1[2] * m_col4[1];
	float det2_03_13 = m_col1[1] * m_col4[3] - m_col1[3] * m_col4[1];
	float det2_03_23 = m_col1[2] * m_col4[3] - m_col1[3] * m_col4[2];

	float det2_13_01 = m_col2[0] * m_col4[1] - m_col2[1] * m_col4[0];
	float det2_13_02 = m_col2[0] * m_col4[2] - m_col2[2] * m_col4[0];
	float det2_13_03 = m_col2[0] * m_col4[3] - m_col2[3] * m_col4[0];
	float det2_13_12 = m_col2[1] * m_col4[2] - m_col2[2] * m_col4[1];
	float det2_13_13 = m_col2[1] * m_col4[3] - m_col2[3] * m_col4[1];
	float det2_13_23 = m_col2[2] * m_col4[3] - m_col2[3] * m_col4[2];

	//remaining 3x3 sub-determinants
	float det3_203_012 = m_col3[0] * det2_03_12 - m_col3[1] * det2_03_02 + m_col3[2] * det2_03_01;
	float det3_203_013 = m_col3[0] * det2_03_13 - m_col3[1] * det2_03_03 + m_col3[3] * det2_03_01;
	float det3_203_023 = m_col3[0] * det2_03_23 - m_col3[2] * det2_03_03 + m_col3[3] * det2_03_02;
	float det3_203_123 = m_col3[1] * det2_03_23 - m_col3[2] * det2_03_13 + m_col3[3] * det2_03_12;

	float det3_213_012 = m_col3[0] * det2_13_12 - m_col3[1] * det2_13_02 + m_col3[2] * det2_13_01;
	float det3_213_013 = m_col3[0] * det2_13_13 - m_col3[1] * det2_13_03 + m_col3[3] * det2_13_01;
	float det3_213_023 = m_col3[0] * det2_13_23 - m_col3[2] * det2_13_03 + m_col3[3] * det2_13_02;
	float det3_213_123 = m_col3[1] * det2_13_23 - m_col3[2] * det2_13_13 + m_col3[3] * det2_13_12;

	float det3_301_012 = m_col4[0] * det2_01_12 - m_col4[1] * det2_01_02 + m_col4[2] * det2_01_01;
	float det3_301_013 = m_col4[0] * det2_01_13 - m_col4[1] * det2_01_03 + m_col4[3] * det2_01_01;
	float det3_301_023 = m_col4[0] * det2_01_23 - m_col4[2] * det2_01_03 + m_col4[3] * det2_01_02;
	float det3_301_123 = m_col4[1] * det2_01_23 - m_col4[2] * det2_01_13 + m_col4[3] * det2_01_12;

	Mat4 inv = Mat4( 0.0f );

	inv.m_col1[0] =	-det3_213_123 * invDet;
	inv.m_col2[0] = det3_213_023 * invDet;
	inv.m_col3[0] = -det3_213_013 * invDet;
	inv.m_col4[0] = det3_213_012 * invDet;
	
	inv.m_col1[1] = det3_203_123 * invDet;
	inv.m_col2[1] = -det3_203_023 * invDet;
	inv.m_col3[1] = det3_203_013 * invDet;
	inv.m_col4[1] = -det3_203_012 * invDet;

	inv.m_col1[2] = det3_301_123 * invDet;
	inv.m_col2[2] = -det3_301_023 * invDet;
	inv.m_col3[2] = det3_301_013 * invDet;
	inv.m_col4[2] = -det3_301_012 * invDet;

	inv.m_col1[3] = -det3_201_123 * invDet;
	inv.m_col2[3] = det3_201_023 * invDet;
	inv.m_col3[3] = -det3_201_013 * invDet;
	inv.m_col4[3] = det3_201_012 * invDet;

	return inv;
}

Mat2 Mat4::as_Mat2() {
	Mat2 newMat = Mat2( 0.0f );
	newMat.m_col1 = m_col1.as_Vec2();
	newMat.m_col2 = m_col2.as_Vec2();
	return newMat;
}

Mat3 Mat4::as_Mat3() {
	Mat3 newMat = Mat3( 0.0f );
	newMat.m_col1 = m_col1.as_Vec3();
	newMat.m_col2 = m_col2.as_Vec3();
	newMat.m_col3 = m_col3.as_Vec3();
	return newMat;
}

float Mat4::dot( const float * colVec, const float row_0, const float row_1, const float row_2, const float row_3 ) const {
	return colVec[0] * row_0 + colVec[1] * row_1 + colVec[2] * row_2 + colVec[3] * row_3;
}