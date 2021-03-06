//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle system
//////////////////////////////////////////////////////////////////////////////////

#include "EqParticles.h"

#include "core/IConsoleCommands.h"
#include "utils/global_mutex.h"

#include "ViewParams.h"

using namespace Threading;

static CParticleLowLevelRenderer s_pfxRenderer;

CParticleLowLevelRenderer*	g_pPFXRenderer = &s_pfxRenderer;

//----------------------------------------------------------------------------------------------------

ConVar r_particleBufferSize("r_particleBufferSize", "16384", "particle buffer size, change requires restart game", CV_ARCHIVE);
ConVar r_drawParticles("r_drawParticles", "1", "Render particles", CV_CHEAT);

CParticleRenderGroup::CParticleRenderGroup() :
	m_pMaterial(NULL),
	m_useCustomProjMat(false),
	//m_vertexFormat(NULL),
	m_invertCull(false)
{
	//memset(m_vertexBuffer, 0, sizeof(m_vertexBuffer));
	//memset(m_indexBuffer, 0, sizeof(m_indexBuffer));
}

CParticleRenderGroup::~CParticleRenderGroup()
{
	Shutdown();
}

void CParticleRenderGroup::Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads )
{
	maxQuads = min(r_particleBufferSize.GetInt(), maxQuads);

	// init sprite stuff
	CSpriteBuilder::Init(maxQuads);

	m_pMaterial = materials->GetMaterial(pszMaterialName);
	m_pMaterial->Ref_Grab();
}

void CParticleRenderGroup::Shutdown()
{
	if(!m_initialized)
		return;

	CSpriteBuilder::Shutdown();

	materials->FreeMaterial(m_pMaterial);
	m_pMaterial = NULL;
}

void CParticleRenderGroup::SetCustomProjectionMatrix(const Matrix4x4& mat)
{
	m_useCustomProjMat = true;
	m_customProjMat = mat;
}

int CParticleRenderGroup::AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices )
{
	if(!g_pPFXRenderer->IsInitialized())
		return -1;

	Threading::CScopedMutex m(g_pPFXRenderer->m_mutex);

	return _AllocateGeom(nVertices, nIndices, verts, indices, preSetIndices);
}

void CParticleRenderGroup::AddParticleStrip(PFXVertex_t* verts, int nVertices)
{
	if(!g_pPFXRenderer->IsInitialized())
		return;

	Threading::CScopedMutex m(g_pPFXRenderer->m_mutex);

	_AddParticleStrip(verts, nVertices);
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleRenderGroup::Render(int nViewRenderFlags, void* userdata)
{
	if(!m_initialized || !r_drawParticles.GetBool())
	{
		m_numIndices = 0;
		m_numVertices = 0;
		return;
	}

	if(m_numVertices == 0 || (!m_triangleListMode && m_numIndices == 0))
		return;

	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();

		// copy buffers
		if(!g_pPFXRenderer->MakeVBOFrom(this))
			return;

		g_pShaderAPI->SetVertexFormat(g_pPFXRenderer->m_vertexFormat);
		g_pShaderAPI->SetVertexBuffer(g_pPFXRenderer->m_vertexBuffer, 0);

		if(m_numIndices)
			g_pShaderAPI->SetIndexBuffer(g_pPFXRenderer->m_indexBuffer);
	}

	bool invertCull = m_invertCull || (nViewRenderFlags & EPRFLAG_INVERT_CULL);

	materials->SetCullMode(invertCull ? CULL_BACK : CULL_FRONT);

	if(m_useCustomProjMat)
		materials->SetMatrix(MATRIXMODE_PROJECTION, m_customProjMat);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->BindMaterial(m_pMaterial, 0);
	materials->Apply();

	//ASSERTMSG(!m_triangleListMode, "Shadow rederer, %d verts");

	// draw
	if(m_numIndices)
		g_pShaderAPI->DrawIndexedPrimitives(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, 0, m_numIndices, 0, m_numVertices);
	else
		g_pShaderAPI->DrawNonIndexedPrimitives(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, 0, m_numVertices);

	HOOK_TO_CVAR(r_wireframe)

	if(r_wireframe->GetBool())
	{
		materials->SetRasterizerStates(CULL_FRONT, FILL_WIREFRAME);

		materials->SetDepthStates(false,false);

		static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

		g_pShaderAPI->Reset(STATE_RESET_SHADER);
		g_pShaderAPI->SetShader(flat);
		g_pShaderAPI->Apply();

		if(m_numIndices)
			g_pShaderAPI->DrawIndexedPrimitives(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, 0, m_numIndices, 0, m_numVertices);
		else
			g_pShaderAPI->DrawNonIndexedPrimitives(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, 0, m_numVertices);
	}

	if(!(nViewRenderFlags & EPRFLAG_DONT_FLUSHBUFFERS))
	{
		m_numVertices = 0;
		m_numIndices = 0;
		m_useCustomProjMat = false;
	}
}

