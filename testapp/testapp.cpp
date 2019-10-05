#include <EnginePch.h>
#include <EngineCore.h>
#include <App.h>
#include <AppEntry.h>
#include <Engine.h>
#include <Render.h>
#include <Input.h>
#include <SceneManager.h>
#include <Scene.h>
#include <ResourceManager.h>
#include <SceneNode.h>
#include <Camera.h>
#include <MeshInstance.h>
#include <Gui.h>
#include <Control.h>
#include "FpsCamera.h"

class Game : public App, public Control
{
public:
	Game()
	{
		Logger::SetInstance(new ConsoleLogger);

		engine = new Engine;
		app::render->SetShadersDir("shaders");
	}

	~Game()
	{
		delete engine;
	}

	void Start()
	{
		engine->Start(this);
	}

	bool OnInit() override
	{
		scene = new Scene;
		camera = new FpsCamera;
		app::scene_mgr->SetActiveScene(scene);
		app::scene_mgr->SetCamera(camera);
		app::res_mgr->AddDir("data");
		scene->clear_color = Color(0xFF0094FF);
		scene->use_fog = true;
		scene->fog_color = Color(0xFF0094FF);
		scene->fog_range = Vec2(7.5f, 15.f);
		scene->use_dir_light = false;
		scene->use_point_light = false;
		app::scene_mgr->use_fog = false;
		app::scene_mgr->use_lighting = true;

		SceneNode* floor = SceneNode::Get();
		floor->pos = Vec3::Zero;
		floor->rot = Vec3::Zero;
		floor->scale = Vec3::One;
		floor->tint = Vec4::One;
		floor->SetMesh(app::res_mgr->Load<Mesh>("floor.qmsh"));
		scene->Add(floor);

		node_box = SceneNode::Get();
		node_box->pos = Vec3::Zero;
		node_box->rot = Vec3::Zero;
		node_box->scale = Vec3::One;
		node_box->tint = Vec4::One;
		node_box->SetMesh(app::res_mgr->Load<Mesh>("skrzynka.qmsh"));
		scene->Add(node_box);

		node_human = SceneNode::Get();
		node_human->pos = Vec3(1, 0, 0);
		node_human->rot = Vec3::Zero;
		node_human->scale = Vec3::One;
		node_human->tint = Vec4::One;
		node_human->SetMesh(new MeshInstance(app::res_mgr->Load<Mesh>("human.qmsh")));
		node_human->mesh_inst->Play("idzie", PLAY_NO_BLEND);
		scene->Add(node_human);

		font = gui->CreateFont("Arial", 14, 600, 512);
		gui->Add(this);

		return true;
	}

	void OnUpdate(float dt)
	{
		if (app::input->Pressed(Key::Escape) || app::input->Shortcut(ShortcutKey::KEY_ALT, Key::F4))
			app::engine->Shutdown();
		node_box->rot.y += 3.f * dt;
		app::scene_mgr->Update(dt);
		camera->Update(dt);

		if(app::input->Pressed(Key::F1))
			app::scene_mgr->use_fog = !app::scene_mgr->use_fog;
		if(app::input->Pressed(Key::F2))
		{
			scene->use_dir_light = !scene->use_dir_light;
			scene->use_point_light = false;
		}
		if(app::input->Pressed(Key::F3))
		{
			scene->use_point_light = !scene->use_point_light;
			scene->use_dir_light = false;
		}

		if(app::input->Shortcut(ShortcutKey::KEY_SHIFT, Key::R))
			app::render->ReloadShaders();

		light_dir += dt;
		scene->light_dir = Vec3(sin(light_dir) * 5, 5, cos(light_dir) * 5).Normalized();
	}

	void Draw(ControlDrawData*) override
	{
		Rect r = { 0,0,500,500 };
		gui->DrawText(font, Format(
			"[F1] Fog - %s\n"
			"[F2] Dir light - %s\n"
			"[F3] Point light - %s\n"
			"Shift+R - reload shaders",
			app::scene_mgr->use_fog ? "ON" : "OFF",
			scene->use_dir_light ? "ON" : "OFF",
			scene->use_point_light ? "ON" : "OFF"),
			0, Color::Black, r);
	}

private:
	Engine* engine;
	Scene* scene;
	SceneNode* node_box;
	SceneNode* node_human;
	FpsCamera* camera;
	Font* font;
	float light_dir;
};

int AppEntry()
{
	Game game;
	game.Start();
	return 0;
}
