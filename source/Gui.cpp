#include "EnginePch.h"
#include "EngineCore.h"
#include "Gui.h"
#include "Layout.h"
#include "Container.h"
#include "DialogBox.h"
#include "GuiRect.h"
#include "Engine.h"
#include "Profiler.h"
#include "Input.h"
#include "ResourceManager.h"
#include "GuiShader.h"
#include "FontLoader.h"
#include "WindowsIncludes.h"

Gui* app::gui;

//=================================================================================================
Gui::Gui() : cursor_mode(CURSOR_NORMAL), focused_ctrl(nullptr), master_layout(nullptr), layout(nullptr), overlay(nullptr), grayscale(false),
font_loader(nullptr), shader(nullptr), current_tex(nullptr), in_buffer(0), outline_color(Color::Black)
{
}

//=================================================================================================
Gui::~Gui()
{
	DeleteElements(created_dialogs);
	delete master_layout;
	delete layer;
	delete dialog_layer;
	delete shader;
	delete font_loader;
}

//=================================================================================================
void Gui::Init()
{
	shader = new GuiShader;
	font_loader = new FontLoader;

	Control::input = app::input;
	Control::gui = this;

	wnd_size = app::engine->GetWindowSize();
	cursor_pos = wnd_size / 2;
	v = &verts[0];

	layer = new Container;
	layer->auto_focus = true;
	dialog_layer = new Container;
}

//=================================================================================================
void Gui::SetText(cstring ok, cstring yes, cstring no, cstring cancel)
{
	txOk = ok;
	txYes = yes;
	txNo = no;
	txCancel = cancel;
}

//=================================================================================================
bool Gui::AddFont(cstring filename)
{
	cstring path = Format("data/fonts/%s", filename);
	int result = AddFontResourceExA(path, FR_PRIVATE, nullptr);
	if(result == 0)
	{
		Error("Failed to load font '%s' (%d)!", filename, GetLastError());
		return false;
	}
	else
	{
		Info("Added font resource '%s'.", filename);
		return true;
	}
}

//=================================================================================================
Font* Gui::GetFont(cstring name, int size, int weight, int outline)
{
	assert(name && size > 0 && InRange(weight, 1, 9) && outline >= 0);

	string res_name = Format("%s;%d;%d;%d", name, size, weight, outline);
	Font* existing_font = app::res_mgr->TryGet<Font>(res_name);
	if(existing_font)
		return existing_font;

	Font* font = font_loader->Load(name, size, weight, outline);
	font->type = ResourceType::Font;
	font->state = ResourceState::Loaded;
	font->path = res_name;
	font->filename = font->path.c_str();
	app::res_mgr->AddResource(font);

	return font;
}

