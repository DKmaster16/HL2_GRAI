//The following include files are necessary to allow your MyPanel.cpp to compile.
#include "cbase.h"
#include "Igraidifficultypanel.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>


//CMyPanel class: Tutorial example class
class CMyPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CMyPanel, vgui::Frame);
	//CMyPanel : This Class / vgui::Frame : BaseClass

	CMyPanel(vgui::VPANEL parent); 	// Constructor
	~CMyPanel(){};				// Destructor

	virtual void Activate()
	{
		BaseClass::Activate();
		OnResetData();
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		m_ArmedBgColor = GetSchemeColor("ListPanel.SelectedBgColor", pScheme);

		m_FgColor1 = GetSchemeColor("ListPanel.TextColor", pScheme);
		m_FgColor2 = GetSchemeColor("ListPanel.SelectedTextColor", pScheme);

		m_BgColor = GetSchemeColor("ListPanel.BgColor", GetBgColor(), pScheme);
		m_BgColor = GetSchemeColor("ListPanel.TextBgColor", m_BgColor, pScheme);
		m_SelectionBG2Color = GetSchemeColor("ListPanel.SelectedOutOfFocusBgColor", pScheme);

		SetBgColor(m_BgColor);
		SetFgColor(m_FgColor1);
	}

	virtual void PerformLayout()
	{
		if (m_bSelected)
		{
			SetFgColor(m_FgColor2);
		}
		else
		{
			SetFgColor(m_FgColor1);
		}
		BaseClass::PerformLayout();
		Repaint();
	}

	virtual void PaintBackground()
	{
		int wide, tall;
		GetSize(wide, tall);

		if (m_bSelected)
		{
			VPANEL focus = input()->GetFocus();
			// if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
			if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())))
			{
				surface()->DrawSetColor(m_ArmedBgColor);
			}
			else
			{
				surface()->DrawSetColor(m_SelectionBG2Color);
			}
		}
		else
		{
			surface()->DrawSetColor(GetBgColor());
		}
		surface()->DrawFilledRect(0, 0, wide, tall);
	}

	virtual void Paint()
	{
//		BaseClass::Paint();
		surface()->DrawSetColor(128, 128, 128, 128); //RGBA
		surface()->DrawFilledRect(32, 32, 480, 352); //x0,y0,x1,y1
		surface()->DrawSetColor(196, 196, 196, 128); //RGBA
		surface()->DrawOutlinedRect(32, 32, 480, 352); //x0,y0,x1,y1
		SetPaintBackgroundEnabled(true);
	}


	void SetSelected(bool state)
	{
		if (m_bSelected != state)
		{
			if (state)
			{
				RequestFocus();
			}
			m_bSelected = state;
			SetPaintBackgroundEnabled(state);
			InvalidateLayout();
			Repaint();
		}
	}

	bool IsSelected()
	{
		return m_bSelected;
	}

	virtual void OnSetFocus()
	{
		InvalidateLayout(); // force the layout to be redone so we can change text color according to focus
		BaseClass::OnSetFocus();
	}

	virtual void OnKillFocus()
	{
		InvalidateLayout(); // force the layout to be redone so we can change text color according to focus
		BaseClass::OnSetFocus();
	}

//	virtual void OnResetData();
//	virtual void OnApplyChanges();

	MESSAGE_FUNC(OnRadioButtonChecked, "RadioButtonChecked");

	// Called when page is loaded.  Data should be reloaded from document into controls.
	MESSAGE_FUNC(OnResetData, "ResetData");

	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	MESSAGE_FUNC(OnApplyChanges, "ApplyChanges");

protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

private:
	bool m_bSelected;

	Color m_FgColor1;
	Color m_FgColor2;
	Color m_BgColor;
	Color m_ArmedBgColor;
	Color m_SelectionBG2Color;

	//Other used VGUI control Elements:
	Button *m_pCloseButton;
	Button *m_pApplyButton;
	Button *m_pOKButton;

	RadioButton *m_pStoryRadio;
	RadioButton *m_pEasyRadio;
	RadioButton *m_pNormalRadio;
	RadioButton *m_pHardRadio;
	RadioButton *m_pDiabolicalRadio;

	CheckButton		*m_pAutoAim;
};

ConVarRef difficulty("difficulty");
ConVarRef AutoAim("sk_allow_autoaim");

