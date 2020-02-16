//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQBRUSH_H
#define EQBRUSH_H

#include "BrushFace.h"

#include "materialsystem/IMaterialSystem.h"
#include "math/Volume.h"
#include "math/BoundingBox.h"
#include "utils/DkLinkedList.h"

#include "world.h"

enum ClassifyPoly_e
{
	CPL_FRONT = 0,
	CPL_SPLIT,
	CPL_BACK,
	CPL_ONPLANE,
};

class CBrushPrimitive;
struct winding_t;

inline void CopyWinding(const winding_t *from, winding_t *to);

struct winding_t
{
	// calculates the texture coordinates for this 
	void							CalculateTextureCoordinates();

	// sorts the vertices, makes them as triangle list
	bool							SortVerticesAsTriangleList();

	// splits the face by this face, and results a
	void							Split(const winding_t *w, winding_t *front, winding_t *back );

	// classifies the next face over this
	ClassifyPoly_e					Classify(winding_t *w);

	CBrushPrimitive*					pAssignedBrush;

	// assingned face
	brushFace_t*					pAssignedFace;

	// vertex data
	DkList<lmodeldrawvertex_t>			vertices;

	// make valid assignment
	winding_t & operator = (const winding_t &u)
	{
		if(this != &u)
			CopyWinding(&u, this);

		return *this;
	}
};

inline void CopyWinding(const winding_t *from, winding_t *to)
{
	to->pAssignedBrush = from->pAssignedBrush;
	to->pAssignedFace = from->pAssignedFace;

	for(int i = 0; i < from->vertices.numElem(); i++)
		to->vertices.append(from->vertices[i]);
}

void MakeInfiniteWinding(winding_t &w, Plane &plane);

// editable brush class
class CBrushPrimitive
{
public:
	CBrushPrimitive();
	~CBrushPrimitive();

// CBaseEditableObject members

	void							OnRemove(bool bOnLevelCleanup = false);

	// draw brush
	void							Render(int nViewRenderFlags);
	void							RenderGhost();

	// rendering bbox
	Vector3D						GetBBoxMins()	{return m_bbox.minPoint;}
	Vector3D						GetBBoxMaxs()	{return m_bbox.maxPoint;}

	float							CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos, int& face);

	// updates texturing
	void							UpdateSurfaceTextures();

	// saves this object
	bool							WriteObject(IVirtualStream*	pStream);

	// read this object
	bool							ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void							SaveToKeyValues(kvkeybase_t* pSection);

	// stores object in keyvalues
	bool							LoadFromKeyValues(kvkeybase_t* pSection);

	// called when whole level builds
//	void							BuildObject(level_build_data_t* pLevelBuildData);

// brush members

	// calculates bounding box for this brush
	void							CalculateBBOX();

	// is brush aabb intersects another brush aabb
	bool							IsBrushIntersectsAABB(CBrushPrimitive *pBrush);
	bool							IsPointInside_Epsilon(Vector3D &point, float eps);
	bool							IsPointInside(Vector3D &point);
	bool							IsWindingFullyInsideBrush(winding_t* pWinding);
	bool							IsWindingIntersectsBrush(winding_t* pWinding);
	bool							IsTouchesBrush(winding_t* pWinding);

	int								GetFaceCount() const				{return m_faces.numElem();}
	brushFace_t*					GetFace(int nFace) const			{return (brushFace_t*)&m_faces[nFace];}
	winding_t*						GetFacePolygon(int nFace) const		{return (winding_t*)&m_polygons[nFace];}

	void							UpdateRenderData();
	void							UpdateRenderBuffer();

	void							AddFace(brushFace_t &face);

	void							CutBrushByPlane(Plane &plane, CBrushPrimitive** ppNewBrush);

	void							RemoveEmptyFaces();

	// calculates the vertices from faces
	bool							CreateFromPlanes();

	// copies this object
	CBrushPrimitive*				CloneObject();

	void							Translate(const Vector3D& offset);
	void							Scale(const Vector3D &scale, bool use_center, const Vector3D &scale_center);

protected:

	// sort to draw
	void							SortVertsToDraw();

	BoundingBox						m_bbox;
	DkList<brushFace_t>				m_faces;
	DkList<winding_t>				m_polygons;

	IVertexBuffer*					m_pVB;

	int								m_nAdditionalFlags;
};

// creates a brush from volume, e.g a selection box
CBrushPrimitive* CreateBrushFromVolume(const Volume& volume, IMaterial* material);


#endif // EQBRUSH_H