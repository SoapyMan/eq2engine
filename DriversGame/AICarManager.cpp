//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI car manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "AICarManager.h"
#include "heightfield.h"
#include "session_stuff.h"

//------------------------------------------------------------------------------------------

ConVar g_trafficMaxCars("g_trafficMaxCars", "48", "Maximum traffic cars", CV_ARCHIVE);
ConVar g_traffic_mindist("g_traffic_mindist", "50", "Min traffic car distance to spawn", CV_CHEAT);
ConVar g_traffic_maxdist("g_traffic_maxdist", "51", "Max traffic car distance, to disappear", CV_CHEAT);

const float AI_COP_SPEECH_DELAY = 3.0f;		// delay before next speech
const float AI_COP_TAUNT_DELAY = 11.0f;		// delay before next taunt

#define COP_DEFAULT_DAMAGE			(4.0f)

#define TRAFFIC_BETWEEN_DISTANCE	5

//------------------------------------------------------------------------------------------

static CAICarManager s_AIManager;
CAICarManager* g_pAIManager = &s_AIManager;

DECLARE_CMD(aezakmi, "", CV_CHEAT)
{
	if(!g_pGameSession)
		return;

	g_pGameSession->GetPlayerCar()->SetFelony(0.0f);

	for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
	{
		g_pAIManager->m_copCars[i]->EndPursuit(false);
	}
}

CAICarManager::CAICarManager()
{
	m_trafficUpdateTime = 0.0f;
	m_enableCops = true;

	m_copMaxDamage = COP_DEFAULT_DAMAGE;
	m_copAccelerationModifier = 1.0f;

	m_numMaxCops = 2;
	m_copRespawnInterval = 20;	// spawn on every 20th traffic car
	m_numMaxTrafficCars = 32;
	m_copSpawnIntervalCounter = 0;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;
}

CAICarManager::~CAICarManager()
{

}

void CAICarManager::Init()
{
	// reset variables
	m_copMaxDamage = COP_DEFAULT_DAMAGE;
	m_copAccelerationModifier = 1.0f;
	m_numMaxTrafficCars = g_trafficMaxCars.GetInt();

	m_trafficUpdateTime = 0.0f;
	m_copSpawnIntervalCounter = 0;

	for (int i = 0; i < COP_NUMTYPES; i++)
		m_copCarName[i] = "";

	m_copTauntTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY + 5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}

void CAICarManager::Shutdown()
{
	m_civCarEntries.clear();

	m_trafficCars.clear();
	m_copCars.clear();
	m_roadBlockCars.clear();
	m_speechQueue.clear();
}

#pragma fixme("SpawnRandomTrafficCar: Replay solution for cop road blocks")

