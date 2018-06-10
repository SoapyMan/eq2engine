//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: System and module loader
//////////////////////////////////////////////////////////////////////////////////

#include "sys_host.h"
#include "sys_state.h"
#include "sys_in_console.h"

#include "luabinding/LuaBinding_Engine.h"

#include "IConCommandFactory.h"

#include "IEqCPUServices.h"
#include "eqParallelJobs.h"
#include "KeyBinding/InputCommandBinder.h"
#include "FontCache.h"

#include "DebugOverlay.h"

#include "EqUI/EqUI_Manager.h"

#include "GameSoundEmitterSystem.h"

#include "network/net_defs.h"

#include "cfgloader.h"

#include "Shiny.h"

#define DEFAULT_USERCONFIG_PATH		"cfg/user.cfg"

ConVar user_cfg("user_cfg", DEFAULT_USERCONFIG_PATH, "User configuration file location", CV_INITONLY);

DECLARE_CMD(exec_user_cfg, "Executes user configuration file (from cvar 'cfg_user' value)", 0)
{
	MsgInfo("* Executing user configuration file '%s'\n", user_cfg.GetString());
	g_sysConsole->ParseFileToCommandBuffer( user_cfg.GetString() );
}

ConCommand cc_exit("exit",CGameHost::HostExitCmd,"Closes current instance of engine");
ConCommand cc_quit("quit",CGameHost::HostExitCmd,"Closes current instance of engine");
ConCommand cc_quti("quti",CGameHost::HostExitCmd,"This made for keyboard writing errors");

DECLARE_CVAR(r_clear,0,"Clear the backbuffer",CV_ARCHIVE);
DECLARE_CVAR(r_vSync,0,"Vertical syncronization",CV_ARCHIVE);
DECLARE_CVAR(r_antialiasing,0,"Multisample antialiasing",CV_ARCHIVE);

DECLARE_INTERNAL_SHADERS();

static EQCURSOR s_defaultCursor[20];

// TODO: Move this to GUI
enum CursorCode
{
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_last,
};

DKMODULE*			g_matsysmodule = NULL;

IMaterialSystem*	materials = NULL;
IShaderAPI*			g_pShaderAPI = NULL;

static CDebugOverlay g_DebugOverlays;
IDebugOverlay* debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

DECLARE_CVAR(m_invert,0,"Mouse inversion enabled?", CV_ARCHIVE);

void CGameHost::HostQuitToDesktop()
{
	g_pHost->m_nQuitState = CGameHost::QUIT_TODESKTOP;
}

void CGameHost::HostExitCmd(CONCOMMAND_ARGUMENTS)
{
	HostQuitToDesktop();
}

bool CGameHost::LoadModules()
{
	// first, load matsystem module
#ifdef _WIN32
	g_matsysmodule = g_fileSystem->LoadModule("eqMatSystem.dll");
#else
    g_matsysmodule = g_fileSystem->LoadModule("libeqMatSystem.so");
#endif // _WIN32

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load eqMatSystem!");
		return false;
	}

	materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);
	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of eqMatSystem!");
		return false;
	}

	return true;
}

void CGameHost::SetWindowTitle(const char* windowTitle)
{
	SDL_SetWindowTitle(m_pWindow, windowTitle);
}

#ifdef ANDROID
void* CGameHost::GetEGLSurfaceFromSDL()
{
    // set window info
    SDL_SysWMinfo winfo;
    SDL_VERSION(&winfo.version); // initialize info structure with SDL version info

    if( !SDL_GetWindowWMInfo(m_pWindow, &winfo) )
    {
        MsgError("SDL_GetWindowWMInfo failed %s\n\tWindow handle: %p", SDL_GetError(), m_pWindow);
        ErrorMsg("Can't get SDL window WM info!\n");
        return nullptr;
    }

    return (void*)winfo.info.android.surface;
}

void* Helper_GetEGLSurfaceFromSDL()
{
	return g_pHost->GetEGLSurfaceFromSDL();
}

#endif // ANDROID

