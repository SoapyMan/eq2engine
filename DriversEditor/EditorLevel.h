//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level data
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORLEVEL_H
#define EDITORLEVEL_H

#include "math/Vector.h"
#include "TextureAtlas.h"
#include "EditorPreviewable.h"
#include "utils/DkLinkedList.h"

#include "level_generator.h"
#include "region.h"
#include "Level.h"

class CLevelModel;
class IMaterial;
class IVirtualStream;
struct kvkeybase_t;

class CLayerModel : public CEditorPreviewable
{
public:
	PPMEM_MANAGED_OBJECT()

	CLayerModel();
	~CLayerModel();

	void			RefreshPreview();

	CLevelModel*	m_model;
	EqString		m_name;
};

struct buildLayer_t
{
	~buildLayer_t();

	// floor variants
	DkList<CLayerModel*>	models;
};

struct buildLayerColl_t
{
	void					Save(IVirtualStream* stream, kvkeybase_t* kvs);
	void					Load(IVirtualStream* stream, kvkeybase_t* kvs);

	EqString				name;
	DkList<buildLayer_t>	layers;
};

struct buildSegmentPoint_t
{
	FVector3D	position;
	int			layerId;
	int			modelId;
	float		scale;
};

struct buildingSource_t
{
	buildingSource_t()
	{
		layerColl = NULL;
		order = 1;
		model = NULL;
		modelPosition = vec3_zero;
		hide = false;
	}

	buildingSource_t(buildingSource_t& copyFrom);
	~buildingSource_t();

	void InitFrom(buildingSource_t& copyFrom);

	void ToKeyValues(kvkeybase_t* kvs);
	void FromKeyValues(kvkeybase_t* kvs);

	EqString							loadBuildingName;

	DkLinkedList<buildSegmentPoint_t>	points;
	buildLayerColl_t*					layerColl; 
	int									order;

	CLevelModel*						model;
	Vector3D							modelPosition;
	int									regObjectId;	// region object index

	bool								hide;
};

struct outlineDef_t
{
	EqString							name;

	DkLinkedList<buildSegmentPoint_t>	points;
	bool								hide;
};

int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, float layerXSize);
float GetSegmentLength(buildLayer_t& layer, int modelId = 0);

void CalculateBuildingSegmentTransform(	Matrix4x4& partTransform, 
										buildLayer_t& layer, 
										const Vector3D& startPoint, 
										const Vector3D& endPoint, 
										int order, 
										Vector3D& size, float scale,
										int iteration );

// Rendering the building
void RenderBuilding( buildingSource_t* building, buildSegmentPoint_t* extraSegment );

// Generates new levelmodel of building
// Returns local-positioned model, and it's position in the world
bool GenerateBuildingModel( buildingSource_t* building );

//-----------------------------------------------------------------------------------

enum EPrefabCreationFlags
{
	PREFAB_HEIGHTFIELDS		= (1 << 0),
	PREFAB_OBJECTS			= (1 << 1),			// including buildings
	PREFAB_ROADS			= (1 << 2),
	PREFAB_OCCLUDERS		= (1 << 3),

	PREFAB_ALL	 = 0xFFFFFFFF,
};

class CEditorLevel : public CGameLevel, public IMaterialRenderParamCallbacks
{
	friend class CEditorLevelRegion;
public:

	bool			Load(const char* levelname, kvkeybase_t* kvDefs);
	bool			Save(const char* levelname, bool isfinal = false);

	bool			LoadPrefab(const char* prefabName);
	bool			SavePrefab(const char* prefabName);

	void			Ed_InitPhysics();
	void			Ed_DestroyPhysics();

	void			OnPreApplyMaterial( IMaterial* pMaterial );

	void			Ed_Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj);

	void			Ed_Prerender(const Vector3D& cameraPosition);

	int				Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist);
	int				Ed_SelectBuildingAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist);

	bool			Ed_GenerateMap(LevelGenParams_t& genParams, const CImage* img);

	// moves object to new region if possible
	CEditorLevelRegion*	Ed_MakeObjectRegionValid(regionObject_t* obj, CLevelRegion* itsRegion);

	void			WriteLevelRegions(IVirtualStream* stream, bool isFinal);
	void			WriteObjectDefsLump(IVirtualStream* stream);
	void			WriteHeightfieldsLump(IVirtualStream* stream);

	void			SaveEditorBuildings( const char* levelName );
	void			LoadEditorBuildings( const char* levelName );

	void			PostLoadEditorBuildings( DkList<buildLayerColl_t*>& buildingTemplates );

	CEditorLevel*	CreatePrefab(const IVector2D& minCell, const IVector2D& maxCell, int flags /*EPrefabCreationFlags*/);
	void			PlacePrefab(const IVector2D& position, int height, int rotation, CEditorLevel* prefab, int flags /*EPrefabCreationFlags*/);
protected:

	void			PrefabHeightfields(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const IVector2D& regionMinCell, const IVector2D& regionMaxCell, bool cloneRoads);
	void			PrefabObjects(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const BoundingBox& prefabBounds);
	void			PrefabOccluders(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const BoundingBox& prefabBounds);
	void			PrefabRoads(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const IVector2D& regionMinCell, const IVector2D& regionMaxCell);

	void			PlacePrefabHeightfields(const IVector2D& globalTile, int height, int rotation, CEditorLevel* prefab, int regionIdx);
};

//-----------------------------------------------------------------------------------

class CEditorLevelRegion : public CLevelRegion
{
	friend class CEditorLevel;
public:

	CEditorLevelRegion();

	void						Cleanup();

	void						Ed_Prerender();

	void						Ed_InitPhysics();
	void						Ed_DestroyPhysics();

	void						WriteRegionData(IVirtualStream* stream, DkList<CLevObjectDef*>& models, bool isFinal);
	void						WriteRegionOccluders(IVirtualStream* stream);
	void						WriteRegionRoads(IVirtualStream* stream);
		
	void						ReadRegionBuildings( IVirtualStream* stream );
	void						WriteRegionBuildings( IVirtualStream* stream );

	void						ClearRoadTrafficLightStates();
	void						PostprocessCellObject(regionObject_t* obj);

	int							Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist);
	int							Ed_SelectBuilding(const Vector3D& start, const Vector3D& dir, float& dist);

	int							Ed_ReplaceDefs(CLevObjectDef* whichReplace, CLevObjectDef* replaceTo);
	void						Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	int							GetLowestTile() const;
	int							GetHighestTile() const;

	bool						m_modified; // needs saving?

	bool						m_physicsPreview;

	DkList<buildingSource_t*>	m_buildings;
};

#endif // EDITORLEVEL_H
