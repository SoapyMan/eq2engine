//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Skinned shader for DrvSyn
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(Skinned)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture	= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;
		SHADER_PASS(Ambient_noskin) = NULL;
		SHADER_FOGPASS(Ambient_noskin) = NULL;

		SHADER_PASS(AmbientInst) = NULL;
		SHADER_FOGPASS(AmbientInst) = NULL;
		SHADER_PASS(AmbientInst_noskin) = NULL;
		SHADER_FOGPASS(AmbientInst_noskin) = NULL;

		m_nFlags |= MATERIAL_FLAG_SKINNED;
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().editormode, "EDITOR");

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "Skinned");
		SHADER_FIND_OR_COMPILE(Ambient_noskin, "Skinned_NoSkin");
		SHADER_FIND_OR_COMPILE(AmbientInst, "Skinned_Inst");
		SHADER_FIND_OR_COMPILE(AmbientInst_noskin, "Skinned_Inst_NoSkin");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "Skinned");
		SHADER_FIND_OR_COMPILE(Ambient_noskin_fog, "Skinned_NoSkin");
		SHADER_FIND_OR_COMPILE(AmbientInst_fog, "Skinned_Inst");
		SHADER_FIND_OR_COMPILE(AmbientInst_noskin_fog, "Skinned_Inst_NoSkin");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(materials->IsSkinningEnabled())
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient)
		}
		else
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst_noskin)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient_noskin)
		}
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);
		SetupDefaultParameter(SHADERPARAM_BUMPMAP);
		SetupDefaultParameter(SHADERPARAM_SPECULARILLUM);
		SetupDefaultParameter(SHADERPARAM_CUBEMAP);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetConfiguration().wireframeMode ? ColorRGBA(0,1,1,1) : materials->GetAmbientColor());
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTexture", 0);
	}

	ITexture*	GetBaseTexture(int stage)	{ return m_pBaseTexture; }
	ITexture*	GetBumpTexture(int stage)	{ return NULL; }

	ITexture*			m_pBaseTexture;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(Ambient_noskin);
	SHADER_DECLARE_FOGPASS(Ambient_noskin);

	SHADER_DECLARE_PASS(AmbientInst);
	SHADER_DECLARE_FOGPASS(AmbientInst);

	SHADER_DECLARE_PASS(AmbientInst_noskin);
	SHADER_DECLARE_FOGPASS(AmbientInst_noskin);

END_SHADER_CLASS