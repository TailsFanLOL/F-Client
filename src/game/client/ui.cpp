/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/input.h>
#include "ui.h"

/********************************************************
 UI
*********************************************************/

const float CUI::ms_ListheaderHeight = 17.0f;
const float CUI::ms_FontmodHeight = 0.8f;

const vec4 CUI::ms_DefaultTextColor(1.0f, 1.0f, 1.0f, 1.0f);
const vec4 CUI::ms_DefaultTextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
const vec4 CUI::ms_HighlightTextColor(0.0f, 0.0f, 0.0f, 1.0f);
const vec4 CUI::ms_HighlightTextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
const vec4 CUI::ms_TransparentTextColor(1.0f, 1.0f, 1.0f, 0.5f);

CUI::CUI()
{
	m_pHotItem = 0;
	m_pActiveItem = 0;
	m_pLastActiveItem = 0;
	m_pBecommingHotItem = 0;

	m_MouseX = 0;
	m_MouseY = 0;
	m_MouseWorldX = 0;
	m_MouseWorldY = 0;
	m_MouseButtons = 0;
	m_LastMouseButtons = 0;
	m_Enabled = true;

	m_HotkeysPressed = 0;

	m_Screen.x = 0;
	m_Screen.y = 0;

	m_NumClips = 0;

	m_pActiveTooltip = 0;
	m_aTooltipText[0] = '\0';
}

void CUI::Init(class CConfig *pConfig, class IGraphics *pGraphics, class IInput *pInput, class ITextRender *pTextRender)
{
	m_pConfig = pConfig;
	m_pGraphics = pGraphics;
	m_pInput = pInput;
	m_pTextRender = pTextRender;
	CUIRect::Init(pGraphics);
	CLineInput::Init(pInput, pTextRender, pGraphics);
}

void CUI::Update(float MouseX, float MouseY, float MouseWorldX, float MouseWorldY)
{
	unsigned MouseButtons = 0;
	if(Enabled())
	{
		if(Input()->KeyIsPressed(KEY_MOUSE_1)) MouseButtons |= 1;
		if(Input()->KeyIsPressed(KEY_MOUSE_2)) MouseButtons |= 2;
		if(Input()->KeyIsPressed(KEY_MOUSE_3)) MouseButtons |= 4;
	}

	m_MouseX = MouseX;
	m_MouseY = MouseY;
	m_MouseWorldX = MouseWorldX;
	m_MouseWorldY = MouseWorldY;
	m_LastMouseButtons = m_MouseButtons;
	m_MouseButtons = MouseButtons;
	m_pHotItem = m_pBecommingHotItem;
	if(m_pActiveItem)
		m_pHotItem = m_pActiveItem;
	m_pBecommingHotItem = 0;

	if(Enabled())
	{
		CLineInput *pActiveInput = CLineInput::GetActiveInput();
		if(pActiveInput && m_pLastActiveItem && pActiveInput != m_pLastActiveItem)
			pActiveInput->SetActive(false);
	}
}

bool CUI::KeyPress(int Key) const
{
	return Enabled() && Input()->KeyPress(Key);
}

bool CUI::KeyIsPressed(int Key) const
{
	return Enabled() && Input()->KeyIsPressed(Key);
}

bool CUI::ConsumeHotkey(unsigned Hotkey)
{
	bool Pressed = m_HotkeysPressed&Hotkey;
	m_HotkeysPressed &= ~Hotkey;
	return Pressed;
}

bool CUI::OnInput(const IInput::CEvent &e)
{
	if(!Enabled())
		return false;

	CLineInput *pActiveInput = CLineInput::GetActiveInput();
	if(pActiveInput && pActiveInput->ProcessInput(e))
		return true;

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		unsigned LastHotkeysPressed = m_HotkeysPressed;
		if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
			m_HotkeysPressed |= HOTKEY_ENTER;
		else if(e.m_Key == KEY_ESCAPE)
			m_HotkeysPressed |= HOTKEY_ESCAPE;
		else if(e.m_Key == KEY_TAB && !Input()->KeyIsPressed(KEY_LALT) && !Input()->KeyIsPressed(KEY_RALT))
			m_HotkeysPressed |= HOTKEY_TAB;
		else if(e.m_Key == KEY_DELETE)
			m_HotkeysPressed |= HOTKEY_DELETE;
		else if(e.m_Key == KEY_UP)
			m_HotkeysPressed |= HOTKEY_UP;
		else if(e.m_Key == KEY_DOWN)
			m_HotkeysPressed |= HOTKEY_DOWN;
		return LastHotkeysPressed != m_HotkeysPressed;
	}
	return false;
}

