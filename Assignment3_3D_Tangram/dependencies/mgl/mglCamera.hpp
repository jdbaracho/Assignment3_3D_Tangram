////////////////////////////////////////////////////////////////////////////////
//
// Camera Abstraction Class
//
// Copyright (c)2023 by Carlos Martinho
// 
// modified by João Baracho & Henrique Martins
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MGL_CAMERA_HPP
#define MGL_CAMERA_HPP

#include <GL/glew.h>

#include <glm/glm.hpp>

namespace mgl {

	class Camera;

	///////////////////////////////////////////////////////////////////////// Camera

	class Camera {
	private:
		GLuint UboId;
		GLuint BindingPoint;
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

	public:
		explicit Camera(GLuint bindingpoint);
		virtual ~Camera();
		void activate();

		glm::mat4 getViewMatrix();
		void setViewMatrix(const glm::mat4& viewmatrix);
		glm::mat4 getProjectionMatrix();
		void setProjectionMatrix(const glm::mat4& projectionmatrix);

		float yaw = 0.0f;
		float pitch = 0.0f;
		void yawCamera(float angle);
		void pitchCamera(float angle);
		void getEulerAngles(float& yaw, float& pitch);
	};

	////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl

#endif /* MGL_CAMERA_HPP */
