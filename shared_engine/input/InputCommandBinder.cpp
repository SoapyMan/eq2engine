//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: KeyBinding list
//////////////////////////////////////////////////////////////////////////////////

#include "InputCommandBinder.h"

#include "core/IConsoleCommands.h"
#include "core/DebugInterface.h"
#include "core/InterfaceManager.h"

#include "utils/strtools.h"
#include "utils/KeyValues.h"
#include "utils/strtools.h"
#include "utils/IVirtualStream.h"

#include "materialsystem1/IMaterialSystem.h"

#include "font/IFontCache.h"

#include <SDL.h>

static CInputCommandBinder s_inputCommandBinder;
CInputCommandBinder* g_inputCommandBinder = &s_inputCommandBinder;

ConVar in_keys_debug("in_keys_debug", "0", "Debug CInputCommandBinder");
ConVar in_touchzones_debug("in_touchzones_debug", "0", "Debug touch zones on screen and messages", CV_CHEAT);

DECLARE_CMD(in_touchzones_reload, "Reload touch zones", 0)
{
	g_inputCommandBinder->InitTouchZones();
}

//---------------------------------------------------------
// UTILITY FUNCTIONS

bool UTIL_GetBindingKeyIndices(int outKeys[3], const char* pszKeyStr)
{
	// parse pszKeyStr into modifiers
	const char* keyStr = pszKeyStr;

	// init
	outKeys[0] = -1;
	outKeys[1] = -1;
	outKeys[2] = -1;

	for (int i = 0; i < 2; i++)
	{
		const char* subStart = xstristr(keyStr, "+");
		if (subStart)
		{
			EqString modifier(keyStr, subStart - keyStr);

			outKeys[i] = KeyStringToKeyIndex(modifier.ToCString());

			if (outKeys[i] == -1)
			{
				MsgError("Unknown key/mapping '%s'\n", modifier.ToCString());
				return false;
			}

			keyStr = subStart + 1;
		}
	}

	// Find the final key matching the *keychar
	outKeys[2] = KeyStringToKeyIndex(keyStr);

	if (outKeys[2] == -1)
	{
		MsgError("Unknown key/mapping '%s'\n", keyStr);
		return false;
	}

	return true;
}

void UTIL_GetBindingKeyString(EqString& outStr, in_binding_t* binding, bool humanReadable /*= false*/)
{
	if (!binding)
		return;

	outStr.Clear();

	int validModifiers = (binding->mod_index[1] != -1) ? 2 : ((binding->mod_index[0] != -1) ? 1 : 0);

	for (int i = 0; i < validModifiers; i++)
	{
		if(humanReadable)
			outStr.Append(s_keyMapList[binding->mod_index[i]].hrname); // TODO: apply localizer
		else
			outStr.Append(s_keyMapList[binding->mod_index[i]].name);
		outStr.Append('+');
	}

	if (humanReadable)
		outStr.Append(s_keyMapList[binding->key_index].hrname); // TODO: apply localizer
	else
		outStr.Append(s_keyMapList[binding->key_index].name);
}

//---------------------------------------------------------

CInputCommandBinder::CInputCommandBinder() : m_init(false)
{
}

void CInputCommandBinder::Init()
{
	InitTouchZones();

#ifdef PLAT_SDL
	// init key names
	for (keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		if (!kn->hrname)
		{
			
			kn->hrname = xstrdup(SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(kn->keynum)));
		}
	}
#endif // PLAT_SDL

	// resolve bindings
	for(int i = 0; i < m_bindings.numElem(); i++)
	{
		if(!ResolveCommandBinding( m_bindings[i] ))
		{
			delete m_bindings[i];

			m_bindings.removeIndex(i);
			i--;
		}
	}

	m_init = true;
}