bool CGameHost::InitSystems( EQWNDHANDLE pWindow, bool bWindowed )
{
	m_pWindow = pWindow;

	int bpp = 32;

	ETextureFormat format = FORMAT_RGB8;

	// Figure display format to use
	if(bpp == 32)
	{
		format = FORMAT_RGB8;
	}
	else if(bpp == 24)
	{
		format = FORMAT_RGB8;
	}
	else if(bpp == 16)
	{
		format = FORMAT_RGB565;
	}

#ifdef _WIN32
	bool useOpenGLRender = g_cmdLine->FindArgument("-ogl") != -1;
#else
    bool useOpenGLRender = true; // I don't see that other APIs could appear widely soon
#endif // _WIN32

	matsystem_render_config_t materials_config;

	materials_config.enableBumpmapping = true;
	materials_config.enableSpecular = true;
	materials_config.enableShadows = true;
	materials_config.threadedloader = !useOpenGLRender;

	materials_config.shaderapi_params.windowedMode = bWindowed;
	materials_config.shaderapi_params.windowHandle = pWindow;
	materials_config.shaderapi_params.multiSamplingMode = r_antialiasing.GetInt();

	// set window info
	SDL_SysWMinfo winfo;
	SDL_VERSION(&winfo.version); // initialize info structure with SDL version info

	if( !SDL_GetWindowWMInfo(m_pWindow, &winfo) )
	{
		MsgError("SDL_GetWindowWMInfo failed %s\n\tWindow handle: %p", SDL_GetError(), m_pWindow);
		ErrorMsg("Can't get SDL window WM info!\n");
		return false;
	}

	const char* materialsPath = "materials/";

#ifdef _WIN32
	materials_config.shaderapi_params.windowHandle = winfo.info.win.window;
#elif LINUX
	materials_config.shaderapi_params.windowHandle = (void*)winfo.info.x11.window;
#elif APPLE
	materials_config.shaderapi_params.windowHandle = (void*)winfo.info.cocoa.window;
#elif ANDROID

    externalWindowDisplayParams_t winParams;
    winParams.window = (void*)winfo.info.android.window;

    void* paramArray[] = { (void*)Helper_GetEGLSurfaceFromSDL };

	winParams.paramArray = paramArray;
	winParams.numParams = 1;

	materials_config.shaderapi_params.windowHandle = &winParams;

	materialsPath = "mmaterials/";
#endif

	materials_config.shaderapi_params.screenFormat = format;
	materials_config.shaderapi_params.verticalSyncEnabled = r_vSync.GetBool();

    bool materialSystemStatus = false;

    const char* rendererName = "eqNullRHI";

#ifdef _WIN32
	if(useOpenGLRender)
		rendererName = "eqGLRHI";
	else if(g_cmdLine->FindArgument("-norender") != -1)
		rendererName = "eqNullRHI";
	else
		rendererName = "eqD3D9RHI";
#else
	if(g_cmdLine->FindArgument("-norender") != -1)
		rendererName = "libeqNullRHI";
    else
        rendererName = "libeqGLRHI";
#endif // _WIN32

	materialSystemStatus = materials->Init(materialsPath, rendererName, materials_config);

	if(!materialSystemStatus)
		return false;

#ifdef _WIN32
	if(!materials->LoadShaderLibrary("eqBaseShaders.dll"))
		return false;
#else
	if(!materials->LoadShaderLibrary("libeqBaseShaders.so"))
		return false;
#endif // _WIN32

	int wide,tall;
	SDL_GetWindowSize(m_pWindow, &wide, &tall);
	OnWindowResize(wide,tall);

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	g_pShaderAPI = materials->GetShaderAPI();

	// Initialize sound system
	soundsystem->Init();

	Networking::InitNetworking();

	s_defaultCursor[dc_none]     = NULL;
	s_defaultCursor[dc_arrow]    =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	s_defaultCursor[dc_ibeam]    =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	s_defaultCursor[dc_hourglass]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	s_defaultCursor[dc_crosshair]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	s_defaultCursor[dc_up]       =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
	s_defaultCursor[dc_sizenwse] =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	s_defaultCursor[dc_sizenesw] =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	s_defaultCursor[dc_sizewe]   =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	s_defaultCursor[dc_sizens]   =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	s_defaultCursor[dc_sizeall]  =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	s_defaultCursor[dc_no]       =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
	s_defaultCursor[dc_hand]     =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

	// Set default cursor
	SDL_SetCursor(s_defaultCursor[dc_arrow] );

	// make job threads
	g_parallelJobs->Init( (int)ceil((float)g_cpuCaps->GetCPUCount() / 2.0f) );

	if( !g_fontCache->Init() )
		return false;

	m_pDefaultFont = g_fontCache->GetFont("default",0);

	// init console
	g_consoleInput->Initialize();

	g_consoleInput->SetAlternateHandler( LuaBinding_ConsoleHandler );

	debugoverlay->Init();
	equi::Manager->Init();

	// init game states and proceed
	if(!EqStateMgr::InitRegisterStates())
	{
		return false;
	}

	// finally init input and adjust bindings
	g_inputCommandBinder->Init();

	MsgInfo("EqEngine systems init successfully\n");

	return true;
}