//----------------------------------------------------------------------------------------------------------

CPFXAtlasGroup::CPFXAtlasGroup() : CParticleRenderGroup()
{

}

void CPFXAtlasGroup::Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads )
{
	CParticleRenderGroup::Init( pszMaterialName, bCreateOwnVBO, maxQuads);
}

void CPFXAtlasGroup::Shutdown()
{
	CParticleRenderGroup::Shutdown();
}

TexAtlasEntry_t* CPFXAtlasGroup::GetEntry(int idx)
{
	if (!m_pMaterial->GetAtlas())
		return nullptr;

	return m_pMaterial->GetAtlas()->GetEntry(idx);
}

int	CPFXAtlasGroup::GetEntryIndex(TexAtlasEntry_t* entry) const
{
	if (!m_pMaterial->GetAtlas())
		return -1;

	return m_pMaterial->GetAtlas()->GetEntryIndex(entry);
}

TexAtlasEntry_t* CPFXAtlasGroup::FindEntry(const char* pszName) const
{
	if (!m_pMaterial->GetAtlas())
		return nullptr;

	return m_pMaterial->GetAtlas()->FindEntry(pszName);
}

int CPFXAtlasGroup::FindEntryIndex(const char* pszName) const
{
	if (!m_pMaterial->GetAtlas())
		return -1;

	return m_pMaterial->GetAtlas()->FindEntryIndex(pszName);
}

int CPFXAtlasGroup::GetEntryCount() const
{
	if (!m_pMaterial->GetAtlas())
		return 0;

	return m_pMaterial->GetAtlas()->GetEntryCount();
}

//----------------------------------------------------------------------------------------------------------

CParticleLowLevelRenderer::CParticleLowLevelRenderer() : m_mutex(GetGlobalMutex(MUTEXPURPOSE_PARTICLES))
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_vertexFormat = nullptr;

	m_initialized = false;
}

void CParticleLowLevelRenderer::PreloadCache()
{

}

void CParticleLowLevelRenderer::ClearParticleCache()
{
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		m_renderGroups[i]->Shutdown();

		delete m_renderGroups[i];
	}

	m_renderGroups.clear();
}

bool MatSysFn_InitParticleBuffers();
bool MatSysFn_ShutdownParticleBuffers();

void CParticleLowLevelRenderer::Init()
{
	materials->AddDestroyLostCallbacks(MatSysFn_ShutdownParticleBuffers, MatSysFn_InitParticleBuffers);

	InitBuffers();
}

void CParticleLowLevelRenderer::Shutdown()
{
	materials->RemoveLostRestoreCallbacks(MatSysFn_ShutdownParticleBuffers, MatSysFn_InitParticleBuffers);

	ShutdownBuffers();
}

bool CParticleLowLevelRenderer::InitBuffers()
{
	if(m_initialized)
		return true;

	MsgWarning("Initializing particle buffers...\n");

	m_vbMaxQuads = r_particleBufferSize.GetInt();

	m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, m_vbMaxQuads * 4, sizeof(PFXVertex_t), nullptr);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(m_vbMaxQuads * 6, sizeof(int16), BUFFER_DYNAMIC, nullptr);

	if(!m_vertexFormat)
		m_vertexFormat = g_pShaderAPI->CreateVertexFormat(g_PFXVertexFormatDesc, elementsOf(g_PFXVertexFormatDesc));

	if(m_vertexBuffer && m_indexBuffer && m_vertexFormat)
		m_initialized = true;

	return false;
}

