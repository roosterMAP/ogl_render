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

		void UpdateView() { m_view.LookAt( m_position, m_position + m_look, m_up ); }
		void UpdateProjection( float aspect ) { m_projection.Perspective( to_radians( m_fov ), aspect, m_near, m_far ); }
		void FPLookOffset( float offset_x, float offset_y, float sensitivity );

		Vec3 GetLook() const { return m_look; }

		const Mat4 & GetView() const { return m_view; }
		const Mat4 & GetProjection() const { return m_projection; }
		const Vec3 & GetPosition() const  { return m_position; }

		Vec3 m_position;
		Vec3 m_right, m_look;
		float m_fov;
		float m_near, m_far;		

	private:
		Mat4 m_view, m_projection;
		float m_pitch, m_yaw;		
		Vec3 m_up;		
};

#endif