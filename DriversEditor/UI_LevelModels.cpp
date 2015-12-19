//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Level model placement editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "UI_LevelModels.h"

#include "EditorMain.h"

#include "imaging/ImageLoader.h"

#include "IDebugOverlay.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "math/math_util.h"

#include "TextureView.h"

#include "world.h"

extern Vector3D g_camera_target;



//--------------------------------------------------------------------------------

class CUI_ModelProps : public wxDialog 
{
public:
	CUI_ModelProps( wxWindow* parent, int modelFlags, int modPlace )
	   : wxDialog( parent, wxID_ANY, wxT("Model properties"), wxDefaultPosition, wxSize( 272,203 ), wxDEFAULT_DIALOG_STYLE )
	{
		this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
		wxBoxSizer* bSizer11;
		bSizer11 = new wxBoxSizer( wxVERTICAL );
	
		wxBoxSizer* bSizer13;
		bSizer13 = new wxBoxSizer( wxVERTICAL );
	
		bSizer13->Add( new wxStaticText( this, wxID_ANY, wxT("Object flags"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
		wxGridSizer* gSizer2;
		gSizer2 = new wxGridSizer( 0, 2, 0, 0 );
	
		m_isGround = new wxCheckBox( this, wxID_ANY, wxT("is ground"), wxDefaultPosition, wxDefaultSize, 0 );
		gSizer2->Add( m_isGround, 0, wxALL, 5 );
	
		m_noCollide = new wxCheckBox( this, wxID_ANY, wxT("no collision"), wxDefaultPosition, wxDefaultSize, 0 );
		gSizer2->Add( m_noCollide, 0, wxALL, 5 );

		m_alignToGround = new wxCheckBox( this, wxID_ANY, wxT("align to ground"), wxDefaultPosition, wxDefaultSize, 0 );
		gSizer2->Add( m_alignToGround, 0, wxALL, 5 );
	
		m_nonUnique = new wxCheckBox( this, wxID_ANY, wxT("non unique"), wxDefaultPosition, wxDefaultSize, 0 );
		gSizer2->Add( m_nonUnique, 0, wxALL, 5 );
	
	
		bSizer13->Add( gSizer2, 0, wxEXPAND, 5 );
	
		bSizer13->Add( new wxStaticText( this, wxID_ANY, wxT("Tiled place level"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
		m_plcLevel = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 50,-1 ), wxSP_ARROW_KEYS, 0, 10, 0 );
		bSizer13->Add( m_plcLevel, 0, wxALL, 5 );
	
	
		bSizer11->Add( bSizer13, 1, wxEXPAND, 5 );
	
		bSizer11->Add( new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL|wxALIGN_RIGHT, 5 );
	
	
		this->SetSizer( bSizer11 );
		this->Layout();
	
		this->Centre( wxBOTH );

		//-------------------------------------------

		if(modelFlags & LMODEL_FLAG_ISGROUND)
			m_isGround->SetValue(true);

		if(modelFlags & LMODEL_FLAG_NOCOLLIDE)
			m_noCollide->SetValue(true);

		if(modelFlags & LMODEL_FLAG_ALIGNTOCELL)
			m_alignToGround->SetValue(true);

		if(modelFlags & LMODEL_FLAG_NONUNIQUE)
			m_nonUnique->SetValue(true);

		m_plcLevel->SetValue(modPlace);
	}

	~CUI_ModelProps()
	{

	}

	int GetModelFlags()
	{
		int nFlags = 0;

		nFlags |= m_isGround->GetValue() ? LMODEL_FLAG_ISGROUND : 0;
		nFlags |= m_noCollide->GetValue() ? LMODEL_FLAG_NOCOLLIDE : 0;
		nFlags |= m_alignToGround->GetValue() ? LMODEL_FLAG_ALIGNTOCELL : 0;
		nFlags |= m_nonUnique->GetValue() ? LMODEL_FLAG_NONUNIQUE : 0;

		return nFlags;
	}

	int	GetModelLevel()
	{
		return m_plcLevel->GetValue();
	}

protected:
	wxCheckBox* m_isGround;
	wxCheckBox* m_noCollide;
	wxCheckBox* m_alignToGround;
	wxCheckBox* m_nonUnique;

	wxSpinCtrl* m_plcLevel;
};


//--------------------------------------------------------------------------------

class CReplaceModelDialog : public wxDialog 
{
public:
		
	~CReplaceModelDialog()
	{
		// Disconnect Events
		m_selection1->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CReplaceModelDialog::OnChangeSelection1 ), NULL, this );
		m_selection2->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CReplaceModelDialog::OnChangeSelection2 ), NULL, this );
		m_replaceBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CReplaceModelDialog::OnReplaceClick ), NULL, this );
		m_replaceAllBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CReplaceModelDialog::OnReplaceClick ), NULL, this );
	}

	CReplaceModelDialog( wxWindow* parent ) : wxDialog( parent, wxID_ANY, wxT("Replace objects"), wxDefaultPosition, wxSize( 768,229 ), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
	{
		this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
		wxBoxSizer* bSizer13;
		bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
		wxStaticBoxSizer* sbSizer7;
		sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Which to replace") ), wxVERTICAL );
	
		m_preview1 = new CTextureView( this, wxDefaultPosition, wxSize( 128,128 ) );
		//m_panel13->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
		sbSizer7->Add( m_preview1, 0, wxALL, 5 );
	
		m_selection1 = new wxComboBox( this, wxID_ANY, wxT("select..."), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
		sbSizer7->Add( m_selection1, 0, wxALL|wxEXPAND, 5 );
	
	
		bSizer13->Add( sbSizer7, 1, wxEXPAND, 5 );
	
		wxStaticBoxSizer* sbSizer71;
		sbSizer71 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Replace to") ), wxVERTICAL );
	
		m_preview2 = new CTextureView( this, wxDefaultPosition, wxSize( 128,128 ) );
		//m_panel131->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
		sbSizer71->Add( m_preview2, 0, wxALL, 5 );
	
		m_selection2 = new wxComboBox( this, wxID_ANY, wxT("select..."), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
		sbSizer71->Add( m_selection2, 0, wxALL|wxEXPAND, 5 );
	
	
		bSizer13->Add( sbSizer71, 1, wxEXPAND, 5 );
	
		wxBoxSizer* bSizer14;
		bSizer14 = new wxBoxSizer( wxVERTICAL );
	
		m_replaceBtn = new wxButton( this, BTN_REPLACE, wxT("Replace"), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer14->Add( m_replaceBtn, 0, wxALL|wxALIGN_BOTTOM, 5 );
	
		m_replaceAllBtn = new wxButton( this, BTN_REPLACE_ALL, wxT("Replace ALL"), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer14->Add( m_replaceAllBtn, 0, wxALL, 5 );
	
	
		bSizer13->Add( bSizer14, 0, wxEXPAND, 5 );
	
	
		this->SetSizer( bSizer13 );
		this->Layout();
	
		this->Centre( wxBOTH );

		// Connect Events
		m_selection1->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CReplaceModelDialog::OnChangeSelection1 ), NULL, this );
		m_selection2->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CReplaceModelDialog::OnChangeSelection2 ), NULL, this );
		m_replaceBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CReplaceModelDialog::OnReplaceClick ), NULL, this );
		m_replaceAllBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CReplaceModelDialog::OnReplaceClick ), NULL, this );

		m_whichReplace = NULL;
		m_replaceTo = NULL;
	}

	void OnChangeSelection1( wxCommandEvent& event )
	{
		int sel = event.GetSelection();

		if(sel == -1)
		{
			m_preview1->SetTexture( NULL );
			m_whichReplace = NULL;

			return;
		}

		m_whichReplace = (objectcont_t*)m_selection1->GetClientData(sel);

		m_preview1->SetTexture( m_whichReplace->m_preview );
	}

	void OnChangeSelection2( wxCommandEvent& event )
	{
		int sel = event.GetSelection();

		if(sel == -1)
		{
			m_preview2->SetTexture( NULL );
			m_replaceTo = NULL;

			return;
		}

		m_replaceTo = (objectcont_t*)m_selection2->GetClientData(sel);

		m_preview2->SetTexture( m_replaceTo->m_preview );
	}

	void OnReplaceClick( wxCommandEvent& event )
	{
		if(!m_whichReplace || !m_replaceTo)
			return;

		if(event.GetId() == BTN_REPLACE)
		{
			int numReplaced = 0;

			CUI_LevelModels* modModels = dynamic_cast<CUI_LevelModels*>(GetParent());

			ASSERT(modModels);

			for(int i = 0; i < modModels->m_selRefs.numElem(); i++)
			{
				if(modModels->m_selRefs[i].selRef->container == m_whichReplace)
				{
					modModels->m_selRefs[i].selRef->container = m_replaceTo;
					numReplaced++;
				}
			}

			wxMessageBox(varargs("Replaced %d objects", numReplaced), "Done"); 
		}
		else if(event.GetId() == BTN_REPLACE_ALL)
		{
			int numReplaced = 0;

			// REPLACE
			for(int x = 0; x < g_pGameWorld->m_level.m_wide; x++)
			{
				for(int y = 0; y < g_pGameWorld->m_level.m_tall; y++)
				{
					int idx = y*g_pGameWorld->m_level.m_wide + x;

					CLevelRegion* pReg = g_pGameWorld->m_level.GetRegionAt(IVector2D(x,y));

					if(pReg)
						numReplaced += pReg->Ed_ReplaceDefs( m_whichReplace, m_replaceTo );
				}
			}

			wxMessageBox(varargs("Replaced %d objects", numReplaced), "Done"); 
		}
	}

	void SetWhichToReplaceDef( objectcont_t* ref )
	{
		for(int i = 0; i < m_selection1->GetCount(); i++)
		{
			if(m_selection1->GetClientData(i) == ref)
			{
				m_selection1->SetSelection(i);
			}
		}

		m_whichReplace = ref;

		if(m_whichReplace)
			m_preview1->SetTexture( m_whichReplace->m_preview );
	}

	void RefreshObjectDefLists()
	{
		m_selection1->Clear();
		m_selection2->Clear();

		int numObjectDefs = g_pGameWorld->m_level.m_objectDefs.numElem();

		for(int i = 0; i < numObjectDefs; i++)
		{
			EqString model_name = g_pGameWorld->m_level.m_objectDefs[i]->m_name;

			if(g_pGameWorld->m_level.m_objectDefs[i]->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
			{
				int numRefs = g_pGameWorld->m_level.m_objectDefs[i]->m_model->Ref_Count();

				model_name = model_name + varargs(" (%d)", numRefs);
			}
			else
			{
				model_name = model_name + " (dyn obj)";
			}

			int idx1 = m_selection1->Append( model_name.c_str() );
			int idx2 = m_selection2->Append( model_name.c_str() );

			m_selection1->SetClientData(idx1, g_pGameWorld->m_level.m_objectDefs[i]);
			m_selection2->SetClientData(idx2, g_pGameWorld->m_level.m_objectDefs[i]);
		}

		m_preview1->SetTexture( NULL );
		m_preview2->SetTexture( NULL );
		m_whichReplace = NULL;
		m_replaceTo = NULL;
	}
	
protected:
	enum
	{
		BTN_REPLACE = 1000,
		BTN_REPLACE_ALL
	};
		
	CTextureView*	m_preview1;
	wxComboBox*		m_selection1;

	CTextureView*	m_preview2;
	wxComboBox*		m_selection2;

	wxButton*		m_replaceBtn;
	wxButton*		m_replaceAllBtn;

	objectcont_t*	m_whichReplace;
	objectcont_t*	m_replaceTo;
};

