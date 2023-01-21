//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "OptionsSubDifficulty.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COverrideOptionsSubDifficulty::COverrideOptionsSubDifficulty(vgui::Panel *parent) : BaseClass(parent, NULL)
{
	m_pStoryRadio = new RadioButton(this, "Skill0Radio", "#GameUI_SkillStory");
	m_pEasyRadio = new RadioButton(this, "Skill1Radio", "#GameUI_SkillEasy");
	m_pNormalRadio = new RadioButton(this, "Skill2Radio", "#GameUI_SkillNormal");
	m_pHardRadio = new RadioButton(this, "Skill3Radio", "#GameUI_SkillHard");
	m_pDiabolicalRadio = new RadioButton(this, "Skill4Radio", "#GameUI_SkillDiabolical");

	LoadControlSettings("Resource/OptionsSubDifficulty.res");
}

//-----------------------------------------------------------------------------
// Purpose: resets controls
//-----------------------------------------------------------------------------
void COverrideOptionsSubDifficulty::OnResetData()
{
	ConVarRef var("difficulty");

	if (var.GetInt() == 0)
	{
		m_pStoryRadio->SetSelected(true);
	}
	else if (var.GetInt() == 1)
	{
		m_pEasyRadio->SetSelected(true);
	}
	else if (var.GetInt() == 3)
	{
		m_pHardRadio->SetSelected(true);
	}
	else if (var.GetInt() == 4)
	{
		m_pDiabolicalRadio->SetSelected(true);
	}
	else
	{
		m_pNormalRadio->SetSelected(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets data based on control settings
//-----------------------------------------------------------------------------
void COverrideOptionsSubDifficulty::OnApplyChanges()
{
	ConVarRef var("difficulty");

	if (m_pStoryRadio->IsSelected())
	{
		var.SetValue(0);
	}
	else if (m_pEasyRadio->IsSelected())
	{
		var.SetValue(1);
	}
	else if (m_pHardRadio->IsSelected())
	{
		var.SetValue(3);
	}
	else if (m_pDiabolicalRadio->IsSelected())
	{
		var.SetValue(4);
	}
	else
	{
		var.SetValue(2);
	}
}


//-----------------------------------------------------------------------------
// Purpose: enables apply button on radio buttons being pressed
//-----------------------------------------------------------------------------
void COverrideOptionsSubDifficulty::OnRadioButtonChecked()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
