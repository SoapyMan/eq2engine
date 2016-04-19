//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_SINGLEPLAYER
#define GAME_SINGLEPLAYER

#include "physics.h"
#include "AICarManager.h"
#include "LuaBinding_Drivers.h"

#define PHYSICS_ITERATION_COUNT		2

class CCar;

enum ESessionType
{
	SESSION_SINGLE,
	SESSION_NETWORK,
};

enum EMissionStatus
{
	MIS_STATUS_INGAME = 0,
	MIS_STATUS_SUCCESS,
	MIS_STATUS_FAILED
};

//--------------------------------------------------------------------------------------

class CGameSession
{
public:
								CGameSession();
	virtual						~CGameSession();

	virtual void				Init();
	virtual void				Shutdown();

	virtual void				Update( float fDt );

	virtual void				UpdateLocalControls( int nControls );

	CCar*						CreateCar(const char* name, int type = CAR_TYPE_NORMAL);

	CAIPursuerCar*				CreatePursuerCar(const char* name, int type = PURSUER_TYPE_COP);

	virtual CCar*				GetPlayerCar() const;
	void						SetPlayerCar(CCar* pCar);

	virtual CCar*				GetViewCar() const;

	CCar*						GetLeadCar() const;
	void						SetLeadCar(CCar* pCar);

	float						LoadCarReplay(CCar* pCar, const char* filename);

	virtual int					GetSessionType() const {return SESSION_SINGLE;}

	virtual bool				IsClient() const {return true;}
	virtual bool				IsServer() const {return true;}

	virtual bool				IsGameDone(bool checkTime = true) const;
	void						SignalMissionStatus(int missionStatus, float waitTime);
	int							GetMissionStatus() const;

	bool						IsReplay() const;
	void						ResetReplay();

	int							GenScriptID();
	CGameObject*				FindScriptObjectById( int scriptID );

	void						LoadCarData();
	carConfigEntry_t*			FindCarEntryByName(const char* name);

protected:

	float						m_waitForExitTime;
	int							m_missionStatus;

	int							m_scriptIDCounter;

	DkList<carConfigEntry_t*>	m_carEntries;

	int							m_localControls;
	CCar*						m_playerCar;

	CCar*						m_leadCar;

	OOLUA::Table				m_missionManagerTable;		// missionmanager
	EqLua::LuaTableFuncRef		m_lua_misman_InitMission;	// lua CMissionManager:InitMission
	EqLua::LuaTableFuncRef		m_lua_misman_Update;		// lua CMissionManager:Update
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CGameSession)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(CreateCar)
	OOLUA_MFUNC(CreatePursuerCar)

	OOLUA_MFUNC(LoadCarReplay)

	OOLUA_MFUNC_CONST(IsClient)
	OOLUA_MFUNC_CONST(IsServer)

	OOLUA_MFUNC_CONST(GetSessionType)

	OOLUA_MFUNC(SetPlayerCar)
	OOLUA_MFUNC_CONST(GetPlayerCar)

	OOLUA_MFUNC(SetLeadCar)
	OOLUA_MFUNC_CONST(GetLeadCar)

	OOLUA_MFUNC_CONST(IsGameDone)
	OOLUA_MFUNC(SignalMissionStatus)
	OOLUA_MFUNC_CONST(GetMissionStatus)

	OOLUA_MFUNC_CONST(IsReplay)
	OOLUA_MFUNC(ResetReplay)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // GAME_SINGLEPLAYER