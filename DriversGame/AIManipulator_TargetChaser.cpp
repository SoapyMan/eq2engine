//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI chaser manipulator
//////////////////////////////////////////////////////////////////////////////////

#include "AIManipulator_TargetChaser.h"
#include "AIPursuerCar.h"

#include "world.h"

ConVar ai_debug_chaser("ai_debug_chaser", "0");

CAITargetChaserManipulator::CAITargetChaserManipulator()
{
	m_driveTarget = vec3_zero;
}

void CAITargetChaserManipulator::Setup(const Vector3D&driveTargetPos, const Vector3D& targetVelocity, CEqCollisionObject* excludeCollObj)
{
	m_driveTarget = driveTargetPos;
	m_driveTargetVelocity = targetVelocity;
	m_excludeColl = excludeCollObj;
}

void CAITargetChaserManipulator::UpdateAffector(ai_handling_t& handling, CCar* car, float fDt)
{
	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(car->GetPhysicsBody());
	collFilter.AddObject(m_excludeColl);

	CollisionData_t steeringTargetColl;

	// use vehicle world matrix
	const Matrix4x4& bodyMat = car->m_worldMatrix;

	Vector3D carPos = car->GetOrigin();
	const Vector3D& carBodySize = car->m_conf->physics.body_size;

	const float AI_OBSTACLE_SPHERE_RADIUS = carBodySize.x*1.25f;// *2.0f;
	const float AI_OBSTACLE_SPHERE_SPEED_SCALE = 0.01f;

	Vector3D carForward = car->GetForwardVector();
	Vector3D carRight = car->GetRightVector();

	Plane frontBackCheckPlane(carForward, -dot(carForward, carPos));

	const Vector3D& carVelocity = car->GetVelocity();

	float speedMPS = car->GetSpeed()*KPH_TO_MPS;

	float traceShapeRadius = AI_OBSTACLE_SPHERE_RADIUS;// +speedMPS * AI_OBSTACLE_SPHERE_SPEED_SCALE;

	btSphereShape sphereTraceShape(traceShapeRadius);

	// add half car length to the car position
	carPos += carForward * carBodySize.z;

	const float AI_CHASE_TARGET_VELOCITY_SCALE = 0.5f;
	const float AI_CHASE_TARGET_VELOCITY_DISTANCE_START = 20.0f;
	const float AI_CHASE_TARGET_VELOCITY_DISTANCE_END = 4.0f;

	const float AI_STEERING_START_AGRESSIVE_DISTANCE_START = 10.0f;
	const float AI_STEERING_START_AGRESSIVE_DISTANCE_END = 4.0f;

	// brake curve
	const float AI_CHASE_BRAKE_CURVE = 0.45f;

	const float AI_CHASE_BRAKE_MIN = 0.05f;

	const float AI_BRAKE_TARGET_DISTANCE_SCALING = 2.5f;
	const float AI_BRAKE_TARGET_DISTANCE_CURVE = 0.5f;

	const float AI_STEERING_TARGET_DISTANCE_SCALING = 0.1f;
	const float AI_STEERING_TARGET_DISTANCE_CURVE = 2.5f;

	const float AI_LOWSPEED_CURVE = 15.0f;
	const float AI_LOWSPEED_END = 8.0f;	// 8 meters per second

	float tireFrictionModifier = 0.0f;
	{
		int numWheels = car->GetWheelCount();

		for (int i = 0; i < numWheels; i++)
		{
			CCarWheel& wheel = car->GetWheel(i);

			if (wheel.GetSurfaceParams())
				tireFrictionModifier += wheel.GetSurfaceParams()->tirefriction;
		}

		tireFrictionModifier /= float(numWheels);
	}

	float weatherBrakeDistModifier = s_weatherBrakeDistanceModifier[g_pGameWorld->m_envConfig.weatherType] * (1.0f - tireFrictionModifier);
	float weatherBrakePow = s_weatherBrakeCurve[g_pGameWorld->m_envConfig.weatherType] * tireFrictionModifier;

	float lowSpeedFactor = pow(RemapValClamp(speedMPS, 0.0f, AI_LOWSPEED_END, 0.0f, 1.0f), AI_LOWSPEED_CURVE);

	int traceContents = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE;

	float distanceToTarget = length(m_driveTarget - carPos);

	float targetOffsetScaling = 1.0f - RemapValClamp(distanceToTarget, AI_CHASE_TARGET_VELOCITY_DISTANCE_END, AI_CHASE_TARGET_VELOCITY_DISTANCE_START, 0.0f, 1.0f);

	

	// add velocity offset
	Vector3D steeringTargetPos = m_driveTarget + (m_driveTargetVelocity-carForward*5.0f) * AI_CHASE_TARGET_VELOCITY_SCALE * targetOffsetScaling;

	// preliminary steering dir
	Vector3D steeringDir = fastNormalize(steeringTargetPos - carPos);

	

	float distanceScaling = 1.0f - RemapValClamp(distanceToTarget, AI_STEERING_START_AGRESSIVE_DISTANCE_END, AI_STEERING_START_AGRESSIVE_DISTANCE_START, 0.0f, 1.0f);

	float steeringScaling = 1.0f + pow(distanceScaling, AI_STEERING_TARGET_DISTANCE_CURVE) * AI_STEERING_TARGET_DISTANCE_SCALING;

	if (ai_debug_chaser.GetBool())
	{
		debugoverlay->Line3D(carPos, steeringTargetPos, ColorRGBA(1, 0, 0, 1.0f), ColorRGBA(1, 0, 0, 1.0f), fDt);
		debugoverlay->Box3D(steeringTargetPos - 0.5f, steeringTargetPos + 0.5f, ColorRGBA(1, 1, 0, 1.0f), fDt);

		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "distance To Target: %.2f", distanceToTarget);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "distance scaling: %g", distanceScaling);
		debugoverlay->TextFadeOut(0, color4_white, 10.0f, "steering scaling: %g", steeringScaling);
	}

	// calculate brake first
	float brakeScaling = 1.0f + pow(distanceScaling, AI_BRAKE_TARGET_DISTANCE_CURVE) * AI_BRAKE_TARGET_DISTANCE_SCALING;
	float brakeFac = (1.0f - fabs(dot(steeringDir, fastNormalize(carVelocity))))*brakeScaling;

	if (brakeFac < AI_CHASE_BRAKE_MIN)
		brakeFac = 0.0f;

	float forwardTraceDistanceBySpeed = RemapValClamp(speedMPS, 0.0f, 50.0f, 6.0f, 25.0f);

	float distToTargetFactorForSteering = RemapValClamp(distanceToTarget, 0.0f, 10.0f, 0.0f, 1.0f);

	// final steering dir after collision tests
	steeringDir = lerp(fastNormalize(m_driveTargetVelocity), fastNormalize(steeringTargetPos - carPos), distToTargetFactorForSteering);

	// store output steering target
	m_outSteeringTargetPos = steeringTargetPos;

	// trace forward from car using speed
	g_pPhysics->TestConvexSweep(&sphereTraceShape, identity(),
		carPos, carPos + carForward * forwardTraceDistanceBySpeed, steeringTargetColl,
		traceContents,
		&collFilter);

	if (steeringTargetColl.fract < 1.0f)
	{
		bool shouldDoSteeringCorrection = true;

		if (steeringTargetColl.hitobject)
		{
			CGameObject* obj = (CGameObject*)steeringTargetColl.hitobject->GetUserData();
			if (obj && obj->ObjType() == GO_CAR_AI)
			{
				CAIPursuerCar* pursuer = UTIL_CastToPursuer((CCar*)obj);

				if (pursuer && pursuer->InPursuit() && dot(pursuer->GetVelocity(), carVelocity) > 0)
					shouldDoSteeringCorrection = false;
			}
		}

		if (shouldDoSteeringCorrection)
		{
			float AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT = 0.65f;

			Vector3D collPoint(steeringTargetColl.position);

			Plane dPlane(carRight, -dot(carPos, carRight));
			float posSteerFactor = -dPlane.Distance(collPoint);

			steeringDir += carRight * posSteerFactor * (1.0f - steeringTargetColl.fract) * AI_OBSTACLE_FRONTAL_CORRECTION_AMOUNT;
			steeringDir = normalize(steeringDir);
		}

		brakeFac += saturate(dot(steeringTargetColl.normal, fastNormalize(carVelocity))) * (1.0f - steeringTargetColl.fract);
	}

	// calculate steering
	Vector3D relateiveSteeringDir = fastNormalize((!bodyMat.getRotationComponent()) * steeringDir);

	handling.steering = atan2(relateiveSteeringDir.x, relateiveSteeringDir.z) * steeringScaling;
	handling.steering = clamp(handling.steering, -1.0f, 1.0f);

	handling.braking = pow(brakeFac, AI_CHASE_BRAKE_CURVE*weatherBrakePow) * lowSpeedFactor * 0.5f;
	handling.braking = min(handling.braking, 1.0f);
	handling.acceleration = 1.0f - handling.braking;

	// if target is behind or steering is too hard, do stunts
	handling.autoHandbrake = fabs(handling.steering) > 0.55f || (frontBackCheckPlane.ClassifyPoint(m_driveTarget) == CP_BACK);
}