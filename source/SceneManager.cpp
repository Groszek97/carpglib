#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "MeshInstance.h"
#include "SuperShader.h"

SceneManager* app::scene_mgr;

//=================================================================================================
SceneManager::SceneManager() : scene(nullptr), camera(nullptr), shader(nullptr), fog_enabled(true), lighting_enabled(true)
{
}

//=================================================================================================
SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	DeleteElements(cameras);
	delete shader;
}

//=================================================================================================
void SceneManager::Init()
{
	shader = new SuperShader;
}

//=================================================================================================
void SceneManager::Draw()
{
	if(!scene || !camera)
		return;

	shader->Prepare(*camera);

	visible_nodes.clear();
	scene->ListNodes(*camera, visible_nodes);
	ProcessNodes();

	const bool use_fog = (fog_enabled && scene->use_fog);
	if(use_fog)
		shader->SetFog(scene->fog_color, scene->fog_range);

	if(lighting_enabled)
		shader->SetAmbientColor(scene->ambient_color);
	else
		shader->SetAmbientColor(Color::White);

	for(SceneNodeGroup& group : groups)
	{
		uint id = shader->GetShaderId(IsSet(group.flags, SceneNode::HAVE_WEIGHT),
			IsSet(group.flags, SceneNode::HAVE_BINORMALS),
			IsSet(group.flags, SceneNode::ANIMATED), use_fog, false, false, false, false);
		shader->SetShader(id);

		for(auto it = visible_nodes.begin() + group.start, end = visible_nodes.begin() + group.end + 1; it != end; ++it)
			shader->Draw(*it);
	}

	for(SceneNode* node : visible_nodes)
	{
		uint id = shader->GetShaderId(IsSet(node->mesh->head.flags, Mesh::F_ANIMATED), IsSet(node->mesh->head.flags, Mesh::F_TANGENTS),
			node->mesh_inst != nullptr, use_fog, false, false, false, false);
		shader->SetShader(id);
		shader->Draw(node);
	}
}

//=================================================================================================
void SceneManager::ProcessNodes()
{
	groups.clear();

	if(visible_nodes.empty())
		return;

	int flag_filter = SceneNode::ANIMATED | SceneNode::HAVE_BINORMALS /*| SceneNode::TRANSPARENT*/;
	/*if(use_normal_map)
		flag_filter |= SceneNode::NORMAL_MAP;
	if(use_specular_map)
		flag_filter |= SceneNode::SPECULAR_MAP;*/

	for(SceneNode* node : visible_nodes)
	{
		node->tmp_flags = node->flags & flag_filter;
		/*if(IsSet(node->tmp_flags, SceneNode::TRANSPARENT))
		{
			node->dist = Vec3::DistanceSquared(node->pos, camera->from);
			transparent.push_back(node);
		}
		else
			non_transparent.push_back(node);*/
	}

	// sort non transparent nodes
	std::sort(visible_nodes.begin(), visible_nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
		{
			if(node1->tmp_flags != node2->tmp_flags)
				return node1->tmp_flags > node2->tmp_flags;
			else
				return node1->mesh > node2->mesh;
		});
}

//=================================================================================================
void SceneManager::Update(float dt)
{
	if(scene)
		scene->Update(dt);
}

//=================================================================================================
Vec4 SceneManager::GetClearColor() const
{
	return (Vec4)(scene ? scene->clear_color : Color::Black);
}