//--------------------------------------------------------------------------------------

ConVar in_joy_debug("in_joy_debug", "0", "Joystick debug messages", 0);
ConVar in_mouse_to_touch("in_mouse_to_touch", "0", "Convert mouse clicks to touch input", CV_ARCHIVE);

void InputCommands_SDL(SDL_Event* event)
{
	static int lastX, lastY;

	switch(event->type)
	{
		case SDL_KEYUP:
		case SDL_KEYDOWN:
		{
			int nKey = event->key.keysym.scancode;

			// do translation
			if(nKey == SDL_SCANCODE_RSHIFT)
				nKey = SDL_SCANCODE_LSHIFT;
			else if(nKey == SDL_SCANCODE_RCTRL)
				nKey = SDL_SCANCODE_LCTRL;
			else if(nKey == SDL_SCANCODE_RALT)
				nKey = SDL_SCANCODE_LALT;

			g_pHost->TrapKey_Event( nKey, (event->type == SDL_KEYUP) ? false : true );
		}
		case SDL_TEXTINPUT:
		{
			// FIXME: is it good?
			g_pHost->ProcessKeyChar( event->text.text[0] );
			break;
		}
		case SDL_MOUSEMOTION:
		{
			int x, y;
			x = event->motion.x;
			y = event->motion.y;

			g_pHost->TrapMouseMove_Event(x, y);

			lastX = x;
			lastY = y;

			break;
		}
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		{
			int x, y;
			x = event->button.x;
			y = event->button.y;

			int mouseButton = MOU_B1;

			if(event->button.button == 1)
				mouseButton = MOU_B1;
			else if(event->button.button == 2)
				mouseButton = MOU_B3;
			else if(event->button.button == 3)
				mouseButton = MOU_B2;
			else if(event->button.button == 4)
				mouseButton = MOU_B4;
			else if(event->button.button == 5)
				mouseButton = MOU_B5;

			g_pHost->TrapMouse_Event(x, y, mouseButton, (event->type == SDL_MOUSEBUTTONUP) ? false : true);

			break;
		}
		case SDL_FINGERMOTION:
		{
			g_pHost->TouchMotion_Event( event->tfinger.x,event->tfinger.y, event->tfinger.fingerId);
			break;
		}
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:
		{
			g_pHost->Touch_Event( event->tfinger.x,event->tfinger.y, event->tfinger.fingerId, (event->type == SDL_FINGERUP) ? false : true);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			int x, y;
			x = event->wheel.x;
			y = event->wheel.y;

			g_pHost->TrapMouseWheel_Event(lastX, lastY, y);

			break;
		}
		case SDL_JOYAXISMOTION:
		{
			if(in_joy_debug.GetBool())
			{
				Msg("Joystick %d axis %d value: %d\n",
							event->jaxis.which,
							event->jaxis.axis+1, event->jaxis.value);
			}

			g_pHost->TrapJoyAxis_Event(event->jaxis.axis, event->jaxis.value);
			break;
		}
		case SDL_JOYHATMOTION:
		{
			if(in_joy_debug.GetBool())
			{
				Msg("Joystick %d hat %d value:",
					event->jhat.which, event->jhat.hat+1);
			}

			if (event->jhat.value == SDL_HAT_CENTERED)
				Msg(" centered\n");
			if (event->jhat.value & SDL_HAT_UP)
				Msg(" up\n");
			if (event->jhat.value & SDL_HAT_RIGHT)
				Msg(" right\n");
			if (event->jhat.value & SDL_HAT_DOWN)
				Msg(" down\n");
			if (event->jhat.value & SDL_HAT_LEFT)
				Msg(" left\n");

			// g_pHost->TrapJoyHat_Event(event->jaxis.axis, event->jaxis.value);
		}
        case SDL_JOYBALLMOTION:
		{
			if(in_joy_debug.GetBool())
			{
				Msg("Joystick %d ball %d delta: (%d,%d)\n",
					event->jball.which,
					event->jball.ball+1, event->jball.xrel, event->jball.yrel);
			}

			g_pHost->TrapJoyBall_Event(event->jball.ball, event->jball.xrel, event->jball.yrel);

            break;
		}
        case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		{
			bool down = (event->type == SDL_JOYBUTTONDOWN) ? true : false;

			if(in_joy_debug.GetBool())
			{
				Msg("Joystick %d button %d %s\n",
					event->jbutton.which, event->jbutton.button+1, down ? "down" : "up");
			}

			g_pHost->TrapJoyButton_Event(event->jbutton.button, down);
            break;
		}
	}
}

