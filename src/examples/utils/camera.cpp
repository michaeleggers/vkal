#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

#include "camera.h"

void Camera::MoveForward(float distance)
{
	glm::vec3 forward = glm::normalize(m_Center - m_Pos);
	m_Pos += distance * forward;
	m_Center = m_Pos + forward;
}

void Camera::MoveSide(float distance)
{
	glm::vec3 forward = glm::normalize(m_Center - m_Pos);
	glm::vec3 side = glm::normalize(glm::cross(m_Up, forward));
	m_Pos += distance * side;
	m_Center = m_Pos + forward;
}
void Camera::MoveUp(float distance)
{
	glm::vec3 forward = glm::normalize(m_Center - m_Pos);
	glm::vec3 side = m_Up;
	m_Pos += distance * side;
	m_Center = m_Pos + forward;
}

void Camera::RotateAroundUp(float angle)
{
	glm::quat q = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));

	m_Forward = glm::rotate(q, m_Forward);
	m_Up = glm::rotate(q, m_Up);
	m_Center = m_Pos + m_Forward;

	m_Orientation = q * m_Orientation;
}

void Camera::RotateAroundSide(float angle)
{
	glm::vec3 side = glm::normalize(glm::cross(m_Up, m_Forward));
	glm::quat q = glm::angleAxis(angle, side);
	m_Up = glm::rotate(q, m_Up);
	m_Forward = glm::rotate(q, m_Forward);
	m_Center = m_Pos + m_Forward;

	m_Orientation = q * m_Orientation;
}

void Camera::ResetOrientation(void)
{
	m_Up = glm::rotate(m_InitOrientation, glm::vec3(0, 1, 0)); // in our (right-handed) coordinate system, up is +z and forward is +y
	m_Forward = glm::rotate(m_InitOrientation, glm::vec3(0, 0, -1));
	m_Pos = m_InitPos;
	m_Center = m_Pos + m_Forward;
}

glm::mat4 Camera::ViewMat()
{
	return glm::lookAt(m_Pos, m_Center, m_Up);
}
