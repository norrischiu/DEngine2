#pragma once

#include <DECore/Math/simdmath.h>
#include <DECore/Input/Keyboard.h>
#include <DECore/Input/Mouse.h>
#include <DERendering/Imgui/imgui.h>

namespace DE
{

class Camera
{
public:
	void Init(const Vector3 &vPos, const Vector3 &vLookAt, const Vector3 &vUp, const float fFov, const float fRatio, const float fZNear, const float fZFar)
	{
		m_mPerspective = Matrix4::PerspectiveProjection(fFov, fRatio, fZNear, fZFar);

		m_vTranslation = vPos;
		if (Cross(Vector3::UnitZ, vLookAt - vPos).Dot(Vector3::UnitX) > 0)
		{
			m_vRotation.SetX(Vector3::AngleBetween(Vector3::UnitZ, vLookAt - vPos));
		}
		else 
		{
			m_vRotation.SetX(-Vector3::AngleBetween(Vector3::UnitZ, vLookAt - vPos));
		}

		m_fZNear = fZNear;
		m_fZFar = fZFar;
	}

	void ParseInput(float dt)
	{
		// mouse
		Vector3 rotation;
		ImGuiIO &io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			if (Mouse::m_currState.Buttons[1])
			{
				auto deltaX = Mouse::m_currState.cursorPos.x - Mouse::m_lastState.cursorPos.x;
				rotation.SetY(deltaX * yawSpeed * dt);

				auto deltaY = Mouse::m_currState.cursorPos.y - Mouse::m_lastState.cursorPos.y;
				rotation.SetX(deltaY * pitchSpeed * dt);
			}
		}

		m_vRotation.Add(rotation);

		m_mLocalTransform = Matrix4::RotationX(m_vRotation.GetX()) * Matrix4::RotationY(m_vRotation.GetY());
		Vector3 forward = m_mLocalTransform.GetForward();
		Vector3 right = m_mLocalTransform.GetRight();

		// keyboard
		Vector3 translation = {};
		for (int key = 0; key < Keyboard::KEY_NUM; ++key)
		{
			if (Keyboard::m_currState.Keys[key])
			{
				if (key == VK_W)
				{
					m_vTranslation += forward * dt;
				}
				if (key == VK_S)
				{
					m_vTranslation -= forward * dt;
				}
				if (key == VK_A)
				{
					m_vTranslation -= right * dt;
				}
				if (key == VK_D)
				{
					m_vTranslation += right * dt;
				}
			}
		};

		m_mLocalTransform.SetPosition(m_vTranslation);

		m_mView = m_mLocalTransform.Inverse();
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

	Vector3 GetPosition() const
	{
		return m_mLocalTransform.GetPosition();
	}

	float GetZNear() const
	{
		return m_fZNear;
	}

	float GetZFar() const
	{
		return m_fZFar;
	}


public:
	float yawSpeed = 0.25f;
	float pitchSpeed = 0.25f;
	float translateSpeed = 1.0f;

private:
	Vector3 m_vTranslation;
	Vector3 m_vRotation;

	Matrix4 m_mLocalTransform;

	Matrix4 m_mPerspective;
	Matrix4 m_mView;

	float m_fZNear;
	float m_fZFar;
};

}; // namespace DE
