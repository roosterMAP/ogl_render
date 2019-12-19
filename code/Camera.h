#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

/*
==============================
Camera
==============================
*/
class Camera {
	public:
		Camera();
		Camera( float fieldOfView, glm::vec3 camPos, glm::vec3 camLook );
		~Camera() {};

		glm::mat4 viewMatrix();
		glm::mat4 projectionMatrix( float aspect );
		glm::mat4 projectionMatrix( float aspect, float near, float far );
		void FPLookOffset( float offset_x, float offset_y, float sensitivity );

		float fov;
		glm::vec3 position;
		glm::vec3 right, look, up;

	private:
		float pitch, yaw;
};