void CInputCommandBinder::Shutdown()
{
#ifdef PLAT_SDL
	// free key names
	for (keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		if (kn->hrname && *kn->hrname != '#')
		{
			delete[] kn->hrname;
			kn->hrname = nullptr;
		}
	}
#endif // PLAT_SDL

	m_touchZones.clear();
	UnbindAll();

	m_init = false;
}

void CInputCommandBinder::InitTouchZones()
{
	m_touchZones.clear();

	KeyValues kvs;
	if(!kvs.LoadFromFile("resources/in_touchzones.res"))
		return;

	kvkeybase_t* zones = kvs.GetRootSection()->FindKeyBase("zones");

	for(int i = 0; i < zones->keys.numElem(); i++)
	{
		kvkeybase_t* zoneDef = zones->keys[i];

		in_touchzone_t newZone;
		newZone.name = zoneDef->name;

		kvkeybase_t* zoneCmd = zoneDef->FindKeyBase("bind");

		newZone.commandString = KV_GetValueString(zoneCmd, 0, "zone_no_bind");
		newZone.argumentString = KV_GetValueString(zoneCmd, 1, "");

		newZone.position = KV_GetVector2D(zoneDef->FindKeyBase("position"));
		newZone.size = KV_GetVector2D(zoneDef->FindKeyBase("size"));

		// resolve commands

		// if we connecting libraries dynamically, that wouldn't properly execute
		newZone.cmd_act = (ConCommand*)g_consoleCommands->FindCommand(varargs("+%s", newZone.commandString.ToCString()));
		newZone.cmd_deact = (ConCommand*)g_consoleCommands->FindCommand(varargs("-%s", newZone.commandString.ToCString()));

		// if found only one command with plus or minus
		if(!newZone.cmd_act || !newZone.cmd_deact)
			newZone.cmd_act = (ConCommand*)g_consoleCommands->FindCommand( newZone.commandString.ToCString() );

		DevMsg(DEVMSG_CORE, "Touchzone: %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n", 
			newZone.name.ToCString(), newZone.commandString.ToCString(), 
			newZone.position.x, newZone.position.y, 
			newZone.size.x, newZone.size.y);

		// if anly command found
		if(newZone.cmd_act || newZone.cmd_deact)
		{
			m_touchZones.append( newZone );
		}
		else
		{
			MsgError("touchzone %s: unknown command '%s'\n", newZone.name.ToCString(), newZone.commandString.ToCString());
		}
	}
}

// saves binding using file handle
void CInputCommandBinder::WriteBindings(IVirtualStream* stream)
{
	if(!stream)
		return;

	stream->Print("unbindall\n" );

	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		// resolve key name
		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		stream->Print("bind %s %s %s\n",	keyNameString.GetData(),
											binding->commandString.GetData(),
											binding->argumentString.GetData() );
	}
}

bool CInputCommandBinder::BindKey( const char* pszKeyStr, const char* pszCommand, const char* pszArgs )
{
	in_binding_t* binding = AddBinding(pszKeyStr, pszCommand, pszArgs);

	if (!binding)
		return false;

	if(m_init && !ResolveCommandBinding(binding))
	{
		DeleteBinding( binding );
		return false;
	}

	return true;
}

