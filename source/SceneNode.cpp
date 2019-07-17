#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"

Matrix& SceneNode::GetWorldMatrix()
{
	if(changed)
	{
		mat_world = Matrix::Transform(pos, rot, scale);
		changed = false;
	}
	return mat_world;
}
