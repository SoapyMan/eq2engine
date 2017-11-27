//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//
// TODO:	rewrite some parts, add geometry rendering
//			fix line rendering perfomance
//			add geometry drawer
//////////////////////////////////////////////////////////////////////////////////

#ifndef IDEBUG_TEXT
#define IDEBUG_TEXT

#include "InterfaceManager.h"

#include "math/Vector.h"
#include "math/Matrix.h"
#include "utils/DkLinkedList.h"

#define DBGOVERLAY_INTERFACE_VERSION "DebugOverlay_001"

#ifdef ENGINE_EXPORT
#	define ENGINE_EXPORTS		extern "C" __declspec(dllexport)
#else
#	define ENGINE_EXPORTS		extern "C" __declspec(dllimport)
#endif

class IEqFont;

struct debugGraphBucket_t
{
	char					pszName[256];
	ColorRGBA				color;
	float					fMaxValue;

	float					update_time;
	float					remaining_update_time;

	bool					isDynamic;

	DkLinkedList<float>		points;
};

typedef void (*OnDebugDrawFn)( void* );

class IDebugOverlay
{
public:
	virtual IEqFont*			GetFont() = 0;

	virtual void				Init() = 0;

	virtual void				Text(const ColorRGBA &color, char const *fmt,...) = 0;
	virtual void				TextFadeOut(int position, const ColorRGBA &color, float fFadeTime, char const *fmt,...) = 0;

	virtual void				Text3D(const Vector3D &origin, float distance, const ColorRGBA &color, float fTime, char const *fmt,...) = 0;

	virtual void				Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f) = 0;
	virtual void				Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color, float fTime = 0.0f) = 0;
	//virtual void				OrientedBox3D(const Vector3D &mins, const Vector3D &maxs, Matrix4x4& transform, const ColorRGBA &color, float fTime = 0.0f) = 0;
	virtual void				Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime = 0.0f) = 0;
	virtual void				Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f) = 0;

	virtual void				Draw3DFunc( OnDebugDrawFn func, void* args = NULL ) = 0;

	//virtual void				Line2D(Vector2D &start, Vector2D &end, ColorRGBA &color1, ColorRGBA &color2, float fTime = 0.0f) = 0;
	//virtual void				Box2D(Vector2D &mins, Vector2D &maxs, ColorRGBA &color, float fTime = 0.0f) = 0;

	virtual debugGraphBucket_t*	Graph_AddBucket( const char* pszName, const ColorRGBA &color, float fMaxValue, float fUpdateTime = 0.0f) = 0;
	virtual void				Graph_RemoveBucket(debugGraphBucket_t* pBucket) = 0;
	virtual void				Graph_AddValue( debugGraphBucket_t* pBucket, float value) = 0;
	virtual debugGraphBucket_t*	Graph_FindBucket(const char* pszName) = 0;

	virtual void				RemoveAllGraphs() = 0;

	virtual void				SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view ) = 0;
	virtual void				Draw(int winWide, int winTall) = 0;	// draws debug overlay.

	void						Draw(const Matrix4x4 &proj, const Matrix4x4 &view, int winWide, int winTall) { SetMatrices(proj, view); Draw(winWide, winTall); }
};

extern IDebugOverlay *debugoverlay;

#endif //IDEBUG_TEXT
