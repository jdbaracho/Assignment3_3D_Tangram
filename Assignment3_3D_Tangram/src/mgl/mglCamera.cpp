////////////////////////////////////////////////////////////////////////////////
//
// Camera Abstraction Class
//
// Copyright (c)2023 by Carlos Martinho
// 
// modified by João Baracho & Henrique Martins
//
////////////////////////////////////////////////////////////////////////////////

#include "./mglCamera.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace mgl {

    ///////////////////////////////////////////////////////////////////////// Camera

    Camera::Camera(GLuint bindingpoint)
        : BindingPoint(bindingpoint), ViewMatrix(glm::mat4(1.0f)), ProjectionMatrix(glm::mat4(1.0f)) {
        glGenBuffers(1, &UboId);
        glBindBuffer(GL_UNIFORM_BUFFER, UboId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, 0, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingpoint, UboId);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    Camera::~Camera() {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glDeleteBuffers(1, &UboId);
    }

    void Camera::activate() {
        glBindBuffer(GL_UNIFORM_BUFFER, UboId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, 0, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, BindingPoint, UboId);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glm::mat4 Camera::getViewMatrix() { return ViewMatrix; }

    void Camera::setViewMatrix(const glm::mat4& viewmatrix) {
        ViewMatrix = viewmatrix;
        glBindBuffer(GL_UNIFORM_BUFFER, UboId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
            glm::value_ptr(ViewMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glm::mat4 Camera::getProjectionMatrix() { return ProjectionMatrix; }

    void Camera::setProjectionMatrix(const glm::mat4& projectionmatrix) {
        ProjectionMatrix = projectionmatrix;
        glBindBuffer(GL_UNIFORM_BUFFER, UboId);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
            glm::value_ptr(ProjectionMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Camera::yawCamera(float angle) {
        yaw += angle;
    }

    void Camera::pitchCamera(float angle) {
        pitch += angle;
    }

    void Camera::getEulerAngles(float& yaw, float& pitch) {
        yaw = this->yaw;
        pitch = this->pitch;
    }

    ////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl
