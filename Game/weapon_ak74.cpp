//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Weapon AK-74 class
//////////////////////////////////////////////////////////////////////////////////

#include "BaseWeapon.h"
#include "BaseEngineHeader.h"
#include "Player.h"

#include "env_explode.h"

#define VIEWMODEL_NAME "models/weapons/v_har.egf"
#define WORLDMODEL_NAME "models/weapons/w_har.egf"
#define WORLDMODEL_GRENADE "models/weapons/w_grenade.egf"

/*
class CHybridAssaultRifleGrenade : public BaseEntity, public CBaseRenderableObject
{
	DEFINE_CLASS_BASE(BaseEntity);

public:
	CHybridAssaultRifleGrenade()
	{
		m_bStartAsleep = false;
		fDeathTime = 3.0f;
	}

	void Precache()
	{
		PrecacheStudioModel(WORLDMODEL_GRENADE);
		PrecacheScriptSound("grenade.explode");
	}

	void Spawn()
	{
		BaseClass::Spawn();

		SetModel(WORLDMODEL_GRENADE);

		PhysicsInitNormal(0, false);

		// collide only with world
		m_physObj->SetCollisionMask(COLLIDE_OBJECT);
		m_physObj->SetContents(COLLISION_GROUP_OBJECTS);

		m_physObj->SetPosition(GetAbsOrigin());
		m_physObj->SetAngles(GetAbsAngles());

		SetHealth(15);
	}

	void Activate()
	{
		BaseEntity::Activate();
	}

	void OnRemove()
	{
		BaseClass::OnRemove();
	}

	void SetAbsOrigin(Vector3D &orig)
	{
		m_physObj->SetPosition(orig);
		BaseEntity::SetAbsOrigin(orig);
	}

	void SetAbsAngles(Vector3D &angles)
	{
		m_physObj->SetAngles(angles);
		BaseEntity::SetAbsAngles(angles);
	}

	void SetAbsVelocity(Vector3D &vel)
	{
		m_physObj->SetVelocity(vel);
		BaseEntity::SetAbsVelocity(vel);
	}

	void OnPreRender()
	{
		g_modelList->AddRenderable(this);

		m_physObj->WakeUp();

		if(true)
		{
			explodedata_t exp;
			exp.magnitude = 5000;
			exp.radius = 350;
			exp.damage = 100;
			exp.origin = GetAbsOrigin();
			exp.pInflictor = GetOwner();

			exp.pIgnore.append(m_physObj);

			physics->DoForEachObject(ExplodeAddImpulseToEachObject, &exp);
			exp.fast = false;

			EmitSound("grenade.explode");

			SetState(IEntity::ENTITY_REMOVE);

			UTIL_ExplosionEffect(GetAbsOrigin(), Vector3D(0,1,0), false);

			// create a light
			dlight_t *light = viewrenderer->AllocLight();
			SetLightDefaults(light);

			light->position = GetAbsOrigin() + Vector3D(0,15,0);
			light->color = Vector3D(0.9,0.4,0.25);
			light->intensity = 2;
			light->radius = 530;
			light->dietime = 0.1f;
			light->fadetime = 0.1f;

			viewrenderer->AddLight(light);

		}

		fDeathTime -= gpGlobals->frametime;
	}

	void ApplyDamage(Vector3D &origin, Vector3D &direction, Vector3D &normal, BaseEntity* pInflictor, float fHitImpulse, float fDamage)
	{
		fDeathTime -= fDamage / 60.0f;
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return m_pModel->GetBBoxMins();
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return m_pModel->GetBBoxMaxs();
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return PhysicsGetObject()->GetTransformMatrix();
	}

	void Render(int nViewRenderFlags)
	{
		// render if in frustum
		Matrix4x4 wvp;
		Matrix4x4 world = GetRenderWorldTransform();

		materials->SetMatrix(MATRIXMODE_WORLD, GetRenderWorldTransform());

		if(materials->GetLight() && materials->GetLight()->nFlags & LFLAG_MATRIXSET)
			wvp = materials->GetLight()->lightWVP * world;
		else 
			materials->GetWorldViewProjection(wvp);

		Volume frustum;
		frustum.LoadAsFrustum(wvp);

		Vector3D mins = GetBBoxMins();
		Vector3D maxs = GetBBoxMaxs();

		if(!frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
			return;

		//m_pModel->PrepareForSkinning(m_boneTransforms);

		studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
		int nLod = 0;

		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
			int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

			while(nLod > 0 && nModDescId != -1)
			{
				nLod--;
				nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
			}

			if(nModDescId == -1)
				continue;

			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
			{
				viewrenderer->DrawModelPart(m_pModel, nModDescId, j, nViewRenderFlags);
			}
		}
	}

	void Update(float dt)
	{
		BaseEntity::SetAbsAngles(m_physObj->GetAngles());
		BaseEntity::SetAbsOrigin(m_physObj->GetPosition());

		Vector3D up;
		AngleVectors(GetAbsAngles(), NULL, NULL, &up);

		ObjectSoundEvents(m_physObj, this);
	}

private:
	bool m_bStartAsleep;

	float fDeathTime;

	//DECLARE_DATAMAP();
};
*/

class CWeaponAK74 : public CBaseWeapon
{
	DEFINE_CLASS_BASE(CBaseWeapon);

public:
	void Spawn();
	void Precache();

	void IdleThink();

	const char*	GetViewmodel();
	const char*	GetWorldmodel();

	const char*	GetShootSound()				{return "ak74.shoot";} // shoot sound script

	const char*	GetReloadSound()			{return "ak74.reload";} // reload sound script

	const char*	GetFastReloadSound()		{return "ak74.fastreload";} // reload sound script

	int			GetMaxClip() {return 30;}

	float		GetSpreadingMultiplier()	{return 0.2f;}

	Vector3D	GetDevationVector()			{return Vector3D(0.2f,0.5f,0.0f);} // view kick

protected:


};

DECLARE_ENTITYFACTORY(weapon_ak74, CWeaponAK74)

void CWeaponAK74::Spawn()
{
	m_nAmmoType = GetAmmoTypeDef("har_bullet");
	BaseClass::Spawn();
}

void CWeaponAK74::Precache()
{
	BaseClass::Precache();
}

const char*	CWeaponAK74::GetViewmodel()
{
	return VIEWMODEL_NAME;
}

const char*	CWeaponAK74::GetWorldmodel()
{
	return WORLDMODEL_NAME;
}

void CWeaponAK74::IdleThink()
{
	BaseClass::IdleThink();

	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(!pActor)
		return;

	if(!(pActor->GetButtons() & IN_PRIMARY))
		m_nShotsFired = 0;

	if(pActor->GetButtons() & IN_PRIMARY && m_fPrimaryAttackTime < gpGlobals->curtime)
	{
		PrimaryAttack();

		m_fPrimaryAttackTime = gpGlobals->curtime+RandomFloat(0.075f,0.08f); ;
	}

	CWhiteCagePlayer* pWCPlayer = dynamic_cast<CWhiteCagePlayer*>(m_pOwner);

	if(!pWCPlayer)
		return;
	
	if(pWCPlayer->GetButtons() & IN_SECONDARY)
	{
		pWCPlayer->SetZoomMultiplier( 2.0 );
	}
	else
	{
		pWCPlayer->SetZoomMultiplier( 1.0 );
	}
}