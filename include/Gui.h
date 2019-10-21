#pragma once

//-----------------------------------------------------------------------------
#include "Font.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
// Gui events (in comment is new gui meaning)
enum GuiEvent
{
	GuiEvent_GainFocus, // control get focus (old)
	GuiEvent_LostFocus, // control lost focus (old)
	GuiEvent_Moved, // control is moved
	GuiEvent_Resize, // control is resized
	GuiEvent_Show, // control is shown
	GuiEvent_WindowResize, // game window size change, only send to parent controls
	GuiEvent_Close, // window is closed (old)
	GuiEvent_Initialize, // send at control initialization
	GuiEvent_Hide, // control is hidden
	GuiEvent_LostMouseFocus, // control lost mouse focus
	GuiEvent_Custom
};

//-----------------------------------------------------------------------------
struct Hitbox
{
	Rect rect;
	int index, index2;
};

//-----------------------------------------------------------------------------
enum class HitboxOpen
{
	No,
	Yes,
	Group
};

//-----------------------------------------------------------------------------
struct HitboxContext
{
	vector<Hitbox>* hitbox;
	int counter, group_index, group_index2;
	HitboxOpen open;
	Rect region;
};

//-----------------------------------------------------------------------------
enum CursorMode
{
	CURSOR_NORMAL,
	CURSOR_HOVER,
	CURSOR_TEXT,
	CURSOR_MAX
};

//-----------------------------------------------------------------------------
enum GUI_DialogType
{
	DIALOG_OK,
	DIALOG_YESNO,
	DIALOG_CUSTOM
};

//-----------------------------------------------------------------------------
typedef delegate<void(int)> DialogEvent;

//-----------------------------------------------------------------------------
enum DialogOrder
{
	ORDER_NORMAL,
	ORDER_TOP,
	ORDER_TOPMOST
};

//-----------------------------------------------------------------------------
struct TextLine
{
	uint begin, end;
	int width;

	TextLine(uint begin, uint end, int width) : begin(begin), end(end), width(width)
	{
	}
};

//-----------------------------------------------------------------------------
struct DialogInfo
{
	DialogInfo() : custom_names(nullptr), img(nullptr), have_tick(false), ticked(false), auto_wrap(false), type(DIALOG_OK), parent(nullptr), order(ORDER_TOP), pause(true)
	{
	}

	string name, text;
	GUI_DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	cstring* custom_names, tick_text;
	Texture* img;
	bool pause, have_tick, ticked, auto_wrap;
};

//-----------------------------------------------------------------------------
struct DrawTextOptions
{
	Font* font;
	cstring str;
	uint flags;
	Color color;
	Rect rect;
	const Rect* clipping;
	vector<Hitbox>* hitboxes;
	int* hitbox_counter;
	const vector<TextLine>* lines;
	Vec2 scale;
	uint lines_start;
	uint lines_end;
	uint str_length;

	DrawTextOptions(Font* font, Cstring str) : font(font), str(str), rect(Rect::Zero), flags(DTF_LEFT), color(Color::Black),
		clipping(nullptr), hitboxes(nullptr), hitbox_counter(nullptr), lines(nullptr), scale(Vec2::One), lines_start(0), lines_end(UINT_MAX),
		str_length(strlen(str))
	{
	}
};

//-----------------------------------------------------------------------------
class OnCharHandler
{
public:
	virtual void OnChar(char c) {}
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct Gui;
}

//-----------------------------------------------------------------------------
// GUI
class Gui
{
public:
	Gui();
	~Gui();
	void Init();
	void SetText(cstring ok, cstring yes, cstring no, cstring cancel);
	void Draw();
	void Update(float dt);
	void OnResize();
	void OnChar(char c);
	void Add(Control* ctrl);
	bool AddFont(cstring filename);
	Font* GetFont(cstring name, int size, int weight = 4, int outline = 0);
	void AddOnCharHandler(OnCharHandler* h) { on_char.push_back(h); }
	void RemoveOnCharHandler(OnCharHandler* h) { RemoveElement(on_char, h); }

