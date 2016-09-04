//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "CGLTexture.h"
#include "DebugInterface.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

#include "shaderapigl_def.h"

CGLTexture::CGLTexture()
{
	m_flLod = 0.0f;
	glTarget = 0;
	glDepthID = 0;
	m_texSize = 0;
	m_nLockLevel = 0;
	m_bIsLocked = false;

	m_lockPtr = 0;
	m_lockOffs = 0;
	m_lockSize = 0;
	m_lockDiscard = false;
	m_lockReadOnly = false;
}

CGLTexture::~CGLTexture()
{
	Release();
}

void CGLTexture::Release()
{
	ReleaseTextures();
}

void CGLTexture::ReleaseTextures()
{
	if (glTarget == GL_RENDERBUFFER)
	{
		glDeleteRenderbuffers(1, &glDepthID);
	}
	else
	{
		for(int i = 0; i < textures.numElem(); i++)
			glDeleteTextures(1, &textures[i].glTexID);
	}
}

eqGlTex& CGLTexture::GetCurrentTexture()
{
	static eqGlTex nulltex = {0};

	if(m_nAnimatedTextureFrame > textures.numElem()-1)
		return nulltex;

	return textures[m_nAnimatedTextureFrame];
}

// locks texture for modifications, etc
void CGLTexture::Lock(texlockdata_t* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( !m_bIsLocked );

	m_nLockLevel = nLevel;
	m_bIsLocked = true;

	if( IsCompressedFormat(m_iFormat) )
	{
		pLockData->pData = NULL;
		pLockData->nPitch = 0;
		return;
	}

	int lockOffset = 0;
	int sizeToLock = GetWidth()*GetHeight();

	IRectangle lock_rect;
	if(pRect)
	{
		lock_rect.vleftTop = pRect->vleftTop;
		lock_rect.vrightBottom = pRect->vrightBottom;

		sizeToLock = lock_rect.GetSize().x*lock_rect.GetSize().y;
		lockOffset = lock_rect.vleftTop.x*lock_rect.vleftTop.y;
	}
	else
	{
		lock_rect.vleftTop.x = 0;
		lock_rect.vleftTop.y = 0;

		lock_rect.vrightBottom.x = GetWidth();
		lock_rect.vrightBottom.y = GetHeight();
	}

	int nLockByteCount = GetBytesPerPixel(m_iFormat) * sizeToLock;

	// allocate memory for lock data
	m_lockPtr = (ubyte*)malloc(nLockByteCount);

	pLockData->pData = m_lockPtr;
	pLockData->nPitch = lock_rect.GetSize().y;

	m_lockSize = sizeToLock;
	m_lockOffs = lockOffset;
	m_lockReadOnly = bReadOnly;

#ifdef USE_GLES2
	// Always need to discard data from GLES :(
#else
    if(!bDiscard)
    {
    	ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

        pGLRHI->GL_CRITICAL();

        glBindTexture(glTarget, textures[0].glTexID);

        GLenum srcFormat = chanCountTypes[GetChannelCount(m_iFormat)];
        GLenum srcType = chanTypePerFormat[m_iFormat];

        glGetTexImage(glTarget,m_nLockLevel, srcFormat, srcType, m_lockPtr);

        glBindTexture(glTarget, 0);
    }
#endif // USE_GLES2
}

// unlocks texture for modifications, etc
void CGLTexture::Unlock()
{
	if(m_bIsLocked)
	{
		if( !m_lockReadOnly )
		{
			ShaderAPIGL* pGLRHI = (ShaderAPIGL*)g_pShaderAPI;

			GLenum srcFormat = chanCountTypes[GetChannelCount(m_iFormat)];
			GLenum srcType = chanTypePerFormat[m_iFormat];

            pGLRHI->GL_CRITICAL();

			glBindTexture(glTarget, textures[0].glTexID);
			glTexSubImage2D(glTarget, 0, 0, 0, m_iWidth, m_iHeight, srcFormat, srcType, m_lockPtr);
			glBindTexture(glTarget, 0);
		}

		free(m_lockPtr);
		m_lockPtr = NULL;
	}
	else
		ASSERT(!"Texture is not locked!");

	m_bIsLocked = false;
}
