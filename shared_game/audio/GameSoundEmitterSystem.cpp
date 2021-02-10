//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

#include "GameSoundEmitterSystem.h"
#include "core_base_header.h"
#include "IEngineWorld.h"
#include "IEngineHost.h"
#include "IViewRenderer.h"
#include "IDebugOverlay.h"
#include "math/Random.h"
#include "eqGlobalMutex.h"
#include "eqParallelJobs.h"

#ifndef NO_ENGINE
#include "IEngineHost.h"
#include "GlobalVarsBase.h"
#endif

static CSoundEmitterSystem s_ses;

CSoundEmitterSystem* g_sounds = &s_ses;

void sounds_list(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	s_ses.GetAllSoundNames(list);
}

DECLARE_CMD_VARIANTS(test_scriptsound, "Test the scripted sound", sounds_list, 0)
{
	if(CMD_ARGC > 0)
	{
		g_sounds->PrecacheSound( CMD_ARGV(0).c_str() );

		EmitSound_t ep;
		ep.nFlags = (EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D);
		ep.fPitch = 1.0;
		ep.fRadiusMultiplier = 1.0f;
		ep.fVolume = 1.0f;
		ep.origin = Vector3D(0);
		ep.name = (char*)CMD_ARGV(0).c_str();
		ep.pObject = NULL;

		if(g_sounds->EmitSound( &ep ) == CHAN_INVALID)
			MsgError("Cannot play - not valid sound '%s'\n", CMD_ARGV(0).c_str());
	}
}

ConVar emitsound_debug("scriptsound_debug", "0", NULL, CV_CHEAT);

ConVar snd_effectsvolume("snd_effectsvolume", "1.0", NULL, CV_UNREGISTERED);
ConVar snd_musicvolume("snd_musicvolume", "0.5", NULL, CV_ARCHIVE);
ConVar snd_voicevolume("snd_voicevolume", "0.5", NULL, CV_ARCHIVE);

bool CSoundController::IsStopped() const
{
	if(!m_emitData)
		return true;

	if(!m_emitData->soundSource)
		return true;

	return (m_emitData->soundSource->GetState() == SOUND_STATE_STOPPED);
}

void CSoundController::StartSound(const char* newSoundName)
{
	// if stopped
	if(!m_emitData)
	{
		if (newSoundName != NULL)
		{
			m_soundName = newSoundName;
			m_emitParams.name = (char*)m_soundName.c_str();
		}

		m_emitParams.pController = this;

		// sound must be aquired
		int chan = g_sounds->EmitSound(&m_emitParams);

		// can't start
		if(chan == CHAN_INVALID)
		{
			MsgError("Can't emit sound '%s', CHAN_INVALID\n", m_emitParams.name);
			return;
		}

		if(m_emitParams.emitterIndex < 0 || m_emitParams.emitterIndex > g_sounds->m_emitters.numElem()-1)
		{
			MsgError("Can't emit sound '%s', no out of emitters\n", m_emitParams.name);
			return;
		}

		// get emitter
		m_emitData = g_sounds->m_emitters[m_emitParams.emitterIndex];

		return;
	}

	if(!m_emitData->soundSource)
	{
		m_emitData = NULL;
		return;
	}

	// resume previous sample
	m_emitData->soundSource->Play();
}

void CSoundController::Play()
{
	StartSound(NULL);
}

void CSoundController::Pause()
{
	if(!m_emitData)
		return;

	if(!m_emitData->soundSource)
		return;

	ASSERT(soundsystem->IsValidEmitter( m_emitData->soundSource ));

	m_emitData->soundSource->Pause();
}

void CSoundController::Stop(bool force)
{
	if(!m_emitData)
	{
		return;
	}

	if(!m_emitData->soundSource)
	{
		m_emitData->controller = NULL;
		m_emitParams.emitterIndex = -1;
		return;
	}

	if(!force && m_emitData->script && m_emitData->script->stopLoop)
		m_emitData->soundSource->StopLoop();
	else
		m_emitData->soundSource->Stop();

	// at stop we invalidate channel and detach emitter
	m_emitData->controller = NULL;
	m_emitParams.emitterIndex = -1;
	m_emitData = NULL;
}