// Constuctor: Initializes the Panel
CMyPanel::CMyPanel(vgui::VPANEL parent)	: BaseClass(NULL, "MyPanel")
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetDeleteSelfOnClose(false);
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	SetTitle("#GameUI_Difficulty", true);

	SetSize(512, 384);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	LoadControlSettings("resource/ui/graidifficultypanel.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	DevMsg("MyPanel has been constructed\n");

	//Button done
	m_pCloseButton = new Button(this, "Button1", "#GameUI_Cancel", this, "turnoff");
	m_pCloseButton->SetPos(362, 354);
	m_pApplyButton = new Button(this, "Button2", "#GameUI_Apply", this, "Apply");
	m_pApplyButton->SetPos(432, 354);
	m_pOKButton = new Button(this, "Button3", "#GameUI_OK", this, "OK");
	m_pOKButton->SetPos(292, 354);
//	m_pCloseButton->SetDepressedSound("common/bugreporter_succeeded.wav");
//	m_pCloseButton->SetReleasedSound("ui/buttonclick.wav");

	m_pStoryRadio = new RadioButton(this, "Skill0Radio", "#GameUI_SkillStory");
	m_pStoryRadio->SetPos(35, 65);
	m_pStoryRadio->SetWide(96);
	m_pEasyRadio = new RadioButton(this, "Skill1Radio", "#GameUI_SkillEasy");
	m_pEasyRadio->SetPos(35, 115);
	m_pEasyRadio->SetWide(96);
	m_pNormalRadio = new RadioButton(this, "Skill2Radio", "#GameUI_SkillNormal");
	m_pNormalRadio->SetPos(35, 165);
	m_pNormalRadio->SetWide(96);
	m_pHardRadio = new RadioButton(this, "Skill3Radio", "#GameUI_SkillHard");
	m_pHardRadio->SetPos(35, 215);
	m_pHardRadio->SetWide(96);
	m_pDiabolicalRadio = new RadioButton(this, "Skill4Radio", "#GameUI_SkillDiabolical");
	m_pDiabolicalRadio->SetPos(35, 265);
	m_pDiabolicalRadio->SetWide(96);

//	ConVarRef difficulty("difficulty");

	m_pAutoAim = new CheckButton(this, "AutoAim", "#GameUI_AutoAim");
	m_pAutoAim->SetPos(40, 315);
	m_pAutoAim->SetWide(96);

	if (difficulty.GetInt() <= 2)
		m_pAutoAim->SetEnabled(true);
	else
		m_pAutoAim->SetEnabled(false);
}

//Class: CMyPanelInterface Class. Used for construction.
class CMyPanelInterface : public MyPanel
{
private:
	CMyPanel *MyPanel;
public:
	CMyPanelInterface()
	{
		MyPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		MyPanel = new CMyPanel(parent);
	}
	void Destroy()
	{
		if (MyPanel)
		{
			MyPanel->SetParent((vgui::Panel *)NULL);
			delete MyPanel;
		}
	}
	void Activate(void)
	{
		if (MyPanel)
		{
			MyPanel->Activate();
		}
	}
};
static CMyPanelInterface g_MyPanel;
MyPanel* mypanel = (MyPanel*)&g_MyPanel;

ConVar cl_showmypanel("cl_showmypanel", "0", FCVAR_CLIENTDLL, "Sets the state of myPanel <state>");

void CMyPanel::OnTick()
{
	BaseClass::OnTick();
	SetVisible(cl_showmypanel.GetBool());
}

CON_COMMAND(ToggleMyPanel, "Toggles testpanelfenix on or off")
{
	cl_showmypanel.SetValue(!cl_showmypanel.GetBool());
	mypanel->Activate();
};

void CMyPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
	if (!Q_stricmp(pcCommand, "OK"))
	{
		// apply the data
		OnApplyChanges();
		cl_showmypanel.SetValue(0);
	}
	else if(!Q_stricmp(pcCommand, "Apply"))
	{
		// apply the data
		OnApplyChanges();
	}
	else if (!Q_stricmp(pcCommand, "turnoff"))
		cl_showmypanel.SetValue(0);
}

//-----------------------------------------------------------------------------
// Purpose: resets controls
//-----------------------------------------------------------------------------
void CMyPanel::OnResetData()
{
//	ConVarRef var("difficulty");

	if (difficulty.GetInt() == 0)
	{
		m_pStoryRadio->SetSelected(true);
	}
	else if (difficulty.GetInt() == 1)
	{
		m_pEasyRadio->SetSelected(true);
	}
	else if (difficulty.GetInt() == 3)
	{
		m_pHardRadio->SetSelected(true);
	}
	else if (difficulty.GetInt() == 4)
	{
		m_pDiabolicalRadio->SetSelected(true);
	}
	else
	{
		m_pNormalRadio->SetSelected(true);
	}
	
//	ConVarRef AutoAim("sk_allow_autoaim");

	if (difficulty.GetInt() <= 2)
		m_pAutoAim->SetEnabled(true);
	else
		m_pAutoAim->SetEnabled(false);

	if (AutoAim.GetBool() == 1)
	{
		m_pAutoAim->SetSelected(true);
	}
	else
	{
		m_pAutoAim->SetSelected(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets data based on control settings
//-----------------------------------------------------------------------------
void CMyPanel::OnApplyChanges()
{
//	ConVarRef var("difficulty");

	if (m_pStoryRadio->IsSelected())
	{
		difficulty.SetValue(0);
	}
	else if (m_pEasyRadio->IsSelected())
	{
		difficulty.SetValue(1);
	}
	else if (m_pHardRadio->IsSelected())
	{
		difficulty.SetValue(3);
	}
	else if (m_pDiabolicalRadio->IsSelected())
	{
		difficulty.SetValue(4);
	}
	else
	{
		difficulty.SetValue(2);
	}

	if (difficulty.GetInt() <= 2)
		m_pAutoAim->SetEnabled(true);
	else
		m_pAutoAim->SetEnabled(false);

//	ConVarRef AutoAim("sk_allow_autoaim");

	if (m_pAutoAim->IsSelected())
	{
		AutoAim.SetValue(1);
	}
	else
	{
		AutoAim.SetValue(0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: enables apply button on radio buttons being pressed
//-----------------------------------------------------------------------------
void CMyPanel::OnRadioButtonChecked()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