// binds a command with arguments to known key
in_binding_t* CInputCommandBinder::AddBinding( const char* pszKeyStr, const char* pszCommand, const char *pszArgs )
{
	in_binding_t* binding = nullptr;
	while (binding = FindBindingByCommandName(pszCommand, pszArgs, binding))
	{
		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		if (!keyNameString.CompareCaseIns(pszKeyStr))
		{
			if(strlen(pszArgs))
				MsgWarning("Command '%s %s' already bound to '%s'\n", pszCommand, pszArgs, pszKeyStr);
			else
				MsgWarning("Command '%s' already bound to '%s'\n", pszCommand, pszKeyStr);

			return nullptr;
		}
	}

	int bindingKeyIndices[3];

	if (!UTIL_GetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return nullptr;

	// create new binding
	in_binding_t* newBind = new in_binding_t;

	newBind->mod_index[0] = bindingKeyIndices[0];
	newBind->mod_index[1] = bindingKeyIndices[1];
	newBind->key_index = bindingKeyIndices[2];

	newBind->commandString = pszCommand;

	if(pszArgs)
		newBind->argumentString = pszArgs;

	m_bindings.append( newBind );

	return newBind;
}

bool CInputCommandBinder::ResolveCommandBinding(in_binding_t* binding)
{
	bool isJoyAxis = (s_keyMapList[binding->key_index].keynum >= JOYSTICK_START_AXES);

	// resolve axis first
	if(isJoyAxis)
		binding->boundAction = FindAxisAction( binding->commandString.ToCString() );

	// if no axis action is bound, try bind concommands
	if(!binding->boundAction)
	{
		// if we connecting libraries dynamically, that wouldn't properly execute
		binding->cmd_act = (ConCommand*)g_consoleCommands->FindCommand(varargs("+%s", binding->commandString.ToCString()));
		binding->cmd_deact = (ConCommand*)g_consoleCommands->FindCommand(varargs("-%s", binding->commandString.ToCString()));

		// if found only one command with plus or minus
		if(!binding->cmd_act || !binding->cmd_deact)
			binding->cmd_act = (ConCommand*)g_consoleCommands->FindCommand( binding->commandString.ToCString() );
	}

	// if anly command found
	if(	binding->cmd_act || binding->cmd_deact ||
		binding->boundAction)
	{

	}
	else
	{
		MsgError("Cannot bind command '%s' to key '%s'\n", binding->commandString.ToCString(), s_keyMapList[binding->key_index].name);
		return false;
	}

	return true;
}

axisAction_t* CInputCommandBinder::FindAxisAction(const char* name)
{
	for(int i = 0; i < m_axisActs.numElem();i++)
	{
		if(!m_axisActs[i].name.CompareCaseIns(name))
			return &m_axisActs[i];
	}

	return nullptr;
}

// searches for binding
in_binding_t* CInputCommandBinder::FindBinding(const char* pszKeyStr)
{
	int bindingKeyIndices[3];

	if (!UTIL_GetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return nullptr;

	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		if (binding->mod_index[0] == bindingKeyIndices[0] &&
			binding->mod_index[1] == bindingKeyIndices[1] &&
			binding->key_index == bindingKeyIndices[2])
			return binding;
	}

	return nullptr;
}

in_binding_t* CInputCommandBinder::FindBindingByCommand(ConCommandBase* cmdBase, const char* argStr /*= nullptr*/, in_binding_t* startFrom /*= nullptr*/)
{
	int startIdx = m_bindings.findIndex(startFrom) + 1;

	for (int i = startIdx; i < m_bindings.numElem(); i++)
	{
		in_binding_t* binding = m_bindings[i];

		if ((binding->cmd_act == cmdBase || binding->cmd_deact == cmdBase) && 
			(!argStr || argStr && !binding->argumentString.CompareCaseIns(argStr)))
			return binding;
	}

	return nullptr;
}

in_binding_t* CInputCommandBinder::FindBindingByCommandName(const char* name, const char* argStr /*= nullptr*/, in_binding_t* startFrom /*= nullptr*/)
{
	int startIdx = m_bindings.findIndex(startFrom) + 1;

	for (int i = startIdx; i < m_bindings.numElem(); i++)
	{
		in_binding_t* binding = m_bindings[i];

		if(!binding->commandString.CompareCaseIns(name) &&
			(!argStr || argStr && !binding->argumentString.CompareCaseIns(argStr)))
			return binding;
	}

	return nullptr;
}

void CInputCommandBinder::DeleteBinding( in_binding_t* binding )
{
	if(binding == nullptr)
		return;

	if(m_bindings.remove(binding))
		delete binding;
}

// removes single binding on specified keychar
void CInputCommandBinder::UnbindKey(const char* pszKeyStr)
{
	int bindingKeyIndices[3];

	if (!UTIL_GetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return;

	int results = 0;

	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		if (binding->mod_index[0] == bindingKeyIndices[0] &&
			binding->mod_index[1] == bindingKeyIndices[1] &&
			binding->key_index == bindingKeyIndices[2])
		{
			delete binding;
			m_bindings.removeIndex(i);
			i--;

			results++;
		}
	}

	if(results)
		MsgInfo("'%s' unbound (%d matches)\n", pszKeyStr, results);
}

void CInputCommandBinder::UnbindCommandByName(const char* name, const char* argStr /*= nullptr*/)
{
	in_binding_t* binding = nullptr;
	while (binding = FindBindingByCommandName(name, argStr, binding))
	{
		DeleteBinding(binding);
	}
}

// clears and removes all key bindings
void CInputCommandBinder::UnbindAll()
{
	for(int i = 0; i < m_bindings.numElem();i++)
		delete m_bindings[i];

	m_bindings.clear();
}

void CInputCommandBinder::UnbindAll_Joystick()
{
	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		int keyNum = s_keyMapList[binding->key_index].keynum;

		if(keyNum >= JOYSTICK_START_KEYS && keyNum < MOU_B1)
		{
			delete binding;
			m_bindings.removeIndex(i);
			i--;
		}
	}
}

