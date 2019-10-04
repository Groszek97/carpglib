#include <EnginePch.h>
#include <EngineCore.h>
#include <App.h>
#include <Engine.h>
#include <Render.h>
#include <Input.h>
#include <SceneManager.h>
#include <Scene.h>
#include <ResourceManager.h>
#include <SceneNode.h>
#include <Camera.h>
#include <Mesh.h>
#include <Windows.h>

class Game : public App
{
public:
	bool OnInit() override
	{
		scene = new Scene;
		app::scene_mgr->SetActiveScene(scene);
		app::scene_mgr->SetCamera(new Camera);
		app::res_mgr->AddDir("data");
		scene->clear_color = Color(0x0094FF);
		node = SceneNode::Get();
		node->pos = Vec3::Zero;
		node->rot = Vec3::Zero;
		node->scale = Vec3::One;
		node->tint = Vec4::One;
		node->SetMesh(app::res_mgr->Load<Mesh>("skrzynka.qmsh"));
		scene->Add(node);
		return true;
	}

	void OnUpdate(float dt)
	{
		if (app::input->Pressed(Key::Escape) || app::input->Shortcut(ShortcutKey::KEY_ALT, Key::F4))
			app::engine->Shutdown();
		node->rot.y += 3.f * dt;
	}

private:
	Scene* scene;
	SceneNode* node;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Game game;
	Engine* engine = new Engine;
	app::render->SetShadersDir("shaders");
	engine->Start(&game);
	return 0;
}