void CUI::ConvertCursorMove(float *pX, float *pY, int CursorType) const
{
	float Factor = 1.0f;
	switch(CursorType)
	{
		case IInput::CURSOR_MOUSE:
			Factor = Config()->m_UiMousesens/100.0f;
			break;
		case IInput::CURSOR_JOYSTICK:
			Factor = Config()->m_UiJoystickSens/100.0f;
			break;
	}
	*pX *= Factor;
	*pY *= Factor;
}

const CUIRect *CUI::Screen()
{
	m_Screen.h = 600;
	m_Screen.w = Graphics()->ScreenAspect()*m_Screen.h;
	return &m_Screen;
}

float CUI::PixelSize()
{
	return Screen()->w/Graphics()->ScreenWidth();
}

void CUI::ClipEnable(const CUIRect *pRect)
{
	if(IsClipped())
	{
		dbg_assert(m_NumClips < MAX_CLIP_NESTING_DEPTH, "max clip nesting depth exceeded");
		const CUIRect *pOldRect = ClipArea();
		CUIRect Intersection;
		Intersection.x = max(pRect->x, pOldRect->x);
		Intersection.y = max(pRect->y, pOldRect->y);
		Intersection.w = min(pRect->x+pRect->w, pOldRect->x+pOldRect->w) - pRect->x;
		Intersection.h = min(pRect->y+pRect->h, pOldRect->y+pOldRect->h) - pRect->y;
		m_aClips[m_NumClips] = Intersection;
	}
	else
	{
		m_aClips[m_NumClips] = *pRect;
	}
	m_NumClips++;
	UpdateClipping();
}

void CUI::ClipDisable()
{
	dbg_assert(m_NumClips > 0, "no clip region");
	m_NumClips--;
	UpdateClipping();
}

const CUIRect *CUI::ClipArea() const
{
	dbg_assert(m_NumClips > 0, "no clip region");
	return &m_aClips[m_NumClips - 1];
}

void CUI::UpdateClipping()
{
	if(IsClipped())
	{
		const CUIRect *pRect = ClipArea();
		const float XScale = Graphics()->ScreenWidth()/Screen()->w;
		const float YScale = Graphics()->ScreenHeight()/Screen()->h;
		Graphics()->ClipEnable((int)(pRect->x*XScale), (int)(pRect->y*YScale), (int)(pRect->w*XScale), (int)(pRect->h*YScale));
	}
	else
	{
		Graphics()->ClipDisable();
	}
}

bool CUI::DoButtonLogic(const void *pID, const CUIRect *pRect, int Button)
{
	// logic
	bool Clicked = false;
	static int s_LastButton = -1;
	const bool Hovered = MouseHovered(pRect);

	if(CheckActiveItem(pID))
	{
		if(s_LastButton == Button && !MouseButton(s_LastButton))
		{
			if(Hovered)
				Clicked = true;
			SetActiveItem(0);
			s_LastButton = -1;
		}
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(Button))
		{
			SetActiveItem(pID);
			s_LastButton = Button;
		}
	}

	if(Hovered && !MouseButton(Button))
		SetHotItem(pID);

	return Clicked;
}

bool CUI::DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY)
{
	if(CheckActiveItem(pID))
	{
		if(!MouseButton(0))
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
			SetActiveItem(pID);
	}

	if(MouseHovered(pRect))
		SetHotItem(pID);

	if(!CheckActiveItem(pID))
		return false;

	if(pX)
		*pX = clamp(m_MouseX - pRect->x, 0.0f, pRect->w);
	if(pY)
		*pY = clamp(m_MouseY - pRect->y, 0.0f, pRect->h);

	return true;
}

