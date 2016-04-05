//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: debris object
//////////////////////////////////////////////////////////////////////////////////

#include "object_debris.h"
#include "materialsystem/IMaterialSystem.h"
#include "world.h"
#include "car.h"

#include "Shiny.h"

#define DEBRIS_COLLISION_RELAY	0.15f
#define DEBRIS_PHYS_LIFETIME	10.0f

extern void EmitHitSoundEffect(CGameObject* obj, const char* prefix, const Vector3D& origin, float power, float maxPower);

CObject_Debris::CObject_Debris( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_physBody = NULL;
	m_collOccured = false;
}

CObject_Debris::~CObject_Debris()
{
	
}

void CObject_Debris::OnRemove()
{
	CGameObject::OnRemove();

	if(m_physBody)
	{
		g_pPhysics->m_physics.DestroyBody(m_physBody);
		m_physBody = NULL;
	}

	if(m_userData)
	{
		regionObject_t* ref = (regionObject_t*)m_userData;
		ref->game_object = NULL;
	}
}

void CObject_Debris::Spawn()
{
	SetModel( KV_GetValueString(m_keyValues->FindKeyBase("model"), 0, "models/error.egf") );

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_objectInstVertexFormat, g_pGameWorld->m_objectInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashsound"), 0, "");

	if(m_smashSound.GetLength() > 0)
	{
		ses->PrecacheSound(m_smashSound.c_str());
		ses->PrecacheSound((m_smashSound + "_light").c_str());
		ses->PrecacheSound((m_smashSound + "_medium").c_str());
		ses->PrecacheSound((m_smashSound + "_hard").c_str());
	}

	CEqRigidBody* body = new CEqRigidBody();

	BoundingBox bbox(m_pModel->GetBBoxMins(), m_pModel->GetBBoxMaxs());

	if( body->Initialize(&m_pModel->GetHWData()->m_physmodel, 0) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->m_physmodel.objects[0].object;

		body->SetFriction( 0.8f );
		body->SetRestitution( 0.15f );
		body->SetMass(obj->mass);
		body->SetGravity(body->GetGravity() * 3.5f);
		body->SetDebugName("hubcap");

		// additional error correction required
		body->m_erp = 0.05f;

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = COLLOBJ_COLLISIONLIST | COLLOBJ_DISABLE_RESPONSE | BODY_FORCE_FREEZE;

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_DEBRIS );
		body->SetCollideMask( COLLIDEMASK_DEBRIS );

		body->SetUserData(this);

		m_physBody = body;

		// initially this object is not moveable
		g_pPhysics->m_physics.AddToWorld( m_physBody, false );

		m_bbox = body->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	// baseclass spawn
	CGameObject::Spawn();
}

void CObject_Debris::SpawnAsHubcap(IEqModel* model, int8 bodyGroup)
{
	m_pModel = model;
	m_bodyGroupFlags = (1 << bodyGroup);
	m_collOccured = true;

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_objectInstVertexFormat, g_pGameWorld->m_objectInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}

	// TODO: hubcap sounds
	m_smashSound = "";
	
	CEqRigidBody* body = new CEqRigidBody();

	BoundingBox bbox(m_pModel->GetBBoxMins(), m_pModel->GetBBoxMaxs());

	if( body->Initialize(&m_pModel->GetHWData()->m_physmodel, 0) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->m_physmodel.objects[0].object;

		body->SetFriction( 0.8f );
		body->SetRestitution( 0.15f );
		body->SetMass(obj->mass);
		body->SetGravity(body->GetGravity() * 3.5f);
		body->SetDebugName("debris");

		// additional error correction required
		body->m_erp = 0.05f;

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = COLLOBJ_DISABLE_RESPONSE;

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_DEBRIS );
		body->SetCollideMask( COLLIDEMASK_DEBRIS );

		body->SetUserData(this);

		m_physBody = body;

		// initially this object is not moveable
		g_pPhysics->m_physics.AddToWorld( m_physBody, true );

		m_bbox = body->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	// baseclass spawn
	CGameObject::Spawn();
}