// registers axis action
void CInputCommandBinder::RegisterJoyAxisAction( const char* name, JOYAXISFUNC axisFunc )
{
	axisAction_t act;
	act.name = "ax_" + _Es(name);
	act.func = axisFunc;

	m_axisActs.append( act );
}

bool CInputCommandBinder::CheckModifiersAndDepress(in_binding_t* binding, int currentKeyIdent, bool bPressed)
{
	int validModifiers = (binding->mod_index[1] != -1) ? 2 : ((binding->mod_index[0] != -1) ? 1 : 0);

	// if we don't have modifiers, skip the check with returning TRUE
	if (!validModifiers)
		return true;

	int numModifiers = 0;

	for (int i = 0; i < m_currentButtons.numElem(); i++)
	{
		int keyIdent = s_keyMapList[binding->mod_index[numModifiers]].keynum;

		if (!bPressed && currentKeyIdent == keyIdent)
			ExecuteBinding(binding, false);

		if (m_currentButtons[i] == keyIdent)
			numModifiers++;

		if (numModifiers >= validModifiers)
			break;
	}

	return (numModifiers == validModifiers);
}

//
// Event processing
//
void CInputCommandBinder::OnKeyEvent(int keyIdent, bool bPressed)
{
	if(in_keys_debug.GetBool())
		MsgWarning("-- KeyPress: %s (%d)\n", KeyIndexToString(keyIdent), bPressed);

	if (bPressed)
		m_currentButtons.addUnique(keyIdent);
	else
		m_currentButtons.fastRemove(keyIdent);

	DkList<in_binding_t*> complexExecuteList;
	DkList<in_binding_t*> executeList;

	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];
		
		// here we also depress modifiers if has any
		if (!CheckModifiersAndDepress(binding, keyIdent, bPressed))
			continue;

		// check on the keymap
		if (s_keyMapList[binding->key_index].keynum == keyIdent)
		{
			int validModifiers = (binding->mod_index[1] != -1) ? 2 : ((binding->mod_index[0] != -1) ? 1 : 0);

			if (validModifiers)
				complexExecuteList.append(binding);
			else
				executeList.append(binding);
		}
	}

	// complex actions are in favor
	if (complexExecuteList.numElem())
	{
		for (int i = 0; i < complexExecuteList.numElem(); i++)
		{
			ExecuteBinding(complexExecuteList[i], bPressed);
		}

		return;
	}

	for (int i = 0; i < executeList.numElem(); i++)
	{
		ExecuteBinding(executeList[i], bPressed);
	}
}

