//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#include "pedestrian.h"
#include "CameraAnimator.h"
#include "input.h"
#include "world.h"

#define DEFAULT_PED_MODEL "models/characters/ped1.egf"

const float PEDESTRIAN_RADIUS = 0.85f;

CPedestrian::CPedestrian() : CAnimatingEGF(), CControllableGameObject(), m_thinker(this)
{
	m_physBody = nullptr;
	m_onGround = false;
	m_pedState = 0;

	m_thinkTime = 0;
	m_hasAI = false;
}

CPedestrian::CPedestrian(pedestrianConfig_t* config) : CPedestrian()
{
	SetModel(config->model.c_str());
	m_hasAI = config->hasAI;
}

CPedestrian::~CPedestrian()
{

}

void CPedestrian::Precache()
{

}

void CPedestrian::SetModelPtr(IEqModel* modelPtr)
{
	BaseClass::SetModelPtr(modelPtr);

	InitAnimating(m_pModel);
}

void CPedestrian::OnRemove()
{
	if (m_physBody)
	{
		g_pPhysics->m_physics.DestroyBody(m_physBody);
		m_physBody = NULL;
	}

	BaseClass::OnRemove();
}

void CPedestrian::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	BaseClass::OnCarCollisionEvent(pair, hitBy);
}

void CPedestrian::Spawn()
{
	m_physBody = new CEqRigidBody();
	m_physBody->Initialize(PEDESTRIAN_RADIUS);

	m_physBody->SetCollideMask(COLLIDEMASK_PEDESTRIAN);
	m_physBody->SetContents(OBJECTCONTENTS_PEDESTRIAN);

	m_physBody->SetPosition(m_vecOrigin);

	m_physBody->m_flags |= BODY_DISABLE_DAMPING | COLLOBJ_DISABLE_RESPONSE | BODY_FROZEN;

	m_physBody->SetMass(85.0f);
	m_physBody->SetFriction(0.0f);
	m_physBody->SetRestitution(0.0f);
	m_physBody->SetAngularFactor(vec3_zero);
	m_physBody->m_erp = 0.15f;
	m_physBody->SetGravity(18.0f);
	
	m_physBody->SetUserData(this);

	m_bbox = m_physBody->m_aabb_transformed;

	g_pPhysics->m_physics.AddToWorld(m_physBody);

	if(m_hasAI)
		m_thinker.FSMSetState(AI_State(&CPedestrianAI::SearchDaWay));

	BaseClass::Spawn();
}

void CPedestrian::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	conf.dist = 3.8f;
	conf.height = 1.0f;
	conf.distInCar = 0.0f;
	conf.widthInCar = 0.0f;
	conf.heightInCar = 0.72f;
	conf.fov = 60.0f;

	filter.AddObject(m_physBody);
}

void CPedestrian::Draw(int nRenderFlags)
{
	RecalcBoneTransforms();

	//m_physBody->ConstructRenderMatrix(m_worldMatrix);

	UpdateTransform();
	DrawEGF(nRenderFlags, m_boneTransforms);
}

const float ACCEL_RATIO = 12.0f;
const float DECEL_RATIO = 25.0f;

const float MAX_WALK_VELOCITY = 1.35f;
const float MAX_RUN_VELOCITY = 9.0f;

const float PEDESTRIAN_THINK_TIME = 0.0f;

