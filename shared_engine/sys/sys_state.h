//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: State handlers
//////////////////////////////////////////////////////////////////////////////////

#ifndef SYS_STATE_H
#define SYS_STATE_H

#include "utils/KeyValues.h"

#include "font/IFont.h"
#include "materialsystem1/IMaterialSystem.h"

#define GAME_STATE_NONE 0

// forward and extern
class CBaseStateHandler;
extern CBaseStateHandler*	g_states[];

//--------------------------------------------------------------------------------
// game state handler
//--------------------------------------------------------------------------------

class CBaseStateHandler
{
public:
						CBaseStateHandler() : m_nextState(NULL), m_forceNextState(false) {}
				virtual ~CBaseStateHandler(){}

	virtual int			GetType() const = 0;

	// when changed to this state
	// @from - used to transfer data
	virtual void		OnEnter( CBaseStateHandler* from ) {}

	// when the state changes to something
	// @to - used to transfer data
	virtual void		OnLeave( CBaseStateHandler* to ) {}

	// when 'false' returned the next state goes on
	virtual bool		Update( float fDt ) = 0;

	virtual void		HandleKeyPress( int key, bool down ) {}
	virtual void		HandleMouseClick( int x, int y, int buttons, bool down ) {}
	virtual void		HandleMouseMove( int x, int y, float deltaX, float deltaY ) {}
	virtual void		HandleMouseWheel( int x,int y,int scroll ) {}

	virtual void		HandleJoyAxis( short axis, short value ) {}

	virtual void		GetMouseCursorProperties(bool& visible, bool& centered) {visible = false; centered = false;}
	virtual float		GetTimescale() const { return 1.0f; }

	void				SetNextState(CBaseStateHandler* state, bool force = false)	{m_nextState = state;m_forceNextState = force;}
	CBaseStateHandler*	GetNextState(bool* force = NULL) const						{if(force)*force = m_forceNextState; return m_nextState;}

private:
	CBaseStateHandler*	m_nextState;
	bool				m_forceNextState;
};

//--------------------------------------------------------------------------------

namespace EqStateMgr
{
	// forces the current state
	void					SetCurrentState(CBaseStateHandler* state, bool force = false);

	void					ChangeState(CBaseStateHandler* state);

	// returns the current state
	CBaseStateHandler*		GetCurrentState();

	// returns the current state type
	int						GetCurrentStateType();
	void					SetCurrentStateType(int stateType);

	void					ChangeStateType(int stateType);

	void					ScheduleNextState(CBaseStateHandler* state);
	void					ScheduleNextStateType(int stateType);

	// updates and manages the states
	bool					UpdateStates(float fDt);
	void					GetStateMouseCursorProperties(bool& visible, bool& centered);

	bool					IsMultiplayerGameState();
	bool					IsInGameState();

	void					SignalPause();

	bool					InitRegisterStates();
};

//---------------------------------------------------------------------------------

#endif // SYS_STATE_H