void CInputCommandBinder::OnMouseEvent( int button, bool bPressed )
{
	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		if (!CheckModifiersAndDepress(binding, button, bPressed))
			continue;

		if(s_keyMapList[binding->key_index].keynum == button)
		{
			ExecuteBinding( binding, bPressed);
		}
	}
}

void CInputCommandBinder::OnMouseWheel( int scroll )
{
	int button = (scroll > 0) ?  MOU_WHUP : MOU_WHDN;

	for(int i = 0; i < m_bindings.numElem();i++)
	{
		in_binding_t* binding = m_bindings[i];

		if (!CheckModifiersAndDepress(binding, 0, true))
			continue;

		if(s_keyMapList[binding->key_index].keynum == button)
		{
			ExecuteBinding( binding, (scroll > 0));
		}
	}
}

void CInputCommandBinder::OnTouchEvent( const Vector2D& pos, int finger, bool down )
{
	if(in_touchzones_debug.GetInt() == 2)
		MsgWarning("-- Touch [%g %g] (%d)\n", pos.x, pos.y, down);

	for(int i = 0; i < m_touchZones.numElem(); i++)
	{
		in_touchzone_t* tz = &m_touchZones[i];

		Rectangle_t rect(tz->position - tz->size*0.5f, tz->position + tz->size*0.5f);

		if(!down)
		{
			if(tz->finger == finger) // if finger up
			{
				ExecuteTouchZone( tz, down );
				tz->finger = -1;
			}
		}
		else if( rect.IsInRectangle(pos) )
		{
			if (in_touchzones_debug.GetInt() == 2)
				Msg("found zone %s\n", tz->name.ToCString());

			tz->finger = finger;

			ExecuteTouchZone( tz, down );
			continue;
		}
	}
}

void CInputCommandBinder::OnJoyAxisEvent( short axis, short value )
{
	int joyAxisCode = axis + JOYSTICK_START_AXES;
	in_binding_t* axisBinding = nullptr;

	for (int i = 0; i < m_bindings.numElem(); i++)
	{
		in_binding_t* binding = m_bindings[i];

		// run through all axes
		if (binding->mod_index[0] == -1 &&
			binding->mod_index[1] == -1 &&
			s_keyMapList[binding->key_index].keynum == joyAxisCode)
		{
			if (binding->boundAction)
				binding->boundAction->func(value);
			else
				ExecuteBinding(binding, (abs(value) > 16384));
		}
	}
}

void CInputCommandBinder::DebugDraw(const Vector2D& screenSize)
{
	if(!in_touchzones_debug.GetBool())
		return;

	materials->Setup2D(screenSize.x,screenSize.y);

	eqFontStyleParam_t fontParams;
	fontParams.styleFlag |= TEXT_STYLE_SHADOW;
	fontParams.textColor = color4_white;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	static IEqFont* defaultFont = g_fontCache->GetFont("default", 30);

	for(int i = 0; i < m_touchZones.numElem(); i++)
	{
		in_touchzone_t* tz = &m_touchZones[i];

		Rectangle_t rect((tz->position-tz->size*0.5f)*screenSize, (tz->position+tz->size*0.5f)*screenSize);

		defaultFont->RenderText( tz->name.ToCString() , rect.vleftTop, fontParams);

		Vertex2D_t touchQuad[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,touchQuad,elementsOf(touchQuad), nullptr, ColorRGBA(0.435,0.435,0.435, tz->finger >= 0 ? 0.25f : 0.35f  ), &blending);
	}

}

// executes binding with selected state
void CInputCommandBinder::ExecuteBinding( in_binding_t* pBinding, bool bState )
{
	ExecuteBoundCommands(pBinding, bState);
}

void CInputCommandBinder::ExecuteTouchZone( in_touchzone_t* zone, bool bState )
{
	ExecuteBoundCommands(zone, bState);
}