void CSoundController::StopLoop()
{
	if(!m_emitData)
	{
		return;
	}

	if(!m_emitData->soundSource)
	{
		m_emitData->controller = NULL;
		m_emitParams.emitterIndex = -1;
		return;
	}

	m_emitData->soundSource->StopLoop();
}

void CSoundController::SetPitch(float fPitch)
{
	if(!m_emitData)
		return;

	if(!m_emitData->soundSource)
		return;

	float fSoundPitch = m_emitData->script->fPitch;

	soundParams_t params;

	m_emitData->soundSource->GetParams(&params);

	params.pitch = fSoundPitch*fPitch;

	m_emitData->soundSource->SetParams(&params);
}

void CSoundController::SetVolume(float fVolume)
{
	m_volume = fVolume;
}

void CSoundController::SetOrigin(const Vector3D& origin)
{
	m_emitParams.origin = origin;

	if(!m_emitData)
		return;

	m_emitData->origin = origin;

	if (!m_emitData->soundSource)
		return;

	m_emitData->soundSource->SetPosition(m_emitData->origin);
}

void CSoundController::SetVelocity(const Vector3D& velocity)
{
	if(!m_emitData)
		return;

	m_emitData->velocity = velocity;

	if (!m_emitData->soundSource)
		return;

	m_emitData->soundSource->SetVelocity(m_emitData->velocity);
}

//----------------------------------------------------------------------------

CSoundChannelObject::CSoundChannelObject() : m_volumeScale(1.0f)
{
	memset(m_numChannelSounds, 0, sizeof(m_numChannelSounds));
}

CSoundChannelObject::~CSoundChannelObject()
{
	g_sounds->InvalidateSoundChannelObject(this);
}

void CSoundChannelObject::EmitSound(const char* name)
{
	EmitSound_t ep;
	ep.pObject = this;
	ep.name = (char*)name;

	int channelType = g_sounds->EmitSound(&ep);

	if(channelType == CHAN_INVALID)
		return;

	m_numChannelSounds[channelType]++;
}

void CSoundChannelObject::EmitSoundWithParams(EmitSound_t *ep)
{
	ep->pObject = this;

	int channelType = g_sounds->EmitSound(ep);

	if(channelType == CHAN_INVALID)
		return;

	m_numChannelSounds[channelType]++;
}

int CSoundChannelObject::GetChannelSoundCount(ESoundChannelType chan)
{
	return m_numChannelSounds[chan];
}

void CSoundChannelObject::DecrementChannelSoundCount(ESoundChannelType chan)
{
	if(chan == CHAN_INVALID)
		return;

	if(m_numChannelSounds[chan] > 0)
		m_numChannelSounds[chan]--;
}

void EmitSound_t::Init( const char* pszName, const Vector3D& pos, float volume, float pitch, float radius, CSoundChannelObject* obj, int flags )
{
	name = pszName;
	fVolume = volume;
	fPitch = pitch;
	fRadiusMultiplier = radius;
	pObject = nullptr;
	pController = nullptr;
	nFlags = flags;	// all sounds forced to use simple occlusion
	origin = pos;
	sampleId = -1;
	emitterIndex = -1;
}

//----------------------------------------------------------------------------
//
//    SOUND EMITTER SYSTEM
//
//----------------------------------------------------------------------------

CSoundEmitterSystem::CSoundEmitterSystem() :
	m_isInit(false),
	m_defaultMaxDistance(100.0f),
	m_isPaused(false),
	m_mutex(Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_AUDIO))
{

}

void CSoundEmitterSystem::Init(float maxDistance, fnSoundEmitterUpdate updFunc /* = nullptr*/)
{
	if(m_isInit)
		return;

	m_defaultMaxDistance = maxDistance;
	m_fnEmitterProcess = updFunc;

	m_totalErrors = 0;

	for (int i = 0; i < CHAN_COUNT; i++)
	{
		m_2dScriptVolume[i] = 1.0f;
		m_2dChannelVolume[i] = 1.0f;
	}

	kvkeybase_t* soundSettings = GetCore()->GetConfig()->FindKeyBase("Sound");

	const char* baseScriptFilePath = soundSettings ? KV_GetValueString(soundSettings->FindKeyBase("EmitterScripts"), 0, NULL) : NULL;

	if(baseScriptFilePath == NULL)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EmitterScripts !\n");
		return;
	}

	LoadScriptSoundFile(baseScriptFilePath);

	m_isPaused = false;
	m_isInit = true;
}