void CObject_Debris::SetOrigin(const Vector3D& origin)
{
	if(m_physBody)
		m_physBody->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Debris::SetAngles(const Vector3D& angles)
{
	if(m_physBody)
		m_physBody->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Debris::SetVelocity(const Vector3D& vel)
{
	if(m_physBody)
		m_physBody->SetLinearVelocity( vel  );
}

const Vector3D& CObject_Debris::GetOrigin()
{
	m_vecOrigin = m_physBody->GetPosition();
	return m_vecOrigin;
}

const Vector3D& CObject_Debris::GetAngles()
{ 
	m_vecAngles = eulers(m_physBody->GetOrientation());
	m_vecAngles = VRAD2DEG(m_vecAngles);

	return m_vecAngles;
}

const Vector3D& CObject_Debris::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}

extern ConVar r_enableObjectsInstancing;

void CObject_Debris::Draw( int nRenderFlags )
{
	if(!m_physBody)
		return;

	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(objBody->m_aabb.maxPoint)))
	//	return;

	if(m_collOccured)
	{
		m_physBody->UpdateBoundingBoxTransform();
		m_bbox = m_physBody->m_aabb_transformed;
	}

	m_physBody->ConstructRenderMatrix(m_worldMatrix);

	if(r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
		{
			if(!(m_bodyGroupFlags & (1 << i)))
				continue;

			gameObjectInstance_t& inst = instancer->NewInstance( i , nLOD );
			inst.world = m_worldMatrix;
		}
	}
	else
		CGameObject::Draw( nRenderFlags );
}

ConVar g_debris_as_physics("g_debris_as_physics", "0", "Thread debris as physics objects", CV_CHEAT);

void CObject_Debris::Simulate(float fDt)
{
	PROFILE_FUNC();

	if(m_collOccured)
		m_fTimeToRemove -= fDt;

	if (!m_physBody)
		return;

	m_vecOrigin = m_physBody->GetPosition();

	if (m_collOccured && m_fTimeToRemove < 0.0f)
	{
		//g_pGameWorld->RemoveObject(this);

		m_physBody->SetContents( OBJECTCONTENTS_DEBRIS );
		m_physBody->SetCollideMask( COLLIDEMASK_DEBRIS );

		m_physBody->m_flags &= ~BODY_FORCE_FREEZE;
	}

	if(m_collOccured && m_fTimeToRemove < -DEBRIS_PHYS_LIFETIME)
		g_pGameWorld->RemoveObject(this);

	/*-------------------------------------------------------------*/

	for(int i = 0; i < m_physBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = m_physBody->m_collisionList[i];

		float impulse = pair.appliedImpulse * m_physBody->GetInvMass();

		if(impulse > 10.0f)
		{
			// change look of models
			if(m_pModel->GetHWData()->pStudioHdr->numbodygroups > 1)
				m_bodyGroupFlags = (1 << 1);
		}

		CEqCollisionObject* obj = pair.GetOppositeTo(m_physBody);
		
		//if (m_collOccured)
		EmitHitSoundEffect(this, m_smashSound.c_str(), pair.position, pair.impactVelocity, 50.0f);

		if(obj->IsDynamic() && !m_collOccured)
		{
			m_collOccured = true;
			g_pPhysics->m_physics.AddToMoveableList(m_physBody);
			m_physBody->Wake();

			CEqRigidBody* body = (CEqRigidBody*)obj;
			if(body->m_flags & BODY_ISCAR)
			{
				CCar* pCar = (CCar*)body->GetUserData();

				if( !g_debris_as_physics.GetBool() )
				{
					EmitSound_t ep;

					ep.name = (char*)m_smashSound.c_str();

					ep.fPitch = RandomFloat(1.0f, 1.1f);
					ep.fVolume = 1.0f;
					ep.origin = pair.position;

					EmitSoundWithParams( &ep );

					m_fTimeToRemove = 0;//DEBRIS_COLLISION_RELAY;

					float fCarVel = length(body->GetLinearVelocity());

					float fImpUpVel = clamp(fCarVel*0.6f, 0.5f, 7.0f);

					FVector3D randPos = pair.position + Vector3D(RandomFloat(-0.18,0.18),RandomFloat(-0.18,0.18), RandomFloat(-0.18,0.18));

					Vector3D impulse = body->GetLinearVelocity();

					m_physBody->ApplyWorldImpulse(pair.position, impulse * 1.5f);

					pCar->EmitCollisionParticles(randPos, body->GetLinearVelocity(), pair.normal, 6, pair.appliedImpulse);
				}
				else
				{
					m_physBody->m_flags = BODY_COLLISIONLIST;
					m_physBody->SetContents(OBJECTCONTENTS_OBJECT);
					m_physBody->SetCollideMask(COLLIDEMASK_OBJECT);
				}
			}
		}
	}
}

void CObject_Debris::OnPhysicsFrame(float fDt)
{

}

void CObject_Debris::L_SetContents(int contents)
{
	if (!m_physBody)
		return;

	m_physBody->SetContents(contents);
}

void CObject_Debris::L_SetCollideMask(int contents)
{
	if (!m_physBody)
		return;

	m_physBody->SetCollideMask(contents);
}

int	CObject_Debris::L_GetContents() const
{
	if (!m_physBody)
		return 0;

	return m_physBody->GetCollideMask();
}

int	CObject_Debris::L_GetCollideMask() const
{
	if (!m_physBody)
		return 0;

	return m_physBody->GetCollideMask();
}