bool CParticleLowLevelRenderer::ShutdownBuffers()
{
	if(!m_initialized)
		return true;

	MsgWarning("Destroying particle buffers...\n");

	g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);
	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);

	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_vertexFormat = nullptr;

	m_initialized = false;

	return true;
}

void CParticleLowLevelRenderer::AddRenderGroup(CParticleRenderGroup* pRenderGroup, CParticleRenderGroup* after/* = nullptr*/)
{
	if (after)
	{
		int idx = m_renderGroups.findIndex(after);
		m_renderGroups.insert(pRenderGroup, idx+1);
	}
	else
		m_renderGroups.append(pRenderGroup);
}

void CParticleLowLevelRenderer::RemoveRenderGroup(CParticleRenderGroup* pRenderGroup)
{
	m_renderGroups.remove(pRenderGroup);
}

void CParticleLowLevelRenderer::PreloadMaterials()
{
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		materials->PutMaterialToLoadingQueue(m_renderGroups[i]->m_pMaterial);
	}
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleLowLevelRenderer::Render(int nRenderFlags)
{
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		m_renderGroups[i]->Render( nRenderFlags, nullptr );
	}
}

void CParticleLowLevelRenderer::ClearBuffers()
{
	int groups = m_renderGroups.numElem();

	for(int i = 0; i < groups; i++)
		m_renderGroups[i]->ClearBuffers();
}

bool CParticleLowLevelRenderer::MakeVBOFrom(CSpriteBuilder<PFXVertex_t>* pGroup)
{
	if(!m_initialized)
		return false;

	uint16 nVerts	= pGroup->m_numVertices;
	uint16 nIndices	= pGroup->m_numIndices;

	if(nVerts == 0)
		return false;

	if(nVerts > SVBO_MAX_SIZE(m_vbMaxQuads, PFXVertex_t))
		return false;

	m_vertexBuffer->Update((void*)pGroup->m_pVerts, nVerts, 0, true);

	if(nIndices)
		m_indexBuffer->Update((void*)pGroup->m_pIndices, nIndices, 0, true);

	return true;
}

//----------------------------------------------------------------------------------------------------

bool MatSysFn_InitParticleBuffers()
{
	return g_pPFXRenderer->InitBuffers();
}

bool MatSysFn_ShutdownParticleBuffers()
{
	return g_pPFXRenderer->ShutdownBuffers();
}

//----------------------------------------------------------------------------------------------------

void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum)
{
	if(!(effect->nFlags & EFFECT_FLAG_NO_FRUSTUM_CHECK))
	{
		float fBillboardSize = (effect->fWide > effect->fTall) ? effect->fWide : effect->fTall;

		if(!frustum->IsSphereInside(effect->vOrigin, fBillboardSize))
			return;
	}

	PFXVertex_t* verts;
	if(effect->group->AllocateGeom(4,4,&verts, NULL, true) < 0)
		return;

	Vector3D angles, vRight, vUp;

	if(effect->nFlags & EFFECT_FLAG_RADIAL_ALIGNING)
		angles = VectorAngles(fastNormalize(effect->vOrigin - view->GetOrigin()));
	else
		angles = view->GetAngles() + Vector3D(0,0,effect->fZAngle);

	if(effect->nFlags & EFFECT_FLAG_LOCK_X)
		angles.x = 0;

	if(effect->nFlags & EFFECT_FLAG_LOCK_Y)
		angles.y = 0;

	AngleVectors(angles, NULL, &vRight, &vUp);

	Rectangle_t texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;

	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = effect->vColor;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = effect->vColor;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = effect->vColor;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
	verts[3].color = effect->vColor;
}

void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum)
{
	if(!(effect->nFlags & EFFECT_FLAG_NO_FRUSTUM_CHECK))
	{
		float fBillboardSize = (effect->fWide > effect->fTall) ? effect->fWide : effect->fTall;

		if(!frustum->IsSphereInside(effect->vOrigin, fBillboardSize))
			return;
	}


	PFXVertex_t* verts;
	if(effect->group->AllocateGeom(4,4,&verts, NULL, true) < 0)
		return;

	Vector3D vRight, vUp;

	vRight = viewMatrix.rows[0].xyz();
	vUp = viewMatrix.rows[1].xyz();

	Rectangle_t texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;

	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = effect->vColor;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = effect->vColor;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = effect->vColor;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
	verts[3].color = effect->vColor;
}
