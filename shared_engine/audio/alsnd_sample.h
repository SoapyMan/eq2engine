//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Sound system
//
//				Sound sample loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSND_SAMPLE_H
#define ALSND_SAMPLE_H

#include "utils/eqthread.h"
#include "utils/DkList.h"

using namespace Threading;

//
// Threaded sound loader
//

class DkSoundSampleLocal : public ISoundSample
{
	friend class DkSoundAmbient;
	friend class DkSoundEmitterLocal;
public:
					DkSoundSampleLocal();
					~DkSoundSampleLocal();

	static void		SampleLoaderJob( void* loadSampleData, int i );
	static void		Job(DkSoundSampleLocal* job);

	void			Init(const char *name, int flags);

	bool			Load();
	bool			LoadWav(const char *name, unsigned int buffer);
	bool			LoadOgg(const char *name, unsigned int buffer);

	const char*		GetName() const {return m_szName.c_str();}

	void			WaitForLoad();
	bool			IsLoaded() const;

	int				GetFlags() const {return m_flags;}

	float			GetDuration() const;

private:
	EqString		m_szName;
	unsigned int	m_alBuffer;
	
	volatile int	m_loadState;

	uint8			m_nChannels;
	uint8			m_flags;

	int				m_loopStart;
	int				m_loopEnd;

	float			m_duration;
};

//----------------------------------------------------------------------------------------------------------

#endif // ALSND_SAMPLE_H