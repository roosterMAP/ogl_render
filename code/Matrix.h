#pragma once
#ifndef __MATRIX_H_INCLUDE__
#define __MATRIX_H_INCLUDE__

class Vec2;
class Vec3;
class Vec4;

class Mat2 {
	public:
		Mat2();
		Mat2( float n );
		Mat2( float * data );

		void operator=( Mat2 other );
		bool operator==( Mat2 other );

		Vec2 operator[]( int i ) const;
		Vec2 & operator[]( int i );

		Mat2 operator+( Mat2 other ) const;
		void operator+=( Mat2 other );

		Mat2 operator-( Mat2 other ) const;
		void operator-=( Mat2 other );

		Mat2 operator*( float scalar ) const;
		void operator*=( float scalar );
		Mat2 operator*( Vec2 vec ) const;
		void operator*=( Vec2 vec );
		Mat2 operator*( Mat2 other ) const;
		void operator*=( Mat2 other );

		Mat2 operator/( float scalar ) const;
		void operator/=( float scalar );

		unsigned int size() const { return 2; }

		void transposed();
		Mat2 transpose();
		float determinant() const;
		Mat2 inverse() const;

		Mat3 as_Mat3();
		Mat4 as_Mat4();
		const float * as_ptr() const { return m_col1.as_ptr(); }

	private:
		float dot( const float * colVec, const float row_0, const float row_1 ) const;

		Vec2 m_col1;
		Vec2 m_col2;

	friend class Vec2;
	friend class Vec3;
	friend class Vec4;
	friend class Mat3;
	friend class Mat4;
};

class Mat3 {
	public:
		Mat3();
		Mat3( float n );
		Mat3( float * data );

		void operator=( Mat3 other );
		bool operator==( Mat3 other );

		Vec3 operator[]( int i ) const;
		Vec3 & operator[]( int i );

		Mat3 operator+( Mat3 other ) const;
		void operator+=( Mat3 other );

		Mat3 operator-( Mat3 other ) const;
		void operator-=( Mat3 other );

		Mat3 operator*( float scalar ) const;
		void operator*=( float scalar );
		Mat3 operator*( Vec3 vec ) const;
		void operator*=( Vec3 vec );
		Mat3 operator*( Mat3 other ) const;
		void operator*=( Mat3 other );

		Mat3 operator/( float scalar ) const;
		void operator/=( float scalar );

		unsigned int size() const { return 3; }

		const bool isRotationMatrix() const;

		void transposed();
		Mat3 transpose();
		float determinant() const;
		Mat3 inverse() const;

		Mat2 as_Mat2();
		Mat4 as_Mat4();
		const float * as_ptr() const { return m_col1.as_ptr(); }

	private:
		float dot( const float * colVec, const float row_0, const float row_1, const float row_2 ) const;

		Vec3 m_col1;
		Vec3 m_col2;
		Vec3 m_col3;

	friend class Vec2;
	friend class Vec3;
	friend class Vec4;
	friend class Mat2;
	friend class Mat4;
};

class Mat4 {
	public:
		Mat4();
		Mat4( float n );
		Mat4( const float * data );

		void LookAt( const Vec3 eye, const Vec3 center, const Vec3 up );
		void LookAt2( const Vec3 eye, const Vec3 center, const Vec3 up );
		void Perspective( const float verticalFOV, const float aspect, const float near, const float far );
		void Orthographic( const float left, const float right, const float bottom, const float top );
		void Orthographic( const float left, const float right, const float bottom, const float top, const float near, const float far );
		void Translate( const Vec3 offset );

		void operator=( Mat4 other );
		bool operator==( Mat4 other );

		Vec4 operator[]( int i ) const;
		Vec4 & operator[]( int i );

		Mat4 operator+( Mat4 other ) const;
		void operator+=( Mat4 other );

		Mat4 operator-( Mat4 other ) const;
		void operator-=( Mat4 other );

		Mat4 operator*( float scalar ) const;
		void operator*=( float scalar );
		Mat4 operator*( Vec4 vec ) const;
		void operator*=( Vec4 vec );
		Mat4 operator*( Mat4 other ) const;
		void operator*=( Mat4 other );

		Mat4 operator/( float scalar ) const;
		void operator/=( float scalar );

		unsigned int size() const { return 3; }

		void transposed();
		Mat4 transpose();
		float determinant() const;
		Mat4 inverse() const;

		Mat2 as_Mat2();
		Mat3 as_Mat3();
		const float * as_ptr() const { return m_col1.as_ptr(); }

	private:
		float dot( const float * colVec, const float row_0, const float row_1, const float row_2, const float row_3 ) const;

		Vec4 m_col1;
		Vec4 m_col2;
		Vec4 m_col3;
		Vec4 m_col4;

	friend class Vec2;
	friend class Vec3;
	friend class Vec4;
	friend class Mat2;
	friend class Mat3;
};

#endif