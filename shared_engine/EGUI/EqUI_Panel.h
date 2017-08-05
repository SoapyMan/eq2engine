//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGUI_PANEL_H
#define EGUI_PANEL_H

#include "equi_defs.h"
#include "IEqUI_Control.h"

#include "utils/KeyValues.h"

namespace equi
{

class Button;
class Label;

// eq panel class
class Panel : public IUIControl
{
	friend class CUIManager;

public:
	EQUI_CLASS(Panel, IUIControl)

	Panel();
	~Panel();

	virtual void			InitFromKeyValues( kvkeybase_t* sec = NULL );

	virtual void			Hide();

	// apperance
	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color) const;

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color) const;

protected:

	// rendering
	virtual void			Render();
	virtual void			DrawSelf(const IRectangle& rect);

	bool					ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);

	ColorRGBA				m_color;
	ColorRGBA				m_selColor;

	bool					m_windowControls;

	bool					m_grabbed;

	equi::Label*			m_labelCtrl;
	equi::Button*			m_closeButton;
};

};

#endif // EGUI_PANEL_H
