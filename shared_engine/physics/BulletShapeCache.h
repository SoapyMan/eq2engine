//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model cache for bullet physics
//				Generates real shapes for Bullet Collision
// 
///////////////////////////////////////////////////////////////////////////////////

#ifndef BULLETSHAPECACHE_H
#define BULLETSHAPECACHE_H

#include "physics/IStudioShapeCache.h"
#include "utils/eqthread.h"

class btCollisionShape;

class CBulletStudioShapeCache : public IStudioShapeCache
{
public:
								CBulletStudioShapeCache();

	// checks the shape is initialized for the cache
	bool						IsShapeCachePresent( studioPhysShapeCache_t* shapeInfo );

	// initializes whole studio shape model with all objects
	void						InitStudioCache( studioPhysData_t* studioData );
	void						DestroyStudioCache( studioPhysData_t* studioData );

	// does all shape cleanup
	void						Cleanup_Invalidate();

protected:

	Threading::CEqMutex&		m_mutex;

	// cached shapes
	DkList<btCollisionShape*>	m_collisionShapes;
};

#endif