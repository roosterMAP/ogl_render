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
		void FPLookOffset( float offset_x, float offset_y, float sensitivity );

		Vec3 GetLook() const { return m_look; }

		Vec3 m_position;
		Vec3 m_right, m_look;
		float m_fov;
		float m_near, m_far;

	private:
		float m_pitch, m_yaw;		
		Vec3 m_up;		
};

#endif