void CUI::ApplyCursorAlign(class CTextCursor *pCursor, const CUIRect *pRect, int Align)
{
	pCursor->m_Align = Align;

	float x = pRect->x;
	if((Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_CENTER)
		x += pRect->w / 2.0f;
	else if((Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_RIGHT)
		x += pRect->w;

	float y = pRect->y;
	if((Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_MIDDLE)
		y += pRect->h / 2.0f;
	else if((Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_BOTTOM)
		y += pRect->h;

	pCursor->MoveTo(x, y);
}

void CUI::DoLabel(const CUIRect *pRect, const char *pText, float FontSize, int Align, float LineWidth, bool MultiLine)
{
	// TODO: FIX ME!!!!
	// Graphics()->BlendNormal();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxLines = MultiLine ? -1 : 1;
	s_Cursor.m_MaxWidth = LineWidth;
	s_Cursor.m_Flags = MultiLine ? TEXTFLAG_WORD_WRAP : 0;
	ApplyCursorAlign(&s_Cursor, pRect, Align);

	TextRender()->TextOutlined(&s_Cursor, pText, -1);
}

void CUI::DoLabelHighlighted(const CUIRect *pRect, const char *pText, const char *pHighlighted, float FontSize, const vec4 &TextColor, const vec4 &HighlightColor, int Align)
{
	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxWidth = pRect->w;
	ApplyCursorAlign(&s_Cursor, pRect, Align);

	TextRender()->TextColor(TextColor);
	const char *pMatch = pHighlighted && pHighlighted[0] ? str_find_nocase(pText, pHighlighted) : 0;
	if(pMatch)
	{
		TextRender()->TextDeferred(&s_Cursor, pText, (int)(pMatch - pText));
		TextRender()->TextColor(HighlightColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch, str_length(pHighlighted));
		TextRender()->TextColor(TextColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch + str_length(pHighlighted), -1);
	}
	else
		TextRender()->TextDeferred(&s_Cursor, pText, -1);

	TextRender()->DrawTextOutlined(&s_Cursor);
}

void CUI::DoLabelSelected(const CUIRect *pRect, const char *pText, bool Selected, float FontSize, int Align)
{
	if(Selected)
	{
		TextRender()->TextColor(CUI::ms_HighlightTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_HighlightTextOutlineColor);
	}
	DoLabel(pRect, pText, FontSize, TEXTALIGN_CENTER);
	if(Selected)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	}
}

bool CUI::DoEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, bool Hidden, int Corners, const IButtonColorFunction *pColorFunction)
{
	CTextCursor *pCursor = pLineInput->GetCursor();
	pCursor->m_FontSize = FontSize;
	pCursor->m_Align = TEXTALIGN_ML;

	const bool Inside = MouseHovered(pRect);
	const bool Active = LastActiveItem() == pLineInput;
	const bool Changed = pLineInput->WasChanged();
	const char *pDisplayStr = pLineInput->GetString();

	bool UpdateOffset = false;
	float ScrollOffset = pLineInput->GetScrollOffset();

	static bool s_DoScroll = false;
	static int s_SelectionStartOffset = -1;

	const float VSpacing = 2.0f;
	CUIRect Textbox;
	pRect->VMargin(VSpacing, &Textbox);

	if(Active)
	{
		static float s_ScrollStartX = 0.0f;

		int CursorOffset = pLineInput->GetCursorOffset();

		if(Inside && MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStartX = MouseX();
			const float MxRel = MouseX() - Textbox.x;
			float PreviousWidth = 0.0f;
			for(int i = 1, Offset = 0; i <= pLineInput->GetNumChars(); i++)
			{
				int PrevOffset = Offset;
				Offset = str_utf8_forward(pDisplayStr, Offset);
				const float TextWidth = TextRender()->TextWidth(FontSize, pDisplayStr, Offset);
				if(PreviousWidth + (TextWidth - PreviousWidth)/2.0f - ScrollOffset > MxRel)
				{
					CursorOffset = PrevOffset;
					if(s_SelectionStartOffset < 0)
						s_SelectionStartOffset = CursorOffset;
					break;
				}
				PreviousWidth = TextWidth;

				if(i == pLineInput->GetNumChars())
				{
					CursorOffset = pLineInput->GetLength();
					if(s_SelectionStartOffset < 0)
						s_SelectionStartOffset = CursorOffset;
				}
			}
		}
		else if(!MouseButton(0))
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
		}
		else if(s_DoScroll)
		{
			// do scrolling
			if(MouseX() < Textbox.x && s_ScrollStartX-MouseX() > 10.0f)
			{
				CursorOffset = str_utf8_rewind(pDisplayStr, CursorOffset);
				s_ScrollStartX = MouseX();
				UpdateOffset = true;
			}
			else if(MouseX() > Textbox.x+Textbox.w && MouseX()-s_ScrollStartX > 10.0f)
			{
				CursorOffset = str_utf8_forward(pDisplayStr, CursorOffset);
				s_ScrollStartX = MouseX();
				UpdateOffset = true;
			}
		}
		else if(!Inside && MouseButton(0))
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
			SetActiveItem(0);
			ClearLastActiveItem();
		}

		if(s_SelectionStartOffset >= 0)
		{
			pLineInput->SetCursorOffset(CursorOffset);
			pLineInput->SetSelection(s_SelectionStartOffset, CursorOffset);
		}
	}

	bool JustGotActive = false;

	if(CheckActiveItem(pLineInput))
	{
		if(!MouseButton(0))
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
			SetActiveItem(0);
		}
	}
	else if(HotItem() == pLineInput)
	{
		if(MouseButton(0))
		{
			if(!Active)
				JustGotActive = true;
			SetActiveItem(pLineInput);
		}
	}

	if(Inside)
		SetHotItem(pLineInput);

	char aStars[128];
	if(Hidden)
	{
		unsigned NumStars = pLineInput->GetNumChars();
		if(NumStars >= sizeof(aStars))
			NumStars = sizeof(aStars)-1;
		for(unsigned int i = 0; i < NumStars; ++i)
			aStars[i] = '*';
		aStars[NumStars] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(Active && !JustGotActive && (UpdateOffset || Changed))
	{
		float w = TextRender()->TextWidth(FontSize, pDisplayStr, pLineInput->GetCursorOffset());
		if(w-ScrollOffset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(FontSize, pDisplayStr, -1);
			do
			{
				ScrollOffset += min(wt-ScrollOffset-Textbox.w, Textbox.w/3);
			}
			while(w-ScrollOffset > Textbox.w);
		}
		else if(w-ScrollOffset < 0.0f)
		{
			// move to the right
			do
			{
				ScrollOffset = max(0.0f, ScrollOffset-Textbox.w/3);
			}
			while(w-ScrollOffset < 0.0f);
		}
	}

	pLineInput->SetScrollOffset(ScrollOffset);
	pLineInput->SetActive(Enabled() && Active && !JustGotActive);

	// render
	pRect->Draw(pColorFunction->GetColor(Active, Inside), 5.0f, Corners);
	ClipEnable(pRect);
	Textbox.x -= ScrollOffset;
	pCursor->MoveTo(Textbox.x, Textbox.y + Textbox.h/2.0f);
	pLineInput->Render();
	ClipDisable();

	return Changed;
}

