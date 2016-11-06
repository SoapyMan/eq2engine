//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVERTEXFORMAT_H
#define IVERTEXFORMAT_H


#include "platform/Platform.h"
#include "ShaderAPI_defs.h"
#include "ppmem.h"

class IVertexFormat
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual	~IVertexFormat() {}


	virtual int		GetVertexSize(int stream) = 0;
	virtual void	GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) = 0;
};

#endif // IVERTEXFORMAT_H