CGameHost::CGameHost() :
	m_winSize(0), m_prevMousePos(0), m_mousePos(0), m_pWindow(NULL), m_nQuitState(QUIT_NOTQUITTING),
	m_bTrapMode(false), m_skipMouseMove(false), m_bDoneTrapping(false), m_nTrapKey(0), m_nTrapButtons(0), m_cursorCentered(false),
	m_pDefaultFont(NULL)
{
	m_fCurTime = 0;
	m_fFrameTime = 0;
	m_fOldTime = 0;

	m_fGameCurTime = 0;
	m_fGameFrameTime = 0;
	m_fGameOldTime = 0;
}

void CGameHost::ShutdownSystems()
{
	Msg("---------  ShutdownSystems ---------\n");

	// calls OnLeave and unloads state
	EqStateMgr::ChangeState( NULL );

	// Save configuration before full unload
	WriteCfgFile( user_cfg.GetString(), true );

	// shutdown any dependent bindings to avoid crashes
	LuaBinding_ShutdownEngineBindings();

	g_parallelJobs->Shutdown();

	equi::Manager->Shutdown();
	g_fontCache->Shutdown();

	ses->Shutdown();
	soundsystem->Shutdown();

	g_inputCommandBinder->Shutdown();

	// shutdown systems...
	Networking::ShutdownNetworking();

	materials->Shutdown();
	g_fileSystem->FreeModule( g_matsysmodule );

	SDL_DestroyWindow(g_pHost->m_pWindow);

	PROFILE_DESTROY();
}

ConVar sys_maxfps("sys_maxfps", "0", "Frame rate limit", CV_CHEAT);
ConVar sys_timescale("sys_timescale", "1.0f", "Time scale", CV_CHEAT);

#define GAME_MAX_FRAMERATE (sys_maxfps.GetFloat()) // 60 fps limit

#define MIN_FPS 0.01
#define MAX_FPS 10000.0

#define MAX_FRAMETIME	0.1
#define MIN_FRAMETIME	0.001

bool CGameHost::FilterTime( double dTime )
{
	// Accumulate some time
	m_fGameCurTime += dTime;

	double timescale = sys_timescale.GetFloat();

	double fps = GAME_MAX_FRAMERATE;
	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		double minframetime = 1.0 / fps;

		m_fGameFrameTime = (m_fGameCurTime - m_fGameOldTime) * (double)sys_timescale.GetFloat();

		if(m_fGameFrameTime < minframetime )
			return false;

		//m_fGameFrameTime = minframetime * (double)sys_timescale.GetFloat();
	}
	else
	{
		m_fGameFrameTime = (m_fGameCurTime - m_fGameOldTime) * (double)sys_timescale.GetFloat();
	}

	m_fGameFrameTime = min( m_fGameFrameTime, MAX_FRAMETIME );
	m_fGameFrameTime = max( m_fGameFrameTime, MIN_FRAMETIME );

	m_fGameOldTime = m_fGameCurTime;

	return true;
}

void CGameHost::UpdateCursorState()
{
	bool cursorVisible = false;

	EqStateMgr::GetStateMouseCursorProperties(cursorVisible, m_cursorCentered);

	cursorVisible = cursorVisible || g_consoleInput->IsVisible() || equi::Manager->IsWindowsVisible();
	m_cursorCentered = m_cursorCentered && !(g_consoleInput->IsVisible() || equi::Manager->IsWindowsVisible());

	// update cursor visibility state
	SetCursorShow( cursorVisible );
}

void CGameHost::SetCursorPosition(int x, int y)
{
	SDL_WarpMouseInWindow(m_pWindow, x, y);
	//m_skipMouseMove = true;

	m_mousePos = IVector2D(x,y);

	//SDL_GetMouseState(&m_mousePos.x, &m_mousePos.y);
}

void CGameHost::SetCursorShow(bool bShow)
{
	bool state = SDL_ShowCursor(-1) > 0;

	if(state == bShow)
		return;

	if(bShow)
		SDL_SetCursor(s_defaultCursor[dc_arrow]);

	SDL_ShowCursor(bShow);
}