void CUI::DoEditBoxOption(CLineInput *pLineInput, const CUIRect *pRect, const char *pStr, float VSplitVal, bool Hidden)
{
	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, EditBox;
	pRect->VSplitLeft(VSplitVal, &Label, &EditBox);

	const float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pStr);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_MC);

	DoEditBox(pLineInput, &EditBox, FontSize, Hidden);
}

float CUI::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	// layout
	CUIRect Handle;
	pRect->HSplitTop(min(pRect->h/8.0f, 33.0f), &Handle, 0);
	Handle.y += (pRect->h-Handle.h)*Current;
	Handle.VMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);

	// logic
	static float s_OffsetY;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	float ReturnValue = Current;
	bool Grabbed = false; // whether to apply the offset

	if(CheckActiveItem(pID))
	{
		if(MouseButton(0))
			Grabbed = true;
		else
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
		{
			s_OffsetY = MouseY()-Handle.y;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetY = Handle.h * 0.5f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->y;
		const float Max = pRect->h-Handle.h;
		const float Cur = MouseY()-s_OffsetY;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.w/2.0f);

	vec4 Color;
	if(Grabbed)
		Color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	else if(InsideHandle)
		Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else
		Color = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	Handle.Draw(Color, Handle.w/2.0f);

	return ReturnValue;
}

