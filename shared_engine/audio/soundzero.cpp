//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Sound system
//
//				Special zero sound emittter
//////////////////////////////////////////////////////////////////////////////////


#include "DebugInterface.h"
#include "soundzero.h"

DkSoundSampleZero zeroSample;

void DkSoundEmitterZero::SetPosition(Vector3D &position)
{

}

void DkSoundEmitterZero::SetVelocity(Vector3D &vec)
{

}

bool DkSoundEmitterZero::IsStopped()
{
	return true;
}

bool DkSoundEmitterZero::IsVirtual()
{
	return false;
}

void DkSoundEmitterZero::Play(ISoundSample* sample, soundParams_t *param)
{

}

void DkSoundEmitterZero::GetParams(soundParams_t *param)
{

}

void DkSoundEmitterZero::SetParams(soundParams_t *param)
{

}

void DkSoundEmitterZero::Stop()
{

}

void DkSoundEmitterZero::Pause()
{

}