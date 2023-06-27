#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Camera
{
public:
	Camera(glm::vec3 pos) :
		m_Pos(pos),
		m_InitPos(pos),
		m_Center(glm::vec3(0)),
		m_Up(glm::vec3(0, 1, 0))
	{
		m_Forward = glm::normalize(m_Center - m_Pos);
		glm::vec3 side = glm::normalize(glm::cross(m_Up, m_Forward));
		glm::quat qUp = glm::angleAxis(0.0f, m_Up);
		glm::quat qSide = glm::angleAxis(0.0f, side);
		m_Orientation = qUp * qSide;
		// TODO: This only works if pos is eg (0, -10, 0) so the camera is axis aligned!
		//       Calculate world angles later to get correct init orientation quaternion.
		m_InitOrientation = m_Orientation;
	}

	void		MoveForward(float distance);
	void		MoveSide(float distance);
	void		MoveUp(float distance);
	void		RotateAroundUp(float angle);
	void		RotateAroundSide(float angle);
	void        ResetOrientation(void);
	glm::mat4	ViewMat();

	glm::vec3 m_Pos;
	glm::vec3 m_InitPos;
	glm::vec3 m_Center;
	glm::vec3 m_Up;
	glm::vec3 m_Forward;
	glm::quat m_Orientation;
	glm::quat m_InitOrientation;
};


#endif