float CUI::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	// layout
	CUIRect Handle;
	pRect->VSplitLeft(max(min(pRect->w/8.0f, 33.0f), pRect->h), &Handle, 0);
	Handle.x += (pRect->w-Handle.w)*clamp(Current, 0.0f, 1.0f);
	Handle.HMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);

	// logic
	static float s_OffsetX;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	float ReturnValue = Current;
	bool Grabbed = false; // whether to apply the offset

	if(CheckActiveItem(pID))
	{
		if(MouseButton(0))
			Grabbed = true;
		else
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
		{
			s_OffsetX = MouseX()-Handle.x;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetX = Handle.w * 0.5f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->x;
		const float Max = pRect->w-Handle.w;
		const float Cur = MouseX()-s_OffsetX;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.h/2.0f);

	vec4 Color;
	if(Grabbed)
		Color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	else if(InsideHandle)
		Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else
		Color = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	Handle.Draw(Color, Handle.h/2.0f);

	return ReturnValue;
}

void CUI::DoScrollbarOption(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, const IScrollbarScale *pScale, bool Infinite)
{
	int Value = *pOption;
	if(Infinite)
	{
		Min += 1;
		Max += 1;
		if(Value == 0)
			Value = Max;
	}

	char aBufMax[128];
	str_format(aBufMax, sizeof(aBufMax), "%s: %i", pStr, Max);
	char aBuf[128];
	if(!Infinite || Value != Max)
		str_format(aBuf, sizeof(aBuf), "%s: %i", pStr, Value);
	else
		str_format(aBuf, sizeof(aBuf), "%s: \xe2\x88\x9e", pStr);

	float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;
	float VSplitVal = max(TextRender()->TextWidth(FontSize, aBuf, -1), TextRender()->TextWidth(FontSize, aBufMax, -1));

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+10.0f+VSplitVal, &Label, &ScrollBar);
	Label.VSplitLeft(Label.h+5.0f, 0, &Label);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, Min, Max)), Min, Max);
	if(Infinite && Value == Max)
		Value = 0;

	*pOption = Value;
}

void CUI::DoScrollbarOptionLabeled(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, const char* aLabels[], int NumLabels, const IScrollbarScale *pScale)
{
	int Value = clamp(*pOption, 0, NumLabels - 1);
	const int Max = NumLabels - 1;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s: %s", pStr, aLabels[Value]);

	float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+5.0f, 0, &Label);
	Label.VSplitRight(60.0f, &Label, &ScrollBar);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, 0, Max)), 0, Max);

	if(HotItem() != pID && MouseHovered(pRect) && MouseButtonClicked(0))
		Value = (Value + 1) % NumLabels;

	*pOption = clamp(Value, 0, Max);
}

float CUI::DrawClientID(float FontSize, vec2 CursorPosition, int ID,
							const vec4& BgColor, const vec4& TextColor)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;

	char aBuf[4];
	str_format(aBuf, sizeof(aBuf), "%d", ID);

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_Align = TEXTALIGN_CENTER;

	vec4 OldColor = TextRender()->GetColor();
	TextRender()->TextColor(TextColor);
	TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
	TextRender()->TextColor(OldColor);

	const float LinebaseY = CursorPosition.y + s_Cursor.BaseLineY();
	const float Width = 1.4f * FontSize;

	CUIRect Rect;
	Rect.x = CursorPosition.x;
	Rect.y = LinebaseY - FontSize + 0.15f * FontSize;
	Rect.w = Width;
	Rect.h = FontSize;
	Rect.Draw(BgColor, 0.25f * FontSize);

	s_Cursor.MoveTo(Rect.x + Rect.w / 2.0f, CursorPosition.y);
	TextRender()->DrawTextPlain(&s_Cursor);

	return Width + 0.2f * FontSize;
}