//=================================================================================================
// Draw text - rewritten from TFQ
bool Gui::DrawText(Font* font, Cstring str, uint flags, Color color, const Rect& rect, const Rect* clipping, vector<Hitbox>* hitboxes,
	int* hitbox_counter, const vector<TextLine>* lines)
{
	assert(font);

	uint line_begin, line_end, line_index = 0;
	int line_width, width = rect.SizeX();
	cstring text = str;
	uint text_end = strlen(str);
	Vec4 current_color = Color(color);
	Vec4 default_color = current_color;
	const Vec2 scale(1, 1);

	const bool parse_special = IsSet(flags, DTF_PARSE_SPECIAL);
	use_outline = IsSet(flags, DTF_OUTLINE) && font->tex_outline;
	if(use_outline)
	{
		current_outline_color = outline_color;
		current_outline_color.w *= float(color.a) / 255;
	}
	bool bottom_clip = false;

	HitboxContext* hc;
	if(hitboxes)
	{
		hc = &tmpHitboxContext;
		hc->hitbox = hitboxes;
		hc->counter = (hitbox_counter ? *hitbox_counter : 0);
		hc->open = HitboxOpen::No;
	}
	else
		hc = nullptr;

	if(!IsSet(flags, DTF_VCENTER | DTF_BOTTOM))
	{
		int y = rect.Top();

		if(!lines)
		{
			// tekst pionowo po œrodku lub na dole
			while(font->SplitLine(line_begin, line_end, line_width, line_index, text, text_end, flags, width))
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(flags, DTF_CENTER))
					x = rect.Left() + (width - line_width) / 2;
				else if(IsSet(flags, DTF_RIGHT))
					x = rect.Right() - line_width;
				else
					x = rect.Left();

				int clip_result = (clipping ? Clip(x, y, line_width, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(font, text, line_begin, line_end, default_color, current_color, x, y, nullptr, hc, parse_special, scale);
				else if(clip_result == 5)
					DrawTextLine(font, text, line_begin, line_end, default_color, current_color, x, y, clipping, hc, parse_special, scale);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipTextLine(text, line_begin, line_end, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
		else
		{
			for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(flags, DTF_CENTER))
					x = rect.Left() + (width - it->width) / 2;
				else if(IsSet(flags, DTF_RIGHT))
					x = rect.Right() - it->width;
				else
					x = rect.Left();

				int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(font, text, it->begin, it->end, default_color, current_color, x, y, nullptr, hc, parse_special, scale);
				else if(clip_result == 5)
					DrawTextLine(font, text, it->begin, it->end, default_color, current_color, x, y, clipping, hc, parse_special, scale);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipTextLine(text, it->begin, it->end, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
	}
	else
	{
		// tekst u góry
		if(!lines)
		{
			static vector<TextLine> lines_data;
			lines_data.clear();

			// oblicz wszystkie linijki
			while(font->SplitLine(line_begin, line_end, line_width, line_index, text, text_end, flags, width))
				lines_data.push_back(TextLine(line_begin, line_end, line_width));

			lines = &lines_data;
		}

		// pocz¹tkowa pozycja y
		int y;
		if(IsSet(flags, DTF_BOTTOM))
			y = rect.Bottom() - lines->size() * font->height;
		else
			y = rect.Top() + (rect.SizeY() - int(lines->size()) * font->height) / 2;

		for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
		{
			// pocz¹tkowa pozycja x w tej linijce
			int x;
			if(IsSet(flags, DTF_CENTER))
				x = rect.Left() + (width - it->width) / 2;
			else if(IsSet(flags, DTF_RIGHT))
				x = rect.Right() - it->width;
			else
				x = rect.Left();

			int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

			// znaki w tej linijce
			if(clip_result == 0)
				DrawTextLine(font, text, it->begin, it->end, default_color, current_color, x, y, nullptr, hc, parse_special, scale);
			else if(clip_result == 5)
				DrawTextLine(font, text, it->begin, it->end, default_color, current_color, x, y, clipping, hc, parse_special, scale);
			else if(clip_result == 2)
			{
				// tekst jest pod widocznym regionem, przerwij rysowanie
				bottom_clip = true;
				break;
			}
			else if(hitboxes)
			{
				// pomiñ hitbox
				SkipTextLine(text, it->begin, it->end, hc);
			}

			// zmieñ y na kolejn¹ linijkê
			y += font->height;
		}
	}

	if(hitbox_counter)
		*hitbox_counter = hc->counter;

	return !bottom_clip;
}

//=================================================================================================
void Gui::DrawTextLine(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& default_color, Vec4& current_color,
	int x, int y, const Rect* clipping, HitboxContext* hc, bool parse_special, const Vec2& scale)
{
	if(use_outline)
		DrawTextOutline(font, text, line_begin, line_end, x, y, clipping, parse_special, scale);

	Lock(font->tex, 1);

	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = text[i];
		if(c == '$' && parse_special)
		{
			++i;
			assert(i < line_end);
			c = text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < line_end);
				c = text[i];
				switch(c)
				{
				case '-':
					current_color = default_color;
					break;
				case 'r':
					current_color = Vec4(1, 0, 0, default_color.w);
					break;
				case 'g':
					current_color = Vec4(0, 1, 0, default_color.w);
					break;
				case 'b':
					current_color = Vec4(0, 0, 1, default_color.w);
					break;
				case 'y':
					current_color = Vec4(1, 1, 0, default_color.w);
					break;
				case 'w':
					current_color = Vec4(1, 1, 1, default_color.w);
					break;
				case 'k':
					current_color = Vec4(0, 0, 0, default_color.w);
					break;
				case '0':
					current_color = Vec4(0.5f, 1, 0, default_color.w);
					break;
				case '1':
					current_color = Vec4(1, 0.5f, 0, default_color.w);
					break;
				default:
					// nieznany kolor
					assert(0);
					break;
				}

				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < line_end);
				c = text[i];
				if(c == '+')
				{
					// rozpocznij hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::No);
						hc->open = HitboxOpen::Yes;
						hc->region.Left() = INT_MAX;
					}
				}
				else if(c == '-')
				{
					// zamknij hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::Yes);
						hc->open = HitboxOpen::No;
						if(hc->region.Left() != INT_MAX)
						{
							Hitbox& h = Add1(hc->hitbox);
							h.rect = hc->region;
							h.index = hc->counter;
							h.index2 = -1;
						}
						++hc->counter;
					}
				}
				else
				{
					// nieznana opcja hitboxa
					assert(0);
				}

				continue;
			}
			else if(c == 'g')
			{
				// group hitbox
				// $g+123[,123];
				// $g-
				++i;
				assert(i < line_end);
				c = text[i];
				if(c == '+')
				{
					// start group hitbox
					int index, index2;
					++i;
					assert(i < line_end);
					if(font->ParseGroupIndex(text, line_end, i, index, index2) && hc)
					{
						assert(hc->open == HitboxOpen::No);
						hc->open = HitboxOpen::Group;
						hc->region.Left() = INT_MAX;
						hc->group_index = index;
						hc->group_index2 = index2;
					}
				}
				else if(c == '-')
				{
					// close group hitbox
					if(hc)
					{
						assert(hc->open == HitboxOpen::Group);
						hc->open = HitboxOpen::No;
						if(hc->region.Left() != INT_MAX)
						{
							Hitbox& h = Add1(hc->hitbox);
							h.rect = hc->region;
							h.index = hc->group_index;
							h.index2 = hc->group_index2;
						}
					}
				}
				else
				{
					// invalid format
					assert(0);
				}

				continue;
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		Font::Glyph& g = font->glyph[byte(c)];
		Int2 glyph_size = Int2(g.width, font->height) * scale;
		int clip_result = (clipping ? Clip(x, y, glyph_size.x, glyph_size.y, clipping) : 0);

		if(clip_result == 0)
		{
			// dodaj znak do bufora
			v->pos = Vec3(float(x), float(y), 0);
			v->color = current_color;
			v->tex = g.uv.LeftTop();
			++v;

			v->pos = Vec3(float(x + glyph_size.x), float(y), 0);
			v->color = current_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = Vec3(float(x), float(y + glyph_size.y), 0);
			v->color = current_color;
			v->tex = g.uv.LeftBottom();
			++v;

			v->pos = Vec3(float(x + glyph_size.x), float(y), 0);
			v->color = current_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = Vec3(float(x + glyph_size.x), float(y + glyph_size.y), 0);
			v->color = current_color;
			v->tex = g.uv.RightBottom();
			++v;

			v->pos = Vec3(float(x), float(y + glyph_size.y), 0);
			v->color = current_color;
			v->tex = g.uv.LeftBottom();
			++v;

			if(hc && hc->open != HitboxOpen::No)
			{
				Rect r_clip = Rect::Create(Int2(x, y), glyph_size);
				if(hc->region.Left() == INT_MAX)
					hc->region = r_clip;
				else
					hc->region.Resize(r_clip);
			}

			++in_buffer;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			Box2d orig_pos = Box2d::Create(Int2(x, y), glyph_size);
			Box2d clip_pos(float(max(x, clipping->Left())), float(max(y, clipping->Top())),
				float(min(x + glyph_size.x, clipping->Right())), float(min(y + glyph_size.y, clipping->Bottom())));
			Vec2 orig_size = orig_pos.Size();
			Vec2 clip_size = clip_pos.Size();
			Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
			Vec2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			Vec2 uv_size = g.uv.Size();
			Box2d clip_uv(g.uv.v1 + Vec2(shift.x * uv_size.x, shift.y * uv_size.y));
			clip_uv.v2 += Vec2(uv_size.x * s.x, uv_size.y * s.y);

			// dodaj znak do bufora
			v->pos = clip_pos.LeftTop().XY();
			v->color = current_color;
			v->tex = clip_uv.LeftTop();
			++v;

			v->pos = clip_pos.RightTop().XY();
			v->color = current_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = clip_pos.LeftBottom().XY();
			v->color = current_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			v->pos = clip_pos.RightTop().XY();
			v->color = current_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = clip_pos.RightBottom().XY();
			v->color = current_color;
			v->tex = clip_uv.RightBottom();
			++v;

			v->pos = clip_pos.LeftBottom().XY();
			v->color = current_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			if(hc && hc->open != HitboxOpen::No)
			{
				Rect r_clip(clip_pos);
				if(hc->region.Left() == INT_MAX)
					hc->region = r_clip;
				else
					hc->region.Resize(r_clip);
			}

			++in_buffer;
		}
		else if(clip_result == 3)
		{
			// tekst jest ju¿ poza regionem z prawej, mo¿na przerwaæ
			break;
		}

		if(in_buffer == MAX_QUADS)
			Flush();

		x += glyph_size.x;
	}

	// zamknij region
	if(hc && hc->open != HitboxOpen::No && hc->region.Left() != INT_MAX)
	{
		Hitbox& h = Add1(hc->hitbox);
		h.rect = hc->region;
		if(hc->open == HitboxOpen::Yes)
			h.index = hc->counter;
		else
		{
			h.index = hc->group_index;
			h.index2 = hc->group_index2;
		}
		hc->region.Left() = INT_MAX;
	}
}