void CSoundEmitterSystem::Shutdown()
{
	m_fnEmitterProcess = nullptr;
	m_isPaused = false;

	StopAllSounds();

	for(int i = 0; i < m_controllers.numElem(); i++)
		delete m_controllers[i];
	m_controllers.clear();

	for(int i = 0; i < m_allSounds.numElem(); i++)
	{
		for(int j = 0; j < m_allSounds[i]->pSamples.numElem(); j++)
			soundsystem->ReleaseSample( m_allSounds[i]->pSamples[j] );

		delete [] m_allSounds[i]->pszName;
		delete m_allSounds[i];
	}
	m_allSounds.clear();

	m_isInit = false;
}

void CSoundEmitterSystem::SetPaused(bool paused)
{
	m_isPaused = paused;
}

bool CSoundEmitterSystem::IsPaused()
{
	return m_isPaused;
}

int CSoundEmitterSystem::GetEmitterIndexByEntityAndChannel(CSoundChannelObject* pEnt, ESoundChannelType chan)
{
	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		if(m_emitters[i]->channelObj == pEnt && m_emitters[i]->channelType == chan)
			return i;
	}

	return -1;
}

ISoundController* CSoundEmitterSystem::CreateSoundController(EmitSound_t *ep)
{
	ASSERT(ep);
	ASSERT(ep->name);

	// check
	if(!FindSound(ep->name))
		MsgWarning("CreateSoundController: invalid sound '%s'!\n", ep->name);

	CSoundController* pController = new CSoundController();

	{
		Threading::CScopedMutex m(m_mutex);
		m_controllers.append(pController);
	}

	// copy sound name
	pController->m_soundName.Assign(ep->name); 

	pController->m_emitParams = *ep;
	pController->m_emitParams.name = (char*)pController->m_soundName.c_str();

	return pController;
}

void CSoundEmitterSystem::RemoveSoundController(ISoundController* cont)
{
	if(!cont)
		return;

	{
		Threading::CScopedMutex m(m_mutex);
		if (m_controllers.fastRemove(cont))
		{
			cont->Stop();
			delete cont;
		}
	}
}

void CSoundEmitterSystem::InvalidateSoundChannelObject(CSoundChannelObject* pEnt)
{
	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		if (m_emitters[i]->channelObj == pEnt)
			m_emitters[i]->channelObj = nullptr;
	}
}

void CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	// find the present sound file
	soundScriptDesc_t* pSound = FindSound(pszName);

	if(!pSound)
		return;

	if(pSound->pSamples.numElem() > 0)
		return;

#ifndef NO_ENGINE
	g_pEngineHost->EnterResourceLoading();
#endif

	Threading::CScopedMutex m(m_mutex);

	for(int i = 0; i < pSound->soundFileNames.numElem(); i++)
	{
		int flags = (pSound->extraStreaming ? SAMPLE_FLAG_STREAMED : 0) | (pSound->loop ? SAMPLE_FLAG_LOOPING : 0);

		ISoundSample* pCachedSample = soundsystem->LoadSample(pSound->soundFileNames[i].GetData(), flags);

		if (pCachedSample)
			pSound->pSamples.append(pCachedSample);
		else
			m_totalErrors++;
	}

#ifndef NO_ENGINE
	 if(gpGlobals->curtime > 0)
		 MsgWarning("WARNING! Late precache of sound '%s'\n", pszName);

	 g_pEngineHost->EndResourceLoading();
#endif
}

void CSoundEmitterSystem::GetAllSoundNames(DkList<EqString>& soundNames) const
{
	for (int i = 0; i < m_allSounds.numElem(); i++)
	{
		soundNames.append(m_allSounds[i]->pszName);
	}
}

soundScriptDesc_t* CSoundEmitterSystem::FindSound(const char* soundName) const
{
	EqString sname(soundName);
	sname = sname.LowerCase();

	int snameHash = StringToHash( sname.c_str(), true );

	for(int i = 0; i < m_allSounds.numElem(); i++)
	{
		if(m_allSounds[i]->namehash == snameHash)
			return m_allSounds[i];
	}

	return NULL;
}

