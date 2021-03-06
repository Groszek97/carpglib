#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct TabControl : public Control
	{
		AreaLayout background;
		AreaLayout line;
		AreaLayout button;
		AreaLayout button_hover;
		AreaLayout button_down;
		AreaLayout close;
		AreaLayout close_hover;
		AreaLayout button_prev;
		AreaLayout button_prev_hover;
		AreaLayout button_next;
		AreaLayout button_next_hover;
		Font* font;
		Int2 padding;
		Int2 padding_active;
		Color font_color;
		Color font_color_hover;
		Color font_color_down;
	};
}

//-----------------------------------------------------------------------------
class TabControl : public Control, public LayoutControl<layout::TabControl>
{
public:
	struct Tab
	{
		friend class TabControl;
	private:
		enum Mode
		{
			Up,
			Hover,
			Down
		};

		TabControl* parent;
		string text, id;
		Panel* panel;
		Mode mode;
		Int2 size;
		Box2d rect, close_rect;
		bool close_hover, have_changes;

	public:
		void Close() { parent->Close(this); }
		bool GetHaveChanges() const { return have_changes; }
		const string& GetId() const { return id; }
		TabControl* GetTabControl() const { return parent; }
		const string& GetText() const { return text; }
		bool IsSelected() const { return mode == Mode::Down; }
		void Select() { parent->Select(this); }
		void SetHaveChanges(bool _have_changes) { have_changes = _have_changes; }
	};

	typedef delegate<bool(int, int)> Handler;

	enum Action
	{
		A_BEFORE_CHANGE,
		A_CHANGED,
		A_BEFORE_CLOSE
	};

	TabControl(bool own_panels = true);
	~TabControl();

	void Dock(Control* c) override;
	void Draw(ControlDrawData* cdd) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Tab* AddTab(cstring id, cstring text, Panel* panel, bool select = true);
	void Clear();
	void Close(Tab* tab);
	Tab* Find(cstring id);
	Int2 GetAreaPos() const;
	Int2 GetAreaSize() const;
	Tab* GetCurrentTab() const { return selected; }
	Handler GetHandler() { return handler; }
	void Select(Tab* tab, bool scroll_to = true);
	void SetHandler(Handler _handler) { handler = _handler; }
	void ScrollTo(Tab* tab);

private:
	void Update(bool move, bool resize);
	void CalculateRect();
	void CalculateRect(Tab& tab, int offset);
	bool SelectInternal(Tab* tab);
	void CalculateTabOffsetMax();

	vector<Tab*> tabs;
	Tab* selected;
	Tab* hover;
	Box2d line;
	Handler handler;
	int height, total_width, tab_offset, tab_offset_max, allowed_size;
	int arrow_hover; // -1-prev, 0-none, 1-next
	bool own_panels;
};