//=================================================================================================
void Gui::DrawTextOutline(Font* font, cstring text, uint line_begin, uint line_end, int x, int y, const Rect* clipping, bool parse_special, const Vec2& scale)
{
	Lock(font->tex_outline, 1);

	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = text[i];
		if(c == '$' && parse_special)
		{
			++i;
			assert(i < line_end);
			c = text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < line_end);
				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < line_end);
				continue;
			}
			else if(c == 'g')
			{
				++i;
				assert(i < line_end);
				c = text[i];
				if(c == '+')
				{
					++i;
					assert(i < line_end);
					int tmp;
					font->ParseGroupIndex(text, line_end, i, tmp, tmp);
				}
				else if(c == '-')
					continue;
				else
				{
					// invalid group format
					assert(0);
				}
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		const int ow = font->outline;
		Font::Glyph& g = font->glyph[byte(c)];
		Int2 glyph_size = Int2(g.width, font->height) * scale;
		int clip_result = (clipping ? Clip(x, y, glyph_size.x, glyph_size.y, clipping) : 0);

		if(clip_result == 0)
		{
			// dodaj znak do bufora
			v->pos = Vec3(float(x - ow), float(y - ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.LeftTop();
			++v;

			v->pos = Vec3(float(x + glyph_size.x + ow), float(y - ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = Vec3(float(x - ow), float(y + glyph_size.y + ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.LeftBottom();
			++v;

			v->pos = Vec3(float(x + glyph_size.x + ow), float(y - ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.RightTop();
			++v;

			v->pos = Vec3(float(x + glyph_size.x + ow), float(y + glyph_size.y + ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.RightBottom();
			++v;

			v->pos = Vec3(float(x - ow), float(y + glyph_size.y + ow), 0);
			v->color = current_outline_color;
			v->tex = g.uv.LeftBottom();
			++v;

			++in_buffer;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			Box2d orig_pos = Box2d(float(x) - ow, float(y) - ow, float(x + g.width) + ow, float(y + font->height) + ow);
			Box2d clip_pos(float(max(x, clipping->Left())), float(max(y, clipping->Top())),
				float(min(x + glyph_size.x, clipping->Right())), float(min(y + glyph_size.y, clipping->Bottom())));
			Vec2 orig_size = orig_pos.Size();
			Vec2 clip_size = clip_pos.Size();
			Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
			Vec2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			Vec2 uv_size = g.uv.Size();
			Box2d clip_uv(g.uv.v1 + Vec2(shift.x * uv_size.x, shift.y * uv_size.y));
			clip_uv.v2 += Vec2(uv_size.x * s.x, uv_size.y * s.y);

			// dodaj znak do bufora
			v->pos = clip_pos.LeftTop().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.LeftTop();
			++v;

			v->pos = clip_pos.RightTop().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = clip_pos.LeftBottom().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			v->pos = clip_pos.RightTop().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.RightTop();
			++v;

			v->pos = clip_pos.RightBottom().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.RightBottom();
			++v;

			v->pos = clip_pos.LeftBottom().XY();
			v->color = current_outline_color;
			v->tex = clip_uv.LeftBottom();
			++v;

			++in_buffer;
		}

		if(in_buffer == MAX_QUADS)
			Flush();

		x += glyph_size.x;
	}
}

//=================================================================================================
void Gui::Lock(TEX tex, uint count)
{
	if(tex != current_tex)
	{
		if(in_buffer > 0)
			Flush();
		current_tex = tex;
	}
	else if(in_buffer + count > MAX_QUADS)
		Flush();
}

//=================================================================================================
void Gui::Flush()
{
	if(in_buffer == 0)
		return;
	shader->Draw(current_tex, verts, in_buffer);
	in_buffer = 0;
	v = &verts[0];
}

//=================================================================================================
void Gui::Draw()
{
	PROFILER_BLOCK("DrawGui");

	wnd_size = app::engine->GetWindowSize();

	shader->Prepare();
	grayscale = false;

	layer->Draw();
	dialog_layer->Draw();
	if(NeedCursor())
	{
		Int2 pos = cursor_pos;
		if(cursor_mode == CURSOR_TEXT)
			pos -= Int2(3, 8);
		DrawSprite(layout->cursor[cursor_mode], pos);
	}
	Flush();
}

//=================================================================================================
void Gui::Add(Control* ctrl)
{
	layer->Add(ctrl);
}

//=================================================================================================
void Gui::DrawItem(Texture* t, const Int2& item_pos, const Int2& item_size, Color color, int corner, int size, const Box2d* clip_rect)
{
	assert(t && t->IsLoaded());

	GuiRect gui_rect;
	gui_rect.Set(item_pos, item_size);

	bool require_clip = false;
	if(clip_rect)
	{
		int result = gui_rect.RequireClip(*clip_rect);
		if(result == -1)
			return;
		else if(result == 1)
			require_clip = true;
	}

	if(item_size.x < corner || item_size.y < corner)
	{
		if(item_size.x == 0 || item_size.y == 0)
			return;

		Rect r = { item_pos.x, item_pos.y, item_pos.x + item_size.x, item_pos.y + item_size.y };
		assert(!clip_rect);
		DrawSpriteRect(t, r, color);
		return;
	}

	Lock(t->tex, 9);

	Vec4 col = Color(color);

	/*
		0---1----------2---3
		| 1 |     2    | 3 |
		|   |          |   |
		4---5----------6---7
		|   |          |   |
		| 4 |     5    | 6 |
		|   |          |   |
		|   |          |   |
		8---9---------10--11
		|   |          |   |
		| 7 |     8    | 9 |
		12-13---------14--15
	*/

	// top left & bottom right indices for each rectangle
	int ids[9 * 2] = {
		0, 5,
		1, 6,
		2, 7,
		4, 9,
		5, 10,
		6, 11,
		8, 13,
		9, 14,
		10, 15
	};

	Vec2 ipos[16] = {
		Vec2(float(item_pos.x), float(item_pos.y)),
		Vec2(float(item_pos.x + corner), float(item_pos.y)),
		Vec2(float(item_pos.x + item_size.x - corner), float(item_pos.y)),
		Vec2(float(item_pos.x + item_size.x), float(item_pos.y)),

		Vec2(float(item_pos.x), float(item_pos.y + corner)),
		Vec2(float(item_pos.x + corner), float(item_pos.y + corner)),
		Vec2(float(item_pos.x + item_size.x - corner), float(item_pos.y + corner)),
		Vec2(float(item_pos.x + item_size.x), float(item_pos.y + corner)),

		Vec2(float(item_pos.x), float(item_pos.y + item_size.y - corner)),
		Vec2(float(item_pos.x + corner), float(item_pos.y + item_size.y - corner)),
		Vec2(float(item_pos.x + item_size.x - corner), float(item_pos.y + item_size.y - corner)),
		Vec2(float(item_pos.x + item_size.x), float(item_pos.y + item_size.y - corner)),

		Vec2(float(item_pos.x), float(item_pos.y + item_size.y)),
		Vec2(float(item_pos.x + corner), float(item_pos.y + item_size.y)),
		Vec2(float(item_pos.x + item_size.x - corner), float(item_pos.y + item_size.y)),
		Vec2(float(item_pos.x + item_size.x), float(item_pos.y + item_size.y)),
	};

	Vec2 itex[16] = {
		Vec2(0,0),
		Vec2(float(corner) / size,0),
		Vec2(float(size - corner) / size,0),
		Vec2(1,0),

		Vec2(0,float(corner) / size),
		Vec2(float(corner) / size,float(corner) / size),
		Vec2(float(size - corner) / size,float(corner) / size),
		Vec2(1,float(corner) / size),

		Vec2(0,float(size - corner) / size),
		Vec2(float(corner) / size,float(size - corner) / size),
		Vec2(float(size - corner) / size,float(size - corner) / size),
		Vec2(1,float(size - corner) / size),

		Vec2(0,1),
		Vec2(float(corner) / size,1),
		Vec2(float(size - corner) / size,1),
		Vec2(1,1),
	};

	if(require_clip)
	{
		for(int i = 0; i < 9; ++i)
		{
			int index1 = ids[i * 2 + 0];
			int index2 = ids[i * 2 + 1];
			gui_rect.Set(ipos[index1], ipos[index2], itex[index1], itex[index2]);
			if(gui_rect.Clip(*clip_rect))
			{
				gui_rect.Populate(v, col);
				++in_buffer;
			}
		}
	}
	else
	{
		for(int i = 0; i < 9; ++i)
		{
			int index1 = ids[i * 2 + 0];
			int index2 = ids[i * 2 + 1];
			gui_rect.Set(ipos[index1], ipos[index2], itex[index1], itex[index2]);
			gui_rect.Populate(v, col);
		}
		in_buffer += 9;
	}
}

//=================================================================================================
void Gui::Update(float dt, float mouse_speed)
{
	PROFILER_BLOCK("UpdateGui");

	// update cursor
	cursor_mode = CURSOR_NORMAL;
	mouse_wheel = app::input->GetMouseWheel();
	prev_cursor_pos = cursor_pos;
	if(NeedCursor() && mouse_speed > 0)
	{
		cursor_pos += app::input->GetMouseDif() * mouse_speed;
		if(cursor_pos.x < 0)
			cursor_pos.x = 0;
		if(cursor_pos.y < 0)
			cursor_pos.y = 0;
		if(cursor_pos.x >= wnd_size.x)
			cursor_pos.x = wnd_size.x - 1;
		if(cursor_pos.y >= wnd_size.y)
			cursor_pos.y = wnd_size.y - 1;
		app::engine->SetUnlockPoint(cursor_pos);
	}
	else
		app::engine->SetUnlockPoint(wnd_size / 2);

	layer->focus = dialog_layer->Empty();

	if(focused_ctrl)
	{
		if(!focused_ctrl->visible)
			focused_ctrl = nullptr;
		else if(dialog_layer->Empty())
			layer->Update(dt);
		else
		{
			focused_ctrl->LostFocus();
			focused_ctrl = nullptr;
		}
	}

	if(!focused_ctrl)
	{
		dialog_layer->focus = true;
		dialog_layer->Update(dt);
		layer->Update(dt);
	}

	app::engine->SetUnlockPoint(wnd_size / 2);
}

//=================================================================================================
void Gui::DrawSprite(Texture* t, const Int2& pos, Color color, const Rect* clipping)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();

	int clip_result = (clipping ? Clip(pos.x, pos.y, size.x, size.y, clipping) : 0);
	if(clip_result > 0 && clip_result < 5)
		return;

	Lock(t->tex, 1);

	Vec4 col = Color(color);

	if(clip_result == 0)
	{
		v->pos = Vec3(float(pos.x), float(pos.y), 0);
		v->color = col;
		v->tex = Vec2(0, 0);
		++v;

		v->pos = Vec3(float(pos.x + size.x), float(pos.y), 0);
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = Vec3(float(pos.x), float(pos.y + size.y), 0);
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = Vec3(float(pos.x), float(pos.y + size.y), 0);
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = Vec3(float(pos.x + size.x), float(pos.y), 0);
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = Vec3(float(pos.x + size.x), float(pos.y + size.y), 0);
		v->color = col;
		v->tex = Vec2(1, 1);
		++v;

		++in_buffer;
	}
	else
	{
		Box2d orig_pos(float(pos.x), float(pos.y), float(pos.x + size.x), float(pos.y + size.y));
		Box2d clip_pos(float(max(pos.x, clipping->Left())), float(max(pos.y, clipping->Top())),
			float(min(pos.x + (int)size.x, clipping->Right())), float(min(pos.y + (int)size.y, clipping->Bottom())));
		Vec2 orig_size = orig_pos.Size();
		Vec2 clip_size = clip_pos.Size();
		Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
		Vec2 shift = clip_pos.v1 - orig_pos.v1;
		shift.x /= orig_size.x;
		shift.y /= orig_size.y;
		Box2d clip_uv(Vec2(shift.x, shift.y));
		clip_uv.v2 += Vec2(s.x, s.y);

		v->pos = clip_pos.LeftTop().XY();
		v->color = col;
		v->tex = clip_uv.LeftTop();
		++v;

		v->pos = clip_pos.RightTop().XY();
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = clip_pos.LeftBottom().XY();
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos = clip_pos.LeftBottom().XY();
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos = clip_pos.RightTop().XY();
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = clip_pos.RightBottom().XY();
		v->color = col;
		v->tex = clip_uv.RightBottom();
		++v;

		++in_buffer;
	}
}

//=================================================================================================
/*
Przycinanie tekstu do wybranego regionu, zwraca:
0 - tekst w ca³oœci w regionie
1 - tekst nad regionem
2 - tekst pod regionem
3 - tekst poza regionem z prawej
4 - tekst poza regionem z lewej
5 - wymaga czêœciowego clippingu, czêœciowo w regionie
*/
int Gui::Clip(int x, int y, int w, int h, const Rect* clipping)
{
	if(x >= clipping->Left() && y >= clipping->Top() && x + w < clipping->Right() && y + h < clipping->Bottom())
	{
		// tekst w ca³oœci w regionie
		return 0;
	}
	else if(y + h < clipping->Top())
	{
		// tekst nad regionem
		return 1;
	}
	else if(y > clipping->Bottom())
	{
		// tekst pod regionem
		return 2;
	}
	else if(x > clipping->Right())
	{
		// tekst poza regionem z prawej
		return 3;
	}
	else if(x + w < clipping->Left())
	{
		// tekst poza regionem z lewej
		return 4;
	}
	else
	{
		// wymaga czêœciowego clippingu
		return 5;
	}
}

//=================================================================================================
void Gui::SkipTextLine(cstring text, uint line_begin, uint line_end, HitboxContext* hc)
{
	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = text[i];
		if(c == '$')
		{
			++i;
			c = text[i];
			if(c == 'h')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Yes;
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Yes);
					hc->open = HitboxOpen::No;
					++hc->counter;
				}
				else
					assert(0);
			}
			else if(c == 'g')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Group;
					int tmp;
					++i;
					Font::ParseGroupIndex(text, line_end, i, tmp, tmp);
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Group);
					hc->open = HitboxOpen::No;
				}
				else
					assert(0);
			}
		}
	}
}

//=================================================================================================
DialogBox* Gui::ShowDialog(const DialogInfo& info)
{
	assert(!(info.have_tick && info.img)); // not allowed together

	DialogBox* d;
	int extra_limit = 0;
	Int2 min_size(0, 0);

	// create dialog
	if(info.have_tick)
		d = new DialogWithCheckbox(info);
	else if(info.img)
	{
		DialogWithImage* dwi = new DialogWithImage(info);
		Int2 size = dwi->GetImageSize();
		extra_limit = size.x + 8;
		min_size.y = size.y;
		d = dwi;
	}
	else
		d = new DialogBox(info);
	created_dialogs.push_back(d);

	// calculate size
	Font* font = d->layout->font;
	Int2 text_size;
	if(!info.auto_wrap)
		text_size = font->CalculateSize(info.text);
	else
		text_size = font->CalculateSizeWrap(info.text, wnd_size, 24 + 32 + extra_limit);
	d->size = text_size + Int2(24 + extra_limit, 24 + max(0, min_size.y - text_size.y));

	// set buttons
	if(info.type == DIALOG_OK)
	{
		Button& bt = Add1(d->bts);
		bt.text = txOk;
		bt.id = GuiEvent_Custom + BUTTON_OK;
		bt.size = font->CalculateSize(bt.text) + Int2(24, 24);
		bt.parent = d;

		min_size.x = bt.size.x + 24;
	}
	else
	{
		d->bts.resize(2);
		Button& bt1 = d->bts[0],
			& bt2 = d->bts[1];

		if(info.custom_names)
		{
			bt1.text = (info.custom_names[0] ? info.custom_names[0] : txYes);
			bt2.text = (info.custom_names[1] ? info.custom_names[1] : txNo);
		}
		else
		{
			bt1.text = txYes;
			bt2.text = txNo;
		}

		bt1.id = GuiEvent_Custom + BUTTON_YES;
		bt1.size = font->CalculateSize(bt1.text) + Int2(24, 24);
		bt1.parent = d;

		bt2.id = GuiEvent_Custom + BUTTON_NO;
		bt2.size = font->CalculateSize(bt2.text) + Int2(24, 24);
		bt2.parent = d;

		bt1.size = bt2.size = Int2::Max(bt1.size, bt2.size);
		min_size.x = bt1.size.x * 2 + 24 + 16;
	}

	// powiêksz rozmiar okna o przyciski
	if(d->size.x < min_size.x)
		d->size.x = min_size.x;
	d->size.y += d->bts[0].size.y + 8;

	// checkbox
	if(info.have_tick)
	{
		d->size.y += 32;
		DialogWithCheckbox* dwc = static_cast<DialogWithCheckbox*>(d);
		dwc->checkbox.bt_size = Int2(32, 32);
		dwc->checkbox.checked = info.ticked;
		dwc->checkbox.id = GuiEvent_Custom + BUTTON_CHECKED;
		dwc->checkbox.parent = dwc;
		dwc->checkbox.text = info.tick_text;
		dwc->checkbox.pos = Int2(12, 40);
		dwc->checkbox.size = Int2(d->size.x - 24, 32);
	}

	// ustaw przyciski
	if(d->bts.size() == 1)
	{
		Button& bt = d->bts[0];
		bt.pos.x = (d->size.x - bt.size.x) / 2;
		bt.pos.y = d->size.y - 8 - bt.size.y;
	}
	else
	{
		Button& bt1 = d->bts[0],
			& bt2 = d->bts[1];
		bt1.pos.y = bt2.pos.y = d->size.y - 8 - bt1.size.y;
		bt1.pos.x = 12;
		bt2.pos.x = d->size.x - bt2.size.x - 12;
	}

	// dodaj
	d->need_delete = true;
	d->Setup(text_size);
	ShowDialog(d);

	return d;
}

//=================================================================================================
void Gui::ShowDialog(DialogBox* d)
{
	d->visible = true;
	d->Event(GuiEvent_Show);

	if(dialog_layer->Empty())
	{
		// nie ma ¿adnych innych dialogów, aktywuj
		dialog_layer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else if(d->order == ORDER_TOPMOST)
	{
		// dezaktywuj aktualny i aktywuj nowy
		Control* prev_d = dialog_layer->Top();
		prev_d->focus = false;
		prev_d->Event(GuiEvent_LostFocus);
		dialog_layer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else
	{
		// szukaj pierwszego dialogu który jest wy¿ej ni¿ ten
		DialogOrder above_order = DialogOrder(d->order + 1);
		vector<DialogBox*>& ctrls = (vector<DialogBox*>&)dialog_layer->GetControls();
		vector<DialogBox*>::iterator first_above = ctrls.end();
		for(vector<DialogBox*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->order >= above_order)
			{
				first_above = it;
				break;
			}
		}

		if(first_above == ctrls.end())
		{
			// brak nadrzêdnego dialogu, dezaktywuj aktualny i aktywuj nowy
			Control* prev_d = dialog_layer->Top();
			prev_d->focus = false;
			prev_d->Event(GuiEvent_LostFocus);
			dialog_layer->Add(d);
			d->focus = true;
			d->Event(GuiEvent_GainFocus);
		}
		else
		{
			// jest nadrzêdny dialog, dodaj przed nim i nie aktywuj
			ctrls.insert(first_above, d);
		}
	}

	dialog_layer->inside_loop = false;
}

//=================================================================================================
bool Gui::CloseDialog(DialogBox* d)
{
	assert(d);

	if(dialog_layer->Empty() || !HaveDialog(d))
		return false;

	Control* prev_top = dialog_layer->Top();
	CloseDialogInternal(d);
	if(!dialog_layer->Empty() && prev_top != dialog_layer->Top())
	{
		Control* next_d = dialog_layer->Top();
		next_d->focus = true;
		next_d->Event(GuiEvent_GainFocus);
	}

	return true;
}

//=================================================================================================
void Gui::CloseDialogInternal(DialogBox* d)
{
	assert(d);

	d->Event(GuiEvent_Close);
	d->visible = false;
	dialog_layer->Remove(d);

	if(!dialog_layer->Empty())
	{
		vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
		static vector<DialogBox*> to_remove;
		for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
		{
			if((*it)->parent == d)
				to_remove.push_back(*it);
		}
		if(!to_remove.empty())
		{
			for(vector<DialogBox*>::iterator it = to_remove.begin(), end = to_remove.end(); it != end; ++it)
				CloseDialogInternal(*it);
			to_remove.clear();
		}
	}

	if(d->need_delete)
	{
		RemoveElement(created_dialogs, d);
		delete d;
	}
}

//=================================================================================================
bool Gui::HaveTopDialog(cstring name) const
{
	assert(name);

	if(dialog_layer->Empty())
		return false;

	DialogBox* d = static_cast<DialogBox*>(dialog_layer->Top());
	return d->name == name;
}

//=================================================================================================
bool Gui::HaveDialog() const
{
	return !dialog_layer->Empty();
}

//=================================================================================================
void Gui::DrawSpriteFull(Texture* t, const Color color)
{
	assert(t && t->IsLoaded());

	Lock(t->tex, 1);

	Vec4 col = Color(color);

	v->pos = Vec3(0, 0, 0);
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = Vec3(float(wnd_size.x), 0, 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec3(0, float(wnd_size.y), 0);
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec3(0, float(wnd_size.y), 0);
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec3(float(wnd_size.x), 0, 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec3(float(wnd_size.x), float(wnd_size.y), 0);
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	++in_buffer;
}

//=================================================================================================
void Gui::OnChar(char c)
{
	if((c != (char)Key::Backspace && c != (char)Key::Enter && byte(c) < 0x20) || c == '`')
		return;

	for(vector<OnCharHandler*>::iterator it = on_char.begin(), end = on_char.end(); it != end; ++it)
	{
		Control* ctrl = dynamic_cast<Control*>(*it);
		if(ctrl->visible)
			(*it)->OnChar(c);
	}
}

//=================================================================================================
void Gui::SimpleDialog(cstring text, Control* parent, cstring name)
{
	DialogInfo di;
	di.event = nullptr;
	di.name = name;
	di.parent = parent;
	di.pause = false;
	di.text = text;
	di.order = ORDER_NORMAL;
	di.type = DIALOG_OK;

	if(parent)
	{
		DialogBox* d = dynamic_cast<DialogBox*>(parent);
		if(d)
			di.order = d->order;
	}

	ShowDialog(di);
}

//=================================================================================================
void Gui::DrawSpriteRect(Texture* t, const Rect& rect, Color color)
{
	assert(t && t->IsLoaded());

	Lock(t->tex, 1);

	Vec4 col = Color(color);

	v->pos = Vec3(float(rect.Left()), float(rect.Top()), 0);
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Top()), 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec3(float(rect.Left()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec3(float(rect.Left()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Top()), 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	++in_buffer;
}

//=================================================================================================
bool Gui::HaveDialog(cstring name)
{
	assert(name);
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
	for(DialogBox* dialog : dialogs)
	{
		if(dialog->name == name)
			return true;
	}
	return false;
}

//=================================================================================================
bool Gui::HaveDialog(DialogBox* dialog)
{
	assert(dialog);
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
	for(auto d : dialogs)
	{
		if(d == dialog)
			return true;
	}
	return false;
}

//=================================================================================================
bool Gui::AnythingVisible() const
{
	return !dialog_layer->Empty() || layer->AnythingVisible();
}

//=================================================================================================
void Gui::OnResize()
{
	wnd_size = app::engine->GetWindowSize();
	cursor_pos = wnd_size / 2;
	app::engine->SetUnlockPoint(cursor_pos);
	layer->Event(GuiEvent_WindowResize);
	dialog_layer->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void Gui::DrawSpriteRectPart(Texture* t, const Rect& rect, const Rect& part, Color color)
{
	assert(t && t->IsLoaded());

	Lock(t->tex, 1);

	Int2 size = t->GetSize();
	Vec4 col = Color(color);
	Box2d uv(float(part.Left()) / size.x, float(part.Top()) / size.y, float(part.Right()) / size.x, float(part.Bottom()) / size.y);

	v->pos = Vec3(float(rect.Left()), float(rect.Top()), 0);
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Top()), 0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = Vec3(float(rect.Left()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = Vec3(float(rect.Left()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Top()), 0);
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = Vec3(float(rect.Right()), float(rect.Bottom()), 0);
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	++in_buffer;
}

//=================================================================================================
void Gui::DrawSpriteTransform(Texture* t, const Matrix& mat, Color color)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();

	Lock(t->tex, 1);

	Vec4 col = Color(color);

	Vec2 leftTop(0, 0),
		rightTop(float(size.x), 0),
		leftBottom(0, float(size.y)),
		rightBottom(float(size.x), float(size.y));

	leftTop = Vec2::Transform(leftTop, mat);
	rightTop = Vec2::Transform(rightTop, mat);
	leftBottom = Vec2::Transform(leftBottom, mat);
	rightBottom = Vec2::Transform(rightBottom, mat);

	v->pos = leftTop.XY();
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = rightTop.XY();
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = leftBottom.XY();
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = leftBottom.XY();
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = rightTop.XY();
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = rightBottom.XY();
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	++in_buffer;
}

//=================================================================================================
bool Gui::NeedCursor()
{
	if(!dialog_layer->Empty())
		return true;
	else
		return layer->visible && layer->NeedCursor();
}

//=================================================================================================
bool Gui::DrawText3D(Font* font, Cstring text, uint flags, Color color, const Vec3& pos, Rect* text_rect)
{
	assert(font);

	Int2 pt;
	if(!To2dPoint(pos, pt))
		return false;

	Int2 size = font->CalculateSize(text);
	Rect r = { pt.x - size.x / 2, pt.y - size.y - 4, pt.x + size.x / 2 + 1, pt.y - 4 };
	if(!IsSet(flags, DTF_DONT_DRAW))
		DrawText(font, text, flags, color, r);

	if(text_rect)
		*text_rect = r;

	return true;
}

//=================================================================================================
bool Gui::To2dPoint(const Vec3& pos, Int2& pt)
{
	Vec4 v4;
	Vec3::Transform(pos, mViewProj, v4);

	if(v4.z < 0)
	{
		// jest poza kamer¹
		return false;
	}

	Vec3 v3;

	// see if we are in world space already
	v3 = Vec3(v4.x, v4.y, v4.z);
	if(v4.w != 1)
	{
		if(v4.w == 0)
			v4.w = 0.00001f;
		v3 /= v4.w;
	}

	pt.x = int(v3.x * (wnd_size.x / 2) + (wnd_size.x / 2));
	pt.y = -int(v3.y * (wnd_size.y / 2) - (wnd_size.y / 2));

	return true;
}

//=================================================================================================
bool Gui::Intersect(vector<Hitbox>& hitboxes, const Int2& pt, int* index, int* index2)
{
	for(vector<Hitbox>::iterator it = hitboxes.begin(), end = hitboxes.end(); it != end; ++it)
	{
		if(it->rect.IsInside(pt))
		{
			if(index)
				*index = it->index;
			if(index2)
				*index2 = it->index2;
			return true;
		}
	}

	return false;
}

//=================================================================================================
void Gui::DrawSpriteTransformPart(Texture* t, const Matrix& mat, const Rect& part, Color color)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();

	Lock(t->tex, 1);

	Box2d uv(float(part.Left()) / size.x, float(part.Top() / size.y), float(part.Right()) / size.x, float(part.Bottom()) / size.y);

	Vec4 col = Color(color);

	Vec2 leftTop(part.LeftTop()),
		rightTop(part.RightTop()),
		leftBottom(part.LeftBottom()),
		rightBottom(part.RightBottom());

	leftTop = Vec2::Transform(leftTop, mat);
	rightTop = Vec2::Transform(rightTop, mat);
	leftBottom = Vec2::Transform(leftBottom, mat);
	rightBottom = Vec2::Transform(rightBottom, mat);

	v->pos = leftTop.XY();
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = rightTop.XY();
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = leftBottom.XY();
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = leftBottom.XY();
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = rightTop.XY();
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = rightBottom.XY();
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	++in_buffer;
}

//=================================================================================================
void Gui::CloseDialogs()
{
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
	for(DialogBox* dialog : dialogs)
	{
		if(!OR2_EQ(dialog->type, DIALOG_OK, DIALOG_YESNO))
			dialog->Event(GuiEvent_Close);
		if(dialog->need_delete)
		{
			DEBUG_DO(RemoveElementTry(created_dialogs, dialog));
			delete dialog;
		}
	}
	dialogs.clear();
	dialog_layer->inside_loop = false;
	assert(created_dialogs.empty());
	created_dialogs.clear();
}

//=================================================================================================
bool Gui::HavePauseDialog() const
{
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
	for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->pause)
			return true;
	}
	return false;
}

//=================================================================================================
DialogBox* Gui::GetDialog(cstring name)
{
	assert(name);
	if(dialog_layer->Empty())
		return nullptr;
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialog_layer->GetControls();
	for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->name == name)
			return *it;
	}
	return nullptr;
}

//=================================================================================================
void Gui::DrawSprite2(Texture* t, const Matrix& mat, const Rect* part, const Rect* clipping, Color color)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();
	GuiRect rect;

	// set pos & uv
	if(part)
		rect.Set(size.x, size.y, *part);
	else
		rect.Set(size.x, size.y);

	// transform
	rect.Transform(mat);

	// clipping
	if(clipping && !rect.Clip(*clipping))
		return;

	Lock(t->tex, 1);

	// fill vertex buffer
	Vec4 col = color;
	rect.Populate(v, col);
	++in_buffer;
}

//=================================================================================================
// Rotation is generaly ignored and shouldn't be used here
Rect Gui::GetSpriteRect(Texture* t, const Matrix& mat, const Rect* part, const Rect* clipping)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();
	GuiRect rect;

	// set pos & uv
	if(part)
		rect.Set(size.x, size.y, *part);
	else
		rect.Set(size.x, size.y);

	// transform
	rect.Transform(mat);

	// clipping
	if(clipping && !rect.Clip(*clipping))
		return Rect::Zero;

	return rect.ToRect();
}

