#include <EnginePch.h>
#include <EngineCore.h>
#include <App.h>
#include <Engine.h>
#include <Render.h>
#include <Windows.h>

class Game : public App
{

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Game game;
	Engine* engine = new Engine;
	app::render->SetShadersDir("shaders");
	engine->Start(&game);
	return 0;
}
