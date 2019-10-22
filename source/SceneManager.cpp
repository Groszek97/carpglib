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

	const bool use_fog = (fog_enabled && scene->use_fog);
	if(use_fog)
		shader->SetFog(scene->fog_color, scene->fog_range);

	bool use_dir_light, use_point_light;
	if(lighting_enabled && (scene->use_dir_light || scene->use_point_light))
	{
		shader->SetAmbientColor(scene->ambient_color);
		if(scene->use_dir_light)
		{
			shader->SetDirectionLight(scene->light_color, scene->light_dir);
			use_dir_light = true;
			use_point_light = false;
		}
		else
		{
			use_dir_light = false;
			use_point_light = true;
		}
	}
	else
	{
		shader->SetAmbientColor(Color::White);
		use_dir_light = false;
		use_point_light = false;
	}

	shader->Prepare(*camera);

	visible_nodes.clear();
	scene->ListNodes(*camera, visible_nodes, use_point_light);
	ProcessNodes();

	for(SceneNodeGroup& group : groups)
	{
		uint id = shader->GetShaderId(IsSet(group.flags, SceneNode::HAVE_WEIGHT),
			IsSet(group.flags, SceneNode::HAVE_BINORMALS),
			IsSet(group.flags, SceneNode::ANIMATED), use_fog, false, false, use_point_light, use_dir_light);
		shader->SetShader(id);

		for(auto it = visible_nodes.begin() + group.start, end = visible_nodes.begin() + group.end + 1; it != end; ++it)
			shader->Draw(*it);
	}
}

//=================================================================================================
void SceneManager::ProcessNodes()
{
	groups.clear();

	if(visible_nodes.empty())
		return;

	int flag_filter = SceneNode::HAVE_WEIGHT | SceneNode::HAVE_BINORMALS | SceneNode::ANIMATED /*| SceneNode::TRANSPARENT*/;
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

	int prev_flags = -1, index = 0;
	for(SceneNode* node : visible_nodes)
	{
		if(node->tmp_flags != prev_flags)
		{
			if(!groups.empty())
				groups.back().end = index - 1;
			groups.push_back({ node->tmp_flags, index, 0 });
			prev_flags = node->tmp_flags;
		}
		++index;
	}
	groups.back().end = index - 1;
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