//=================================================================================================
void Gui::DrawArea(Color color, const Int2& pos, const Int2& size, const Box2d* clip_rect)
{
	GuiRect gui_rect;
	gui_rect.Set(pos, size);
	if(!clip_rect || gui_rect.Clip(*clip_rect))
	{
		Lock(nullptr, 1);
		Vec4 col = Color(color);
		gui_rect.Populate(v, col);
		++in_buffer;
	}
}

//=================================================================================================
void Gui::DrawArea(const Box2d& rect, const AreaLayout& area_layout, const Box2d* clip_rect, Color* tint)
{
	if(area_layout.mode == AreaLayout::Mode::None)
		return;

	Color color = area_layout.color;
	if(tint)
		color *= *tint;

	if(area_layout.mode == AreaLayout::Mode::Item)
	{
		DrawItem(area_layout.tex, Int2(rect.LeftTop()), Int2(rect.Size()), color, area_layout.size.x, area_layout.size.y, clip_rect);
	}
	else
	{
		// background
		if(area_layout.mode == AreaLayout::Mode::Image && area_layout.background_color != Color::None)
		{
			assert(!clip_rect);

			Lock(nullptr, 1);
			AddRect(rect.LeftTop(), rect.RightBottom(), Color(area_layout.background_color));
			++in_buffer;
		}

		// image/color
		TEX tex;
		GuiRect gui_rect;
		if(area_layout.mode >= AreaLayout::Mode::Image)
		{
			tex = area_layout.tex->tex;
			gui_rect.Set(rect, &area_layout.region);
		}
		else
		{
			tex = nullptr;
			gui_rect.Set(rect, nullptr);
		}
		if(clip_rect)
		{
			if(!gui_rect.Clip(*clip_rect))
				return;
		}

		Lock(tex, 1);
		Vec4 col = color;
		gui_rect.Populate(v, col);
		++in_buffer;

		if(area_layout.mode != AreaLayout::Mode::BorderColor)
			return;

		// border
		assert(!clip_rect);
		col = area_layout.border_color;
		Lock(nullptr, 1);

		float s = (float)area_layout.width;
		AddRect(rect.LeftTop(), rect.RightTop() + Vec2(-s, s), col);
		AddRect(rect.LeftTop(), rect.LeftBottom() + Vec2(s, 0), col);
		AddRect(rect.RightTop() + Vec2(-s, 0), rect.RightBottom(), col);
		AddRect(rect.LeftBottom() + Vec2(0, -s), rect.RightBottom(), col);

		in_buffer += 4;
	}
}

