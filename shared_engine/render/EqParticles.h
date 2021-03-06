//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium particles renderer
//				A part of particle system
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQPARTICLES_H
#define EQPARTICLES_H

#include "materialsystem1/IMaterialSystem.h"
#include "BaseRenderableObject.h"
#include "SpriteBuilder.h"

#include "EffectRender.h"

#include "math/coord.h"

enum EPartRenderFlags
{
	EPRFLAG_DONT_FLUSHBUFFERS	= (1 << 24),
	EPRFLAG_INVERT_CULL			= (1 << 25),
};

// particle vertex with color
struct PFXVertex_t
{
	PFXVertex_t()
	{
	}

	PFXVertex_t(const Vector3D &p, const Vector2D &t, const ColorRGBA &c)
	{
		point = p;
		texcoord = t;
		color = c;
	}

	Vector3D		point;
	Vector2D		texcoord;
	Vector4D		color;

	//TVec3D<half>	normal;
	//half			unused;
};

static VertexFormatDesc_s g_PFXVertexFormatDesc[] = {
	{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT, "position" },		// position
	{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT, "texcoord" },		// texture coord
	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT, "color" },		// color
	//{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "normal" },		// normal; unused
};

//
// Particle renderer
//
// It's represented as renderable object
//
// You can derive it
#ifndef NO_ENGINE
class CParticleRenderGroup : public CSpriteBuilder<PFXVertex_t>, public CBaseRenderableObject
#else
class CParticleRenderGroup : public CSpriteBuilder<PFXVertex_t>
#endif
{
	friend class CParticleLowLevelRenderer;

public:
	PPMEM_MANAGED_OBJECT();

						CParticleRenderGroup();
			virtual		~CParticleRenderGroup();

	virtual void		Init( const char* pszMaterialName, bool bCreateOwnVBO = false, int maxQuads = 16384 );
	virtual void		Shutdown();

	//-------------------------------------------------------------------

	// min bbox dimensions
	void				GetBoundingBox(BoundingBox& outBox) {}

	// renders this buffer
	void				Render(int nViewRenderFlags, void* userdata);

	void				SetCustomProjectionMatrix(const Matrix4x4& mat);

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	// terminate with AddStripBreak();
	// this provides less copy operations
	int					AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices = false );

	void				AddParticleStrip(PFXVertex_t* verts, int nVertices);

	void				SetCullInverted(bool invert) {m_invertCull = invert;}
protected:

	IMaterial*			m_pMaterial;

	bool				m_useCustomProjMat;
	Matrix4x4			m_customProjMat;

	//IVertexBuffer*		m_vertexBuffer[PARTICLE_RENDER_BUFFERS];
	//IIndexBuffer*		m_indexBuffer[PARTICLE_RENDER_BUFFERS];
	//IVertexFormat*		m_vertexFormat;

	bool				m_invertCull;
};

//------------------------------------------------------------------------------------

class CPFXAtlasGroup : public CParticleRenderGroup
{
public:
	CPFXAtlasGroup();

	void				Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads = 16384 );
	void				Shutdown();

	TexAtlasEntry_t*	GetEntry(int idx);
	int					GetEntryIndex(TexAtlasEntry_t* entry) const;

	TexAtlasEntry_t*	FindEntry(const char* pszName) const;
	int					FindEntryIndex(const char* pszName) const;

	int					GetEntryCount() const;
};

//------------------------------------------------------------------------------------

class CParticleLowLevelRenderer
{
	friend class CParticleRenderGroup;
	friend class CShadowRenderer;

public:
	CParticleLowLevelRenderer();

	void							Init();
	void							Shutdown();

	bool							IsInitialized() {return m_initialized;}

	void							PreloadCache();
	void							ClearParticleCache();

	void							AddRenderGroup(CParticleRenderGroup* pRenderGroup, CParticleRenderGroup* after = nullptr);
	void							RemoveRenderGroup(CParticleRenderGroup* pRenderGroup);

	void							PreloadMaterials();

	// prepares render buffers and sends renderables to ViewRenderer
	void							Render(int nRenderFlags);
	void							ClearBuffers();

	// returns VBO index
	bool							MakeVBOFrom(CSpriteBuilder<PFXVertex_t>* pGroup);

	bool							InitBuffers();
	bool							ShutdownBuffers();

protected:

	Threading::CEqMutex&			m_mutex;

	DkList<CParticleRenderGroup*>	m_renderGroups;

	IVertexBuffer*					m_vertexBuffer;
	IIndexBuffer*					m_indexBuffer;
	IVertexFormat*					m_vertexFormat;

	int								m_vbMaxQuads;

	bool							m_initialized;
};

//------------------------------------------------------------------------------------

//-----------------------------------
// Effect elementary
//-----------------------------------

enum EffectFlags_e
{
	EFFECT_FLAG_LOCK_X				= (1 << 0),
	EFFECT_FLAG_LOCK_Y				= (1 << 1),
	EFFECT_FLAG_ALWAYS_VISIBLE		= (1 << 2),
	EFFECT_FLAG_NO_FRUSTUM_CHECK	= (1 << 3),
	EFFECT_FLAG_NO_DEPTHTEST		= (1 << 4),
	EFFECT_FLAG_RADIAL_ALIGNING		= (1 << 5),
};

struct PFXBillboard_t
{
	CPFXAtlasGroup*		group;		// atlas
	TexAtlasEntry_t*	tex;	// texture name in atlas

	Vector4D			vColor;
	Vector3D			vOrigin;
	Vector3D			vLockDir;

	float				fWide;
	float				fTall;

	float				fZAngle;

	int					nFlags;
};

// draws particle
void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum);
void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum);

//------------------------------------------------------------------------------------

extern CParticleLowLevelRenderer*	g_pPFXRenderer;

#endif // EQPARTICLES_H
