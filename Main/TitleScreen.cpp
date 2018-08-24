#include "stdafx.h"
#include "TitleScreen.hpp"
#include "Application.hpp"
#include "TransitionScreen.hpp"
#include "SettingsScreen.hpp"
#include <Shared/Profiling.hpp>
#include "Scoring.hpp"
#include "SongSelect.hpp"
#include <Audio/Audio.hpp>
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "HealthGauge.hpp"
#include "Shared/Jobs.hpp"
#include "ScoreScreen.hpp"
#include "Shared/Enum.hpp"

class TitleScreen_Impl : public TitleScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;


	void Exit()
	{
		g_application->Shutdown();
	}

	void Start()
	{
		g_application->AddTickable(SongSelect::Create());
	}

	void Settings()
	{
		g_application->AddTickable(SettingsScreen::Create());
	}

public:
	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;
		//m_canvas = Utility::MakeRef(new Canvas());
		//
		////GUI Buttons
		//{
		//	LayoutBox* box = new LayoutBox();
		//	Canvas::Slot* slot = m_canvas->Add(box->MakeShared());
		//	slot->anchor = Anchor(0.5f, 0.5f);
		//	slot->autoSizeX = true;
		//	slot->autoSizeY = true;
		//	slot->alignment = Vector2(0.5, 0.5);

		//	
		//	box->layoutDirection = LayoutBox::Vertical;

		//	Label* titleLabel = new Label();
		//	titleLabel->SetText(L"unnamed_sdvx_clone");
		//	titleLabel->SetFontSize(100);
		//	box->Add(titleLabel->MakeShared());

		//	LayoutBox::Slot* btnSlot;
		//	Button* startBtn = new Button(m_guiStyle);
		//	startBtn->OnPressed.Add(this, &TitleScreen_Impl::Start);
		//	startBtn->SetText(L"Start");
		//	startBtn->SetFontSize(32);
		//	btnSlot = box->Add(startBtn->MakeShared());
		//	btnSlot->padding = Margin(2);
		//	btnSlot->fillX = true;
		//	
		//	Button* settingsBtn = new Button(m_guiStyle);
		//	settingsBtn->OnPressed.Add(this, &TitleScreen_Impl::Settings);
		//	settingsBtn->SetText(L"Settings");
		//	settingsBtn->SetFontSize(32);
		//	btnSlot = box->Add(settingsBtn->MakeShared());
		//	btnSlot->padding = Margin(2);
		//	btnSlot->fillX = true;

		//	Button* exitBtn = new Button(m_guiStyle);
		//	exitBtn->OnPressed.Add(this, &TitleScreen_Impl::Exit);
		//	exitBtn->SetText(L"Exit");
		//	exitBtn->SetFontSize(32);
		//	btnSlot = box->Add(exitBtn->MakeShared());
		//	btnSlot->padding = Margin(2);
		//	btnSlot->fillX = true;

		//}

		return true;
	}
	~TitleScreen_Impl()
	{
	}


	virtual void OnSuspend()
	{
		//g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		//Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//slot->anchor = Anchors::Full;
		g_gameWindow->SetCursorVisible(true);
	}


};

TitleScreen* TitleScreen::Create()
{
	TitleScreen_Impl* impl = new TitleScreen_Impl();
	return impl;
}