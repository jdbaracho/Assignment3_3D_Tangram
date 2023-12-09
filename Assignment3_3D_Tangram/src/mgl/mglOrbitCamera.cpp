////////////////////////////////////////////////////////////////////////////////
//
// Orbit Camera Class
//
// by João Baracho & Henrique Martins
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include <glm/gtx/quaternion.hpp>

#include "./mglOrbitCamera.hpp"

namespace mgl {



    ///////////////////////////////////////////////////////////////////////// ORBITCAMERA

    OrbitCamera::OrbitCamera(GLuint bindingpoint, char name) : name(name), bindingpoint(bindingpoint), Camera(bindingpoint) {}

    void OrbitCamera::setViewMatrix(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
        d = glm::length(eye - center);
        T = glm::vec3(0, 0, -d);
        q = glm::toQuat(glm::lookAt(eye, center, up));
        Camera::setViewMatrix(glm::translate(T) * glm::toMat4(q));
    }

    void OrbitCamera::setOrthoMatrix(float left, float right, float bottom, float top, float zNear, float zFar) {
        projectionId = 0;
        projections[projectionId] = glm::ortho(left, right, bottom, top, zNear, zFar);
        setProjectionMatrix(projections[projectionId]);
    }

    void OrbitCamera::setPerspectiveMatrix(float fovy, float aspect, float near, float far) {
        projectionId = 1;
        projections[projectionId] = glm::perspective(glm::radians(fovy), aspect, near, far);
        setProjectionMatrix(projections[projectionId]);
    }

    void OrbitCamera::changeProjection() {
        projectionId = (projectionId + 1) % 2;
        setProjectionMatrix(projections[projectionId]);
    }

    void OrbitCamera::update() {

        d += deltaScroll;
        d = d < minZoom ? minZoom : d > maxZoom ? maxZoom : d;
        T = glm::vec3(0, 0, -d);

        qX = glm::angleAxis(deltaX, XX);
        qY = glm::angleAxis(deltaY, YY);
        q = qX * qY * q;

        Camera::setViewMatrix(glm::translate(T) * glm::toMat4(q));
        setProjectionMatrix(getProjectionMatrix());

        deltaScroll = 0.0f;
        deltaX = 0.0f;
        deltaY = 0.0f;
    }

    void OrbitCamera::cursor(double xpos, double ypos) {
        //std::cout << "mouse: " << xpos << " " << ypos << std::endl;
        //std::cout << "camera(" << name << ")" << std::endl;

        if (leftClick) {
            deltaX += (xpos - prevXpos) * moveStep;
            deltaY += (ypos - prevYpos) * moveStep;
            prevXpos = xpos;
            prevYpos = ypos;
        }
    }

    void OrbitCamera::mouseButton(GLFWwindow* win, int button, int action) {
        //std::cout << "button: " << button << " " << action << std::endl;
        //std::cout << "camera(" << name << ")" << std::endl;

        leftClick = button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS;
        if (leftClick) {
            glfwGetCursorPos(win, &prevXpos, &prevYpos);
        }
    }

    void OrbitCamera::scroll(double xoffset, double yoffset) {
        //std::cout << "scroll: " << xoffset << " " << yoffset << std::endl;
        //std::cout << "camera(" << name << ")" << std::endl;

        deltaScroll -= yoffset * zoomStep;
    }

    ////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl
