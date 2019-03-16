#pragma once

// Engine include
#include <DECore/Math/simdmath.h>

namespace DE
{

class Transform
{

public:

	Transform()
		: m_pParent(nullptr)
	{
		m_mWorldTransform = new Matrix4;
		m_mLocalTransform = new Matrix4;
	}

	// Inherited via Component
	void Update(float deltaTime)
	{
		if (m_pParent)
		{
			*m_mWorldTransform = *m_pParent * *m_mLocalTransform;
		}
	};

	// After attachment, the game object's world transform is governed by local transform and parent only
	void AttachTo(Transform* transform)
	{
		m_pParent = transform->m_mWorldTransform;
	};


	
	Matrix4*			m_pParent; // Pointer to the parent attached
	Matrix4*			m_mWorldTransform; 	// Local to world transform
	Matrix4*			m_mLocalTransform; 	// World to local transform
};

}