// simple sound emitter
int CSoundEmitterSystem::EmitSound(EmitSound_t* emit)
{
	ASSERT(emit);

	if((emit->nFlags & EMITSOUND_FLAG_START_ON_UPDATE) || m_isPaused)
	{
		Threading::CScopedMutex m(m_mutex);

		EmitSound_t newEmit = (*emit);
		newEmit.nFlags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.nFlags |= EMITSOUND_FLAG_PENDING;
		newEmit.name = xstrdup(newEmit.name);

		m_pendingStartSounds.append( newEmit );
		return CHAN_INVALID;
	}

	// quickly set
	emit->emitterIndex = -1;

	soundScriptDesc_t* script = FindSound(emit->name);

	if (!script)
	{
		if (emitsound_debug.GetBool())
			MsgError("EmitSound: unknown sound '%s'\n", emit->name);

		return CHAN_INVALID;
	}

	if(script->pSamples.numElem() == 0 && (emit->nFlags & EMITSOUND_FLAG_FORCE_CACHED))
	{
		Threading::CScopedMutex m(m_mutex);
		MsgWarning("Warning! use of EMITSOUND_FLAG_FORCE_CACHED flag!\n");
		PrecacheSound( emit->name );
	}

	ISoundSample* bestSample = FindBestSample(script, emit->sampleId);

	if (!bestSample)
		return CHAN_INVALID;

	CSoundChannelObject* pChanObj = emit->pObject;

	if(pChanObj)
	{
		int usedSounds = pChanObj->GetChannelSoundCount(script->channelType);

		// if entity reached the maximum sound count for self
		if(usedSounds >= s_soundChannelMaxEmitters[script->channelType])
		{
			Threading::CScopedMutex m(m_mutex);
			
			// find currently playing sound index
			int firstPlayingSound = GetEmitterIndexByEntityAndChannel( pChanObj, script->channelType );

			if(firstPlayingSound != -1)
			{
				EmitterData_t* substEmitter = m_emitters[firstPlayingSound];

				// if index is valid, shut up this sound
				if(substEmitter->controller)
					substEmitter->controller->Stop();
				else
					substEmitter->soundSource->Stop();

				// and remove
				soundsystem->FreeEmitter( substEmitter->soundSource );
				substEmitter->soundSource = NULL;
				delete substEmitter;
				m_emitters.fastRemoveIndex(firstPlayingSound);

				// pChanObj sound count
				pChanObj->DecrementChannelSoundCount(script->channelType);
			}
		}
	}

	// alloc SoundEmitterSystem emitter data
	EmitterData_t* edata = new EmitterData_t();

	edata->emitParams = *emit;
	edata->script = script;

	edata->origin = edata->interpolatedOrigin = emit->origin;
	edata->startVolume = script->fVolume * emit->fVolume;
	edata->channelType = script->channelType;

	edata->channelObj = emit->pObject;
	edata->controller = emit->pController;

	// sound parameters to initialize SoundEmitter
	soundParams_t sParams;

	sParams.pitch = emit->fPitch * script->fPitch;
	sParams.volume = edata->startVolume;

	sParams.maxDistance			= script->fMaxDistance;
	sParams.referenceDistance	= script->fAtten * emit->fRadiusMultiplier;

	sParams.rolloff				= script->fRolloff;
	sParams.airAbsorption		= script->fAirAbsorption;
	sParams.is2D				= script->is2d;

	ISoundEmitter* sndSource;
	
	{
		Threading::CScopedMutex m(m_mutex);
		sndSource = soundsystem->AllocEmitter();

		// setup default values
		sndSource->SetPosition(edata->origin);
		sndSource->SetVelocity(edata->velocity);

		// update emitter before playing
		edata->soundSource = sndSource;
		UpdateEmitter(edata, sParams, true);
	}

	if (emit->nFlags & EMITSOUND_FLAG_STARTSILENT)
		sParams.volume = 0.0f;

	if(emitsound_debug.GetBool())
	{
		float boxsize = sParams.referenceDistance;

		debugoverlay->Box3D(edata->origin-boxsize, edata->origin+boxsize, ColorRGBA(1,1,1,0.5f), 1.0f);
		MsgInfo("started sound '%s' ref=%g max=%g\n", script->pszName, sParams.referenceDistance, sParams.maxDistance);
	}

#pragma todo("randomization flag for sounding pitch")
	//if(!script->loop)
	//	sParams.pitch += RandomFloat(-0.05f,0.05f);

	{
		Threading::CScopedMutex m(m_mutex);
		sndSource->SetSample(bestSample);
		sndSource->SetParams(&sParams);
		sndSource->Play();

		emit->emitterIndex = m_emitters.append(edata);
	}

	// return channel type
	return edata->channelType;
}