	bool AnythingVisible() const;
	bool MouseMoved() const { return cursor_pos != prev_cursor_pos; }
	bool NeedCursor();
	cstring GetClipboard();
	Layout* GetLayout() const { return master_layout; }
	Overlay* GetOverlay() const { return overlay; }
	void SetClipboard(cstring text);
	void SetLayout(Layout* layout);
	void SetOutlineColor(Color color) { outline_color = color; }
	void SetOverlay(Overlay* overlay) { this->overlay = overlay; }
	bool To2dPoint(const Vec3& pos, Int2& pt);
	Rect GetSpriteRect(Texture* t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr);
	static bool Intersect(vector<Hitbox>& hitboxes, const Int2& pt, int* index, int* index2 = nullptr);

	// dialogs
	bool HaveTopDialog(cstring name) const;
	bool HavePauseDialog() const;
	bool HaveDialog() const;
	bool HaveDialog(cstring name);
	bool HaveDialog(DialogBox* dialog);
	DialogBox* GetDialog(cstring name);
	DialogBox* ShowDialog(const DialogInfo& info);
	void ShowDialog(DialogBox* dialog);
	void SimpleDialog(cstring text, Control* parent, cstring name = "simple");
	void CloseDialogs();
	bool CloseDialog(DialogBox* d);
	void CloseDialogInternal(DialogBox* d);

	// drawing
	void DrawArea(Color color, const Int2& pos, const Int2& size, const Box2d* clip_rect = nullptr);
	void DrawArea(Color color, const Rect& rect, const Box2d* clip_rect = nullptr)
	{
		DrawArea(color, rect.LeftTop(), rect.Size(), clip_rect);
	}
	void DrawArea(const Box2d& rect, const AreaLayout& area_layout, const Box2d* clip_rect = nullptr, Color* tint = nullptr);
	bool DrawText(Font* font, Cstring str, uint flags, Color color, const Rect& rect, const Rect* clipping = nullptr,
		vector<Hitbox>* hitboxes = nullptr, int* hitbox_counter = nullptr, const vector<TextLine>* lines = nullptr);
	bool DrawText2(DrawTextOptions& options);
	bool DrawText3D(Font* font, Cstring text, uint flags, Color color, const Vec3& pos, Rect* text_rect = nullptr);
	void DrawItem(Texture* t, const Int2& item_pos, const Int2& item_size, Color color, int corner = 16, int size = 64, const Box2d* clip_rect = nullptr);
	void DrawSprite(Texture* t, const Int2& pos, Color color = Color::White, const Rect* clipping = nullptr);
	void DrawSpriteFull(Texture* t, Color color);
	void DrawSpriteRect(Texture* t, const Rect& rect, Color color = Color::White);
	void DrawSpriteRectPart(Texture* t, const Rect& rect, const Rect& part, Color color = Color::White);
	void DrawSpriteTransform(Texture* t, const Matrix& mat, Color color = Color::White);
	void DrawSprite2(Texture* t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr, Color color = Color::White);
	void DrawSpriteTransformPart(Texture* t, const Matrix& mat, const Rect& part, Color color = Color::White);
	void UseGrayscale(bool grayscale);

	Matrix mViewProj;
	Int2 cursor_pos, prev_cursor_pos, wnd_size;
	CursorMode cursor_mode;
	cstring txOk, txYes, txNo, txCancel;
	Control* focused_ctrl;
	float mouse_wheel;

private:
	static constexpr uint MAX_QUADS = 256;

	void DrawTextLine(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& def_color, Vec4& color, int x, int y, const Rect* clipping,
		HitboxContext* hc, bool parse_special, const Vec2& scale);
	void DrawTextOutline(Font* font, cstring text, uint line_begin, uint line_end, int x, int y, const Rect* clipping, bool parse_special, const Vec2& scale);
	int Clip(int x, int y, int w, int h, const Rect* clipping);
	void Lock(TEX tex, uint count);
	void Flush();
	void SkipTextLine(cstring text, uint line_begin, uint line_end, HitboxContext* hc);
	void AddRect(const Vec2& left_top, const Vec2& right_bottom, const Vec4& color);

	GuiShader* shader;
	FontLoader* font_loader;
	TEX current_tex;
	vector<DialogBox*> created_dialogs;
	Container* layer, *dialog_layer;
	VParticle verts[6 * MAX_QUADS];
	VParticle* v;
	uint in_buffer;
	HitboxContext tmpHitboxContext;
	vector<OnCharHandler*> on_char;
	Color outline_color;
	Vec4 current_outline_color;
	bool grayscale, use_outline;
	Layout* master_layout;
	layout::Gui* layout;
	Overlay* overlay;
};
