#include "Camera.h"
#include <math.h>

Camera::Camera() {
	m_fov = 45.0;
	m_position = Vec3( 0.0, 0.0, 3.0 );
	m_look = Vec3( 0.0, 0.0, -1.0 );
	m_up = Vec3( 0.0, 1.0, 0.0 );
	m_right = m_up.cross( m_look );
	m_right.normalize();
	m_pitch = 0.0;
	m_yaw = 0.0;
}

Camera::Camera( float fieldOfView, Vec3 camPos, Vec3 camLook ) {
	m_fov = fieldOfView;
	m_position = camPos;
	m_look = camLook;
	m_up = Vec3( 0.0, 1.0, 0.0 );
	m_right = m_up.cross( m_look );
	m_right.normalize();
	m_pitch = 0.0;
	m_yaw = 0.0;
}

Mat4 Camera::viewMatrix() {
	Mat4 mat = Mat4();
	mat.LookAt( m_position, m_position + m_look, m_up );
	return mat;
}

Mat4 Camera::projectionMatrix( float aspect ) {
	Mat4 mat = Mat4();
	mat.Perspective( to_radians( m_fov ), aspect, 0.001f, 100.0f );
	return mat;
}

Mat4 Camera::projectionMatrix( float aspect, float near, float far ) {
	Mat4 mat = Mat4();
	mat.Perspective( to_radians( m_fov ), aspect, near, far );
	return mat;
}

void Camera::FPLookOffset( float offset_x, float offset_y, float sensitivity ) {
	offset_x *= sensitivity;
	offset_y *= sensitivity;

	m_pitch -= offset_y;
	m_yaw += offset_x;

	//constrain the pitch so the screen doesn't get flipped
    if ( m_pitch > 89.0f ) {
		m_pitch = 89.0f;
	} else if ( m_pitch < -89.0f ) {
		m_pitch = -89.0f;
	}

	Vec3 front;
	front.x = cos( to_radians( m_pitch ) ) * cos( to_radians( m_yaw ) );
	front.y = sin( to_radians( m_pitch ) );
	front.z = cos( to_radians( m_pitch ) ) * sin( to_radians( m_yaw ) );

	m_look = front.normal();
	Vec3 y_axis = Vec3( 0.0f, 1.0f, 0.0f );
	m_right = y_axis.cross( m_look );
	m_right.normalize();
	m_up = m_look.cross( m_right );
}
