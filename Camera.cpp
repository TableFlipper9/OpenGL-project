#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        //TODO
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));

        this->yaw = -90.0f;
        this->pitch = 0.0f;
        this->fov = 45.0f;
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO
        //if (this->cameraTarget == NULL)
        //    this->cameraTarget = cameraPosition + cameraFrontDirection;

        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, this->cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD)
            cameraPosition += speed * cameraFrontDirection;
        if (direction == MOVE_BACKWARD)
            cameraPosition -= speed * cameraFrontDirection;
        if (direction == MOVE_RIGHT)
            cameraPosition += speed * cameraRightDirection;
        if (direction == MOVE_LEFT)
            cameraPosition -= speed * cameraRightDirection;
        if (direction == MOVE_UP)
            cameraPosition += speed * cameraUpDirection;
        if (direction == MOVE_DOWN)
            cameraPosition -= speed * cameraUpDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitchOffset, float yawOffset) {
        yaw += yawOffset;
        pitch += pitchOffset;

        // prevent screen flip
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    void Camera::zoom(float offset) {
        fov -= offset;
        if (fov < 20.0f) fov = 20.0f;
        if (fov > 80.0f) fov = 80.0f;
    }

}