void CSoundEmitterSystem::Emit2DSound(EmitSound_t* emit, int channelType)
{
	ASSERT(emit);

	// simple protection agains audio thiefs
	if (m_totalErrors > 8)
		return;

	if ((emit->nFlags & EMITSOUND_FLAG_START_ON_UPDATE) || m_isPaused)
	{
		Threading::CScopedMutex m(m_mutex);

		EmitSound_t newEmit = (*emit);
		newEmit.nFlags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.nFlags |= EMITSOUND_FLAG_PENDING;
		newEmit.emitterIndex = channelType;	// temporary
		newEmit.name = xstrdup(newEmit.name);

		m_pendingStartSounds2D.append(newEmit);
		return;
	}

	// quickly set
	emit->emitterIndex = -1;

	soundScriptDesc_t* script = FindSound(emit->name);

	if (!script)
	{
		if (emitsound_debug.GetBool())
			MsgError("Emit2DSound: unknown sound '%s'\n", emit->name);

		return;
	}

	if(script->pSamples.numElem() == 0 && (emit->nFlags & EMITSOUND_FLAG_FORCE_CACHED))
		PrecacheSound( emit->name );

	ISoundSample* bestSample = FindBestSample(script, emit->sampleId);

	if (!bestSample)
		return;

	if (emitsound_debug.GetBool())
	{
		MsgInfo("started 2D sound '%s'\n", script->pszName);
	}

	// use the channel defined by the script
	if(channelType == -1)
		channelType = script->channelType;

	{
		Threading::CScopedMutex m(m_mutex);

		ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(channelType);

		if (staticChannel)
		{
			m_2dScriptVolume[channelType] = script->fVolume;

			float startVolume = m_2dChannelVolume[channelType] * m_2dScriptVolume[channelType] * emit->fVolume;
			float startPitch = emit->fPitch*script->fPitch;

			if (channelType == CHAN_STREAM)
				startVolume = m_2dChannelVolume[channelType] * m_2dScriptVolume[channelType] * snd_musicvolume.GetFloat();
			else if (channelType == CHAN_VOICE)
				startVolume = m_2dChannelVolume[channelType] * m_2dScriptVolume[channelType] * snd_voicevolume.GetFloat();

			staticChannel->SetSample(bestSample);
			staticChannel->SetVolume(startVolume);
			staticChannel->SetPitch(startPitch);

			if (emit->nFlags & EMITSOUND_FLAG_PENDING)
			{
				staticChannel->Play();
			}
			else
			{
				// stop stream before we can send a job
				if (staticChannel->GetState() == ESoundState::SOUND_STATE_PLAYING)
					staticChannel->Stop();

				g_parallelJobs->AddJob(JobSchedulePlayStaticChannel, staticChannel);
				g_parallelJobs->Submit();
			}
		}
	}
}

void CSoundEmitterSystem::JobSchedulePlayStaticChannel(void* data, int i)
{
	ISoundPlayable* staticChannel = (ISoundPlayable*)data;
	staticChannel->Play();
}

bool CSoundEmitterSystem::UpdateEmitter( EmitterData_t* emitter, soundParams_t &params, bool bForceNoInterp )
{
	ISoundEmitter* source = emitter->soundSource;

	ASSERT(soundsystem->IsValidEmitter(source));

	CSoundController* controller = (CSoundController*)emitter->controller;

	float fBestVolume = emitter->startVolume;

	if(m_fnEmitterProcess)
		fBestVolume = (m_fnEmitterProcess)(params, emitter, bForceNoInterp);
	
	if (controller)
	{
		fBestVolume *= controller->m_volume;
	}
	
	if(emitter->channelObj)
		fBestVolume *= emitter->channelObj->GetSoundVolumeScale();
	
	params.volume = fBestVolume * snd_effectsvolume.GetFloat();

	if (!controller && (emitter->soundSource->GetState() == SOUND_STATE_STOPPED))
		return false;

	return true;
}