template <typename T>
void CInputCommandBinder::ExecuteBoundCommands(T* zone, bool bState)
{
	DkList<EqString> args;

	xstrsplit( zone->argumentString.GetData(), " ", args);

	ConCommand *cmd = bState ? zone->cmd_act : zone->cmd_deact;

	// dispatch command
	if(cmd)
	{
		if(in_keys_debug.GetBool())
			MsgWarning("dispatch %s\n", cmd->GetName());

		cmd->DispatchFunc( args );
	}
}

#ifndef DLL_EXPORT

void con_key_list(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	keyNameMap_t* names = s_keyMapList;

	do
	{
		keyNameMap_t& name = *names;

		if(name.name == nullptr)
			break;

		if(list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		if(*query == 0 || xstristr(name.name, query))
			list.append(name.name);

	}while(names++);
}

void binding_key_list(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	DkList<in_binding_t*> *bindingList = g_inputCommandBinder->GetBindingList();

	for (int i = 0; i < bindingList->numElem(); i++)
	{
		in_binding_t* binding = bindingList->ptr()[i];

		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		if (list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		list.append(keyNameString.ToCString());
	}
}

DECLARE_CMD_VARIANTS(bind,"Binds action to key", con_key_list, 0)
{
	if(CMD_ARGC > 1)
	{
		EqString agrstr;

		for(int i = 2; i < CMD_ARGC; i++)
			agrstr.Append(varargs((i < CMD_ARGC-1) ? "%s " : "%s", CMD_ARGV(i).ToCString()));

		g_inputCommandBinder->BindKey(CMD_ARGV(0).ToCString(), CMD_ARGV(1).ToCString(),(char*)agrstr.GetData());
	}
	else
		MsgInfo("Usage: bind <key> <command> [args,...]\n");
}

DECLARE_CMD(list_binding,"Shows bound keys",0)
{
	MsgInfo("---- List of bound keys to commands ----\n");
	DkList<in_binding_t*> *bindingList = g_inputCommandBinder->GetBindingList();

	for(int i = 0; i < bindingList->numElem();i++)
	{
		in_binding_t* binding = bindingList->ptr()[i];

		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		Msg("bind %s %s %s\n", keyNameString.ToCString(), binding->commandString.ToCString(), binding->argumentString.ToCString() );
	}

	MsgInfo("---- %d keys/commands bound ----\n", bindingList->numElem());
}

DECLARE_CMD(list_touchzones,"Shows bound keys",0)
{
	MsgInfo("---- List of bound touchzones to commands ----\n");
	DkList<in_touchzone_t> *touchList = g_inputCommandBinder->GetTouchZoneList();

	for(int i = 0; i < touchList->numElem();i++)
	{
		in_touchzone_t* tz = &touchList->ptr()[i];

		Msg("Touchzone %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n", tz->name.ToCString(), tz->commandString.ToCString(), tz->position.x, tz->position.y, tz->size.x, tz->size.y);
	}

	MsgInfo("---- %d touch zones ----\n", touchList->numElem());
}

DECLARE_CMD(list_axisActions,"Shows axis list can be bound",0)
{
	MsgInfo("---- List of axese ----\n");
	DkList<axisAction_t>* axisActs = g_inputCommandBinder->GetAxisActionList();

	for(int i = 0; i < axisActs->numElem();i++)
	{
		axisAction_t* act = &axisActs->ptr()[i];

		Msg("    %s\n", act->name.ToCString());
	}

	MsgInfo("---- %d axes ----\n", axisActs->numElem());
}


DECLARE_CMD_VARIANTS(unbind,"Unbinds a key", binding_key_list, 0)
{
	if(CMD_ARGC > 0)
	{
		g_inputCommandBinder->UnbindKey(CMD_ARGV(0).ToCString());
	}
}

DECLARE_CMD(unbindall,"Unbinds all keys",0)
{
	g_inputCommandBinder->UnbindAll();
}

DECLARE_CMD(unbindjoystick,"Unbinds joystick controls",0)
{
	g_inputCommandBinder->UnbindAll_Joystick();
}


#endif // DLL_EXPORT
