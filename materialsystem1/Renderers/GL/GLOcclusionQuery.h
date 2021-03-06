//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLOCCLUSIONQUERY_H
#define GLOCCLUSIONQUERY_H

#include "renderers/IOcclusionQuery.h"

class CGLOcclusionQuery : public IOcclusionQuery
{
public:
						CGLOcclusionQuery();
						~CGLOcclusionQuery();

	// begins the occlusion query issue
	void				Begin();

	// ends the occlusion query issue
	void				End();

	// returns status
	bool				IsReady();

	// after End() and IsReady() == true it does matter
	uint32				GetVisiblePixels();

protected:

	uint				m_query;
	uint				m_pixelsVisible;
	bool				m_ready;
};

#endif // GLOCCLUSIONQUERY_H