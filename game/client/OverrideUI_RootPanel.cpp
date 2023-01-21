#include "cbase.h"
#include "overrideui_rootpanel.h"
#include "Override_Interface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include <vgui_controls/Frame.h>

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUIDLL("GameUI");

OverrideUI_RootPanel *guiroot = NULL;

void OverrideGameUI()
{
	if (!OverrideUI->GetPanel())
	{
		OverrideUI->Create(NULL);
	}
	if (guiroot->GetGameUI())
	{
		guiroot->GetGameUI()->SetMainMenuOverride(guiroot->GetVPanel());
		return;
	}
}

OverrideUI_RootPanel::OverrideUI_RootPanel(VPANEL parent) : Panel(NULL, "OverrideUIRootPanel")
{
	SetParent(parent);

	guiroot = this;
	gameui = NULL;
	LoadGameUI();
	SetScheme("ClientScheme");

	SetDragEnabled(false);
	SetShowDragHelper(false);
	SetProportional(false);
	SetVisible(true);

	surface()->GetScreenSize(width, height);
	SetSize(width, height);
	SetPos(0, 0);

	LoadControlSettings("resource/graidifficultypanel.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	m_bCopyFrameBuffer = false;
	gameui = NULL;

	m_ExitingFrameCount = 0;
}

IGameUI *OverrideUI_RootPanel::GetGameUI()
{
	if (!gameui)
	{
		if (!LoadGameUI())
			return NULL;
	}

	return gameui;
}

bool OverrideUI_RootPanel::LoadGameUI()
{
	if (!gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (gameUIFactory)
		{
			gameui = (IGameUI *)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
			if (!gameui)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

OverrideUI_RootPanel::~OverrideUI_RootPanel()
{
	gameui = NULL;
	g_GameUIDLL.Unload();
}

void OverrideUI_RootPanel::PerformLayout()
{
	BaseClass::PerformLayout();
};

void OverrideUI_RootPanel::OnCommand(const char* command)
{
	engine->ExecuteClientCmd(command);
}

void OverrideUI_RootPanel::OnTick()
{
	BaseClass::OnTick();
};

void OverrideUI_RootPanel::OnThink()
{
	BaseClass::OnThink();
};

void OverrideUI_RootPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Resize the panel to the screen size
	// Otherwise, it'll just be in a little corner
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
}