//--------------------------------------------------------------------------------

enum EModelContextMenu
{
	MODCONTEXT_PROPERTIES = 1000,
	MODCONTEXT_REMOVE,
	MODCONTEXT_RENAME,

	MODCONTEXT_COUNT,
};

BEGIN_EVENT_TABLE(CModelListRenderPanel, wxPanel)
    EVT_ERASE_BACKGROUND(CModelListRenderPanel::OnEraseBackground)
    EVT_IDLE(CModelListRenderPanel::OnIdle)
	EVT_SCROLLWIN(CModelListRenderPanel::OnScrollbarChange)
	EVT_MOTION(CModelListRenderPanel::OnMouseMotion)
	EVT_MOUSEWHEEL(CModelListRenderPanel::OnMouseScroll)
	EVT_LEFT_DOWN(CModelListRenderPanel::OnMouseClick)
	EVT_RIGHT_UP(CModelListRenderPanel::OnMouseClick)
	EVT_SIZE(CModelListRenderPanel::OnSizeEvent)
	EVT_MENU_RANGE(MODCONTEXT_PROPERTIES, MODCONTEXT_COUNT-1, CModelListRenderPanel::OnContextEvent)
END_EVENT_TABLE()

CModelListRenderPanel::CModelListRenderPanel(wxWindow* parent) : wxPanel( parent, 0,0,640,480 )
{
	m_pFont = NULL;
	m_swapChain = NULL;

	SetScrollbar(wxVERTICAL, 0, 8, 100);

	selection_id = -1;
	pointer_position = vec2_zero;

	m_nPreviewSize = 128;
	m_bAspectFix = true;

	m_contextMenu = new wxMenu();

	m_contextMenu->Append(MODCONTEXT_PROPERTIES, wxT("Properties"), wxT("Show properties of this model"));
	m_contextMenu->Append(MODCONTEXT_RENAME, wxT("Rename..."), wxT("Rename model"));
	m_contextMenu->Append(MODCONTEXT_REMOVE, wxT("Remove"), wxT("Removes this model"));
}