void CPedestrian::Simulate(float fDt)
{
	m_vecOrigin = m_physBody->GetPosition();

	if(!m_physBody->IsFrozen())
		m_onGround = false;

	for (int i = 0; i < m_physBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = m_physBody->m_collisionList[i];

		m_onGround = (pair.bodyB && pair.bodyB->GetContents() == OBJECTCONTENTS_SOLID_GROUND);

		if (m_onGround)
			break;
	}

	const Vector3D& velocity = m_physBody->GetLinearVelocity();

	Vector3D preferredMove(0.0f);

	Vector3D forwardVec;
	AngleVectors(m_vecAngles, &forwardVec);
	
	// do pedestrian thinker
	if(m_hasAI)
	{
		m_thinkTime -= fDt;

		if (m_thinkTime <= 0.0f)
		{
			int res = m_thinker.DoStatesAndEvents(PEDESTRIAN_THINK_TIME + fDt);
			m_thinkTime = PEDESTRIAN_THINK_TIME;
		}
	}
	
	int controlButtons = GetControlButtons();

	Activity bestMoveActivity = (controlButtons & IN_BURNOUT) ? ACT_RUN : ACT_WALK;
	float bestMaxSpeed = (bestMoveActivity == ACT_RUN) ? MAX_RUN_VELOCITY : MAX_WALK_VELOCITY;

	if (fDt > 0.0f)
	{
		if (length(velocity) < bestMaxSpeed)
		{
			if (controlButtons & IN_ACCELERATE)
				preferredMove += forwardVec * ACCEL_RATIO;
			else if (controlButtons & IN_BRAKE)
				preferredMove -= forwardVec * ACCEL_RATIO;
		}

		{
			preferredMove.x -= (velocity.x - preferredMove.x) * DECEL_RATIO;
			preferredMove.z -= (velocity.z - preferredMove.z) * DECEL_RATIO;
		}

		m_physBody->ApplyLinearForce(preferredMove * m_physBody->GetMass());
	}

	if (controlButtons)
		m_physBody->TryWake(false);
	
	if (controlButtons & IN_TURNLEFT)
		m_vecAngles.y += 120.0f * fDt;

	if (controlButtons & IN_TURNRIGHT)
		m_vecAngles.y -= 120.0f * fDt;

	if (controlButtons & IN_BURNOUT)
		m_pedState = 1;
	else if (controlButtons & IN_EXTENDTURN)
		m_pedState = 2;
	else if (controlButtons & IN_HANDBRAKE)
		m_pedState = 0;

	Activity idleAct = ACT_IDLE;

	if (m_pedState == 1)
	{
		idleAct = ACT_IDLE_WPN;
	}
	else if (m_pedState == 2)
	{
		idleAct = ACT_IDLE_CROUCH;
	}
	else
		m_pedState = 0;

	Activity currentAct = GetCurrentActivity();

	if (currentAct != idleAct)
		SetPlaybackSpeedScale(length(velocity) / bestMaxSpeed);
	else
		SetPlaybackSpeedScale(1.0f);

	if (length(velocity.xz()) > 0.5f)
	{
		if (currentAct != bestMoveActivity)
			SetActivity(bestMoveActivity);
	}
	else if(currentAct != idleAct)
		SetActivity(idleAct);

	AdvanceFrame(fDt);
	DebugRender(m_worldMatrix);

	BaseClass::Simulate(fDt);
}

void CPedestrian::UpdateTransform()
{
	Vector3D offset(vec3_up*PEDESTRIAN_RADIUS);

	// refresh it's matrix
	m_worldMatrix = translate(m_vecOrigin - offset)*rotateXYZ4(DEG2RAD(m_vecAngles.x), DEG2RAD(m_vecAngles.y), DEG2RAD(m_vecAngles.z));
}

void CPedestrian::SetOrigin(const Vector3D& origin)
{
	if (m_physBody)
		m_physBody->SetPosition(origin);

	m_vecOrigin = origin;
}

void CPedestrian::SetAngles(const Vector3D& angles)
{
	if (m_physBody)
		m_physBody->SetOrientation(Quaternion(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CPedestrian::SetVelocity(const Vector3D& vel)
{
	if (m_physBody)
		m_physBody->SetLinearVelocity(vel);
}

const Vector3D& CPedestrian::GetOrigin()
{
	if(m_physBody)
		m_vecOrigin = m_physBody->GetPosition();

	return m_vecOrigin;
}

const Vector3D& CPedestrian::GetAngles()
{
	//m_vecAngles = eulers(m_physBody->GetOrientation());
	//m_vecAngles = VRAD2DEG(m_vecAngles);

	return m_vecAngles;
}

const Vector3D& CPedestrian::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}

void CPedestrian::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
{

}

//--------------------------------------------------------------
// PEDESTRIAN THINKER FSM

bool CPedestrianAI::GetNextPath(int dir)
{
	// get the road tile

	CLevelRegion* reg;
	levroadcell_t* cell = g_pGameWorld->m_level.Road_GetGlobalTile(m_host->GetOrigin(), &reg);

	if (cell && cell->type == ROADTYPE_PAVEMENT)
	{
		IVector2D curTile;
		g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(cell->posX, cell->posY), reg, curTile);

		int tileOfsX[] = ROADNEIGHBOUR_OFFS_X(curTile.x);
		int tileOfsY[] = ROADNEIGHBOUR_OFFS_Y(curTile.y);

		// try to walk in usual dae way
		{
			levroadcell_t* nCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(IVector2D(tileOfsX[dir], tileOfsY[dir]), &reg);

			if (nCell && nCell->type == ROADTYPE_PAVEMENT)
			{
				IVector2D nTile;
				g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(nCell->posX, nCell->posY), reg, nTile);

				if (nCell != m_prevRoadCell && nCell != m_nextRoadCell)
				{
					m_prevRoadCell = m_nextRoadCell;
					m_nextRoadCell = nCell;
					m_nextRoadTile = nTile;
					m_prevDir = m_curDir;
					m_curDir = dir;
					return true;
				}
			}
		}
	}
	else
	{
		m_nextRoadTile = 0;
	}

	return false;
}

