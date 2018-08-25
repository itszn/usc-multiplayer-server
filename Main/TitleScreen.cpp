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
extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

class TitleScreen_Impl : public TitleScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;
	lua_State* m_lua;


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

	void MousePressed(MouseButton button)
	{
		lua_getglobal(m_lua, "mouse_pressed");
		lua_pushnumber(m_lua, (int32)button);
		if (lua_pcall(m_lua, 1, 1, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
		int ret = luaL_checkinteger(m_lua, 1);
		lua_settop(m_lua, 0);
		switch (ret)
		{
		case 1:
			Start();
			break;
		case 2:
			Settings();
			break;
		case 3:
			Exit();
			break;
		default:
			break;
		}
	}

public:
	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;
		CheckedLoad(m_lua = g_application->LoadScript("titlescreen"));
		g_gameWindow->OnMousePressed.Add(this, &TitleScreen_Impl::MousePressed);
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

	virtual void Render(float deltaTime)
	{
		if (IsSuspended())
			return;

		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
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