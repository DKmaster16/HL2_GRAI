//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONS_SUB_DIFFICULTY_H
#define OPTIONS_SUB_DIFFICULTY_H
#ifdef _WIN32
#pragma once
#endif

using namespace vgui;

#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/PropertyPage.h>

//-----------------------------------------------------------------------------
// Purpose: Difficulty selection options
//-----------------------------------------------------------------------------
class COverrideOptionsSubDifficulty : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(COverrideOptionsSubDifficulty, vgui::PropertyPage);

public:
	COverrideOptionsSubDifficulty(vgui::Panel *parent);

	virtual void OnResetData();
	virtual void OnApplyChanges();

	MESSAGE_FUNC(OnRadioButtonChecked, "RadioButtonChecked");

private:
	vgui::RadioButton *m_pStoryRadio;
	vgui::RadioButton *m_pEasyRadio;
	vgui::RadioButton *m_pNormalRadio;
	vgui::RadioButton *m_pHardRadio;
	vgui::RadioButton *m_pDiabolicalRadio;
};


#endif // OPTIONS_SUB_DIFFICULTY_H