CCar* CAICarManager::SpawnRandomTrafficCar(const IVector2D& globalCell, int carType, bool doChecks)
{
	if (doChecks && m_trafficCars.numElem() >= m_numMaxTrafficCars)
		return NULL;

	CLevelRegion* pReg = NULL;
	levroadcell_t* roadCell = g_pGameWorld->m_level.GetGlobalRoadTileAt(globalCell, &pReg);

	// no tile - no spawn
	if (!pReg || !roadCell)
		return NULL;

	if (!pReg->m_isLoaded)
		return NULL;

	if (roadCell->type != ROADTYPE_STRAIGHT)
		return NULL;

	if(doChecks)
	{
		straight_t str = g_pGameWorld->m_level.GetStraightAtPoint(globalCell, 2);

		if (str.breakIter <= 1)
			return NULL;

		bool canSpawn = true;

		for (int j = 0; j < m_trafficCars.numElem(); j++)
		{
			if (!m_trafficCars[j]->GetPhysicsBody())
				continue;

			IVector2D trafficPosGlobal;
			if (!g_pGameWorld->m_level.GetTileGlobal(m_trafficCars[j]->GetOrigin(), trafficPosGlobal))
				continue;

			if (distance(trafficPosGlobal, globalCell) < TRAFFIC_BETWEEN_DISTANCE*TRAFFIC_BETWEEN_DISTANCE)
			{
				canSpawn = false;
				break;
			}
		}

		if (!canSpawn)
			return NULL;

		Vector3D newSpawnPos = g_pGameWorld->m_level.GlobalTilePointToPosition(globalCell);

		// if velocity is negative to new spawn origin, cancel spawning
		if( dot(newSpawnPos-m_leadPosition, m_leadVelocity) < 0 )
			return NULL;
	}

	CAITrafficCar* pNewCar = NULL;

	if(carType == CAR_TYPE_NORMAL)
	{
		m_copSpawnIntervalCounter++;

		if (m_copSpawnIntervalCounter >= m_copRespawnInterval && m_enableCops)	// every 8 car is a cop car
		{
			m_copSpawnIntervalCounter = 0;

			if (doChecks && m_copCars.numElem() >= GetMaxCops())
				return NULL;

			carConfigEntry_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[COP_LIGHT].c_str());

			if (!conf)
				return NULL;

			CAIPursuerCar* pCopCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);
			pNewCar = pCopCar;

			m_copCars.append(pCopCar);
		}
		else
		{
			// regenerate random number two times
			g_pGameWorld->m_random.Regenerate();

			int randCar = g_pGameWorld->m_random.Get(0, m_civCarEntries.numElem() - 1);

			g_pGameWorld->m_random.Regenerate();

			pNewCar = new CAITrafficCar(m_civCarEntries[randCar]);
		}
	}
	else
	{
		switch(carType)
		{
			case CAR_TYPE_TRAFFIC_AI:
			{
				// regenerate random number two times
				g_pGameWorld->m_random.Regenerate();

				int randCar = g_pGameWorld->m_random.Get(0, m_civCarEntries.numElem() - 1);

				g_pGameWorld->m_random.Regenerate();

				pNewCar = new CAITrafficCar(m_civCarEntries[randCar]);
				break;
			}
			case CAR_TYPE_PURSUER_COP_AI:
			{
				carConfigEntry_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[COP_LIGHT].c_str());

				if (!conf)
					return NULL;

				CAIPursuerCar* pCopCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);
				
				pNewCar = pCopCar;

				m_copCars.append(pCopCar);
				break;
			}
			case CAR_TYPE_PURSUER_GANG_AI:
			{
				// regenerate random number two times
				g_pGameWorld->m_random.Regenerate();

				int randCar = g_pGameWorld->m_random.Get(0, m_civCarEntries.numElem() - 1);

				g_pGameWorld->m_random.Regenerate();

				pNewCar = new CAIPursuerCar(m_civCarEntries[randCar], PURSUER_TYPE_GANG);
				break;
			}
		}
	}

	if(!pNewCar)
		return NULL;

	pNewCar->Spawn();

	pNewCar->InitAI(pReg, roadCell);

	g_pGameWorld->AddObject(pNewCar, true);

	m_trafficCars.append(pNewCar);

	return pNewCar;
}

void CAICarManager::CircularSpawnTrafficCars(int x0, int y0, int radius)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	
	SpawnRandomTrafficCar(IVector2D(x0, y0 + radius));
	SpawnRandomTrafficCar(IVector2D(x0, y0 - radius));
	SpawnRandomTrafficCar(IVector2D(x0 + radius, y0));
	SpawnRandomTrafficCar(IVector2D(x0 - radius, y0));
	
	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		SpawnRandomTrafficCar(IVector2D(x0 + x, y0 + y));
		SpawnRandomTrafficCar(IVector2D(x0 - x, y0 + y));
		SpawnRandomTrafficCar(IVector2D(x0 + x, y0 - y));
		SpawnRandomTrafficCar(IVector2D(x0 - x, y0 - y));
		SpawnRandomTrafficCar(IVector2D(x0 + y, y0 + x));
		SpawnRandomTrafficCar(IVector2D(x0 - y, y0 + x));
		SpawnRandomTrafficCar(IVector2D(x0 + y, y0 - x));
		SpawnRandomTrafficCar(IVector2D(x0 - y, y0 - x));
	}
}

void CAICarManager::RemoveTrafficCar(CAITrafficCar* car)
{
	m_copCars.remove((CAIPursuerCar*)car);
	m_roadBlockCars.remove(car);
	g_pGameWorld->RemoveObject(car);
}

