//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: UnLit shader. Used for model
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------
BEGIN_SHADER_CLASS(BaseParticle)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Particle) = NULL;
		SHADER_FOGPASS(Particle) = NULL;

		m_pBaseTexture			= NULL;

		//IMatVar* mv_color			= GetAssignedMaterial()->GetMaterialVar("color", "[1,1,1,1]");
		IMatVar* mv_depthSetup		= GetAssignedMaterial()->FindMaterialVar("depthRangeSetup");

		if(mv_depthSetup && mv_depthSetup->GetInt() > 0)
			SetParameterFunctor(SHADERPARAM_RASTERSETUP, &ThisShaderClass::DepthRangeRasterSetup);
		
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);
	}

	SHADER_INIT_RHI()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Particle))
			return true;

		bool useAdvLighting = false;

		SHADER_PARAM_BOOL(AdvancedLighting, useAdvLighting);

		SHADERDEFINES_BEGIN

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ADDITIVE), "ADDITIVE")

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		SHADER_DECLARE_SIMPLE_DEFINITION(useAdvLighting, "ADVANCED_LIGHTING");

		SHADER_FIND_OR_COMPILE(Particle, "BaseParticle")

		SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG")

		SHADER_FIND_OR_COMPILE(Particle_fog, "BaseParticle")

		return true;
	}

	void SetupShader()
	{
		FogInfo_t fg;
		materials->GetFogInfo(fg);

		SHADER_BIND_PASS_FOGSELECT(Particle);
	}

	void SetupConstants()
	{
		// If we has shader
		if(!IsError())
		{
			materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);

			SetupDefaultParameter(SHADERPARAM_TRANSFORM);

			SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

			SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
			SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
			SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
			SetupDefaultParameter(SHADERPARAM_COLOR);
			SetupDefaultParameter(SHADERPARAM_FOG);
		}
		else
		{
			// Set error texture
			g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
		}
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetAdditiveColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void DepthRangeRasterSetup()
	{
		g_pShaderAPI->SetDepthRange(0.0f - 0.00008f, 1.0f - 0.00001f);
		CBaseShader::ParamSetup_RasterState();
	}

	ITexture*	GetBaseTexture(int stage) {return m_pBaseTexture;}
	ITexture*	GetBumpTexture(int stage) {return NULL;}

	SHADER_DECLARE_PASS(Particle);
	SHADER_DECLARE_FOGPASS(Particle);

	ITexture*			m_pBaseTexture;
	IMatVar*			m_pBaseTextureFrame;
END_SHADER_CLASS