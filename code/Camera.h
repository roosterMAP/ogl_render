#pragma once
#ifndef __CAMERA_H_INCLUDE__
#define __CAMERA_H_INCLUDE__

#include "Vector.h"
#include "Matrix.h"

/*
==============================
Camera
==============================
*/
class Camera {
	public:
		Camera();
		Camera( float fieldOfView, Vec3 camPos, Vec3 camLook );
		~Camera() {};

		Mat4 viewMatrix();
		Mat4 projectionMatrix( float aspect );
		Mat4 projectionMatrix( float aspect, float near, float far );
		void FPLookOffset( float offset_x, float offset_y, float sensitivity );

		Vec3 m_position;
		Vec3 m_right, m_look;
		float m_fov;

	private:
		float m_pitch, m_yaw;		
		Vec3 m_up;
};

#endif