void CAICarManager::UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{
#ifdef BIG_REPLAYS
	if (g_replayData->m_state == REPL_PLAYING)
		return;
#endif // BIG_REPLAYS

	m_trafficUpdateTime -= fDt;

	if (m_trafficUpdateTime <= 0.0f)
		m_trafficUpdateTime = 0.5f;
	else
		return;

	//-------------------------------------------------

	IVector2D spawnCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(spawnOrigin, spawnCenterCell))
		return;

	IVector2D removeCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(removeOrigin, removeCenterCell))
		return;

	m_leadPosition = spawnOrigin;
	m_leadRemovePosition = removeOrigin;
	m_leadVelocity = leadVelocity;

	// Try to remove cars
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		Vector3D carPos = m_trafficCars[i]->GetOrigin();

		CLevelRegion* reg = g_pGameWorld->m_level.GetRegionAtPosition(carPos);

		if (!reg || (reg && !reg->m_isLoaded))
		{
			RemoveTrafficCar(m_trafficCars[i]);
			m_trafficCars.fastRemoveIndex(i);
			i--;
			continue;
		}

		// non-pursuer vehicles are removed by distance.
		// pursuers are not, if in pursuit only.
		if( m_trafficCars[i]->IsPursuer() )
		{
			CAIPursuerCar* pursuer = (CAIPursuerCar*)m_trafficCars[i];

			if(pursuer->InPursuit())
				continue;
		}

		IVector2D trafficCell;
		reg->m_heightfield[0]->PointAtPos(carPos, trafficCell.x, trafficCell.y);

		g_pGameWorld->m_level.LocalToGlobalPoint(trafficCell, reg, trafficCell);

		int distToCell = length(trafficCell - removeCenterCell);
		int distToCell2 = length(trafficCell - spawnCenterCell);

		if (distToCell > g_traffic_maxdist.GetInt() && 
			distToCell2 > g_traffic_maxdist.GetInt())
		{
			RemoveTrafficCar(m_trafficCars[i]);
			m_trafficCars.fastRemoveIndex(i);
			i--;
			continue;
		}
	}

	CircularSpawnTrafficCars(spawnCenterCell.x, spawnCenterCell.y, g_traffic_mindist.GetInt());
}

//-----------------------------------------------------------------------------------------

void CAICarManager::UpdateCopStuff(float fDt)
{
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "cops spawned: %d (max %d) (cntr=%d, lvl=%d)\n", m_copCars.numElem(), GetMaxCops(), m_copSpawnIntervalCounter, m_copRespawnInterval);
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "num traffic cars: %d\n", m_trafficCars.numElem());

	debugoverlay->Text(ColorRGBA(1, 1, 1, 1), "cop speech time: normal %g taunt %g\n", m_copSpeechTime, m_copTauntTime);

	m_copSpeechTime -= fDt;
	m_copTauntTime -= fDt;

	ISoundPlayable* copVoiceChannel = soundsystem->GetStaticStreamChannel(CHAN_VOICE);

	// say something
	if (copVoiceChannel->GetState() == SOUND_STATE_STOPPED && m_speechQueue.numElem() > 0)
	{
		EmitSound_t ep;
		ep.name = m_speechQueue[0].c_str();

		ses->Emit2DSound(&ep);

		m_speechQueue.removeIndex(0);
	}
}

void CAICarManager::RemoveAllCars()
{
	// Try to remove cars
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		RemoveTrafficCar(m_trafficCars[i]);
	}
	m_trafficCars.clear();
}

DECLARE_CMD(force_roadblock, "Forces spawn roadblock on cur car", CV_CHEAT)
{
	float targetAngle = VectorAngles(normalize(g_pGameSession->GetPlayerCar()->GetVelocity())).y+45.0f;

	g_pAIManager->SpawnRoadBlockFor( g_pGameSession->GetPlayerCar(), targetAngle );
}

IVector2D GetDirectionVec(int dirIdx);

