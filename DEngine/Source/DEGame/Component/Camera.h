#pragma once

#include <DECore/Math/simdmath.h>
#include <DECore/Input/Keyboard.h>
#include <DECore/Input/Mouse.h>

namespace DE
{

class Camera
{
public:

	void Init(const Vector3& vPos, const Vector3& vLookAt, const Vector3& vUp, const float fFov, const float fRatio, const float fZNear, const float fZFar)
	{
		m_vPos = vPos;
		m_vLookAt = vLookAt;
		m_vUp = vUp;
		m_mPerspective = Matrix4::PerspectiveProjection(fFov, fRatio, fZNear, fZFar);
		m_mView = Matrix4::LookAtMatrix(vPos, vLookAt, vUp);
	}

	void ParseInput(float dt)
	{
		// keyboard
		Vector3 translation = {};
		for (int key = 0; key < Keyboard::KEY_NUM; ++key)
		{
			if (Keyboard::m_currState.Keys[key])
			{
				if (key == VK_W)
				{
					Vector3 forward = -m_mView.GetForward(); // view matrix is inverted
					translation += forward * dt;
				}
				if (key == VK_S)
				{
					Vector3 back = m_mView.GetForward();
					translation += back * dt;
				}
				if (key == VK_A)
				{
					Vector3 left = m_mView.GetRight();
					translation += left * dt;
				}
				if (key == VK_D)
				{
					Vector3 right = -m_mView.GetRight();
					translation += right * dt;
				}
			}
		};

		// mouse
		float yaw = 0, pitch = 0;
		if (Mouse::m_currState.Buttons[0])
		{	
			auto deltaX = Mouse::m_currState.cursorPos.x - Mouse::m_lastState.cursorPos.x;
			pitch = deltaX * pitchSpeed * dt;
		}		
		if (Mouse::m_currState.Buttons[1])
		{
			auto deltaY = Mouse::m_currState.cursorPos.y - Mouse::m_lastState.cursorPos.y;
			yaw = deltaY * yawSpeed * dt;
		}

		m_mView *= Matrix4::Translation(translation) * Matrix4::RotationX(yaw) * Matrix4::RotationY(pitch);
	}

	// view matrix: world to camera
	Matrix4 GetV() const
	{
		return m_mView;
	}

	// perspective matrix: camera to screen
	Matrix4 GetP() const
	{
		return m_mPerspective;
	}

	Matrix4 GetCameraToScreen() const
	{
		return m_mView * m_mPerspective;
	}

public:
	float yawSpeed = 0.5f; // pitch
	float pitchSpeed = 0.5f; // yaw
	float translateSpeed = 1.0f;

private:
	Vector3 m_vPos;
	Vector3 m_vLookAt;
	Vector3 m_vUp;
	Matrix4 m_mPerspective;
	Matrix4 m_mView;
};

};
