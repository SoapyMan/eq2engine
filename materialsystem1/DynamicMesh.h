//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#ifndef DYNAMICMESH_H
#define DYNAMICMESH_H

#include "materialsystem1/IDynamicMesh.h"
#include "materialsystem1/renderers/IShaderAPI.h"

class CDynamicMesh : public IDynamicMesh
{
public:
	CDynamicMesh();
	~CDynamicMesh();

	bool			Init( VertexFormatDesc_t* desc, int numAttribs );
	void			Destroy();

	// sets the primitive type (chooses the way how to allocate geometry parts)
	void			SetPrimitiveType( ER_PrimitiveType primType );
	ER_PrimitiveType	GetPrimitiveType() const;

	// returns a pointer to vertex format description
	void			GetVertexFormatDesc(VertexFormatDesc_t** desc, int& numAttribs);

	// allocates geometry chunk. Returns the start index. Will return -1 if failed
	// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
	int				AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak = true );

	// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
	void			Render();

	// resets the dynamic mesh
	void			Reset();

	void			AddStripBreak();

protected:

	bool			Lock();
	void			Unlock();

	ER_PrimitiveType	m_primType;

	void*			m_vertices;
	uint16*			m_indices;

	uint16			m_numVertices;
	uint16			m_numIndices;

	void*			m_lockVertices;
	uint16*			m_lockIndices;

	IVertexFormat*	m_vertexFormat;
	IVertexBuffer*	m_vertexBuffer;
	IIndexBuffer*	m_indexBuffer;

	int				m_vertexStride;

	int				m_vboAqquired;
	int				m_vboDirty;
};

#endif // DYNAMICMESH_H
