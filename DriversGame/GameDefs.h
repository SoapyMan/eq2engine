//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include "dktypes.h"

enum EEventType
{
	EVT_INVALID = -1,

	EVT_COLLISION,		// has data as ContactPair_t*

	// GO_CAR events
	EVT_CAR_HORN,
	EVT_CAR_SIREN,
	EVT_CAR_DEATH,

	EVT_PEDESTRIAN_SCARED,

	EVT_COUNT,
};

// There are some traffic infraction types that cop can recognize
enum EInfractionType
{
	INFRACTION_NONE = 0,

	INFRACTION_HAS_FELONY,		// this car has a felony

	INFRACTION_MINOR,			// minor infraction ratio

	INFRACTION_SPEEDING,			// speeding
	INFRACTION_RED_LIGHT,			// he's went on red light
	INFRACTION_WRONG_LANE,			// he drives opposite lane

	INFRACTION_HIT_MINOR,			// hit the debris
	INFRACTION_HIT,					// hit the wall / building
	INFRACTION_HIT_VEHICLE,			// hit another vehicle
	INFRACTION_HIT_SQUAD_VEHICLE,	// hit a squad car

	INFRACTION_SCARING_PEDESTRIANS,	// scaring pedestrians

	INFRACTION_COUNT,
};

enum ECarDriverType
{
	CAR_DRIVER_SIMPLE = 0,
	CAR_DRIVER_TRAFFIC,
	CAR_DRIVER_PURSUER
};

const float INFRACTION_SKIP_REGISTER_TIME = 2.0f;

// cop data
struct PursuerData_t
{
	DECLARE_CLASS_NOBASE(PursuerData_t);
	DECLARE_NETWORK_TABLE();
	DECLARE_EMBEDDED_NETWORKVAR();

	float	lastInfractionTime[INFRACTION_COUNT];
	bool	hasInfraction[INFRACTION_COUNT];
	float	felonyRating;

	float	lastSeenTimer;
	float	hitCheckTimer;
	float	loudhailerTimer;
	float	speechTimer;

	int		pursuedByCount;
	float	lastDirectionTimer;
	int		lastDirection;		// swne

	bool	announced;
	bool	copsHasAttention;
};

struct RoadBlockInfo_t
{
	RoadBlockInfo_t() : targetEnteredRoadblock(nullptr), runARoadblock(false)
	{}

	~RoadBlockInfo_t();

	Vector3D roadblockPosA;
	Vector3D roadblockPosB;

	int totalSpawns;
	DkList<CCar*> activeCars;

	CCar*	targetEnteredRoadblock;
	float	targetEnteredSign;
	bool	runARoadblock;
};

// curve params
struct slipAngleCurveParams_t
{
	float fInitialGradient;
	float fEndGradient;
	float fEndOffset;

	float fSegmentEndA;
	float fSegmentEndB;
};

const slipAngleCurveParams_t& GetDefaultSlipCurveParams();
const slipAngleCurveParams_t& GetAISlipCurveParams();

//--------------------------------------------------------------------

struct eventArgs_t
{
	void* handler;		// subscribed object

	void* creator;		// event creator, first argument of Raise()

	Vector3D origin;
	void* data;
};

typedef void(*eventFunc_t)(const eventArgs_t& args);


//--------------------------------------------------------------
struct eventSubscription_t;

struct eventSub_t
{
	eventSub_t() : m_sub(nullptr)
	{
	}

	~eventSub_t()
	{
		Unsubscribe();
	}

	void Unsubscribe();

	eventSub_t& operator = (const eventSubscription_t* sub)
	{
		Unsubscribe();

		m_sub = (eventSubscription_t*)sub;
		return *this;
	}

private:
	eventSubscription_t* m_sub;
};

//--------------------------------------------------------------

class CEventSubject
{
public:
	CEventSubject();
	~CEventSubject();

	eventSubscription_t*	Subscribe(eventFunc_t func, void* handler);

	void					Raise(const eventArgs_t& args);

	void					Raise(void* creator, const Vector3D& origin, void* data = nullptr);

private:
	eventSubscription_t*	m_subs;
};


extern CEventSubject g_worldEvents[EVT_COUNT];

#endif // GAMEDEFS_H