extern bool s_bActive;

ConVar r_showFPS("r_showFPS", "0", "Show the framerate", CV_ARCHIVE);

bool CGameHost::Frame()
{
	m_fCurTime		= m_timer.GetTime();

	// Set frame time
	m_fFrameTime	= m_fCurTime - m_fOldTime;

	// If the time is < 0, that means we've restarted.
	// Set the new time high enough so the engine will run a frame
	//if ( m_fFrameTime < 0.001 ) // 512 FPS
	//	return;

	// Engine frames status
	static float accTime = 0.1f;
	static int fps = 0;
	static int nFrames = 0;

	if (accTime > 0.1f)
	{
		fps = (int) (nFrames / accTime + 0.5f);
		nFrames = 0;
		accTime = 0;
	}

	accTime += m_fFrameTime;
	nFrames++;

	m_fOldTime		= m_fCurTime;

	m_prevMousePos = m_mousePos;

	if( !FilterTime( m_fFrameTime ) )
		return false;

	UpdateCursorState();

	//--------------------------------------------

	BeginScene();

	// always reset scissor rectangle before we start rendering
	g_pShaderAPI->SetScissorRectangle( IRectangle(0,0,m_winSize.x, m_winSize.y) );
	g_pShaderAPI->Clear(r_clear.GetBool(),true,false, ColorRGBA(0.1f,0.1f,0.1f,1.0f));

	HOOK_TO_CVAR(r_wireframe)
	materials->GetConfiguration().wireframeMode = r_wireframe->GetBool();

	PROFILE_BLOCK(UpdateStates);

	if(!EqStateMgr::UpdateStates( m_fGameFrameTime ))
	{
		g_pHost->m_nQuitState = CGameHost::QUIT_TODESKTOP;
		return false;
	}

	PROFILE_END();

	ses->Update();

	// Engine frames status
	static float gameAccTime = 0.1f;
	static int gamefps = 0;
	static int nGameFrames = 0;

	if (gameAccTime > 0.1f)
	{
		gamefps = (int) (nGameFrames / gameAccTime + 0.5f);
		nGameFrames = 0;
		gameAccTime = 0;
	}

	gameAccTime += m_fGameFrameTime;
	nGameFrames++;

	debugoverlay->Text(Vector4D(1,1,0,1), "-----ENGINE STATISTICS-----");
	debugoverlay->Text(Vector4D(1), "System framerate: %i", fps);
	debugoverlay->Text(Vector4D(1), "Game framerate: %i (ft=%g)", gamefps, m_fGameFrameTime);
	debugoverlay->Text(Vector4D(1), "DPS/DIPS: %i/%i", g_pShaderAPI->GetDrawCallsCount(), g_pShaderAPI->GetDrawIndexedPrimitiveCallsCount());
	debugoverlay->Text(Vector4D(1), "primitives: %i", g_pShaderAPI->GetTrianglesCount());
	debugoverlay->Text(Vector4D(1,1,1,1), "engtime: %.2f\n", m_fCurTime);

	debugoverlay->Draw(m_winSize.x, m_winSize.y);

	materials->Setup2D(m_winSize.x, m_winSize.y);

	if(r_showFPS.GetBool())
	{
		eqFontStyleParam_t params;
		params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		params.textColor = ColorRGBA(1,1,1,1);

		if(fps < 30)
			params.textColor = ColorRGBA(1,0,0,1);
		else if(fps < 60)
			params.textColor = ColorRGBA(1,0.8f,0,1);

		m_pDefaultFont->RenderText(varargs("SYS/GAME FPS: %d/%d", min(fps, 1000), gamefps), Vector2D(15), params);
	}

	g_inputCommandBinder->DebugDraw(m_winSize);

	equi::Manager->SetViewFrame(IRectangle(0,0,m_winSize.x,m_winSize.y));
	equi::Manager->Render();

	g_consoleInput->DrawSelf(m_winSize.x, m_winSize.y, m_fGameFrameTime);

	// End frame from render lib
	EndScene();

	g_pShaderAPI->ResetCounters();

	// Remember old time
	m_fOldTime		= m_fCurTime;

	return true;
}

bool CGameHost::IsInMultiplayerGame() const
{
	return EqStateMgr::IsMultiplayerGameState();
}

void CGameHost::SignalPause()
{
	if(EqStateMgr::IsInGameState())
		return;

	EqStateMgr::SignalPause();
}

