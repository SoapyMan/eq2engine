//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI control base
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQUICONTROL_H
#define IEQUICONTROL_H

#include "core/ppmem.h"

#include "utils/DkList.h"
#include "utils/DkLinkedList.h"
#include "utils/eqstring.h"
#include "utils/eqwstring.h"
#include "utils/TextureAtlas.h"

#include "math/DkMath.h"
#include "math/Rectangle.h"

#ifdef GetParent
#undef GetParent
#endif //GetParent

struct kvkeybase_t;
class IEqFont;
struct eqFontStyleParam_t;

#define UICMD_ARGV(index)		args.ptr()[index]
#define UICMD_ARGC				args.numElem()

namespace equi
{

struct eqUIEventCmd_t
{
	DkList<EqString> args;
};

#ifdef DECLARE_CLASS
#undef DECLARE_CLASS
#endif

// a helper macro for baseclass defintion
#define EQUI_CLASS( className, baseClassName )					\
	typedef className ThisClass;								\
	typedef baseClassName BaseClass;							\
	const char*	GetClassname() const { return ThisClass::Classname(); }		\
	static const char* Classname() { return #className; }		\

class IUIControl
{
	friend class CUIManager;
	//friend class IEqUIEventHandler;

public:
	PPMEM_MANAGED_OBJECT();

	IUIControl();
	virtual ~IUIControl();

	virtual void				InitFromKeyValues( kvkeybase_t* sec, bool noClear = false );

	// name and type
	const char*					GetName() const						{return m_name.ToCString();}
	void						SetName(const char* pszName)		{m_name = pszName;}

	// label (UTF-8)
	const char*					GetLabel() const;
	void						SetLabel(const char* pszLabel);

	// visibility
	virtual void				Show()								{m_visible = true;}
	virtual void				Hide()								{m_visible = false;}

	virtual void				SetVisible(bool bVisible)			{bVisible ? Show() : Hide();}
	virtual bool				IsVisible() const;

	virtual void				SetSelfVisible(bool bVisible)		{m_selfVisible = bVisible;}
	virtual bool				IsSelfVisible() const				{return m_selfVisible;}

	// activation
	virtual void				Enable(bool value)					{m_enabled = value;}
	virtual bool				IsEnabled() const;

	// position and size
	void						SetSize(const IVector2D& size);
	void						SetPosition(const IVector2D& pos);

	const IVector2D&			GetSize() const;
	const IVector2D&			GetPosition() const;

	void						SetAchors(int anchor)				{ m_anchors = anchor; }
	int							GetAchors() const					{ return m_anchors; }

	void						SetAlignment(int alignmentFlags)	{ m_alignment = alignmentFlags; }
	int							GetAlignment() const				{ return m_alignment; }

	void						SetScaling(int scalingMode)			{ m_scaling = scalingMode; }
	int							GetScaling() const					{ return m_scaling; }

	// real rectangle, size position
	void						SetRectangle(const IRectangle& rect);
	virtual IRectangle			GetRectangle() const;

	// sets new transformation. Set all zeros to reset
	void						SetTransform(const Vector2D& translate, const Vector2D& scale, float rotate);

	// drawn rectangle
	virtual IRectangle			GetClientRectangle() const;

	// for text only
	virtual IRectangle			GetClientScissorRectangle() const { return GetClientRectangle(); }

	// returns the scaling of element
	Vector2D					CalcScaling() const;

	const Vector2D&				GetSizeDiff() const;
	const Vector2D&				GetSizeDiffPerc() const;

	// child controls
	void						AddChild(IUIControl* pControl);
	void						RemoveChild(IUIControl* pControl, bool destroy = true);
	IUIControl*					FindChild( const char* pszName );
	void						ClearChilds( bool destroy = true );

	IUIControl*					GetParent() const { return m_parent; }

	virtual IEqFont*			GetFont() const;

	virtual void				SetFontScale(const Vector2D& scale) { m_fontScale = scale; }
	virtual const Vector2D&		GetFontScale() const { return m_fontScale; }

	// text properties
	const ColorRGBA&			GetTextColor() const { return m_textColor; }
	void						SetTextColor(const ColorRGBA& color) { m_textColor = color; }

	void						SetTextAlignment(int alignmentFlags) { m_textAlignment = alignmentFlags; }
	int							GetTextAlignment() const { return m_textAlignment; }

	virtual	void				GetCalcFontStyle(eqFontStyleParam_t& style) const;

	// PURE VIRTUAL
	virtual const char*			GetClassname() const = 0;

	// rendering
	virtual void				Render();

	//void						Connect(IEqUIEventHandler* handler);
	//void						Disconnect(IEqUIEventHandler* handler);

protected:
	
	void						ResetSizeDiffs();
	virtual void				DrawSelf(const IRectangle& rect) = 0;

	bool						ProcessCommand(DkList<EqString>& args);

	virtual IUIControl*			HitTest(const IVector2D& point);

	// events
	virtual bool				ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);
	virtual bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	IUIControl*					m_parent;

	DkLinkedList<IUIControl*>	m_childs;		// child panels

	eqUIEventCmd_t				m_commandEvent;

	IVector2D					m_position;
	IVector2D					m_size;

	// for anchors
	Vector2D					m_sizeDiff;	
	Vector2D					m_sizeDiffPerc;

	bool						m_visible;
	bool						m_enabled;

	bool						m_selfVisible;

	int							m_alignment;
	int							m_anchors;
	int							m_scaling;

	EqString					m_name;
	EqWString					m_label;

	IEqFont*					m_font;
	Vector2D					m_fontScale;
	ColorRGBA					m_textColor;
	int							m_textAlignment;

	struct ui_transform
	{
		float					rotation;
		Vector2D				translation;
		Vector2D				scale;
	} m_transform;

};

template <class T> 
T* DynamicCast(IUIControl* control)
{
	if(control == nullptr)
		return nullptr;

	if(!stricmp(T::Classname(), control->GetClassname())) 
		return (T*)control;

	return nullptr;
}

};

#endif // IEQUICONTROL_H