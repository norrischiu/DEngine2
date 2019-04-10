#pragma once

#include <DECore/Math/simdmath.h>
#include <DECore/Input/Keyboard.h>

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
		for (int key = 0; key < Keyboard::KEY_NUM; ++key)
		{
			if (Keyboard::m_currState.Keys[key])
			{
				if (key == VK_W)
				{
					Vector3 vForward = DE::Vector3::UnitZ;
					m_vPos += (vForward * dt);
					m_vLookAt += (vForward * dt);
				}
				if (key == VK_S)
				{
					Vector3 vBack = DE::Vector3::NegativeUnitZ;
					m_vPos += (vBack * dt);
					m_vLookAt += (vBack * dt);
				}
				if (key == VK_A)
				{
					Vector3 vLeft = DE::Vector3::NegativeUnitX;
					m_vPos += (vLeft * dt);
					m_vLookAt += (vLeft * dt);
				}
				if (key == VK_D)
				{
					Vector3 vRight = DE::Vector3::UnitX;
					m_vPos += (vRight * dt);
					m_vLookAt += (vRight * dt);
				}
			}
		}
		m_mView = Matrix4::LookAtMatrix(m_vPos, m_vLookAt, m_vUp);
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

	Matrix4 GetPV() const
	{
		return m_mPerspective * m_mView;
	}

private:
	Vector3						m_vPos;
	Vector3						m_vLookAt;
	Vector3						m_vUp;
	Matrix4						m_mPerspective;
	Matrix4						m_mView;
};

};
