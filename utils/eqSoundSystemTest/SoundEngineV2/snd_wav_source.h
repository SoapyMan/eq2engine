//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_WAV_SOURCE_H
#define SND_WAV_SOURCE_H

#include "snd_source.h"
#include "utils/eqstring.h"

#include "riff.h"

#define STREAM_THRESHOLD    0x10000     // 65k

//---------------------------------------------------------------------

class CSoundSource_Wave : public ISoundSource
{
public:
	virtual soundFormat_t*	GetFormat()						{ return &m_format; }
	virtual const char*		GetFilename() const				{ return m_filename.c_str(); }
	virtual float			GetLoopPosition(float flPosition);

protected:
	void					ParseChunk(CRIFF_Parser &chunk);

	virtual void			ParseFormat(CRIFF_Parser &chunk);
	virtual void			ParseCue(CRIFF_Parser &chunk);
	virtual void			ParseData(CRIFF_Parser &chunk) = 0;

	soundFormat_t			m_format;
	EqString				m_filename;

	int						m_numSamples;
	int						m_loopStart;
};

#endif // SND_WAV_SOURCE_H