//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech Engine.dll spew to console functions
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "math/Vector.h"

#ifndef ENGINESPEW_H
#define ENGINESPEW_H

struct connode_t
{
	ColorRGBA	color;
	char*		text;
};

DkList<connode_t*> *GetAllMessages( void );

void InstallEngineSpewFunction();
void UninstallEngineSpewFunction();

void EngineSpewClear();

#endif // ENGINESPEW_H