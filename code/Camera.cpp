#include "Camera.h"

Camera::Camera() {
	fov = 45.0;
	position = glm::vec3( 0.0, 0.0, 3.0 );
	look = glm::vec3( 0.0, 0.0, -1.0 );
	up = glm::vec3( 0.0, 1.0, 0.0 );
	right = glm::normalize( glm::cross( up, look ) );
	pitch = 0.0;
	yaw = 0.0;
}

Camera::Camera( float fieldOfView, glm::vec3 camPos, glm::vec3 camLook ) {
	fov = fieldOfView;
	position = camPos;
	look = camLook;
	up = glm::vec3( 0.0, 1.0, 0.0 );
	right = glm::normalize( glm::cross( up, look ) );
	pitch = 0.0;
	yaw = 0.0;
}

glm::mat4 Camera::viewMatrix() {
	glm::mat4 mat = glm::lookAt( position, position + look, up );
	return mat;
}

glm::mat4 Camera::projectionMatrix( float aspect ) {
	glm::mat4 mat = glm::perspective( glm::radians( fov ), aspect, 0.001f, 100.0f );
	return mat;
}

glm::mat4 Camera::projectionMatrix( float aspect, float near, float far ) {
	glm::mat4 mat = glm::perspective( glm::radians( fov ), aspect, near, far );
	return mat;
}

void Camera::FPLookOffset( float offset_x, float offset_y, float sensitivity ) {
	offset_x *= sensitivity;
	offset_y *= sensitivity;

	pitch -= offset_y;
	yaw += offset_x;

	//constrain the pitch so the screen doesn't get flipped
    if ( pitch > 89.0f ) {
		pitch = 89.0f;
	} else if ( pitch < -89.0f ) {
		pitch = -89.0f;
	}

	glm::vec3 front;
	front.x = cos( glm::radians( pitch ) ) * cos( glm::radians( yaw ) );
	front.y = sin( glm::radians( pitch ) );
	front.z = cos( glm::radians( pitch ) ) * sin( glm::radians( yaw ) );

	look = glm::normalize( front );
	right = glm::normalize( glm::cross( glm::vec3( 0.0f, 1.0f, 0.0f ), look ) );
	up = glm::cross( look, right );
}