void CModelListRenderPanel::OnMouseMotion(wxMouseEvent& event)
{
	pointer_position.x = event.GetX();
	pointer_position.y = event.GetY();

	Redraw();
}

void CModelListRenderPanel::OnSizeEvent(wxSizeEvent &event)
{
	if(!IsShown())
		return;

	if(materials && g_pMainFrame)
	{
		int w, h;
		//g_pMainFrame->GetMaxRenderWindowSize(w,h);
		g_pMainFrame->GetSize(&w, &h);

		//materials->SetDeviceBackbufferSize(w,h);

		RefreshScrollbar();

		Redraw();
	}
}

void CModelListRenderPanel::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos =  GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation()/100, true);
}

void CModelListRenderPanel::OnScrollbarChange(wxScrollWinEvent& event)
{
	if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else
		SetScrollPos(wxVERTICAL, event.GetPosition(), true);

	Redraw();
}

void CModelListRenderPanel::OnContextEvent(wxCommandEvent& event)
{
	objectcont_t* cont = GetSelectedModelContainer();

	if(!cont)
		return;

	if(event.GetId() == MODCONTEXT_PROPERTIES)
	{
		CUI_ModelProps* propDialog = new CUI_ModelProps(this, cont->m_info.modelflags, cont->m_info.level);

		if( propDialog->ShowModal() == wxID_OK)
		{
			cont->m_info.modelflags = propDialog->GetModelFlags();
			cont->m_info.level = propDialog->GetModelLevel();
		}
		
		propDialog->Destroy(); 

		((CUI_LevelModels*)GetParent())->RefreshModelReplacement();
	}
	else if(event.GetId() == MODCONTEXT_RENAME)
	{
		wxTextEntryDialog* dlg = new wxTextEntryDialog(this, "Rename model (Warning, resolve object def model! )");

		dlg->SetValue( cont->m_name.c_str() );

		if( dlg->ShowModal() == wxID_OK)
		{
			cont->m_name = dlg->GetValue();
			g_pMainFrame->NotifyUpdate();
		}

		delete dlg;

		Redraw();

		((CUI_LevelModels*)GetParent())->RefreshModelReplacement();
	}
	else if(event.GetId() == MODCONTEXT_REMOVE)
	{
		int result = wxMessageBox("All objects with this model will be removed, continue?", "Question", wxYES_NO | wxCENTRE | wxICON_WARNING, this);

		if(result == wxCANCEL)
			return;

		if(result == wxNO)
			return;

		RemoveModel( cont );

		((CUI_LevelModels*)GetParent())->RefreshModelReplacement();
	}
}

void CModelListRenderPanel::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	selection_id = mouseover_id;

	if(selection_id == -1)
		return;

	objectcont_t* cont = GetSelectedModelContainer();

	if(event.RightUp() && cont && cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		PopupMenu(m_contextMenu);
	}

	/*
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pObject = (CBaseEditableObject*)g_pLevel->GetEditable(i);

		bool changed = false;

		for(int j = 0; j < pObject->GetSurfaceTextureCount(); j++)
		{
			if(!(pObject->GetSurfaceTexture(j)->nFlags & STFL_SELECTED))
				continue;

			 pObject->GetSurfaceTexture(j)->pMaterial = GetSelectedMaterial();
			 changed = true;
		}

		if(changed)
			pObject->UpdateSurfaceTextures();
	}
	

	g_editormainframe->UpdateAllWindows();
	*/
}

void CModelListRenderPanel::OnIdle(wxIdleEvent &event)
{

}

void CModelListRenderPanel::OnEraseBackground(wxEraseEvent& event)
{
	//Redraw();
}

void CModelListRenderPanel::Redraw()
{
	if(!materials)
		return;

	if(!m_swapChain)
		m_swapChain = materials->CreateSwapChain((HWND)GetHWND());

	if(!IsShown() && !IsShownOnScreen())
		return;

	if(!m_pFont)
		m_pFont = InternalLoadFont("debug");

	int w, h;
	GetClientSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w, h);

	materials->GetConfiguration().wireframeMode = false;

	materials->SetAmbientColor( color4_white );

	DkList<objectcont_t*>& modellist = g_pGameWorld->m_level.m_objectDefs;

	if( materials->BeginFrame() )
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));

		if(!modellist.numElem())
		{
			materials->EndFrame(m_swapChain);
			return;
		}

		materials->Setup2D(w,h);

		int nLine = 0;
		int nItem = 0;

		Rectangle_t screenRect(0,0, w,h);
		screenRect.Fix();

		float scrollbarpercent = GetScrollPos(wxVERTICAL);///100.0f;

		int numItems = 0;

		float fSize = (float)m_nPreviewSize;

		for(int i = 0; i < m_filteredlist.numElem(); i++)
		{
			float x_offset = 16 + nItem*(fSize+16);

			if(x_offset + fSize > w)
			{
				numItems = i;
				break;
			}

			numItems++;
			nItem++;
		}

		if(numItems > 0)
		{
			int estimated_lines = m_filteredlist.numElem() / numItems;

			nItem = 0;

			for(int i = 0; i < m_filteredlist.numElem(); i++)
			{
				if(nItem >= numItems)
				{
					nItem = 0;
					nLine++;
				}

				float x_offset = 16 + nItem*(fSize+16);
				float y_offset = 8 + nLine*(fSize+48);

				y_offset -= scrollbarpercent*(fSize+48);

				Rectangle_t check_rect(x_offset, y_offset, x_offset + fSize,y_offset + fSize);
				check_rect.Fix();

				if( check_rect.vleftTop.y > screenRect.vrightBottom.y )
					break;

				if( screenRect.IsIntersectsRectangle(check_rect) )
				{
					ITexture* pTex = m_filteredlist[i]->m_preview;

					if(!pTex)
					{
						pTex = g_pShaderAPI->GetErrorTexture();
					}

					float texture_aspect = pTex->GetWidth() / pTex->GetHeight();

					float x_scale = 1.0f;
					float y_scale = 1.0f;

					if(m_bAspectFix && (pTex->GetWidth() > pTex->GetHeight()))
					{
						y_scale /= texture_aspect;
					}

					Vertex2D_t verts[] = {MAKETEXQUAD(x_offset, y_offset, x_offset + fSize*x_scale,y_offset + fSize*y_scale, 0)};
					Vertex2D_t name_line[] = {MAKETEXQUAD(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 25, 0)};
				
					BlendStateParam_t params;
					params.alphaTest = false;
					params.alphaTestRef = 1.0f;
					params.blendEnable = false;
					params.srcFactor = BLENDFACTOR_ONE;
					params.dstFactor = BLENDFACTOR_ZERO;
					params.mask = COLORMASK_ALL;
					params.blendFunc = BLENDFUNC_ADD;

					ColorRGBA nameLineCol = ColorRGBA(0.25,0.25,1,1);

					if( m_filteredlist[i]->m_info.type == LOBJ_TYPE_OBJECT_CFG )
					{
						nameLineCol = ColorRGBA(0.5,0.5,0.25,1);
					}

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, nameLineCol, &params);

					if( check_rect.IsInRectangle( pointer_position ) )
					{
						mouseover_id = i;

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(1,0.5f,0.5f,1), &params);

						Vertex2D verts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};
						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(1,0,0,1));

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.1,0.25,1), &params);
					}
					else
						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, color4_white, &params);

					if(selection_id == i)
					{
						//materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(0.5f,1.0f,0.5f,1), &params);

						Vertex2D verts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};
						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(0,1,0,1));

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(1,0.25,0.25,1), &params);
					}

					EqString material_name = m_filteredlist[i]->m_name;

					if(m_filteredlist[i]->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
						material_name = material_name + varargs(" (%d)", m_filteredlist[i]->m_model->Ref_Count()-1);

					Rectangle_t name_rect(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 400);

					eqFontStyleParam_t fontParam;
					fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
					fontParam.textColor = ColorRGBA(1,1,1,1);

					m_pFont->RenderText( material_name.c_str(), name_rect.vleftTop, fontParam);
				}

				nItem++;
			}
		}

		materials->EndFrame(m_swapChain);
	}
}

