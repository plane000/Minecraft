#ifndef MINECRAFT_RENDERER_CAMERA_H_
#define MINECRAFT_RENDERER_CAMERA_H_

#include "../common.hpp"

class Camera {
public:
	Camera(int w, int h);

	void UpdateView();

	glm::mat4 GetViewMatrix();
	glm::mat4 GetProjectionMatrix();
	void UpdateProjection(int width, int height);

	glm::vec3 GetPos();

	void HandleMouse(SDL_Event e);
	void MoveCamera(Uint8* state);
	void MouseMoved(glm::vec2 mouseDelta);

	float MouseSensitivity = 0.1f;
	float CameraSpeed = 0.2f;

private:
	float roll, pitch, yaw;
	glm::vec3 eyeVector = {};

	glm::mat4 viewMatrix = {};
	glm::mat4 projMatrix = {};

};

#endif