//=================================================================================================
void Gui::AddRect(const Vec2& left_top, const Vec2& right_bottom, const Vec4& color)
{
	v->pos = Vec3(left_top.x, left_top.y, 0);
	v->tex = Vec2(0, 0);
	v->color = color;
	++v;

	v->pos = Vec3(right_bottom.x, left_top.y, 0);
	v->tex = Vec2(1, 0);
	v->color = color;
	++v;

	v->pos = Vec3(right_bottom.x, right_bottom.y, 0);
	v->tex = Vec2(1, 1);
	v->color = color;
	++v;

	v->pos = Vec3(right_bottom.x, right_bottom.y, 0);
	v->tex = Vec2(1, 1);
	v->color = color;
	++v;

	v->pos = Vec3(left_top.x, right_bottom.y, 0);
	v->tex = Vec2(0, 1);
	v->color = color;
	++v;

	v->pos = Vec3(left_top.x, left_top.y, 0);
	v->tex = Vec2(0, 0);
	v->color = color;
	++v;
}

//=================================================================================================
void Gui::SetClipboard(cstring text)
{
	assert(text);

	if(OpenClipboard(app::engine->GetWindowHandle()))
	{
		EmptyClipboard();
		uint length = strlen(text) + 1;
		HANDLE mem = GlobalAlloc(GMEM_FIXED, length);
		char* str = (char*)GlobalLock(mem);
		memcpy(str, text, length);
		GlobalUnlock(mem);
		SetClipboardData(CF_TEXT, mem);
		CloseClipboard();
	}
}

