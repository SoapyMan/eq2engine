//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#include "UI_BlockEditor.h"
#include "EditorLevel.h"
#include "EditorMain.h"

#include "BlockEditor/BrushPrimitive.h"

#include "MaterialAtlasList.h"

const float s_GridSizes[]
{
	HFIELD_HEIGHT_STEP,
	HFIELD_HEIGHT_STEP * 2.5,
	HFIELD_HEIGHT_STEP * 5,
	HFIELD_HEIGHT_STEP * 10,	// default
	HFIELD_HEIGHT_STEP * 20,
	HFIELD_HEIGHT_STEP * 40,
};

const int s_numGridSizes = sizeof(s_GridSizes) / sizeof(float);
const int s_defaultGridSelection = 3;

//--------------------------------------------------------------------------------------
// Some math helpers

float SnapFloat(float grid_spacing, float val)
{
	return round(val / grid_spacing) * grid_spacing;
}

Vector3D SnapVector(float grid_spacing, const Vector3D &vector)
{
	return Vector3D(
		SnapFloat(grid_spacing, vector.x),
		SnapFloat(grid_spacing, vector.y),
		SnapFloat(grid_spacing, vector.z));
}

void BoxToVolume(const BoundingBox& bbox, Volume& volume)
{
	// use old box
	BoundingBox conv_box = bbox;
	conv_box.Fix();

	// make volume
	volume.LoadBoundingBox(conv_box.minPoint, conv_box.maxPoint, true);
}

void VolumeToBox(const Volume& volume, BoundingBox& bbox)
{
	// make bbox from volume
	volume.GetBBOXBack(bbox.minPoint, bbox.maxPoint);
}

//--------------------------------------------------------------------------------------

enum EToolbarEvents
{
	Event_Tools_Selection = 100,
	Event_Tools_Brush,
	Event_Tools_Polygon,
	Event_Tools_VertexManip,
	Event_Tools_Clipper,

	Event_Tools_MAX
};

extern Vector3D g_camera_target;

BEGIN_EVENT_TABLE(CUI_BlockEditor, wxPanel)
	EVT_MENU_RANGE(Event_Tools_Selection, Event_Tools_MAX, CUI_BlockEditor::ProcessAllMenuCommands)
END_EVENT_TABLE()

