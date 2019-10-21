#pragma once

#include "Core.h"
#include "FileFormat.h"

// Engine types
struct Camera;
struct FpsCamera;
struct MeshInstance;
struct ParticleEmitter;
struct Scene;
struct SceneNode;
struct TaskData;
struct Terrain;
struct TrailParticleEmitter;
class App;
class CustomCollisionWorld;
class DebugDrawer;
class Engine;
class FontLoader;
class GrassShader;
class Gui;
class GuiShader;
class Input;
class Render;
class ResourceManager;
class SceneManager;
class SoundManager;
class SuperShader;
class TerrainShader;

// Resource types
struct Font;
struct Mesh;
struct Music;
struct Pak;
struct Resource;
struct Sound;
struct Texture;
struct VertexData;
typedef Font* FontPtr;
typedef Mesh* MeshPtr;
typedef Music* MusicPtr;
typedef Sound* SoundPtr;
typedef Texture* TexturePtr;
typedef VertexData* VertexDataPtr;

// Gui types
struct AreaLayout;
struct Notification;
class Button;
class CheckBox;
class Container;
class Control;
class DialogBox;
class DrawBox;
class GuiDialog;
class GuiElement;
class Label;
class Layout;
class ListBox;
class MenuBar;
class MenuList;
class MenuStrip;
class Notifications;
class Overlay;
class Panel;
class PickItemDialog;
class TextBox;
class TreeView;

// Windows types
struct HWND__;
typedef HWND__* HWND;

// DirectX types
struct _D3D_SHADER_MACRO;
struct D3D11_INPUT_ELEMENT_DESC;
struct ID3D10Blob;
struct ID3D11BlendState;
struct ID3D11Buffer;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11PixelShader;
struct ID3D11RenderTargetView;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct ID3D11VertexShader;
struct IDXGIAdapter;
struct IDXGIFactory;
struct IDXGISwapChain;
typedef _D3D_SHADER_MACRO D3D_SHADER_MACRO;
typedef ID3D10Blob ID3DBlob;
typedef ID3D11ShaderResourceView* TEX;

// FMod types
namespace FMOD
{
	class Channel;
	class ChannelGroup;
	class Sound;
	class System;
}
typedef FMOD::Sound* SOUND;

// Bullet physics types
class btBvhTriangleMeshShape;
class btCollisionObject;
class btCollisionShape;
class btHeightfieldTerrainShape;
class btTriangleIndexVertexArray;

// Globals
namespace app
{
	extern App* app;
	extern Engine* engine;
	extern Gui* gui;
	extern Input* input;
	extern Render* render;
	extern ResourceManager* res_mgr;
	extern SceneManager* scene_mgr;
	extern SoundManager* sound_mgr;
}

// Misc functions
void RegisterCrashHandler(cstring title, cstring version, cstring url, cstring log_file, int minidump_level);
