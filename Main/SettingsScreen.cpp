#include "stdafx.h"
#include "SettingsScreen.hpp"
#include "Application.hpp"
#include <Shared/Profiling.hpp>
#include "GameConfig.hpp"
#include "Scoring.hpp"
#include <Audio/Audio.hpp>
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include <GUI/GUI.hpp>
#include <GUI/CommonGUIStyle.hpp>
#include <GUI/Button.hpp>
#include <GUI/Slider.hpp>
#include <GUI/ScrollBox.hpp>
#include <GUI/SettingsBar.hpp>
#include <GUI/Spinner.hpp>
#include "HealthGauge.hpp"
#include "Shared/Jobs.hpp"
#include "ScoreScreen.hpp"
#include "Shared/Enum.hpp"

class SettingsScreen_Impl : public SettingsScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;

	//TODO: Use argument instead of many functions if possible.
	void SetKey_BTA()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT0));
	}
	void SetKey_BTB()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT1));
	}
	void SetKey_BTC()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT2));
	}
	void SetKey_BTD()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT3));
	}
	void SetKey_FXL()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_FX0));
	}
	void SetKey_FXR()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_FX1));
	}
	void SetKey_ST()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BTS));
	}

	void Exit()
	{
		g_input.Init(*g_gameWindow);
		g_application->RemoveTickable(this);
	}

public:

	//TODO: Controller support and the rest of the options and better layout
	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;
		m_canvas = Utility::MakeRef(new Canvas());

		//Options select
		ScrollBox* scroller = new ScrollBox(m_guiStyle);
		
		Canvas::Slot* slot = m_canvas->Add(scroller->MakeShared());
		slot->anchor = Anchor(0.0f, 0.0f);
		slot->autoSizeX = true;
		slot->autoSizeY = true;
		slot->alignment = Vector2(0.0, 0.0);
		
		LayoutBox* box = new LayoutBox();
		box->layoutDirection = LayoutBox::Vertical;
		scroller->SetContent(box->MakeShared());
		LayoutBox::Slot* btnSlot;
		// Start Button
		{
			LayoutBox* stBox = new LayoutBox();

			stBox->layoutDirection = LayoutBox::Horizontal;
			{
				Button* stBtn = new Button(m_guiStyle);
				stBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_ST);
				stBtn->SetText(L"Start");
				stBtn->SetFontSize(32);
				btnSlot = stBox->Add(stBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			box->Add(stBox->MakeShared());
		}
		// BT Buttons
		{
			LayoutBox* btBox = new LayoutBox();
			btBox->layoutDirection = LayoutBox::Horizontal;

			{
				Button* btaBtn = new Button(m_guiStyle);
				btaBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTA);
				btaBtn->SetText(L"BT-A");
				btaBtn->SetFontSize(32);
				btnSlot = btBox->Add(btaBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btbBtn = new Button(m_guiStyle);
				btbBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTB);
				btbBtn->SetText(L"BT-B");
				btbBtn->SetFontSize(32);
				btnSlot = btBox->Add(btbBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btcBtn = new Button(m_guiStyle);
				btcBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTC);
				btcBtn->SetText(L"BT-C");
				btcBtn->SetFontSize(32);
				btnSlot = btBox->Add(btcBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btdBtn = new Button(m_guiStyle);
				btdBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTD);
				btdBtn->SetText(L"BT-D");
				btdBtn->SetFontSize(32);
				btnSlot = btBox->Add(btdBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			box->Add(btBox->MakeShared());
		}
		// FX Buttons
		{
			LayoutBox* fxBox = new LayoutBox();
			
			fxBox->layoutDirection = LayoutBox::Horizontal;
			{
				Button* fxlBtn = new Button(m_guiStyle);
				fxlBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_FXL);
				fxlBtn->SetText(L"FX-L");
				fxlBtn->SetFontSize(32);
				btnSlot = fxBox->Add(fxlBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* fxrBtn = new Button(m_guiStyle);
				fxrBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_FXR);
				fxrBtn->SetText(L"FX-R");
				fxrBtn->SetFontSize(32);
				btnSlot = fxBox->Add(fxrBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			box->Add(fxBox->MakeShared());
		}


		Button* exitBtn = new Button(m_guiStyle);
		exitBtn->OnPressed.Add(this, &SettingsScreen_Impl::Exit);
		exitBtn->SetText(L"Exit");
		exitBtn->SetFontSize(32);
		btnSlot = box->Add(exitBtn->MakeShared());
		btnSlot->padding = Margin(2);
		btnSlot->fillX = true;

		return true;
	}



	virtual void OnSuspend()
	{
		g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		slot->anchor = Anchors::Full;
	}
};

SettingsScreen* SettingsScreen::Create()
{
	SettingsScreen_Impl* impl = new SettingsScreen_Impl();
	return impl;
}

class ButtonBindingScreen_Impl : public ButtonBindingScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;
	GameConfigKeys m_key;


public:
	ButtonBindingScreen_Impl(GameConfigKeys key)
	{
		m_key = key;
	}

	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;
		m_canvas = Utility::MakeRef(new Canvas());

		//Prompt Text
		LayoutBox* box = new LayoutBox();
		Canvas::Slot* slot = m_canvas->Add(box->MakeShared());
		slot->anchor = Anchor(0.5f, 0.5f);
		slot->autoSizeX = true;
		slot->autoSizeY = true;
		slot->alignment = Vector2(0.5f, 0.5f);

		Label* titleLabel = new Label();
		titleLabel->SetText(L"Press Key");
		titleLabel->SetFontSize(100);
		box->Add(titleLabel->MakeShared());

		return true;
	}

	virtual void OnKeyPressed(int32 key)
	{
		g_gameConfig.Set(m_key, key);
		g_application->RemoveTickable(this);
	}

	virtual void OnSuspend()
	{
		g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		slot->anchor = Anchors::Full;
	}
};

ButtonBindingScreen* ButtonBindingScreen::Create(GameConfigKeys key)
{
	ButtonBindingScreen_Impl* impl = new ButtonBindingScreen_Impl(key);
	return impl;
}