//=================================================================================================
cstring Gui::GetClipboard()
{
	cstring result = nullptr;

	if(OpenClipboard(app::engine->GetWindowHandle()))
	{
		if(IsClipboardFormatAvailable(CF_TEXT) != FALSE)
		{
			HANDLE mem = GetClipboardData(CF_TEXT);
			cstring str = (cstring)GlobalLock(mem);
			result = Format("%s", str);
			GlobalUnlock(mem);
		}
		CloseClipboard();
	}

	return result;
}

//=================================================================================================
void Gui::UseGrayscale(bool grayscale)
{
	if(this->grayscale == grayscale)
		return;

	Flush();
	this->grayscale = grayscale;
	shader->SetGrayscale(grayscale);
}

//=================================================================================================
bool Gui::DrawText2(DrawTextOptions& options)
{
	uint line_begin, line_end, line_index = 0;
	int line_width, width = options.rect.SizeX();
	Vec4 current_color = Color(options.color);
	Vec4 default_color = current_color;

	use_outline = IsSet(options.flags, DTF_OUTLINE) && options.font->tex_outline;
	if(use_outline)
	{
		current_outline_color = outline_color;
		current_outline_color.w *= float(options.color.a) / 255;
	}
	bool parse_special = IsSet(options.flags, DTF_PARSE_SPECIAL);
	bool bottom_clip = false;

	HitboxContext* hc;
	if(options.hitboxes)
	{
		hc = &tmpHitboxContext;
		hc->hitbox = options.hitboxes;
		hc->counter = (options.hitbox_counter ? *options.hitbox_counter : 0);
		hc->open = HitboxOpen::No;
	}
	else
		hc = nullptr;

	if(!IsSet(options.flags, DTF_VCENTER | DTF_BOTTOM))
	{
		int y = options.rect.Top();

		if(!options.lines)
		{
			// tekst pionowo po œrodku lub na dole
			while(options.font->SplitLine(line_begin, line_end, line_width, line_index, options.str, options.str_length, options.flags, width))
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(options.flags, DTF_CENTER))
					x = options.rect.Left() + (width - line_width) / 2;
				else if(IsSet(options.flags, DTF_RIGHT))
					x = options.rect.Right() - line_width;
				else
					x = options.rect.Left();

				int clip_result = (options.clipping ? Clip(x, y, line_width, options.font->height, options.clipping) : 0);
				Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
					int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

				// znaki w tej linijce
				if(clip_result == 0)
				{
					DrawTextLine(options.font, options.str, line_begin, line_end, default_color, current_color, scaled_pos.x, scaled_pos.y, nullptr,
						hc, parse_special, options.scale);
				}
				else if(clip_result == 5)
				{
					DrawTextLine(options.font, options.str, line_begin, line_end, default_color, current_color, scaled_pos.x, scaled_pos.y, options.clipping,
						hc, parse_special, options.scale);
				}
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipTextLine(options.str, line_begin, line_end, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += options.font->height;
			}
		}
		else
		{
			for(uint line_index = options.lines_start, lines_max = min(options.lines_end, options.lines->size()); line_index < lines_max; ++line_index)
			{
				auto& line = options.lines->at(line_index);

				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(options.flags, DTF_CENTER))
					x = options.rect.Left() + (width - line.width) / 2;
				else if(IsSet(options.flags, DTF_RIGHT))
					x = options.rect.Right() - line.width;
				else
					x = options.rect.Left();

				int clip_result = (options.clipping ? Clip(x, y, line.width, options.font->height, options.clipping) : 0);
				Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
					int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

				// znaki w tej linijce
				if(clip_result == 0)
				{
					DrawTextLine(options.font, options.str, line.begin, line.end, default_color, current_color, scaled_pos.x, scaled_pos.y, nullptr,
						hc, parse_special, options.scale);
				}
				else if(clip_result == 5)
				{
					DrawTextLine(options.font, options.str, line.begin, line.end, default_color, current_color, scaled_pos.x, scaled_pos.y, options.clipping,
						hc, parse_special, options.scale);
				}
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipTextLine(options.str, line.begin, line.end, hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += options.font->height;
			}
		}
	}
	else
	{
		// tekst u góry
		if(!options.lines)
		{
			static vector<TextLine> lines_data;
			lines_data.clear();

			// oblicz wszystkie linijki
			while(options.font->SplitLine(line_begin, line_end, line_width, line_index, options.str, options.str_length, options.flags, width))
				lines_data.push_back(TextLine(line_begin, line_end, line_width));

			options.lines = &lines_data;
		}

		// pocz¹tkowa pozycja y
		int y;
		if(IsSet(options.flags, DTF_BOTTOM))
			y = options.rect.Bottom() - options.lines->size() * options.font->height;
		else
			y = options.rect.Top() + (options.rect.SizeY() - int(options.lines->size()) * options.font->height) / 2;

		for(uint line_index = options.lines_start, lines_max = min(options.lines_end, options.lines->size()); line_index < lines_max; ++line_index)
		{
			auto& line = options.lines->at(line_index);

			// pocz¹tkowa pozycja x w tej linijce
			int x;
			if(IsSet(options.flags, DTF_CENTER))
				x = options.rect.Left() + (width - line.width) / 2;
			else if(IsSet(options.flags, DTF_RIGHT))
				x = options.rect.Right() - line.width;
			else
				x = options.rect.Left();

			int clip_result = (options.clipping ? Clip(x, y, line.width, options.font->height, options.clipping) : 0);
			Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
				int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

			// znaki w tej linijce
			if(clip_result == 0)
			{
				DrawTextLine(options.font, options.str, line.begin, line.end, default_color, current_color, scaled_pos.x, scaled_pos.y, nullptr,
					hc, parse_special, options.scale);
			}
			else if(clip_result == 5)
			{
				DrawTextLine(options.font, options.str, line.begin, line.end, default_color, current_color, scaled_pos.x, scaled_pos.y, options.clipping,
					hc, parse_special, options.scale);
			}
			else if(clip_result == 2)
			{
				// tekst jest pod widocznym regionem, przerwij rysowanie
				bottom_clip = true;
				break;
			}
			else if(options.hitboxes)
			{
				// pomiñ hitbox
				SkipTextLine(options.str, line.begin, line.end, hc);
			}

			// zmieñ y na kolejn¹ linijkê
			y += options.font->height;
		}
	}

	Flush();

	if(options.hitbox_counter)
		*options.hitbox_counter = hc->counter;

	return !bottom_clip;
}

//=================================================================================================
void Gui::SetLayout(Layout* master_layout)
{
	assert(master_layout);
	this->master_layout = master_layout;
	if(!layout)
		layout = master_layout->Get<layout::Gui>();
}
