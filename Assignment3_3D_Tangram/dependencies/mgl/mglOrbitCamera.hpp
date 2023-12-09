////////////////////////////////////////////////////////////////////////////////
//
// Orbit Camera Class
//
// by João Baracho & Henrique Martins
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MGL_ORBIT_CAMERA_HPP
#define MGL_ORBIT_CAMERA_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "./mglApp.hpp"
#include "./mglCamera.hpp"
#include "./mglConventions.hpp"

namespace mgl {

	class OrbitCamera;

	enum Projection {
		PERSPECTIVE,
		ORTHO
	};

	///////////////////////////////////////////////////////////////////////// Camera

	class OrbitCamera : public mgl::Camera {
	private:
		bool leftClick = false;
		float prevXpos, prevYpos;
		float deltaX = 0.0f, deltaY = 0.0f;
		const float moveStep = 0.01;

		float deltaScroll = 0.0f;
		const float zoomStep = 0.1;
		const float minZoom = 0.0f;
		const float maxZoom = 10.0f;

		// AXIS
		const glm::vec3 XX = glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::vec3 YY = glm::vec3(1.0f, 0.0f, 0.0f);

		float d;
		glm::vec3 T;
		glm::quat q;
		glm::quat qX;
		glm::quat qY;

	public:
		explicit OrbitCamera(GLuint bindingpoint);
		void setViewMatrix(glm::vec3 eye, glm::vec3 center, glm::vec3 up);
		void setOrthoMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
		void setPerspectiveMatrix(float fovy, float aspect, float near, float far);

		void update();
		void cursor(double xpos, double ypos);
		void mouseButton(int button, int action);
		void scroll(double xoffset, double yoffset);
	};

	////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl

#endif /* MGL_ORBIT_CAMERA_HPP */