void CUI::DoTooltip(const void *pID, const CUIRect *pRect, const char *pText)
{
	if(MouseHovered(pRect))
	{
		m_pActiveTooltip = pID;
		m_TooltipAnchor = *pRect;
		str_copy(m_aTooltipText, pText, sizeof(m_aTooltipText));
	}
	else if(m_pActiveTooltip == pID)
	{
		m_pActiveTooltip = 0;
		m_aTooltipText[0] = '\0';
	}
}

void CUI::RenderTooltip()
{
	if(!m_pActiveTooltip || !m_aTooltipText[0])
		return;
	const int64 Now = time_get();
	static const void *s_pLastTooltip = 0;
	static int64 s_TooltipActivationStart = 0;
	if(s_pLastTooltip != m_pActiveTooltip)
	{
		// Reset the fade in timer if a new tooltip got active
		s_TooltipActivationStart = Now;
		s_pLastTooltip = m_pActiveTooltip;
	}

	const float SecondsBeforeFadeIn = 0.75f;
	const float SecondsSinceActivation = (Now - s_TooltipActivationStart)/(float)time_freq();
	if(SecondsSinceActivation < SecondsBeforeFadeIn)
		return;
	const float SecondsFadeIn = 0.25f;
	const float AlphaFactor = SecondsSinceActivation < SecondsBeforeFadeIn+SecondsFadeIn ? (SecondsSinceActivation-SecondsBeforeFadeIn)/SecondsFadeIn : 1.0f;
	const CUIRect *pScreen = Screen();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = 10.0f;
	s_Cursor.m_MaxLines = -1;
	s_Cursor.m_MaxWidth = pScreen->w/4.0f;
	s_Cursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE|TEXTFLAG_WORD_WRAP;
	vec4 OldTextColor = TextRender()->GetColor();
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, AlphaFactor);
	TextRender()->TextDeferred(&s_Cursor, m_aTooltipText, -1);
	TextRender()->TextColor(OldTextColor);

	// Position tooltip below hovered rect, to the right of the mouse cursor.
	// If the tooltip would overflow the screen, position it to the top and/or left instead.
	CTextBoundingBox BoundingBox = s_Cursor.BoundingBox();
	const float Rounding = 4.0f;
	const float Spacing = 4.0f;
	const float Border = 1.0f;
	CUIRect Tooltip = { MouseX(), m_TooltipAnchor.y + m_TooltipAnchor.h + Spacing, BoundingBox.w + 2 * (Spacing+Border), BoundingBox.h + 2 * (Spacing+Border) };
	if(Tooltip.x + Tooltip.w > pScreen->w)
		Tooltip.x -= Tooltip.w;
	if(Tooltip.y + Tooltip.h > pScreen->h)
		Tooltip.y -= Tooltip.h + 2 * Spacing + m_TooltipAnchor.h;
	Tooltip.Draw(vec4(0.0f, 0.0f, 0.0f, 0.4f * AlphaFactor), Rounding);
	Tooltip.Margin(Border, &Tooltip);
	Tooltip.Draw(vec4(0.5f, 0.5f, 0.5f, 0.8f * AlphaFactor), Rounding);
	Tooltip.Margin(Spacing, &Tooltip);
	ApplyCursorAlign(&s_Cursor, &Tooltip, TEXTALIGN_ML);
	TextRender()->DrawTextOutlined(&s_Cursor);

	// Clear active tooltip. DoTooltip must be called each render call.
	m_pActiveTooltip = 0;
	m_aTooltipText[0] = '\0';
}

float CUI::GetClientIDRectWidth(float FontSize)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;
	return 1.4f * FontSize + 0.2f * FontSize;
}

float CUI::GetListHeaderHeight() const
{
	return ms_ListheaderHeight + (m_pConfig->m_UiWideview ? 3.0f : 0.0f);
}

float CUI::GetListHeaderHeightFactor() const
{
	return 1.0f + (m_pConfig->m_UiWideview ? (3.0f/ms_ListheaderHeight) : 0.0f);
}