void CSoundEmitterSystem::Set2DChannelsVolume(ESoundChannelType channelType, float volume)
{
	if (channelType < 0 || channelType >= CHAN_COUNT)
		return;

	m_2dChannelVolume[channelType] = volume;
}

void CSoundEmitterSystem::StopAllSounds()
{
	StopAll2DSounds();
	StopAllEmitters();
}

void CSoundEmitterSystem::StopAllEmitters()
{
	// remove pending sounds
	for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
	{
		EmitSound_t& snd = m_pendingStartSounds[i];
		delete[] snd.name;
	}
	m_pendingStartSounds.clear();

	for (int i = 0; i < m_controllers.numElem(); i++)
		m_controllers[i]->Stop(true);

	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		Threading::CScopedMutex m(m_mutex);
		
		EmitterData_t* em = m_emitters[i];

		if (em->soundSource)
		{
			em->soundSource->Pause();
			em->soundSource->Stop();
			soundsystem->FreeEmitter(em->soundSource);
		}
		em->soundSource = nullptr;

		delete em;
	}
	m_emitters.clear();
}

void CSoundEmitterSystem::StopAll2DSounds()
{
	// remove pending sounds
	for (int i = 0; i < m_pendingStartSounds2D.numElem(); i++)
	{
		EmitSound_t& snd = m_pendingStartSounds2D[i];
		delete[] snd.name;
	}

	m_pendingStartSounds2D.clear();

	// stop static channels
	int i = 0;
	while(true)
	{
		ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(i);

		i++;
		if(!staticChannel)
			break;

		staticChannel->Stop();
		staticChannel->SetSample(NULL);
	}
}

//
// Updates all emitters and sound system itself
//
void CSoundEmitterSystem::Update(float pitchScale, bool force)
{
	soundsystem->Update(pitchScale);

	// update music volume always
	ISoundPlayable* musicChannel = soundsystem->GetStaticStreamChannel(CHAN_STREAM);

	if (musicChannel)
		musicChannel->SetVolume(m_2dChannelVolume[CHAN_STREAM] * m_2dScriptVolume[CHAN_STREAM] * snd_musicvolume.GetFloat());

	// don't update
	if(!force && soundsystem->GetPauseState())
		return;

	// start all pending sounds we accumulated during sound pause
	if (!m_isPaused)
	{
		if (m_pendingStartSounds2D.numElem())
		{
			Threading::CScopedMutex m(m_mutex);

			// play sounds
			for (int i = 0; i < m_pendingStartSounds2D.numElem(); i++)
			{
				EmitSound_t& snd = m_pendingStartSounds2D[i];
				Emit2DSound(&snd, snd.emitterIndex);

				delete [] snd.name;
			}

			// release
			m_pendingStartSounds2D.clear();
		}

		if (m_pendingStartSounds.numElem())
		{
			Threading::CScopedMutex m(m_mutex);

			// play sounds
			for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
			{
				EmitSound_t& snd = m_pendingStartSounds[i];
				EmitSound(&snd);

				delete[] snd.name;
			}

			// release
			m_pendingStartSounds.clear();
		}
	}

	// update all sound emitters
	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		EmitterData_t* emitter = m_emitters[i];

		bool remove = (!emitter || (emitter && !emitter->soundSource));

		ISoundEmitter* em = emitter->soundSource;

		if( !remove )
		{
			soundParams_t params;
			em->GetParams( &params );

			remove = UpdateEmitter(emitter, params) == false;

			em->SetParams( &params );
		}

		if( remove )
		{
			Threading::CScopedMutex m(m_mutex);
			
			if(emitter->controller)
				emitter->controller->Stop(true);

			emitter->controller = nullptr;

			soundsystem->FreeEmitter(em);

			// delete this emitter
			delete emitter;
			m_emitters.fastRemoveIndex(i);
			i--;
		}
	}
}