#define PREVIEW_BOX_SIZE 256

void CopyRendertargetToTexture(ITexture* rt, ITexture* dest)
{
	g_pShaderAPI->Reset(STATE_RESET_TEX);

	texlockdata_t readFrom;
	texlockdata_t writeTo;
	memset(&readFrom, 0, sizeof(readFrom));

	rt->Lock(&readFrom, NULL, false, true);

	if(readFrom.pData)
	{
		dest->Lock(&writeTo, NULL, true, false);

		if(writeTo.pData)
		{
			memcpy(writeTo.pData, readFrom.pData, writeTo.nPitch*PREVIEW_BOX_SIZE);
		}
	}

	dest->Unlock();
	rt->Unlock();
}

extern void RenderModelDef( objectcont_t* obj, float fDistance, const BoundingBox& aabb, bool preloadMaterials = false, int nRenderFlags = 0);

void RenderNewPreview(objectcont_t* obj, ITexture* dst)
{
	static ITexture* pTempRendertarget = g_pShaderAPI->CreateRenderTarget(PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, FORMAT_RGBA8);

	Vector3D	camera_rotation(35,225,0);
	Vector3D	camera_target(0);
	float		camDistance = 0.0f;
	
	BoundingBox bbox;

	if(obj->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		bbox = obj->m_model->m_bbox;

		//Msg("RENDER preview for STATIC model\n");
		camDistance = length(obj->m_model->m_bbox.GetSize())+0.5f;
		camera_target = obj->m_model->m_bbox.GetCenter();
	}
	else
	{
		//Msg("RENDER preview for DEF model\n");

		bbox = BoundingBox(obj->m_defModel->GetBBoxMins(),obj->m_defModel->GetBBoxMaxs());

		camDistance = length(bbox.GetSize())+0.5f;
		camera_target = bbox.GetCenter();
	}

	Vector3D forward, right;
	AngleVectors(camera_rotation, &forward, &right);

	Vector3D cam_pos = camera_target - forward*camDistance;

	// setup perspective
	Matrix4x4 mProjMat = perspectiveMatrixY(DEG2RAD(60), PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, 0.05f, 512.0f);

	Matrix4x4 mViewMat = rotateZXY4(DEG2RAD(-camera_rotation.x),DEG2RAD(-camera_rotation.y),DEG2RAD(-camera_rotation.z));
	mViewMat.translate(-cam_pos);

	materials->SetMatrix(MATRIXMODE_PROJECTION, mProjMat);
	materials->SetMatrix(MATRIXMODE_VIEW, mViewMat);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// setup render
	g_pShaderAPI->ChangeRenderTarget( pTempRendertarget );

	g_pShaderAPI->Clear(true, true, false, ColorRGBA(0.25,0.25,0.25,1.0f));

	RenderModelDef(obj, 0.0f, bbox, true, 0);

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// copy rendertarget
	CopyRendertargetToTexture(pTempRendertarget, dst);
}

void CModelListRenderPanel::RebuildPreviewShots()
{
	DkList<objectcont_t*>& modellist = g_pGameWorld->m_level.m_objectDefs;

	// build/rebuild preview using rendertarget and blitting
	for(int i = 0; i < modellist.numElem(); i++)
	{
		if( modellist[i]->m_dirtyPreview )
		{
			if(!modellist[i]->m_preview)
			{
				CImage emptyImage;
				emptyImage.Create(FORMAT_RGBA8, PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, 1, 1);

				DkList<CImage*> images;
				images.append(&emptyImage);

				SamplerStateParam_t sampler = g_pShaderAPI->MakeSamplerState(TEXFILTER_TRILINEAR, ADDRESSMODE_CLAMP, ADDRESSMODE_CLAMP, ADDRESSMODE_CLAMP);

				modellist[i]->m_preview = g_pShaderAPI->CreateTexture(images, sampler);
				modellist[i]->m_preview->Ref_Grab();
			}

			// try render preview
			RenderNewPreview( modellist[i],  modellist[i]->m_preview);

			modellist[i]->m_dirtyPreview = false;
		}
	}
}

void CModelListRenderPanel::ReleaseModelPreviews()
{
	m_filteredlist.clear(false);
}

objectcont_t* CModelListRenderPanel::GetSelectedModelContainer()
{
	if(selection_id >= m_filteredlist.numElem())
		selection_id = -1;

	if(m_filteredlist.numElem() == 0)
		return NULL;

	if(selection_id == -1)
		return NULL;

	return m_filteredlist[selection_id];
}

CLevelModel* CModelListRenderPanel::GetSelectedModel()
{
	if(m_filteredlist.numElem() == 0)
		return NULL;

	if(selection_id == -1)
		return NULL;

	return m_filteredlist[selection_id]->m_model;
}

void CModelListRenderPanel::SelectModel(CLevelModel* pModel)
{
	selection_id = -1;

	for(int i = 0; i < m_filteredlist.numElem(); i++)
	{
		if(m_filteredlist[i]->m_model == pModel)
		{
			selection_id = i;
			return;
		}
	}
}

void CModelListRenderPanel::SetPreviewParams(int preview_size, bool bAspectFix)
{
	if(m_nPreviewSize != preview_size)
		RefreshScrollbar();

	m_nPreviewSize = preview_size;
	m_bAspectFix = bAspectFix;
}