void CGameHost::OnWindowResize(int width, int height)
{
	m_winSize.x = width;
	m_winSize.y = height;

	if(materials)
		materials->SetDeviceBackbufferSize( width, height );
}

void CGameHost::BeginScene()
{
	// reset viewport
	g_pShaderAPI->SetViewport(0,0, m_winSize.x, m_winSize.y);

	// Begin frame from render lib
	materials->BeginFrame();
	g_pShaderAPI->Clear(false,true,false);
}

void CGameHost::EndScene()
{
	// issue the rendering of anything
	PROFILE_CODE(g_pShaderAPI->Flush(););

	// End frame from render lib
	PROFILE_CODE(materials->EndFrame(NULL););
}

void CGameHost::TrapKey_Event( int key, bool down )
{
	// Only key down events are trapped
	if ( m_bTrapMode && down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;

		m_nTrapKey			= key;
		m_nTrapButtons		= 0;
		return;
	}

	if(g_consoleInput->KeyPress( key, down ))
		return;

	if( equi::Manager->ProcessKeyboardEvents(key, down ? equi::UIEVENT_DOWN : equi::UIEVENT_UP ) )
		return;

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleKeyPress( key, down );
}

void CGameHost::TrapMouse_Event( float x, float y, int buttons, bool down )
{
	// buttons == 0 for mouse move events
	if ( m_bTrapMode && buttons && !down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;

		m_nTrapKey			= 0;
		m_nTrapButtons		= buttons;
		return;
	}

	if( g_consoleInput->MouseEvent( Vector2D(x,y), buttons, down ) )
		return;

	if( equi::Manager->ProcessMouseEvents( x, y, buttons, down ? equi::UIEVENT_DOWN : equi::UIEVENT_UP) )
		return;

	if(in_mouse_to_touch.GetBool())
		g_pHost->Touch_Event( x/m_winSize.x, y/m_winSize.y, 0, down);

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseClick( x, y, buttons, down );
}

void CGameHost::TrapMouseMove_Event( int x, int y )
{
	if(m_skipMouseMove)
	{
		m_skipMouseMove = false;
		return;
	}

	m_mousePos = IVector2D(x,y);

	g_consoleInput->MousePos( m_mousePos );

	if( equi::Manager->ProcessMouseEvents( x, y, 0, equi::UIEVENT_MOUSE_MOVE) )
		return;

	Vector2D delta = (Vector2D)m_prevMousePos - (Vector2D)m_mousePos;

	delta.y *= (m_invert.GetBool() ? 1.0f : -1.0f);
	delta *= 0.05f;

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseMove(x, y, delta.x, delta.y);

	if(m_cursorCentered)
		SetCursorPosition(m_winSize.x/2, m_winSize.y/2);
}

void CGameHost::TrapMouseWheel_Event(int x, int y, int scroll)
{
	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseWheel(x,y,scroll);
}

void CGameHost::TrapJoyAxis_Event( short axis, short value )
{
	g_inputCommandBinder->OnJoyAxisEvent( axis, value );

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleJoyAxis( axis, value );
}

void CGameHost::TrapJoyBall_Event( short ball, short xrel, short yrel )
{

}

void CGameHost::TrapJoyButton_Event( short button, bool down)
{
	g_inputCommandBinder->OnKeyEvent( JOYSTICK_START_KEYS + button, down );

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleKeyPress( JOYSTICK_START_KEYS + button, down );
}

void CGameHost::TouchMotion_Event( float x, float y, int finger )
{
	// TODO: uses this!
}

void CGameHost::Touch_Event( float x, float y, int finger, bool down )
{
	g_inputCommandBinder->OnTouchEvent( Vector2D(x,y), finger, down );
}

void CGameHost::ProcessKeyChar( int chr )
{
	if(g_consoleInput->KeyChar( chr ))
		return;
}

void CGameHost::StartTrapMode()
{
	if ( m_bTrapMode )
		return;

	m_bDoneTrapping = false;
	m_bTrapMode = true;
}

// Returns true on success, false on failure.
bool CGameHost::IsTrapping()
{
	return m_bTrapMode;
}

bool CGameHost::CheckDoneTrapping( int& buttons, int& key )
{
	if ( m_bTrapMode )
		return false;

	if ( !m_bDoneTrapping )
		return false;

	key			= m_nTrapKey;
	buttons		= m_nTrapButtons;

	// Reset since we retrieved the results
	m_bDoneTrapping = false;
	return true;
}