CUI_BlockEditor::CUI_BlockEditor(wxWindow* parent) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
{
	wxBoxSizer* bSizer28;
	bSizer28 = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer(wxVERTICAL);

	ConstructToolbars(bSizer29);

	bSizer28->Add(bSizer29, 0, wxEXPAND, 5);

	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer(wxVERTICAL);

	wxString gridSizeChoices[s_numGridSizes];

	// populate grid size
	for (int i = 0; i < s_numGridSizes; i++)
		gridSizeChoices[i] = varargs("Grid %.2f", s_GridSizes[i]);
	
	m_gridSize = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_numGridSizes, gridSizeChoices, 0);
	m_gridSize->Select(s_defaultGridSelection);

	bSizer30->Add(m_gridSize, 0, wxALL, 5);

	bSizer28->Add(bSizer30, 0, wxEXPAND, 5);

	m_texPanel = new CMaterialAtlasList(this);

	bSizer28->Add(m_texPanel, 1, wxEXPAND | wxALL, 5);

	m_pSettingsPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 150), wxTAB_TRAVERSAL);
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer(new wxStaticBox(m_pSettingsPanel, wxID_ANY, wxT("Search and Filter")), wxVERTICAL);

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer1->SetFlexibleDirection(wxBOTH);
	fgSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	fgSizer1->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	m_filtertext = new wxTextCtrl(m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), 0);
	m_filtertext->SetMaxLength(0);
	fgSizer1->Add(m_filtertext, 0, wxBOTTOM | wxRIGHT | wxLEFT, 5);

	fgSizer1->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Tags"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	m_pTags = new wxTextCtrl(m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), 0);
	m_pTags->SetMaxLength(0);
	fgSizer1->Add(m_pTags, 0, wxALL, 5);


	sbSizer2->Add(fgSizer1, 0, wxEXPAND, 5);

	m_onlyusedmaterials = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("SHOW ONLY USED MATERIALS"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer2->Add(m_onlyusedmaterials, 0, wxALL, 5);

	m_pSortByDate = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("SORT BY DATE"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer2->Add(m_pSortByDate, 0, wxALL, 5);


	bSizer10->Add(sbSizer2, 0, wxEXPAND, 5);

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(m_pSettingsPanel, wxID_ANY, wxT("Display")), wxVERTICAL);

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer2->SetFlexibleDirection(wxBOTH);
	fgSizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	fgSizer2->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Preview size"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	wxString m_pPreviewSizeChoices[] = { wxT("64"), wxT("128"), wxT("256") };
	int m_pPreviewSizeNChoices = sizeof(m_pPreviewSizeChoices) / sizeof(wxString);
	m_pPreviewSize = new wxChoice(m_pSettingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPreviewSizeNChoices, m_pPreviewSizeChoices, 0);
	m_pPreviewSize->SetSelection(0);
	fgSizer2->Add(m_pPreviewSize, 0, wxBOTTOM | wxRIGHT | wxLEFT, 5);

	m_pAspectCorrection = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("ASPECT CORRECTION"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer2->Add(m_pAspectCorrection, 0, wxALL, 5);


	sbSizer3->Add(fgSizer2, 0, wxEXPAND, 5);


	bSizer10->Add(sbSizer3, 0, wxEXPAND, 5);


	m_pSettingsPanel->SetSizer(bSizer10);
	m_pSettingsPanel->Layout();
	bSizer28->Add(m_pSettingsPanel, 0, wxALL | wxEXPAND, 5);

	this->SetSizer(bSizer28);
	this->Layout();

	m_filtertext->Connect(wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CUI_BlockEditor::OnFilterTextChanged, NULL, this);

	m_onlyusedmaterials->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pSortByDate->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pPreviewSize->Connect(wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pAspectCorrection->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);

	//--------------------------------------------------

	m_cursorPos = vec3_zero;
	m_selectedTool = BlockEdit_Selection;
	m_draggablePlane = -1;

	m_dragOffs = vec3_zero;
	m_dragRot = identity3();
	m_dragInitRot = identity3();
}

CUI_BlockEditor::~CUI_BlockEditor()
{

}

void CUI_BlockEditor::ConstructToolbars(wxBoxSizer* addTo)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxToolBar *pToolBar = new wxToolBar(this, -1, wxDefaultPosition, wxSize(64, 400), wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL/* | wxTB_HORZ_TEXT*/);//CreateToolBar(wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL);

	wxBitmap toolBarBitmaps[15];

	toolBarBitmaps[0] = wxBitmap("Editor/resource/tool_select.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[1] = wxBitmap("Editor/resource/tool_brush.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[2] = wxBitmap("Editor/resource/tool_surface.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[3] = wxBitmap("Editor/resource/tool_vertexmanip.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[4] = wxBitmap("Editor/resource/tool_clipper.bmp", wxBITMAP_TYPE_BMP);
	
	int w = toolBarBitmaps[0].GetWidth(),
		h = toolBarBitmaps[0].GetHeight();

	// this call is actually unnecessary as the toolbar will adjust its tools
	// size to fit the biggest icon used anyhow but it doesn't hurt neither
	pToolBar->SetToolBitmapSize(wxSize(w, h));

	pToolBar->AddSeparator();

#define ADD_TOOLRADIO(event_id, text, bitmap) \
	pToolBar->AddTool(event_id, text, bitmap, wxNullBitmap, wxITEM_RADIO ,text, text);

	ADD_TOOLRADIO(Event_Tools_Selection, DKLOC("TOKEN_SELECTION", "Selection"), toolBarBitmaps[0]);
	ADD_TOOLRADIO(Event_Tools_Brush, DKLOC("TOKEN_BRUSH", "Block tool"), toolBarBitmaps[1]);
	ADD_TOOLRADIO(Event_Tools_Polygon, DKLOC("TOKEN_SURFACES", "Polygon tool"), toolBarBitmaps[2]);
	ADD_TOOLRADIO(Event_Tools_VertexManip, DKLOC("TOKEN_VERTEXMANIP", "Vertex manipulator"), toolBarBitmaps[3]);
	ADD_TOOLRADIO(Event_Tools_Clipper, DKLOC("TOKEN_CLIPPER", "Clipper tool"), toolBarBitmaps[4]);

	pToolBar->Realize();

	addTo->Add(pToolBar);

	m_mode = BLOCK_MODE_READY;
	m_selectedTool = BlockEdit_Selection;
}

float CUI_BlockEditor::GridSize() const
{
	int selectedGridSizeIdx = m_gridSize->GetCurrentSelection();
	return s_GridSizes[selectedGridSizeIdx];
}

void CUI_BlockEditor::ProcessAllMenuCommands(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case Event_Tools_Selection:
		m_selectedTool = BlockEdit_Selection;
		break;
	case Event_Tools_Brush:
		m_selectedTool = BlockEdit_Brush;
		break;
	case Event_Tools_Polygon:
		m_selectedTool = BlockEdit_Polygon;
		break;
	case Event_Tools_VertexManip:
		m_selectedTool = BlockEdit_VertexManip;
		break;
	case Event_Tools_Clipper:
		m_selectedTool = BlockEdit_Clipper;
		break;
	}
}

void CUI_BlockEditor::OnChangePreviewParams(wxCommandEvent& event)
{
	int values[] =
	{
		128,
		256,
		512,
	};

	m_texPanel->SetPreviewParams(values[m_pPreviewSize->GetSelection()], m_pAspectCorrection->GetValue());

	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

void CUI_BlockEditor::OnFilterTextChanged(wxCommandEvent& event)
{
	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

// IEditorTool stuff

void CUI_BlockEditor::MouseEventOnTile(wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos)
{
	Vector3D cursorPos = SnapVector(GridSize(), ppos);

	BoundingBox modeBox = (m_mode == BLOCK_MODE_CREATION) ? m_creationBox : m_selectionBox;

	// selecting brushes within region
	// moving
	// editing
	if (m_selectedTool == BlockEdit_Brush)
	{
		// draw bounding box and press enter to create brush
		if (event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			// draw the box
			if (!event.Dragging())
			{
				m_mode = BLOCK_MODE_CREATION;

				m_creationBox.minPoint.x = cursorPos.x;
				m_creationBox.minPoint.z = cursorPos.z;

				m_creationBox.maxPoint.x = cursorPos.x;
				m_creationBox.maxPoint.z = cursorPos.z;

				if (m_creationBox.minPoint.y > m_creationBox.maxPoint.y)
				{
					m_creationBox.minPoint.y = cursorPos.y;
					m_creationBox.maxPoint.y = cursorPos.y + GridSize();
				}

				m_draggablePlane = -1;
			}
			else if (m_mode == BLOCK_MODE_CREATION)
			{
				BoundingBox newBox = m_creationBox;

				newBox.minPoint.x = cursorPos.x;
				newBox.minPoint.z = cursorPos.z;

				//if (newBox.minPoint.x == newBox.maxPoint.x)
				//	newBox.maxPoint.x += GridSize();

				if (newBox.minPoint.y == newBox.maxPoint.y)
					newBox.maxPoint.y += GridSize();

				//if (newBox.minPoint.z == newBox.maxPoint.z)
				//	newBox.maxPoint.z += GridSize();

				m_creationBox = newBox;

				m_centerAxis.SetProps(identity3(), modeBox.GetCenter());
			}

			debugoverlay->Sphere3D(m_cursorPos, 0.25f, ColorRGBA(1, 0, 0, 1), 0.0f);
		}
	}
	else if (m_selectedTool == BlockEdit_Polygon)
	{
		// draw polygon and press enter to create brushes
	}

	m_cursorPos = cursorPos;
}

void CUI_BlockEditor::ToggleBrushSelection(CBrushPrimitive* brush)
{
	if (m_selectedBrushes.findIndex(brush) == -1)
	{
		m_selectedBrushes.addUnique(brush);

		for (int i = 0; i < brush->GetFaceCount(); i++)
		{
			brush->GetFace(i)->nFlags &= ~BRUSH_FACE_SELECTED;
			ToggleFaceSelection(brush->GetFacePolygon(i));
		}
	}
	else
	{
		for (int i = 0; i < brush->GetFaceCount(); i++)
		{
			brush->GetFace(i)->nFlags |= BRUSH_FACE_SELECTED;
			ToggleFaceSelection(brush->GetFacePolygon(i));
		}

		m_selectedBrushes.fastRemove(brush);
	}
}

void CUI_BlockEditor::ToggleFaceSelection(winding_t* winding)
{
	int selectedFaceIdx = -1;
	for (int i = 0; i < m_selectedFaces.numElem(); i++)
	{
		if (m_selectedFaces[i].winding == winding)
		{
			selectedFaceIdx = i;
			break;
		}
	}

	// toggle face selection
	if ((winding->face.nFlags & BRUSH_FACE_SELECTED))
	{
		winding->face.nFlags &= ~BRUSH_FACE_SELECTED;
		m_selectedFaces.fastRemoveIndex(selectedFaceIdx);
	}
	else
	{
		winding->face.nFlags |= BRUSH_FACE_SELECTED;

		if (selectedFaceIdx == -1)
		{
			faceSelect_t faceSel(winding);
			m_selectedFaces.append(faceSel);
		}
	}
}

void CUI_BlockEditor::CancelSelection()
{
	if (m_selectedTool == BlockEdit_Selection)
	{
		if (m_mode > BLOCK_MODE_READY)
		{
			m_mode = BLOCK_MODE_READY;
			return;
		}

		for (int i = 0; i < m_selectedFaces.numElem(); i++)
			m_selectedFaces[i].winding->face.nFlags &= ~BRUSH_FACE_SELECTED;

		m_selectedFaces.clear();
		m_selectedBrushes.clear();
		m_draggablePlane = -1;
	}
	else if (m_selectedTool == BlockEdit_Brush)
	{
		if (m_mode > BLOCK_MODE_CREATION)
		{
			m_mode = BLOCK_MODE_CREATION;
			return;
		}
			
		m_mode = BLOCK_MODE_READY;
		m_draggablePlane = -1;
	}
}

void CUI_BlockEditor::DeleteSelection()
{
	for (int i = 0; i < m_selectedBrushes.numElem(); i++)
	{
		CBrushPrimitive* brush = m_selectedBrushes[i];

		// TODO: editor level code
		if (m_brushes.fastRemove(brush))
			delete brush;
	}

	m_selectedBrushes.clear();
	m_selectedFaces.clear();
	m_mode = BLOCK_MODE_READY;
}

void CUI_BlockEditor::DeleteBrush(CBrushPrimitive* brush)
{
	// TODO: editor level code
	if (m_brushes.fastRemove(brush))
		delete brush;

	m_selectedBrushes.fastRemove(brush);
	m_selectedFaces.clear();
}

void CUI_BlockEditor::RecalcSelectionBox()
{
	m_selectionBox.Reset();

	for (int i = 0; i < m_selectedBrushes.numElem(); i++)
	{
		CBrushPrimitive* brush = m_selectedBrushes[i];
		
		m_selectionBox.AddVertex(brush->GetBBox().minPoint);
		m_selectionBox.AddVertex(brush->GetBBox().maxPoint);
	}
		
	m_centerAxis.SetProps(identity3(), m_selectionBox.GetCenter());

	float selectionFac = 1.0f / (float)m_selectedFaces.numElem();
	Vector3D selectionCenter(0);
	Vector3D selectionNormal(0);
	for (int i = 0; i < m_selectedFaces.numElem(); i++)
	{
		winding_t* winding = m_selectedFaces[i].winding;

		const Plane& pl = winding->face.Plane;

		selectionCenter += winding->GetCenter()*selectionFac;
		selectionNormal += pl.normal*selectionFac;
	}

	if (m_selectedFaces.numElem())
	{
		Vector3D r, u;
		VectorVectors(selectionNormal, r, u);

		m_faceAxis.SetProps(!Matrix3x3(r, u, selectionNormal), selectionCenter);
	}
}

int VolumeIntersectsRay(const Volume& vol, const Vector3D &start, const Vector3D &dir, Vector3D& intersectionPos)
{
	int planeId = -1;

	Vector3D outintersection;
	float best_dist = V_MAX_COORD;

	for (int i = 0; i < 6; i++)
	{
		const Plane& pl = vol.GetPlane(i);
		if (pl.GetIntersectionWithRay(start, dir, outintersection))
		{
			Vector3D v = start - outintersection;
			float dist = length(start - outintersection);
		
			// check sphere because we have epsilon
			if (dist < best_dist)
			{
				if (vol.IsPointInside(outintersection + pl.normal*0.001f))
				{
					intersectionPos = outintersection;
					best_dist = dist;
					planeId = i;
				}
			}
		}
	}
	return planeId;
}

void CUI_BlockEditor::ProcessMouseEvents(wxMouseEvent& event)
{
	Vector3D ray_start, ray_dir;
	g_pMainFrame->GetMouseScreenVectors(event.GetX(), event.GetY(), ray_start, ray_dir);

	if (!event.Dragging())
	{
		// TODO: as EditorLevel code
		// selection of brushes and faces
		if (event.ControlDown() && (event.ButtonIsDown(wxMOUSE_BTN_LEFT) || event.ButtonIsDown(wxMOUSE_BTN_RIGHT)))
		{
			CBrushPrimitive* nearestBrush = nullptr;
			Vector3D intersectPos;
			float intersectDist = DrvSynUnits::MaxCoordInUnits;
			int intersectFace = -1;

			for (int i = 0; i < m_brushes.numElem(); i++)
			{
				int hitFace = -1;

				CBrushPrimitive* testBrush = m_brushes[i];
				float dist = testBrush->CheckLineIntersection(ray_start, ray_start + ray_dir * DrvSynUnits::MaxCoordInUnits, intersectPos, hitFace);

				if (dist < intersectDist)
				{
					nearestBrush = testBrush;
					intersectFace = hitFace;
					intersectDist = dist;
				}
			}

			if (nearestBrush && intersectFace >= 0)
			{
				if (event.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_selectedTool == BlockEdit_Selection)
					ToggleBrushSelection(nearestBrush);
				else if (event.ButtonIsDown(wxMOUSE_BTN_RIGHT) && intersectFace != -1 && (m_selectedTool == BlockEdit_Selection || m_selectedTool == BlockEdit_VertexManip))
					ToggleFaceSelection(nearestBrush->GetFacePolygon(intersectFace));
			}

			RecalcSelectionBox();
		}
	}

	if (m_selectedTool == BlockEdit_Selection || m_selectedTool == BlockEdit_Brush)
	{
		if (!ProcessSelectionAndBrushMouseEvents(event))
			return;
	}
	if (m_selectedTool == BlockEdit_VertexManip)
	{
		if (!ProcessVertexManipMouseEvents(event))
			return;
	}

	CBaseTilebasedEditor::ProcessMouseEvents(event);
}

bool CUI_BlockEditor::ProcessSelectionAndBrushMouseEvents(wxMouseEvent& event)
{
	Vector3D ray_start, ray_dir;
	g_pMainFrame->GetMouseScreenVectors(event.GetX(), event.GetY(), ray_start, ray_dir);

	BoundingBox& editBox = (m_selectedTool == BlockEdit_Brush) ? m_creationBox : m_selectionBox;
	CEditGizmo& editGizmo = (m_selectedFaces.numElem() == 1 || m_mode == BLOCK_MODE_READY || m_mode == BLOCK_MODE_CREATION || m_mode == BLOCK_MODE_SCALE) ? m_faceAxis : m_centerAxis;
	float editGizmoSize = length(editGizmo.m_position - g_camera_target);
	Vector3D editGizmoDir = normalize(editGizmo.m_position - g_camera_target);
	int editGizmoInitAxes = 0;

	bool isFaceGizmo = (&editGizmo == &m_faceAxis);

	if (!event.Dragging())
	{
		// pick the box plane
		if ((isFaceGizmo && !m_selectedFaces.numElem() || m_selectedBrushes.numElem()) && !event.ButtonUp() && !event.ButtonDown())
		{
			Volume boxVolume;
			BoxToVolume(editBox, boxVolume);

			Vector3D intersectPos;
			int planeIdx = VolumeIntersectsRay(boxVolume, ray_start, ray_dir, intersectPos);
			if (planeIdx != -1)
			{
				m_draggablePlane = planeIdx;

				const Plane& pl = boxVolume.GetPlane(planeIdx);

				Vector3D r, u;
				VectorVectors(pl.normal, r, u);

				m_faceAxis.SetProps(!Matrix3x3(-r, -u, -pl.normal), editBox.GetCenter() - pl.normal*pl.Distance(editBox.GetCenter()));
			}
		}

		// if left button down, try init gizmo axis
		if (event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			bool multipleAxes = !isFaceGizmo && (m_mode == BLOCK_MODE_TRANSLATE);
			editGizmoInitAxes = editGizmo.TestRay(ray_start, ray_dir, editGizmoSize, multipleAxes);

			// if face gizmo dragged - turn it to scale mode
			if (isFaceGizmo && editGizmoInitAxes == AXIS_Z && (m_mode != BLOCK_MODE_ROTATE))
			{
				m_mode = BLOCK_MODE_SCALE;
			}
		}
	}

	if (m_mode == BLOCK_MODE_TRANSLATE || m_mode == BLOCK_MODE_SCALE)
	{
		Vector3D movement = editGizmo.PerformTranslate(ray_start, ray_dir, editGizmoDir, editGizmoInitAxes);
		m_dragOffs += movement;

		editGizmo.m_position += movement;
	}
	else if (m_mode == BLOCK_MODE_ROTATE)
	{
		m_dragRot = editGizmo.PerformRotation(ray_start, ray_dir, editGizmoInitAxes);

		if (!event.Dragging() && !event.ButtonUp())
			m_dragInitRot = m_dragRot;
	}

	// if button was released
	if (event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
		if (m_mode == BLOCK_MODE_TRANSLATE)
		{
			Vector3D dragOfs = SnapVector(GridSize(), m_dragOffs);

			if (m_selectedTool == BlockEdit_Brush)
			{
				//------------------------------------------------------
				// MODIFY CREATION BOX

				editBox.minPoint += dragOfs;
				editBox.maxPoint += dragOfs;
				m_mode = BLOCK_MODE_CREATION;
			}
			else
			{
				//------------------------------------------------------
				// PERFORM MOVEMENT OF SELECTION

				for (int i = 0; i < m_selectedBrushes.numElem(); i++)
				{
					CBrushPrimitive* brush = m_selectedBrushes[i];
					brush->Transform(translate(dragOfs));
				}

				RecalcSelectionBox();
				m_mode = BLOCK_MODE_READY;
			}
		}
		if (m_mode == BLOCK_MODE_ROTATE)
		{
			Matrix3x3 dragRotation = !(!m_dragInitRot*m_dragRot);

			// snap rotation by 15 degrees
			{
				Vector3D dragEulers = EulerMatrixXYZ(dragRotation);
				Vector3D snappedDragEulers = SnapVector(15.0f, VRAD2DEG(dragEulers));

				dragRotation = rotateXYZ3(DEG2RAD(snappedDragEulers.x), DEG2RAD(snappedDragEulers.y), DEG2RAD(snappedDragEulers.z));
			}

			if (isFaceGizmo)
			{
				//------------------------------------------------------
				// PERFORM ROTATION OF WINDING

				winding_t* selWinding = m_selectedFaces[0].winding;

				const Plane& pl = selWinding->face.Plane;
				CBrushPrimitive* brush = selWinding->brush;

				const Vector3D faceOrigin = selWinding->GetCenter();
				const Vector3D faceOriginToSelection = (faceOrigin - m_faceAxis.m_position);
				const Vector3D faceOriginTransformed = (dragRotation*faceOriginToSelection) + m_faceAxis.m_position;

				Matrix4x4 rendermatrix = (translate(faceOriginTransformed)*(Matrix4x4(dragRotation))*translate(-faceOrigin));

				selWinding->Transform(rendermatrix);

				if (!selWinding->brush->UpdateRenderData())
				{
					DeleteBrush(selWinding->brush);
					wxMessageBox("Brush was deleted!");
				}

				Vector3D r, u;
				VectorVectors(pl.normal, r, u);
				m_faceAxis.SetProps(!Matrix3x3(r, u, pl.normal), selWinding->GetCenter());

				RecalcSelectionBox();
				m_mode = BLOCK_MODE_READY;
			}
			else
			{
				//------------------------------------------------------
				// PERFORM ROTATION OF SELECTION

				for (int i = 0; i < m_selectedBrushes.numElem(); i++)
				{
					CBrushPrimitive* brush = m_selectedBrushes[i];

					const Vector3D brushOrigin = brush->GetBBox().GetCenter();
					const Vector3D brushOriginToSelection = (brushOrigin - editBox.GetCenter());
					const Vector3D brushOriginTransformed = (dragRotation*brushOriginToSelection) + editBox.GetCenter();

					Matrix4x4 rendermatrix = (translate(brushOriginTransformed)*(Matrix4x4(dragRotation))*translate(-brushOrigin));

					brush->Transform(rendermatrix);
				}
			}

			RecalcSelectionBox();
			m_mode = BLOCK_MODE_READY;
		}
		else if (m_mode == BLOCK_MODE_SCALE)
		{
			// scale the editing box
			Volume boxVolume;
			BoxToVolume(editBox, boxVolume);

			// modify plane of box
			if (m_draggablePlane != -1)
			{
				Plane pl = boxVolume.GetPlane(m_draggablePlane);

				Vector3D dragOfs = SnapVector(GridSize(), m_dragOffs);
				pl.offset -= dot(pl.normal, dragOfs);

				boxVolume.SetupPlane(pl, m_draggablePlane);
			}

			// convert volume to box back; It will be used to calculate the final scale from difference
			BoundingBox selectionBox;
			VolumeToBox(boxVolume, selectionBox);

			if (m_selectedTool == BlockEdit_Brush)
			{
				//------------------------------------------------------
				// MODIFY CREATION BOX
				editBox = selectionBox;
				m_mode = BLOCK_MODE_CREATION;
			}
			else if (m_selectedFaces.numElem() == 1)
			{
				//------------------------------------------------------
				// MOVE WINDING

				winding_t* selWinding = m_selectedFaces[0].winding;

				Plane& pl = selWinding->face.Plane;
				float distFromPl = SnapFloat(GridSize(), pl.Distance(m_faceAxis.m_position));

				pl.offset -= distFromPl;

				if (!selWinding->brush->UpdateRenderData())
				{
					DeleteBrush(selWinding->brush);
					wxMessageBox("Brush was deleted!");
				}

				RecalcSelectionBox();
				m_mode = BLOCK_MODE_READY;
			}
			else
			{
				//------------------------------------------------------
				// PERFORM SCALE OF SELECTION

				// calculate diffs
				const Vector3D prev_box_size = editBox.GetSize();
				const Vector3D curr_box_size = selectionBox.GetSize();
				const Vector3D scale_factor = curr_box_size / prev_box_size;
				const Vector3D box_center_offset = (selectionBox.GetCenter() - editBox.GetCenter()) / scale_factor;

				// scale up them all using bounding box
				for (int i = 0; i < m_selectedBrushes.numElem(); i++)
				{
					CBrushPrimitive* brush = m_selectedBrushes[i];

					const Vector3D brushOrigin = brush->GetBBox().GetCenter();

					const Vector3D brushOriginToSelection = (brushOrigin - editBox.GetCenter());

					const Vector3D brushOriginTransformed = brushOriginToSelection * scale_factor + selectionBox.GetCenter();

					Matrix4x4 scaleMat = scale4(scale_factor.x, scale_factor.y, scale_factor.z);
					Matrix4x4 rendermatrix = (translate(brushOriginTransformed)*(scaleMat)*translate(-brushOrigin));

					brush->Transform(rendermatrix);
				}

				RecalcSelectionBox();
				m_mode = BLOCK_MODE_READY;
			}
		}

		g_pEditorActionObserver->EndAction();

		editGizmo.EndDrag();
		m_dragOffs = vec3_zero;
		m_dragRot = m_dragInitRot = identity3();
	}

	if (event.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_mode != BLOCK_MODE_READY && m_mode != BLOCK_MODE_CREATION)
		return false;

	return true;
}

bool CUI_BlockEditor::ProcessVertexManipMouseEvents(wxMouseEvent& event)
{
	return true;
}

void CUI_BlockEditor::OnKey(wxKeyEvent& event, bool bDown)
{
	if (bDown)
		return;

	if (event.GetRawKeyCode() >= '1' && event.GetRawKeyCode() <= '9')
	{
		int idx = event.GetRawKeyCode() - '1';

		if(idx < s_numGridSizes)
			m_gridSize->Select(idx);
	}

	if ((m_selectedBrushes.numElem() || m_selectedFaces.numElem() == 1) && (m_mode == BLOCK_MODE_READY || m_mode == BLOCK_MODE_CREATION))
	{
		if (event.GetRawKeyCode() == 'G')
		{
			m_mode = BLOCK_MODE_TRANSLATE;
		}
		else if (event.GetRawKeyCode() == 'R' && m_mode != BLOCK_MODE_CREATION)
		{
			m_mode = BLOCK_MODE_ROTATE;
		}
		else if (event.GetKeyCode() == WXK_DELETE)
		{
			DeleteSelection();
		}
	}

	if (m_mode == BLOCK_MODE_CREATION)
	{
		if (event.GetKeyCode() == WXK_RETURN)
		{
			if (!m_texPanel->GetSelectedMaterial())
			{
				wxMessageBox("You need to select material first");
				return;
			}

			if (m_selectedTool == BlockEdit_Brush)
			{
				// Make a brush
				Volume vol;
				BoxToVolume(m_creationBox, vol);

				CBrushPrimitive* brush = CreateBrushFromVolume(vol, m_texPanel->GetSelectedMaterial());
				m_brushes.append(brush);
			}
			else if (m_selectedTool == BlockEdit_Polygon)
			{
				// make a polygon
			}

			m_mode = BLOCK_MODE_READY;
		}
	}

	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		CancelSelection();
	}
}

void RenderWindingVerts(faceSelect_t& faceSel, const Matrix4x4& view, const Matrix4x4& proj)
{
	IVector2D screenSize = g_pMainFrame->GetRenderPanelDimensions();

	for (int i = 0; i < faceSel.winding->vertices.numElem(); i++)
	{
		lmodeldrawvertex_t& vert = faceSel.winding->vertices[i];

		ColorRGBA vertexCol(1, 1, 0.5f, 1);
		if (faceSel.selVerts.findIndex(i) != -1)
			vertexCol = ColorRGBA(1, 0, 0, 1);

		Vector2D pointScr;
		if (!PointToScreen(vert.position, pointScr, proj*view, screenSize))
		{
			// draw only single point
			Vertex2D_t pointA[] = { MAKETEXQUAD(pointScr.x - 3,pointScr.y - 3,pointScr.x + 3,pointScr.y + 3, 0) };

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, vertexCol);

			// draw only single point
			Vertex2D_t pointR[] = { MAKETEXRECT(pointScr.x - 3,pointScr.y - 3,pointScr.x + 3,pointScr.y + 3, 0) };
			materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0, 0, 0, 1));
		}
	}
}