void CModelListRenderPanel::RefreshScrollbar()
{
	int w, h;
	GetSize(&w, &h);

	wxRect rect = GetScreenRect();
	w = rect.GetWidth();
	h = rect.GetHeight();

	int numItems = 0;
	int nItem = 0;

	float fSize = (float)m_nPreviewSize;

	for(int i = 0; i < m_filteredlist.numElem(); i++)
	{
		float x_offset = 16 + nItem*(fSize+16);

		if(x_offset + fSize > w)
		{
			numItems = i;
			break;
		}

		numItems++;
		nItem++;
	}

	if(numItems > 0)
	{
		int estimated_lines = m_filteredlist.numElem() / numItems;

		SetScrollbar(wxVERTICAL, 0, 8, estimated_lines + 10);
	}
}

void CModelListRenderPanel::ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsed, bool bSortByDate)
{
	if(!IsShown())
		return;

	if(!(m_filter != filter || m_filterTags != tags))
		return;

	// remember filter string
	m_filter = filter;
	m_filterTags = tags;

	selection_id = -1;

	m_filteredlist.clear();

	DkList<objectcont_t*> shownArray;

	if(bOnlyUsed)
	{
		/*
		for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		{
			for(int j = 0; j < g_pLevel->GetEditable(i)->GetSurfaceTextureCount(); j++)
			{
				if(g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial != NULL && !g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial->IsError())
					show_materials.addUnique(g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial);
			}
		}
		*/
	}
	else
	{
		DkList<objectcont_t*>& modellist = g_pGameWorld->m_level.m_objectDefs;

		shownArray.resize(modellist.numElem());
		shownArray.append(g_pGameWorld->m_level.m_objectDefs);
	}

	if(filter.Length() == 0)
	{
		m_filteredlist.append(shownArray);

		RefreshScrollbar();

		return;
	}

	if(filter.length() > 0)
	{
		for(int i = 0; i < shownArray.numElem(); i++)
		{
			int foundIndex = shownArray[i]->m_name.Find(filter.c_str());

			if(foundIndex != -1)
				m_filteredlist.append(shownArray[i]);
		}
	}

	RefreshScrollbar();

	Redraw();
}

void CModelListRenderPanel::AddLevelModels()
{
	for(int i = 0; i < g_pGameWorld->m_level.m_objectDefs.numElem(); i++)
	{
		if(g_pGameWorld->m_level.m_objectDefs[i]->m_info.type == LOBJ_TYPE_OBJECT_CFG &&
			g_pGameWorld->m_level.m_objectDefs[i]->m_defType == "INVALID")
		{
			Msg("Removing invalid object def '%s'\n", g_pGameWorld->m_level.m_objectDefs[i]->m_name.c_str());

			delete g_pGameWorld->m_level.m_objectDefs[i];
			g_pGameWorld->m_level.m_objectDefs.removeIndex(i);
			i--;
		}
	}

	m_filteredlist.clear(false);
	m_filteredlist.append( g_pGameWorld->m_level.m_objectDefs );

	// refresh definition list?

	RebuildPreviewShots();
	Redraw();
}

void CModelListRenderPanel::AddModel(objectcont_t* container)
{
	container->m_model->Ref_Grab();

	g_pGameWorld->m_level.m_objectDefs.append(container);
	AddLevelModels();
}

void CModelListRenderPanel::RemoveModel(objectcont_t* container)
{
	for(int x = 0; x < g_pGameWorld->m_level.m_wide; x++)
	{
		for(int y = 0; y < g_pGameWorld->m_level.m_tall; y++)
		{
			int idx = y*g_pGameWorld->m_level.m_wide + x;

			CLevelRegion* pReg = g_pGameWorld->m_level.GetRegionAt(IVector2D(x,y));

			for(int i = 0; i < pReg->m_objects.numElem(); i++)
			{
				if(pReg->m_objects[i]->container == container)
				{
					FreeLevObjectRef( pReg->m_objects[i] );
					pReg->m_objects.fastRemoveIndex(i);
					i--;
				}
			}
		}
	}

	g_pGameWorld->m_level.m_objectDefs.remove(container);

	FreeLevObjectContainer(container);

	AddLevelModels();
}

//-----------------------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------------------

enum ELevModelsBtnEvents
{
	ELM_IMPORT = 1000,
	ELM_IMPORTOVER,
	ELM_EXPORT,
	ELM_RELOADOBJECTS,
	ELM_REPLACE_REFS,
};

