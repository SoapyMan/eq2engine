//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATGL_H
#define VERTEXFORMATGL_H

#include "IVertexFormat.h"

#define MAX_GL_GENERIC_ATTRIB 24


//**************************************
// Vertex attribute descriptor
//**************************************

struct eqGLVertAttrDesc_t
{
	int					m_nStream;
	int					m_nSize;

	AttributeFormat_e	m_nFormat;
	int					m_nOffset;

};

//**************************************
// Vertex format
//**************************************

class CVertexFormatGL : public IVertexFormat
{
	friend class		ShaderAPIGL;
public:
	CVertexFormatGL(VertexFormatDesc_t* desc, int numAttribs);
	~CVertexFormatGL();

	int					GetVertexSize(int stream);
	void				GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs);

protected:
	int					m_streamStride[MAX_VERTEXSTREAM];
	VertexFormatDesc_t*	m_vertexDesc;
	int					m_numAttribs;

	eqGLVertAttrDesc_t	m_hGeneric[MAX_GL_GENERIC_ATTRIB];

#ifndef GL_NO_DEPRECATED_ATTRIBUTES
	eqGLVertAttrDesc_t	m_hTexCoord[MAX_TEXCOORD_ATTRIB];
	eqGLVertAttrDesc_t	m_hVertex;
	eqGLVertAttrDesc_t	m_hNormal;
	eqGLVertAttrDesc_t	m_hColor;
#endif // GL_NO_DEPRECATED_ATTRIBUTES

};

#endif // VERTEXFORMATGL_H