//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQSOUNDSYSTEMAL_H
#define EQSOUNDSYSTEMAL_H

#include "audio/IEqAudioSystem.h"

#include "core/platform/Platform.h"

#include "utils/DkList.h"

#include "source/snd_source.h"


//-----------------------------------------------------------------

#define STREAM_BUFFER_COUNT		(4)
#define STREAM_BUFFER_SIZE		(1024*8) // 8 kb

//-----------------------------------------------------------------

struct kvkeybase_t;

// Audio system, controls voices
class CEqAudioSystemAL : public IEqAudioSystem
{
	friend class CEqAudioSourceAL;
public:
	CEqAudioSystemAL();
	~CEqAudioSystemAL();

	void						Init();
	void						Shutdown();

	IEqAudioSource*				CreateSource();
	void						DestroySource(IEqAudioSource* source);

	void						Update();

	void						StopAllSounds(int chanType = -1, void* callbackObject = nullptr);
	void						PauseAllSounds(int chanType = -1, void* callbackObject = nullptr);
	void						ResumeAllSounds(int chanType = -1, void* callbackObject = nullptr);

	void						SetMasterVolume(float value);

	// sets listener properties
	void						SetListener(const Vector3D& position,
											const Vector3D& velocity,
											const Vector3D& forwardVec,
											const Vector3D& upVec);

	// gets listener properties
	void						GetListener(Vector3D& position, Vector3D& velocity);

	// loads sample source data
	ISoundSource*				LoadSample(const char* filename);
	void						FreeSample(ISoundSource* sample);

	// finds the effect. May return EFFECT_ID_NONE
	effectId_t					FindEffect(const char* name) const;

	// sets the new effect
	void						SetEffect(int slot, effectId_t effect);

private:
	struct sndEffect_t
	{
		char		name[32];
		ALuint		nAlEffect;
	};

	bool			CreateALEffect(const char* pszName, kvkeybase_t* pSection, sndEffect_t& effect);
	void			SuspendSourcesWithSample(ISoundSource* sample);

	bool			InitContext();
	void			InitEffects();

	void			DestroyContext();
	void			DestroyEffects();

	DkList<CRefPointer<IEqAudioSource*>>	m_sources;	// tracked sources
	DkList<ISoundSource*>					m_samples;
	DkList<sndEffect_t>						m_effects;

	ALCcontext*								m_ctx;
	ALCdevice*								m_dev;

	ALuint									m_effectSlots[SOUND_EFX_SLOTS];
	int										m_currEffectSlotIdx;

	sndEffect_t*							m_currEffect;

	bool									m_noSound;
};

//-----------------------------------------------------------------
// Sound source

class CEqAudioSourceAL : public IEqAudioSource
{
	friend class CEqAudioSystemAL;
public:
	CEqAudioSourceAL();
	CEqAudioSourceAL(int typeId, ISoundSource* sample, UpdateCallback fnCallback, void* callbackObject);
	~CEqAudioSourceAL();

	void					Setup(int chanType, ISoundSource* sample, UpdateCallback fnCallback = nullptr, void* callbackObject = nullptr);
	void					Release();

	// full scale
	void					GetParams(params_t& params);
	void					UpdateParams(params_t params, int mask);

	// atomic
	ESourceState			GetState() const { return m_state; }
	bool					IsLooping() const { return m_looping; }

protected:
	void					Ref_DeleteObject();

	bool					QueueStreamChannel(ALuint buffer);
	void					SetupSample(ISoundSource* sample);

	void					InitSource();

	void					EmptyBuffers();
	bool					DoUpdate();

	ALuint					m_buffers[STREAM_BUFFER_COUNT];
	ISoundSource*			m_sample;

	UpdateCallback			m_callback;
	void*					m_callbackObject;

	ALuint					m_source;
	int						m_streamPos;
	ESourceState			m_state;

	int						m_chanType;
	bool					m_releaseOnStop;
	bool					m_forceStop;
	bool					m_looping;
};

#endif EQSOUNDSYSTEMAL_H