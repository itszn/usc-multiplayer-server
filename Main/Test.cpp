#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include <Shared/Profiling.hpp>
#include "Scoring.hpp"
#include <Audio/Audio.hpp>
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "HealthGauge.hpp"
#include "Shared/Jobs.hpp"
#include "ScoreScreen.hpp"
#include "Shared/Enum.hpp"

#ifdef _WIN32
#include "SDL_keycode.h"
#else
#include "SDL2/SDL_keycode.h"
#endif

class Test_Impl : public Test
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	//Ref<SettingsBar> m_settings;

	WString m_currentText;
	float a = 0.1f; // 0 - 1
	float b = 2.0f; // 0 - 10
	float c = 1.0f; // 0 - 5
	float d = 0.0f; // -2 - 2
	int e = 0;
	int f = 0;
	Ref<Gamepad> m_gamepad;
	Vector<String> m_textSettings;

public:
	static void StaticFunc(int32 arg)
	{
	}
	static int32 StaticFunc1(int32 arg)
	{
		return arg * 2;
	}
	static int32 StaticFunc2(int32 arg)
	{
		return arg * 2;
	}
	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;

		m_gamepad = g_gameWindow->OpenGamepad(0);
		return true;
	}
	~Test_Impl()
	{
	}
	virtual void OnKeyPressed(int32 key) override
	{
		if(key == SDLK_TAB)
		{
			//m_settings->SetShow(!m_settings->IsShown());
		}
	}
	virtual void OnKeyReleased(int32 key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
	}
	virtual void Tick(float deltaTime) override
	{
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	return impl;
}