CUI_LevelModels::CUI_LevelModels( wxWindow* parent ) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize )
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_modelPicker = new CModelListRenderPanel(this);
	//m_modelPicker->SetForegroundColour( wxColour( 0, 0, 0 ) );
	//m_modelPicker->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	
	bSizer9->Add( m_modelPicker, 1, wxEXPAND|wxALL, 5 );
	
	m_pSettingsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,150 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Properties") ), wxVERTICAL );
	
	m_tiledPlacement = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("Tiled placement (T)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tiledPlacement->SetValue(true); 
	sbSizer6->Add( m_tiledPlacement, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer6, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Search and Filter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_filtertext = new wxTextCtrl( m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	m_filtertext->SetMaxLength( 0 ); 
	fgSizer1->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	m_onlyusedmaterials = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("SHOW ONLY USED"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_onlyusedmaterials, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Display") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer2->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Preview size"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_pPreviewSizeChoices[] = { wxT("64"), wxT("128"), wxT("256") };
	int m_pPreviewSizeNChoices = sizeof( m_pPreviewSizeChoices ) / sizeof( wxString );
	m_pPreviewSize = new wxChoice( m_pSettingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPreviewSizeNChoices, m_pPreviewSizeChoices, 0 );
	m_pPreviewSize->SetSelection( 0 );
	fgSizer2->Add( m_pPreviewSize, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_showLevelModels = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("Level models"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showLevelModels->SetValue(true); 
	fgSizer2->Add( m_showLevelModels, 0, wxALL, 5 );
	
	m_showObjects = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("Objects"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showObjects->SetValue(true); 
	fgSizer2->Add( m_showObjects, 0, wxALL, 5 );
	
	fgSizer2->Add( new wxButton( m_pSettingsPanel, ELM_RELOADOBJECTS, wxT("Reload objects"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer3->Add( fgSizer2, 0, wxEXPAND, 5 );
	
	
	bSizer10->Add( sbSizer3, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 3, 0, 0 );
	
	gSizer2->Add( new wxButton( m_pSettingsPanel, ELM_IMPORT, wxT("Import"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	gSizer2->Add( new wxButton( m_pSettingsPanel, ELM_IMPORTOVER, wxT("Import over"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	//gSizer2->Add( new wxButton( m_pSettingsPanel, ELM_EXPORT, wxT("Export OBJ"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	gSizer2->Add( new wxButton( m_pSettingsPanel, ELM_REPLACE_REFS, wxT("Replace..."), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	
	bSizer10->Add( gSizer2, 1, wxEXPAND, 5 );
	
	
	m_pSettingsPanel->SetSizer( bSizer10 );
	m_pSettingsPanel->Layout();
	bSizer9->Add( m_pSettingsPanel, 0, wxALL|wxEXPAND, 5 );

	m_modelReplacement = new CReplaceModelDialog(this);
	
	this->SetSizer( bSizer9 );
	this->Layout();

	m_filtertext->Connect(wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CUI_LevelModels::OnFilterTextChanged, NULL, this);
	m_onlyusedmaterials->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_LevelModels::OnChangePreviewParams, NULL, this);
	m_showLevelModels->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_LevelModels::OnChangePreviewParams, NULL, this);
	m_showObjects->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_LevelModels::OnChangePreviewParams, NULL, this);
	m_pPreviewSize->Connect(wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&CUI_LevelModels::OnChangePreviewParams, NULL, this);

	this->Connect(wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction)&CUI_LevelModels::OnButtons, NULL, this);

	m_rotation = 0;
	m_isSelecting = false;

	m_last_tx = 0;
	m_last_ty = 0;
	m_lastpos = 0;
	m_editMode = MEDIT_PLACEMENT;

	m_dragOffs = vec3_zero;
}

CUI_LevelModels::~CUI_LevelModels()
{

}

void CUI_LevelModels::OnButtons(wxCommandEvent& event)
{
	if(event.GetId() == ELM_IMPORT)
	{
		// import DSM/ESM/OBJ
		
		wxFileDialog* file = new wxFileDialog(NULL, "Import OBJ/ESM", "./", "*.*", "Model files (*.obj, *.esm)|*.obj;*.esm", wxFD_FILE_MUST_EXIST | wxFD_OPEN | wxFD_MULTIPLE);

		if(file->ShowModal() == wxID_OK)
		{
			wxArrayString paths;
			file->GetPaths(paths);

			for(size_t i = 0; i < paths.Count(); i++)
			{
				dsmmodel_t model;

				if( LoadSharedModel(&model, paths[i].c_str()) )
				{
					CLevelModel* pLevModel = new CLevelModel();
					pLevModel->CreateFrom( &model );

					// TODO: create preivew

					FreeDSM(&model);

					EqString path(paths[i].wchar_str());

					objectcont_t* container = new objectcont_t;
					container->m_model = pLevModel;
					container->m_name = path.Path_Extract_Name().Path_Strip_Ext().c_str();

					//g_pGameWorld->AddModel();

					m_modelPicker->AddModel(container);
				}
			}
		}

		RefreshModelReplacement();
	}
	else if(event.GetId() == ELM_IMPORTOVER)
	{
		// find model
		objectcont_t* cont = m_modelPicker->GetSelectedModelContainer();

		if(!cont)
		{
			wxMessageBox("You need to select model first");
			return;
		}

		wxFileDialog* file = new wxFileDialog(NULL, "Import OBJ/ESM over existing model", "./", "*.*", "Model files (*.obj, *.esm)|*.obj;*.esm", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

		if(file->ShowModal() == wxID_OK)
		{
			wxArrayString paths;
			
			dsmmodel_t model;
			if( LoadSharedModel(&model, file->GetPath()) )
			{
				cont->m_model->CreateFrom( &model );
				cont->m_dirtyPreview = true;

				// TODO: create preivew

				FreeDSM(&model);

				EqString path(file->GetPath().wchar_str());
				cont->m_name = path.Path_Extract_Name().Path_Strip_Ext().c_str();

				m_modelPicker->RebuildPreviewShots();
			}
		}

		m_modelReplacement->RefreshObjectDefLists();
	}
	else if(event.GetId() == ELM_EXPORT)
	{
		wxMessageBox("TODO: ELM_EXPORT");
	}
	else if(event.GetId() == ELM_RELOADOBJECTS)
	{

	}
	else if(event.GetId() == ELM_REPLACE_REFS)
	{
		m_modelReplacement->CenterOnParent();
		m_modelReplacement->Show();
	}
}

void CUI_LevelModels::OnFilterTextChanged(wxCommandEvent& event)
{
	m_modelPicker->ChangeFilter(m_filtertext->GetValue(), "", m_onlyusedmaterials->GetValue(), false);
	m_modelPicker->Redraw();
}

void CUI_LevelModels::OnChangePreviewParams(wxCommandEvent& event)
{
	int values[] = 
	{
		128,
		256,
		512,
	};

	m_modelPicker->SetPreviewParams(values[m_pPreviewSize->GetSelection()], true);

	m_modelPicker->ChangeFilter(m_filtertext->GetValue(), "", m_onlyusedmaterials->GetValue(), false);
	m_modelPicker->Redraw();
}

void CUI_LevelModels::SetRotation(int rot)
{
	m_rotation = rot;
}

int CUI_LevelModels::GetRotation()
{
	return m_rotation;
}

// IEditorTool stuff

void CUI_LevelModels::ProcessMouseEvents( wxMouseEvent& event )
{
	CBaseTilebasedEditor::ProcessMouseEvents(event);
}

void CUI_LevelModels::RecalcSelectionCenter()
{
	BoundingBox bbox;

	for(int i = 0; i < m_selRefs.numElem(); i++)
	{
		bbox.AddVertex( m_selRefs[i].selRef->position );
	}

	m_editAxis.m_position = bbox.GetCenter();
}

void CUI_LevelModels::ToggleSelection( refselectioninfo_t& ref )
{
	for(int i = 0; i < m_selRefs.numElem(); i++)
	{
		if(	m_selRefs[i].selRegion == ref.selRegion &&
			m_selRefs[i].selRef == ref.selRef)
		{
			m_selRefs.fastRemoveIndex(i);

			RecalcSelectionCenter();
			return;
		}
	}

	m_selRefs.append( ref );

	RecalcSelectionCenter();
}

void CUI_LevelModels::ClearSelection()
{
	m_selRefs.clear();
}

void CUI_LevelModels::DeleteSelection()
{
	for(int i = 0; i < m_selRefs.numElem(); i++)
	{
		objectcont_t* cont = m_selRefs[i].selRef->container;

		// remove and invalidate
		if(cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			cont->m_model->Ref_Drop();
		}

		delete m_selRefs[i].selRef;

	}

	for(int i = 0; i < m_selRefs.numElem(); i++)
	{
		m_selRefs[i].selRegion->m_objects.remove( m_selRefs[i].selRef );
	}

	m_selRefs.clear();



	g_pMainFrame->NotifyUpdate();
}

void CUI_LevelModels::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	Vector3D ray_start, ray_dir;

	g_pMainFrame->GetMouseScreenVectors(event.GetX(),event.GetY(), ray_start, ray_dir);

	// model placement
	if(event.ControlDown())
	{
		m_isSelecting = true;
		
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			float dist = MAX_COORD_UNITS;

			refselectioninfo_t info;

			int refIdx = g_pGameWorld->m_level.Ed_SelectRefAndReg(ray_start, ray_dir, &info.selRegion, dist);

			if(refIdx != -1 && info.selRegion)
			{
				info.selRef = info.selRegion->m_objects[refIdx];

				ToggleSelection( info );
			}
		}
	}
	else
	{
		m_isSelecting = false;

		if(m_editMode == MEDIT_PLACEMENT)
		{
			MousePlacementEvents(event, tile, tx, ty, ppos);
		}
		else if(m_editMode == MEDIT_TRANSLATE)
		{
			MouseTranslateEvents(event,ray_start,ray_dir);
		}
		else if(m_editMode == MEDIT_ROTATE)
		{
			MouseRotateEvents(event,ray_start,ray_dir);
		}
	}

	m_last_tx = tx;
	m_last_ty = ty;
	m_lastpos = ppos;
}

void CUI_LevelModels::MouseTranslateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir )
{
	if(m_selRefs.numElem() == 0)
		return;

	// display selection
	float clength = length(m_editAxis.m_position-g_camera_target);

	Vector3D plane_dir = normalize(m_editAxis.m_position-g_camera_target);

	if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
	{
		bool isSet = false;

		if(m_draggedAxes == 0)
		{
			m_draggedAxes = m_editAxis.TestRay(ray_start, ray_dir, clength);
			isSet = true;
		}

		{
			// process movement
			Vector3D axsMod((m_draggedAxes & AXIS_X) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Y) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Z) ? 1.0f : 0.0f);

			Vector3D edAxis = Vector3D(	dot(m_editAxis.m_rotation.rows[0],axsMod),
										dot(m_editAxis.m_rotation.rows[1],axsMod),
										dot(m_editAxis.m_rotation.rows[2],axsMod));

			// absolute
			edAxis *= sign(edAxis);

			// movement plane
			Plane pl(plane_dir, -dot(plane_dir, m_editAxis.m_position));

			Vector3D point;

			if( pl.GetIntersectionWithRay(ray_start, ray_dir, point) )
			{
				if(isSet)
				{
					m_dragOffs = m_editAxis.m_position-point;
				}

				Vector3D movement = (point-m_editAxis.m_position) + m_dragOffs;

				movement *= edAxis;

				m_editAxis.m_position += movement;

				for(int i = 0; i < m_selRefs.numElem(); i++)
				{
					// move all
					m_selRefs[i].selRef->position += movement;

#pragma todo("Relocate model in a level regions after movement (or left mouse UP)")
				}

				g_pMainFrame->NotifyUpdate();
			}
		}
	}

	if(event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
		m_draggedAxes = 0;
		m_dragOffs = vec3_zero;
	}
}

void CUI_LevelModels::MouseRotateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir )
{
	if(m_selRefs.numElem() == 0)
		return;

	// display selection
	float clength = length(m_editAxis.m_position-g_camera_target);

	Vector3D plane_dir = normalize(m_editAxis.m_position-g_camera_target);

	if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
	{
		bool isSet = false;

		if(m_draggedAxes == 0)
		{
			m_draggedAxes = m_editAxis.TestRay(ray_start, ray_dir, clength);
			isSet = true;
		}

		{
			// process movement
			Vector3D axsMod((m_draggedAxes & AXIS_X) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Y) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Z) ? 1.0f : 0.0f);

			Vector3D edAxis = Vector3D(	dot(m_editAxis.m_rotation.rows[0],axsMod),
										dot(m_editAxis.m_rotation.rows[1],axsMod),
										dot(m_editAxis.m_rotation.rows[2],axsMod));

			// absolute
			edAxis *= sign(edAxis);

			// movement plane
			Plane pl(plane_dir, -dot(plane_dir, m_editAxis.m_position));

			Vector3D point;

			if( pl.GetIntersectionWithRay(ray_start, ray_dir, point) )
			{
				if(isSet)
				{
					m_dragOffs = m_editAxis.m_position-point;
					m_dragPos = m_dragOffs;
				}

				m_dragPos = m_editAxis.m_position-point;

				Vector3D movement = (point-m_editAxis.m_position) + m_dragOffs;

				movement *= edAxis;

				m_dragPos += movement;

				Matrix3x3 addRot;

				if(m_draggedAxes & AXIS_X)
					addRot = rotateX3( DEG2RAD(length(movement)) );
				else if(m_draggedAxes & AXIS_Y)
					addRot = rotateY3( DEG2RAD(length(movement)) );
				else if(m_draggedAxes & AXIS_Z)
					addRot = rotateZ3( DEG2RAD(length(movement)) );

				m_editAxis.m_rotation = m_editAxis.m_rotation*addRot;

				for(int i = 0; i < m_selRefs.numElem(); i++)
				{
					Vector3D eulRot = m_selRefs[i].selRef->rotation;
					eulRot = VDEG2RAD(eulRot);

					// move all
					Matrix3x3 rot = rotateXYZ3(eulRot.x,eulRot.y,eulRot.z);

					rot = rot * addRot;

					eulRot = EulerMatrixXYZ(rot);
					m_selRefs[i].selRef->rotation = VRAD2DEG(eulRot);

#pragma todo("Relocate model in a level regions after movement (or left mouse UP)")
				}

				g_pMainFrame->NotifyUpdate();
			}
		}
	}

	if(event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
		m_draggedAxes = 0;
		m_dragOffs = vec3_zero;
	}
}

void CUI_LevelModels::RefreshModelReplacement()
{
	m_modelReplacement->RefreshObjectDefLists();
}

void CUI_LevelModels::MousePlacementEvents( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
	{
		if(m_selectedRegion && m_modelPicker->GetSelectedModelContainer())
		{
			// don't place model if we're dragging
			if(event.Dragging() && !m_tiledPlacement->GetValue())
				return;

			regobjectref_t* modelref = NULL;

			int pick_Idx = -1;

			// prevent placement on this tile again
			for(int i = 0; i < m_selectedRegion->m_objects.numElem(); i++)
			{
				if(	m_selectedRegion->m_objects[i]->tile_dependent && 
					(m_selectedRegion->m_objects[i]->tile_dependent == m_tiledPlacement->GetValue()) && 
					(m_selectedRegion->m_objects[i]->tile_x == tx) && 
					(m_selectedRegion->m_objects[i]->tile_y == ty))
				{
					pick_Idx = i;
					modelref = m_selectedRegion->m_objects[i];
				}
			}

			if(!modelref)
			{
				regobjectref_t* ref = new regobjectref_t;

				pick_Idx = m_selectedRegion->m_objects.append( ref );

				modelref = m_selectedRegion->m_objects[pick_Idx];
			}
			else if(modelref->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
			{
				CLevelModel* model = modelref->container->m_model;

				if(model)
					model->Ref_Drop();
			}

			modelref->container = m_modelPicker->GetSelectedModelContainer();

			if(modelref->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
			{
				CLevelModel* model = modelref->container->m_model;

				if(model)
					model->Ref_Grab();
			}

			modelref->position = ppos;
			modelref->rotation = Vector3D(0, -m_rotation*90.0f, 0);
			modelref->tile_dependent = m_tiledPlacement->GetValue();
			modelref->tile_x = tx;
			modelref->tile_y = ty;
			
			ClearSelection();

			// autoselect placed model
			if(!m_tiledPlacement->GetValue())
			{
				refselectioninfo_t sel;
				sel.selRegion = m_selectedRegion;
				sel.selRef = sel.selRegion->m_objects[pick_Idx];

				ToggleSelection(sel);
			}

			g_pMainFrame->NotifyUpdate();
		}
	}
	else if(event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
	{

	}
	else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
	{
		if(m_selectedRegion && m_modelPicker->GetSelectedModel())
		{
			// remove model from tile
			for(int i = 0; i < m_selectedRegion->m_objects.numElem(); i++)
			{
				if(	m_selectedRegion->m_objects[i]->tile_dependent && 
					m_selectedRegion->m_objects[i]->tile_x == tx && 
					m_selectedRegion->m_objects[i]->tile_y == ty)
				{
					regobjectref_t* ref = m_selectedRegion->m_objects[i];

					if(ref->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
						ref->container->m_model->Ref_Drop();

					m_selectedRegion->m_objects.fastRemoveIndex(i);

					ClearSelection();

					g_pMainFrame->NotifyUpdate();
				}
			}
		}
	}
}

void CUI_LevelModels::OnKey(wxKeyEvent& event, bool bDown)
{
	// hotkeys
	if(!bDown)
	{
		if(!event.AltDown())
		{
			if(event.m_keyCode == WXK_SPACE)
			{
				m_rotation += 1;

				if(m_rotation > 3)
					m_rotation = 0;
			}
			else if(event.m_keyCode == WXK_ESCAPE)
			{
				if(m_editMode != MEDIT_PLACEMENT)
				{
					m_editMode = MEDIT_PLACEMENT;
					return;
				}

				ClearSelection();
			}
			else if(event.m_keyCode == WXK_DELETE)
			{
				DeleteSelection();
				m_editMode = MEDIT_PLACEMENT;	// reset mode
			}
			else if(event.GetRawKeyCode() == 'G')
			{
				if(m_selRefs.numElem())
					m_editMode = MEDIT_TRANSLATE;
			}
			else if(event.GetRawKeyCode() == 'R')
			{
				if(m_selRefs.numElem())
					m_editMode = MEDIT_ROTATE;
			}
			else if(event.GetRawKeyCode() == 'T')
			{
				m_tiledPlacement->SetValue(!m_tiledPlacement->GetValue());
			}
		}

		if(event.ControlDown())
		{
			if(event.GetRawKeyCode() == 'R' && m_selRefs.numElem())
			{
				m_modelReplacement->SetWhichToReplaceDef( m_selRefs[0].selRef->container );
				m_modelReplacement->CenterOnParent();
				m_modelReplacement->Show();
			}
		}

		if(m_editMode != MEDIT_PLACEMENT)
		{
			if(event.AltDown() && event.GetRawKeyCode() == 'R')
			{
				for(int i = 0; i < m_selRefs.numElem(); i++)
				{
					m_selRefs[i].selRef->rotation = vec3_zero;
				}
			}
		}
	}
}

void CUI_LevelModels::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(m_selectedRegion)
		m_selectedRegion->m_heightfield[0]->DebugRender(false);

	if(m_editMode == MEDIT_PLACEMENT && !m_isSelecting)
	{
		if(m_tiledPlacement->GetValue())
			CBaseTilebasedEditor::OnRender();
	}

	// render model we want to place
	if( (m_modelPicker->GetSelectedModelContainer() || m_selRefs.numElem()) )
	{
		regobjectref_t tref;

		tref.container = m_modelPicker->GetSelectedModelContainer();

		tref.position = m_lastpos;
		tref.rotation = Vector3D(0, -m_rotation*90.0f, 0);
		tref.tile_dependent = m_tiledPlacement->GetValue();
		tref.tile_x = m_last_tx;
		tref.tile_y = m_last_ty;

		// placement model overview
		if( (m_editMode == MEDIT_PLACEMENT) && m_selectedRegion && !m_isSelecting && tref.container)
		{
			Matrix4x4 wmatrix = GetModelRefRenderMatrix(m_selectedRegion, &tref);

			// render
			materials->SetCullMode(CULL_BACK);
			materials->SetMatrix(MATRIXMODE_WORLD, wmatrix);

			ColorRGBA oldcol = materials->GetAmbientColor();
			materials->SetAmbientColor(ColorRGBA(1,0.5,0.5,1));

			RenderModelDef(tref.container, 0.0f, tref.bbox, false, 0);

			materials->SetAmbientColor(oldcol);
		}

		// display selection

		for(int i = 0; i < m_selRefs.numElem(); i++)
		{
			regobjectref_t* selectionRef = m_selRefs[i].selRef;

			Matrix4x4 wmatrix = GetModelRefRenderMatrix(m_selRefs[i].selRegion, selectionRef);

			// render
			materials->SetCullMode(CULL_BACK);
			materials->SetMatrix(MATRIXMODE_WORLD, wmatrix);

			ColorRGBA oldcol = materials->GetAmbientColor();
			materials->SetAmbientColor(ColorRGBA(1,0.5,0.5,1));

			RenderModelDef(selectionRef->container, 0.0f, tref.bbox, false, 0);
			//selectionRef->model->Render(0);

			materials->SetAmbientColor(oldcol);

			materials->SetMatrix(MATRIXMODE_WORLD, identity4());

			if(m_selRefs.numElem() == 1)
				m_editAxis.SetProps(wmatrix.getRotationComponent(), selectionRef->position);
		}

		if( m_selRefs.numElem() > 0 )
		{
			m_editAxis.SetProps(identity3(), m_editAxis.m_position);

			float clength = length(m_editAxis.m_position-g_camera_target);
			m_editAxis.Draw(clength);
		}
	}
}

void CUI_LevelModels::InitTool()
{
	//CBaseTilebasedEditor::Init();
}

void CUI_LevelModels::ShutdownTool()
{
	//CBaseTilebasedEditor::Init();
	m_modelPicker->ReleaseModelPreviews();
}

void CUI_LevelModels::OnLevelUnload()
{
	ClearSelection();
	m_modelPicker->ReleaseModelPreviews();
	CBaseTilebasedEditor::OnLevelUnload();
}

void CUI_LevelModels::OnLevelLoad()
{
	m_modelPicker->AddLevelModels();
	RefreshModelReplacement();
}

void CUI_LevelModels::Update_Refresh()
{
	//CBaseTilebasedEditor::Update_Refresh();
	m_modelPicker->RebuildPreviewShots();
}