int	CPedestrianAI::SearchDaWay(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}

	int randomDir = RandomInt(0, 3);

	if (GetNextPath(randomDir))
		AI_SetState(&CPedestrianAI::DoWalk);

	return 0;
}

const float AI_PEDESTRIAN_CAR_AFRAID_MAX_RADIUS = 9.0f;
const float AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS = 1.5f;
const float AI_PEDESTRIAN_CAR_AFRAID_STRAIGHT_RADIUS = 2.5f;
const float AI_PEDESTRIAN_CAR_AFRAID_VELOCITY = 1.0f;

void CPedestrianAI::DetectEscape()
{
	Vector3D pedPos = m_host->GetOrigin();

	DkList<CGameObject*> nearestCars;
	g_pGameWorld->QueryObjects(nearestCars, AI_PEDESTRIAN_CAR_AFRAID_MAX_RADIUS, pedPos, [](CGameObject* x) {
		return (x->ObjType() == GO_CAR || x->ObjType() == GO_CAR_AI);
	});

	for (int i = 0; i < nearestCars.numElem(); i++)
	{
		CControllableGameObject* nearCar = (CControllableGameObject*)nearestCars[i];

		Vector3D carPos = nearCar->GetOrigin();
		Vector3D carHeadingPos = carPos + nearCar->GetVelocity();

		float projResult = lineProjection(carPos, carHeadingPos, pedPos);

		bool hasSirenOrHorn = nearCar->GetControlButtons() & IN_HORN;

		float velocity = length(nearCar->GetVelocity());
		float distance = length(carPos - pedPos);

		if (projResult > 0.0f && projResult < 1.0f && (velocity > AI_PEDESTRIAN_CAR_AFRAID_VELOCITY || distance < AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS) || hasSirenOrHorn)
		{
			Vector3D projPos = lerp(carPos, carHeadingPos, projResult);

			if (hasSirenOrHorn || length(projPos - pedPos) < AI_PEDESTRIAN_CAR_AFRAID_STRAIGHT_RADIUS)
			{
				AI_SetState(&CPedestrianAI::DoEscape);

				m_escapeFromPos = pedPos;
				m_escapeDir = normalize(pedPos - projPos + nearCar->GetVelocity()*0.01f);
				return;
			}
		}
	}
}

int	CPedestrianAI::DoEscape(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}

	DetectEscape();

	Vector3D pedPos = m_host->GetOrigin();
	Vector3D pedAngles = m_host->GetAngles();

	if (length(pedPos - m_escapeFromPos) > AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS)
	{
		AI_SetState(&CPedestrianAI::DoWalk);
		return 0;
	}

	int controlButtons = 0;

	Vector3D dirAngles = VectorAngles(m_escapeDir);
	m_host->SetAngles(Vector3D(0.0f, dirAngles.y, 0.0f));
	/*
	float angleDiff = AngleDiff(pedAngles.y, dirAngles.y);

	if (fabs(angleDiff) > 1.0f)
	{
		if (angleDiff > 0)
			controlButtons |= IN_TURNLEFT;
		else
			controlButtons |= IN_TURNRIGHT;
	}
	*/
	controlButtons |= IN_ACCELERATE | IN_BURNOUT;

	m_host->SetControlButtons(controlButtons);

	return 0;
}

int	CPedestrianAI::DoWalk(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}
	
	Vector3D pedPos = m_host->GetOrigin();
	Vector3D pedAngles = m_host->GetAngles();

	DetectEscape();

	CLevelRegion* reg;
	levroadcell_t* cell = g_pGameWorld->m_level.Road_GetGlobalTile(pedPos, &reg);

	if (!m_nextRoadCell || m_nextRoadCell == cell)
	{
		if (!GetNextPath(m_curDir))
		{
			AI_SetState(&CPedestrianAI::SearchDaWay);
			return 0;
		}
	}

	
	int controlButtons = 0;

	Vector3D nextCellPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_nextRoadTile);

	Vector3D dirToCell = normalize(nextCellPos - pedPos);
	Vector3D dirAngles = VectorAngles(dirToCell);

	float angleDiff = AngleDiff(pedAngles.y, dirAngles.y);

	if (fabs(angleDiff) > 1.0f)
	{
		if (angleDiff > 0)
			controlButtons |= IN_TURNLEFT;
		else
			controlButtons |= IN_TURNRIGHT;
	}
	else
		controlButtons |= IN_ACCELERATE;// | IN_BURNOUT;


	m_host->SetControlButtons(controlButtons);


	return 0;
}