bool CAICarManager::SpawnRoadBlockFor( CCar* car, float directionAngle )
{
	IVector2D playerCarCell;
	if (!g_pGameWorld->m_level.GetTileGlobal( car->GetOrigin(), playerCarCell ))
		return false;

	directionAngle = NormalizeAngle360( -directionAngle );

	int targetDir = directionAngle / 90.0f;

	if (targetDir < 0)
		targetDir += 4;

	if (targetDir > 3)
		targetDir -= 4;

	int radius = g_traffic_mindist.GetInt();

	IVector2D carDir = GetDirectionVec(targetDir);

	IVector2D placementVec = playerCarCell - carDir*radius;
	IVector2D perpendicular = GetDirectionVec(targetDir+1);

	int curLane = g_pGameWorld->m_level.GetLaneIndexAtPoint(placementVec, 16)-1;

	placementVec -= perpendicular*curLane;

	int numLanes = g_pGameWorld->m_level.GetRoadWidthInLanesAtPoint(placementVec, 32);

	//Msg("Gonna spawn roadblock on %d cells wide road (start=%d)\n", numLanes, curLane);

	int nCars = 0;

	carConfigEntry_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[COP_LIGHT].c_str());

	if (!conf)
		return 0;

	Vector3D startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec);
	Vector3D endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec+perpendicular*numLanes);

	debugoverlay->Line3D(startPos, endPos, ColorRGBA(1,0,0,1), ColorRGBA(0,1,0,1), 1000.0f);

	for(int i = 0; i < numLanes; i++)
	{
		IVector2D blockPoint = placementVec + carDir*IVector2D(i % 2)*3; // offset in 3 tiles while checker

		placementVec += perpendicular;

		CLevelRegion* pReg = NULL;
		levroadcell_t* roadCell = g_pGameWorld->m_level.GetGlobalRoadTileAt(blockPoint, &pReg);

		// no tile - no spawn
		if (!pReg || !roadCell)
		{
			MsgError( "Can't spawn because no road\n" );
			continue;
		}

		if (!pReg->m_isLoaded)
			continue;

		Vector3D gPos = g_pGameWorld->m_level.GlobalTilePointToPosition(blockPoint);

		debugoverlay->Box3D(gPos-0.5f, gPos+0.5f, ColorRGBA(1,1,0,0.5f), 1000.0f);

		if (roadCell->type != ROADTYPE_STRAIGHT)
		{
			MsgError( "Can't spawn whene no straight road (%d)\n", roadCell->type);
			continue;
		}

		CAIPursuerCar* copBlockCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);

		copBlockCar->Enable(false);
		copBlockCar->m_sirenEnabled = true;

		copBlockCar->Spawn();
		copBlockCar->InitAI(pReg, roadCell);

		g_pGameWorld->AddObject(copBlockCar, true);

		float angle = 120.0f - (i % 2)*60.0f;

		copBlockCar->SetAngles(Vector3D(0.0f, targetDir*-90.0f + angle, 0.0f));

		m_roadBlockCars.append( copBlockCar );
		m_copCars.append( copBlockCar );
		m_trafficCars.append( copBlockCar );
		nCars++;
	}

	//Msg("Made %d cars on %d wide road\n", nCars, numLanes);

	return m_roadBlockCars.numElem() > 0;
}

bool CAICarManager::IsRoadBlockSpawn() const
{
	return m_roadBlockCars.numElem() > 0;
}

// ----- TRAFFIC ------
void CAICarManager::SetMaxTrafficCars(int count)
{
	m_numMaxTrafficCars = count;
}

int CAICarManager::GetMaxTrafficCars() const
{
	return m_numMaxTrafficCars;
}

// ----- COPS ------
// switch to spawn
void CAICarManager::SetCopsEnabled(bool enable)
{
	m_enableCops = enable;
}

bool CAICarManager::IsCopsEnabled() const
{
	return m_enableCops;
}

// maximum cop count
void CAICarManager::SetMaxCops(int count)
{
	m_numMaxCops = count;
}

int CAICarManager::GetMaxCops() const
{
	return m_numMaxCops;
}

// cop respawn interval between traffic car spawns
void CAICarManager::SetCopRespawnInterval(int steps)
{
	m_copRespawnInterval = steps;
}

int CAICarManager::GetCopRespawnInterval() const
{
	return m_copRespawnInterval;
}

// acceleration modifier (e.g. for survival)
void CAICarManager::SetCopAccelerationModifier(float accel)
{
	m_copAccelerationModifier = accel;
}

float CAICarManager::GetCopAccelerationModifier() const
{
	return m_copAccelerationModifier;
}

// sets the maximum hitpoints for cop cars
void CAICarManager::SetCopMaxDamage(float maxHitPoints)
{
	m_copMaxDamage = maxHitPoints;
}
					
float CAICarManager::GetCopMaxDamage() const
{
	return m_copMaxDamage;
}

// sets cop car configuration
void CAICarManager::SetCopCarConfig(const char* car_name, int type )
{
	m_copCarName[type] = car_name;
}

// shedules a cop speech
bool CAICarManager::MakeCopSpeech(const char* soundScriptName, bool force)
{
	// shedule speech
	if (m_copSpeechTime < 0 || force)
	{
		m_speechQueue.append(soundScriptName);

		m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);

		return true;
	}

	return false;
}

bool CAICarManager::IsCopCanSayTaunt() const
{
	return (m_copSpeechTime < 0) && (m_copTauntTime < 0);
}

void CAICarManager::GotCopTaunt()
{
	m_copTauntTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY+5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}


OOLUA_EXPORT_FUNCTIONS(
	CAICarManager,

	RemoveAllCars,
	SetMaxTrafficCars,
	SetCopsEnabled,
	SetCopCarConfig,
	SetCopMaxDamage,
	SetCopAccelerationModifier,
	SetMaxCops,
	SetCopRespawnInterval,
	MakeCopSpeech,
	SpawnRoadBlockFor
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CAICarManager,

	GetMaxTrafficCars,
	IsCopsEnabled,
	GetCopMaxDamage,
	GetCopAccelerationModifier,
	GetMaxCops,
	GetCopRespawnInterval,
	IsRoadBlockSpawn
)
