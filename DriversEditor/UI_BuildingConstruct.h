//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Building editor for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_BUILDINGCOSTRUCT_H
#define UI_BUILDINGCOSTRUCT_H

#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"
#include "DragDropObjects.h"
#include "GenericImageListRenderer.h"

#include "EditorLevel.h"

#include "editaxis.h"

enum EBuildingEditMode
{
	ED_BUILD_READY = 0,

	ED_BUILD_MOVEMENT,

	ED_BUILD_BEGUN,				// 1st point
	ED_BUILD_WAITFORSELECT,		// next points

	ED_BUILD_SELECTEDPOINT,		// selected points

	ED_BUILD_DONE,
};

//-----------------------------------------------------------------------------

class CFloorSetEditDialog : public wxDialog
{
public:
	CFloorSetEditDialog(wxWindow* parent);
	~CFloorSetEditDialog();

	void				UpdateSelection();
	void				SetLayerModelList(buildLayer_t* coll);
	buildLayer_t*		GetLayerModelList() const;

	void				CallAddModel();

protected:
	enum
	{
		FLOOREDIT_NEW = 1000,
		FLOOREDIT_CHOOSEMODEL,
		FLOOREDIT_UP,
		FLOOREDIT_DOWN,
		FLOOREDIT_DELETE
	};

	wxPanel* m_renderPanel;
	wxStaticBoxSizer* m_propertyBox;

	wxPanel* m_panel18;
	wxButton* m_newBtn;
	wxButton* m_btnChoose;
	wxButton* m_upBtn;
	wxButton* m_dnBtn;
	wxButton* m_delBtn;

	IEqSwapChain*	m_pSwapChain;
	IEqFont*		m_pFont;

	// what we modifying
	buildLayer_t*			m_selLayer;
	CLayerModel*			m_selModel;

	Vector2D				m_mousePos;
	int						m_mouseoverItem;
	int						m_selectedItem;

	// Virtual event handlers, overide them in your derived class
	void OnIdle(wxIdleEvent &event) { Redraw(); }
	void OnEraseBackground(wxEraseEvent& event) {}

	void OnBtnsClick(wxCommandEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseScroll(wxMouseEvent& event);
	void OnMouseClick(wxMouseEvent& event);

	void Redraw();
	void RenderList();

	void OnScrollbarChange(wxScrollWinEvent& event);

	CLayerModel* GetModelUsingDialog();
};

//-----------------------------------------------------------------------------

struct buildingSelInfo_t
{
	buildingSource_t*		selBuild;
	CEditorLevelRegion*		selRegion;
};

class CUI_BuildingConstruct : public wxPanel, public CBaseTilebasedEditor
{
public:

	CUI_BuildingConstruct( wxWindow* parent ); 
	~CUI_BuildingConstruct();
	
	void		ProcessMouseEvents( wxMouseEvent& event );
	void		MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos );

	void		CancelBuilding();
	void		CompleteBuilding();

	void		ClearSelection();
	void		DeleteSelection();

	void		EditSelectedBuilding();

	void		DuplicateSelection();

	void		Update_Refresh();

	void		OnKey(wxKeyEvent& event, bool bDown);

	Vector3D	ComputePlacementPointBasedOnMouse();

	void		RecalcSelectionCenter();
	void		ToggleSelection( buildingSelInfo_t& ref );

	void		OnRender();

	void		InitTool();

	void		OnLevelLoad();
	void		OnLevelSave();
	void		OnLevelUnload();

	void		OnSwitchedTo();
	void		OnSwitchedFrom();
	
protected:

	buildLayerColl_t*		GetSelectedTemplate();
	void					SetSelectedTemplate(buildLayerColl_t* templ);

	int						GetSelectedLayerId();
	void					SetSelectedLayerId(int idx);

	void					UpdateAndFilterList();
	void					UpdateLayerList();

	void					LoadTemplates();
	void					SaveTemplates();
	void					RemoveAllTemplates();

	void					CallEditLayer();

	enum
	{
		BUILD_TEMPLATE_NEW = 1000,
		BUILD_TEMPLATE_RENAME,
		BUILD_TEMPLATE_DELETE,
		BUILD_LAYER_NEW,
		BUILD_LAYER_EDIT,
		BUILD_LAYER_DELETE
	};

	// Virtual event handlers, overide them in your derived class
	void OnFilterChange( wxCommandEvent& event );
	void OnBtnsClick(wxCommandEvent& event);
	void OnSelectTemplate(wxCommandEvent& event);
	void OnItemDclick(wxCommandEvent& event);

	void MouseTranslateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir );

	wxListBox*					m_templateList;
	wxButton*					m_button5;
	wxButton*					m_button29;
	wxButton*					m_button8;
	wxListBox*					m_layerList;
	wxButton*					m_button26;
	wxButton*					m_button28;
	wxButton*					m_button27;
	wxPanel*					m_pSettingsPanel;
	wxTextCtrl*					m_filtertext;
	wxCheckBox*					m_tiledPlacement;
	wxCheckBox*					m_offsetCloning;

	CFloorSetEditDialog*		m_layerEditDlg;

	DkList<buildLayerColl_t*>	m_buildingTemplates;

	CEditGizmo				m_editAxis;
	int							m_mouseLastY;

	Vector3D					m_mousePoint;
	bool						m_placeError;

	EBuildingEditMode			m_mode;
	buildingSource_t*			m_editingBuilding;
	buildingSource_t			m_editingCopy;		// for cancel reason
	bool						m_isEditingNewBuilding;

	int							m_curModelId;
	float						m_curSegmentScale;

	DkList<buildingSelInfo_t>	m_selBuildings;

	Vector3D					m_dragPos;
	Vector3D					m_dragOffs;
	bool						m_isSelecting;
	int							m_draggedAxes;

	bool						m_shiftModifier;
};

#endif // UI_BUILDINGCOSTRUCT_H