void CUI_BlockEditor::RenderSelectedWindings()
{
	Matrix4x4 view, proj;
	materials->GetMatrix(MATRIXMODE_PROJECTION, proj);
	materials->GetMatrix(MATRIXMODE_VIEW, view);

	IVector2D screenSize = g_pMainFrame->GetRenderPanelDimensions();
	materials->Setup2D(screenSize.x, screenSize.y);

	// TODO: render only shared vertices

	for (int i = 0; i < m_selectedFaces.numElem(); i++)
	{
		RenderWindingVerts(m_selectedFaces[i], view, proj);
	}

	materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
	materials->SetMatrix(MATRIXMODE_VIEW, view);
}

void CUI_BlockEditor::OnRender()
{
	debugoverlay->Text(color4_white, "BLOCK AXIS: %s", (m_selectedFaces.numElem() == 1 || m_mode == BLOCK_MODE_READY || m_mode == BLOCK_MODE_CREATION || m_mode == BLOCK_MODE_SCALE) ? "face" : "center");

	if (m_selectedTool == BlockEdit_Brush)
	{
		debugoverlay->Text(color4_white, "EDITING: %s", m_mode == BLOCK_MODE_CREATION ? "box" : "none");
	}
	else if (m_selectedTool == BlockEdit_Selection)
	{
		if (m_selectedBrushes.numElem())
		{
			debugoverlay->Text(color4_white, "EDITING: %s", "selected brushes");
		}
		else
		{
			debugoverlay->Text(color4_white, "EDITING: %s", m_selectedFaces.numElem() == 1 ? "face" : "none");
		}
	}
	
	if (m_mode == BLOCK_MODE_READY)
	{
		debugoverlay->Text(color4_white, "mode: ready");
	}
	else if (m_mode == BLOCK_MODE_CREATION)
	{
		debugoverlay->Text(color4_white, "mode: creation");
	}
	else if (m_mode == BLOCK_MODE_TRANSLATE)
	{
		debugoverlay->Text(color4_white, "mode: translate");
	}
	else if (m_mode == BLOCK_MODE_ROTATE)
	{
		debugoverlay->Text(color4_white, "mode: rotate");
	}
	else if (m_mode == BLOCK_MODE_SCALE)
	{
		debugoverlay->Text(color4_white, "mode: scale");
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	CBaseTilebasedEditor::OnRender();

	if (m_selectedTool == BlockEdit_Brush ||
		m_selectedTool == BlockEdit_Polygon)
	{
		debugoverlay->Sphere3D(m_cursorPos, 0.15f, ColorRGBA(1, 1, 0, 1), 0.0f);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// TODO: draw region brushes
	{
		ColorRGBA ambColor = materials->GetAmbientColor();

		for (int i = 0; i < m_brushes.numElem(); i++)
		{
			CBrushPrimitive* brush = m_brushes[i];
			brush->Render(0);

			materials->SetAmbientColor(ambColor);
		}
	}

	Vector3D dragTranslation = SnapVector(GridSize(), m_dragOffs);
	Matrix3x3 dragRotation = !(!m_dragInitRot*m_dragRot);

	// snap rotation by 15 degrees
	{
		Vector3D dragEulers = EulerMatrixXYZ(dragRotation);
		Vector3D snappedDragEulers = SnapVector(15.0f, VRAD2DEG(dragEulers));

		dragRotation = rotateXYZ3(DEG2RAD(snappedDragEulers.x), DEG2RAD(snappedDragEulers.y), DEG2RAD(snappedDragEulers.z));
	}

	// Draw selection
	{
		float clength = length(m_centerAxis.m_position - g_camera_target);
		float flength = length(m_faceAxis.m_position - g_camera_target);

		// this selection box that can be modified
		BoundingBox selectionBox = m_selectionBox;

		Volume selectionBoxVolume;
		BoxToVolume(selectionBox, selectionBoxVolume);

		if (m_mode == BLOCK_MODE_READY)
		{

		}
		else if (m_mode == BLOCK_MODE_SCALE)
		{
			// modify the plane
			Plane pl = selectionBoxVolume.GetPlane(m_draggablePlane);

			// move the dragged plane
			pl.offset -= dot(pl.normal, dragTranslation);
			selectionBoxVolume.SetupPlane(pl, m_draggablePlane);

			// convert volume to box back; It will be used to calculate the final scale from difference
			VolumeToBox(selectionBoxVolume, selectionBox);
		}

		const Vector3D prev_box_size = m_selectionBox.GetSize();
		const Vector3D curr_box_size = selectionBox.GetSize();
		const Vector3D scale_factor = curr_box_size / prev_box_size;
		const Vector3D box_center_offset = (selectionBox.GetCenter() - m_selectionBox.GetCenter()) / scale_factor;

		if (m_mode == BLOCK_MODE_TRANSLATE)
			materials->SetMatrix(MATRIXMODE_WORLD, translate(dragTranslation.x, dragTranslation.y, dragTranslation.z));

		for (int i = 0; i < m_selectedBrushes.numElem(); i++)
		{
			CBrushPrimitive* brush = m_selectedBrushes[i];

			if (m_mode == BLOCK_MODE_SCALE)
			{
				const Vector3D brushOrigin = brush->GetBBox().GetCenter();
				const Vector3D brushOriginToSelection = (brushOrigin - m_selectionBox.GetCenter());
				const Vector3D brushOriginTransformed = brushOriginToSelection*scale_factor + selectionBox.GetCenter();

				Matrix4x4 scaleMat = scale4(scale_factor.x, scale_factor.y, scale_factor.z);
				Matrix4x4 rendermatrix = (translate(brushOriginTransformed)*(scaleMat)*translate(-brushOrigin));

				materials->SetMatrix(MATRIXMODE_WORLD, rendermatrix);
			}
			else if (m_mode == BLOCK_MODE_ROTATE)
			{
				const Vector3D brushOrigin = brush->GetBBox().GetCenter();
				const Vector3D brushOriginToSelection = (brushOrigin - m_selectionBox.GetCenter());
				const Vector3D brushOriginTransformed = (dragRotation*brushOriginToSelection) + selectionBox.GetCenter();

				Matrix4x4 rendermatrix = (translate(brushOriginTransformed)*(Matrix4x4(dragRotation))*translate(-brushOrigin));

				materials->SetMatrix(MATRIXMODE_WORLD, rendermatrix);
			}
			
			brush->RenderGhost();
		}

		if (m_selectedTool == BlockEdit_VertexManip)
		{
			RenderSelectedWindings();
		}

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		if (m_selectedTool == BlockEdit_Selection)
		{
			if (m_selectedBrushes.numElem())
			{
				// draw selection box
				debugoverlay->Box3D(selectionBox.minPoint, selectionBox.maxPoint, ColorRGBA(1, 1, 0, 1), 0.0f);

				if (m_mode == BLOCK_MODE_TRANSLATE || m_mode == BLOCK_MODE_ROTATE)
					m_centerAxis.Draw(clength);
				else if (m_draggablePlane != -1)
					m_faceAxis.Draw(flength, AXIS_Z);
			}
			else if (m_selectedFaces.numElem() == 1)
			{
				winding_t* selWinding = m_selectedFaces[0].winding;

				CBrushPrimitive* brush = selWinding->brush;
				const Plane& pl = selWinding->face.Plane;

				if (m_mode == BLOCK_MODE_ROTATE)
				{
					const Vector3D faceOrigin = selWinding->GetCenter();
					const Vector3D faceOriginToSelection = (faceOrigin - m_faceAxis.m_position);
					const Vector3D faceOriginTransformed = (dragRotation*faceOriginToSelection) + m_faceAxis.m_position;

					Matrix4x4 rendermatrix = (translate(faceOriginTransformed)*(Matrix4x4(dragRotation))*translate(-faceOrigin));
					materials->SetMatrix(MATRIXMODE_WORLD, rendermatrix);
				}
				else
				{
					float distFromPl = SnapFloat(GridSize(), pl.Distance(m_faceAxis.m_position));

					materials->SetMatrix(MATRIXMODE_WORLD, translate(pl.normal*distFromPl));
				}

				brush->RenderGhost(selWinding->faceId);

				materials->SetMatrix(MATRIXMODE_WORLD, identity4());

				if (m_selectedTool == BlockEdit_Selection)
					m_faceAxis.Draw(flength, (m_mode == BLOCK_MODE_ROTATE) ? AXIS_ALL : AXIS_Z);
			}
		}	
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if (m_selectedTool == BlockEdit_Brush)
	{
		if (m_mode >= BLOCK_MODE_CREATION)
		{
			float clength = length(m_centerAxis.m_position - g_camera_target);
			float flength = length(m_faceAxis.m_position - g_camera_target);

			BoundingBox creationBox = m_creationBox;

			Volume creationBoxVolume;
			BoxToVolume(creationBox, creationBoxVolume);

			if (m_mode == BLOCK_MODE_SCALE)
			{
				// modify the plane
				Plane pl = creationBoxVolume.GetPlane(m_draggablePlane);

				// move the dragged plane
				pl.offset -= dot(pl.normal, dragTranslation);
				creationBoxVolume.SetupPlane(pl, m_draggablePlane);

				// convert volume to box back; It will be used to calculate the final scale from difference
				VolumeToBox(creationBoxVolume, creationBox);
			}
			else if (m_mode == BLOCK_MODE_TRANSLATE)
			{
				creationBox.minPoint += dragTranslation;
				creationBox.maxPoint += dragTranslation;
			}

			debugoverlay->Box3D(creationBox.minPoint, creationBox.maxPoint, ColorRGBA(1, 1, 1, 1), 0.0f);

			if(m_mode == BLOCK_MODE_TRANSLATE)
				m_centerAxis.Draw(clength);
			else if (m_draggablePlane != -1)
				m_faceAxis.Draw(flength, AXIS_Z);
		}
	}
}

void CUI_BlockEditor::InitTool()
{
	m_texPanel->ReloadMaterialList(false);
}

void CUI_BlockEditor::OnLevelUnload()
{
	m_texPanel->SelectMaterial(NULL, 0);
	CBaseTilebasedEditor::OnLevelUnload();
}

void CUI_BlockEditor::Update_Refresh()
{
	m_texPanel->Redraw();
}