//
// Loads sound scripts
//
void CSoundEmitterSystem::LoadScriptSoundFile(const char* fileName)
{
	KeyValues kv;
	if(!kv.LoadFromFile(fileName))
	{
		MsgError("*** Error! Failed to open script sound file '%s'!\n", fileName);
		return;
	}

	DevMsg(DEVMSG_SOUND, "Loading sound script file '%s'\n", fileName);

	for(int i = 0; i <  kv.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* kb = kv.GetRootSection()->keys[i];

		if(!stricmp( kb->GetName(), "include"))
			LoadScriptSoundFile( KV_GetValueString(kb) );
	}

	for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* curSec = kv.GetRootSection()->keys[i];

		if(!stricmp(curSec->name, "include"))
			continue;

		soundScriptDesc_t* newSound = new soundScriptDesc_t;

		newSound->pszName = xstrdup(curSec->name);
		newSound->namehash = StringToHash(newSound->pszName, true );

		newSound->fVolume = KV_GetValueFloat( curSec->FindKeyBase("volume"), 0, 1.0f );

		newSound->fAtten = KV_GetValueFloat( curSec->FindKeyBase("distance"), 0, m_defaultMaxDistance * 0.35f );
		newSound->fMaxDistance = KV_GetValueFloat(curSec->FindKeyBase("maxdistance"), 0, m_defaultMaxDistance);

		newSound->fPitch = KV_GetValueFloat( curSec->FindKeyBase("pitch"), 0, 1.0f );
		newSound->fRolloff = KV_GetValueFloat( curSec->FindKeyBase("rolloff"), 0, 1.0f );
		newSound->fAirAbsorption = KV_GetValueFloat( curSec->FindKeyBase("airabsorption"), 0, 0.0f );

		newSound->loop = KV_GetValueBool( curSec->FindKeyBase("loop"), 0, false );
		newSound->stopLoop = KV_GetValueBool( curSec->FindKeyBase("stopLoop"), 0, false );

		newSound->extraStreaming = KV_GetValueBool( curSec->FindKeyBase("streamed"), 0, false );
		newSound->is2d = KV_GetValueBool(curSec->FindKeyBase("is2d"), 0, false);

		kvkeybase_t* pKey = curSec->FindKeyBase("channel");

		if(pKey)
		{
			newSound->channelType = ChannelTypeByName(KV_GetValueString(pKey));

			if(newSound->channelType == CHAN_INVALID)
			{
				Msg("Invalid channel '%s' for sound %s\n", KV_GetValueString(pKey), newSound->pszName);
				newSound->channelType = CHAN_STATIC;
			}
		}
		else
			newSound->channelType = CHAN_STATIC;

		// pick 'rndwave' or 'wave' sections for lists
		pKey = curSec->FindKeyBase("rndwave", KV_FLAG_SECTION);

		if(!pKey)
			pKey = curSec->FindKeyBase("wave", KV_FLAG_SECTION);

		if(pKey)
		{
			for(int j = 0; j < pKey->keys.numElem(); j++)
			{
				kvkeybase_t* ent = pKey->keys[j];

				if(stricmp(ent->name, "wave"))
					continue;

				newSound->soundFileNames.append(KV_GetValueString(ent));
			}
		}
		else
		{
			pKey = curSec->FindKeyBase("wave");

			if(pKey)
				newSound->soundFileNames.append(KV_GetValueString(pKey));
		}

		if(newSound->soundFileNames.numElem() == 0)
			MsgWarning("empty sound script '%s'!\n", newSound->pszName);

		m_allSounds.append(newSound);
	}
}

//
// picks best sample randomly.
//
ISoundSample* CSoundEmitterSystem::FindBestSample(soundScriptDesc_t* script, int sampleId /*= -1*/)
{
	int numSamples = script->pSamples.numElem();

	if (!numSamples)
	{
		MsgWarning("WARNING! Script sound '%s' is not cached!\n", script->pszName);
		return nullptr;
	}

	if (!script->pSamples.inRange(sampleId))	// if it is out of range, randomize
		sampleId = -1;

	if (sampleId < 0)
	{
		if (script->pSamples.numElem() == 1)
			return script->pSamples[0];
		else
			return script->pSamples[RandomInt(0, numSamples - 1)];
	}
	else
		return script->pSamples[sampleId];
}