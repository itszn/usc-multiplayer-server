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
#include "Input.hpp"
#ifdef _WIN32
#include "SDL_keyboard.h"
#else
#include "SDL2/SDL_keyboard.h"
#endif

class SettingsScreen_Impl : public SettingsScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;
	Ref<SettingsBar> m_settings;

	Vector<String> m_speedMods = { "XMod", "MMod", "CMod" };
	Vector<String> m_laserModes = { "Keyboard", "Mouse", "Controller" };
	Vector<String> m_buttonModes = { "Keyboard", "Controller" };
	Vector<String> m_gamePads;
	Vector<String> m_skins;

	Vector<GameConfigKeys> m_keyboardKeys = {
		GameConfigKeys::Key_BTS,
		GameConfigKeys::Key_BT0,
		GameConfigKeys::Key_BT1,
		GameConfigKeys::Key_BT2,
		GameConfigKeys::Key_BT3,
		GameConfigKeys::Key_FX0,
		GameConfigKeys::Key_FX1
	};

	Vector<GameConfigKeys> m_keyboardLaserKeys = {
		GameConfigKeys::Key_Laser0Neg,
		GameConfigKeys::Key_Laser0Pos,
		GameConfigKeys::Key_Laser1Neg,
		GameConfigKeys::Key_Laser1Pos,
	};

	Vector<GameConfigKeys> m_controllerKeys = {
		GameConfigKeys::Controller_BTS,
		GameConfigKeys::Controller_BT0,
		GameConfigKeys::Controller_BT1,
		GameConfigKeys::Controller_BT2,
		GameConfigKeys::Controller_BT3,
		GameConfigKeys::Controller_FX0,
		GameConfigKeys::Controller_FX1
	};

	Vector<GameConfigKeys> m_controllerLaserKeys = {
		GameConfigKeys::Controller_Laser0Axis,
		GameConfigKeys::Controller_Laser1Axis,

	};

	Button* m_buttonButtons[7];
	Button* m_laserButtons[2];
	Panel* m_laserColorPanels[2];

	Texture m_whiteTex;

	int m_speedMod = 0;
	int m_laserMode = 0;
	int m_buttonMode = 0;
	int m_selectedGamepad = 0;
	int m_selectedSkin = 0;
	float m_modSpeed = 400.f;
	float m_hispeed = 1.f;
	float m_laserSens = 1.0f;
	float m_masterVolume = 1.0f;
	float m_laserColors[2] = { 0.25f, 0.75f };

	//TODO: Use argument instead of many functions if possible.
	void SetKey_BTA()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_BT0, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT0));
	}
	void SetKey_BTB()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_BT1, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT1));
	}
	void SetKey_BTC()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_BT2, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT2));
	}
	void SetKey_BTD()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_BT3, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BT3));
	}
	void SetKey_FXL()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_FX0, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_FX0));
	}
	void SetKey_FXR()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_FX1, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_FX1));
	}
	void SetKey_ST()
	{
		if (m_buttonMode == 1)
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_BTS, true, m_selectedGamepad));
		else
			g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Key_BTS));
	}

	void SetLL()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_Laser0Axis, m_laserMode == 2, m_selectedGamepad));
	}
	void SetRL()
	{
		g_application->AddTickable(ButtonBindingScreen::Create(GameConfigKeys::Controller_Laser1Axis, m_laserMode == 2, m_selectedGamepad));
	}

	void Exit()
	{
		Map<String, InputDevice> inputModeMap = {
			{ "Keyboard", InputDevice::Keyboard },
			{ "Mouse", InputDevice::Mouse },
			{ "Controller", InputDevice::Controller },
		};
		
		g_gameConfig.SetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice, inputModeMap[m_buttonModes[m_buttonMode]]);
		g_gameConfig.SetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice, inputModeMap[m_laserModes[m_laserMode]]);

		g_gameConfig.Set(GameConfigKeys::HiSpeed, m_hispeed);
		g_gameConfig.Set(GameConfigKeys::ModSpeed, m_modSpeed);
		g_gameConfig.Set(GameConfigKeys::MasterVolume, m_masterVolume);
		g_gameConfig.Set(GameConfigKeys::Laser0Color, m_laserColors[0]);
		g_gameConfig.Set(GameConfigKeys::Laser1Color, m_laserColors[1]);
		g_gameConfig.Set(GameConfigKeys::Controller_DeviceID, m_selectedGamepad);
		if(m_skins.size() > 0)
			g_gameConfig.Set(GameConfigKeys::Skin, m_skins[m_selectedSkin]);

		switch (inputModeMap[m_laserModes[m_laserMode]])
		{
		case InputDevice::Controller:
			g_gameConfig.Set(GameConfigKeys::Controller_Sensitivity, m_laserSens);
			break;
		case InputDevice::Mouse:
			g_gameConfig.Set(GameConfigKeys::Mouse_Sensitivity, m_laserSens);
			break;
		case InputDevice::Keyboard:
		default:
			g_gameConfig.Set(GameConfigKeys::Key_Sensitivity, m_laserSens);
			break;
		}

		if (m_speedMod == 2)
		{
			g_gameConfig.Set(GameConfigKeys::UseCMod, true);
			g_gameConfig.Set(GameConfigKeys::UseMMod, false);
		}
		else if (m_speedMod == 1)
		{
			g_gameConfig.Set(GameConfigKeys::UseCMod, false);
			g_gameConfig.Set(GameConfigKeys::UseMMod, true);
		}
		else
		{
			g_gameConfig.Set(GameConfigKeys::UseCMod, false);
			g_gameConfig.Set(GameConfigKeys::UseMMod, false);
		}

		g_input.Cleanup();
		g_input.Init(*g_gameWindow);
		g_application->RemoveTickable(this);
	}

public:

	//TODO: Controller support and the rest of the options and better layout
	bool Init()
	{
		m_guiStyle = g_commonGUIStyle;
		m_canvas = Utility::MakeRef(new Canvas());
		m_gamePads = g_gameWindow->GetGamepadDeviceNames();
		m_skins = Path::GetSubDirs("./skins/");

		if (g_gameConfig.GetBool(GameConfigKeys::UseCMod))
			m_speedMod = 2;
		else if (g_gameConfig.GetBool(GameConfigKeys::UseMMod))
			m_speedMod = 1;

		switch (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice))
		{
		case InputDevice::Controller:
			m_buttonMode = 1;
			break;
		case InputDevice::Keyboard:
		default:
			m_buttonMode = 0;
			break;
		}

		switch (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice))
		{
		case InputDevice::Controller:
			m_laserMode = 2;
			m_laserSens = g_gameConfig.GetFloat(GameConfigKeys::Controller_Sensitivity);
			break;
		case InputDevice::Mouse:
			m_laserMode = 1;
			m_laserSens = g_gameConfig.GetFloat(GameConfigKeys::Mouse_Sensitivity);
			break;
		case InputDevice::Keyboard:
		default:
			m_laserMode = 0;
			m_laserSens = g_gameConfig.GetFloat(GameConfigKeys::Key_Sensitivity);
			break;
		}

		m_modSpeed = g_gameConfig.GetFloat(GameConfigKeys::ModSpeed);
		m_hispeed = g_gameConfig.GetFloat(GameConfigKeys::HiSpeed);
		m_masterVolume = g_gameConfig.GetFloat(GameConfigKeys::MasterVolume);
		m_laserColors[0] = g_gameConfig.GetFloat(GameConfigKeys::Laser0Color);
		m_laserColors[1] = g_gameConfig.GetFloat(GameConfigKeys::Laser1Color);
		m_selectedGamepad = g_gameConfig.GetInt(GameConfigKeys::Controller_DeviceID);
		auto skinSearch = std::find(m_skins.begin(), m_skins.end(), g_gameConfig.GetString(GameConfigKeys::Skin));
		if (skinSearch == m_skins.end())
			m_selectedSkin = 0;
		else
			m_selectedSkin = skinSearch - m_skins.begin();


		//Options select
		ScrollBox* scroller = new ScrollBox(m_guiStyle);

		Canvas::Slot* slot = m_canvas->Add(scroller->MakeShared());

		slot->anchor = Anchor(0.1f, 0.f, 0.9f, 1.f);
		//slot->autoSizeX = true;
		//slot->alignment = Vector2(0.5, 0.0);
		
		LayoutBox* box = new LayoutBox();
		box->layoutDirection = LayoutBox::Vertical;
		scroller->SetContent(box->MakeShared());
		LayoutBox::Slot* btnSlot;
		// Start Button & Knob buttons
		{
			LayoutBox* stBox = new LayoutBox();

			stBox->layoutDirection = LayoutBox::Horizontal;
			{
				Button* stBtn = new Button(m_guiStyle);
				m_laserButtons[0] = stBtn;
				stBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetLL);
				stBtn->SetText(L"LL");
				stBtn->SetFontSize(32);
				btnSlot = stBox->Add(stBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->padding = Margin(60.f, 2.f);

			}
			{
				Button* stBtn = new Button(m_guiStyle);
				m_buttonButtons[0] = stBtn;
				stBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_ST);
				stBtn->SetText(L"Start");
				stBtn->SetFontSize(32);
				btnSlot = stBox->Add(stBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->alignment = Vector2(.5f, 0.f);
			}
			{
				Button* stBtn = new Button(m_guiStyle);
				m_laserButtons[1] = stBtn;
				stBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetRL);
				stBtn->SetText(L"RL");
				stBtn->SetFontSize(32);
				btnSlot = stBox->Add(stBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->alignment = Vector2(1.f, 0.f);
				btnSlot->padding = Margin(60.f, 2.f);

			}
			LayoutBox::Slot* stSlot = box->Add(stBox->MakeShared());
			stSlot->alignment = Vector2(0.5f, 0.f);
		}
		// BT Buttons
		{
			LayoutBox* btBox = new LayoutBox();
			btBox->layoutDirection = LayoutBox::Horizontal;

			{
				Button* btaBtn = new Button(m_guiStyle);
				m_buttonButtons[1] = btaBtn;
				btaBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTA);
				btaBtn->SetText(L"BT-A");
				btaBtn->SetFontSize(32);
				btnSlot = btBox->Add(btaBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btbBtn = new Button(m_guiStyle);
				m_buttonButtons[2] = btbBtn;
				btbBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTB);
				btbBtn->SetText(L"BT-B");
				btbBtn->SetFontSize(32);
				btnSlot = btBox->Add(btbBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btcBtn = new Button(m_guiStyle);
				m_buttonButtons[3] = btcBtn;
				btcBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTC);
				btcBtn->SetText(L"BT-C");
				btcBtn->SetFontSize(32);
				btnSlot = btBox->Add(btcBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			{
				Button* btdBtn = new Button(m_guiStyle);
				m_buttonButtons[4] = btdBtn;
				btdBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_BTD);
				btdBtn->SetText(L"BT-D");
				btdBtn->SetFontSize(32);
				btnSlot = btBox->Add(btdBtn->MakeShared());
				btnSlot->padding = Margin(2);
				btnSlot->fillX = true;
			}
			LayoutBox::Slot* btSlot = box->Add(btBox->MakeShared());
			btSlot->alignment = Vector2(0.5f, 0.f);
		}
		// FX Buttons
		{
			LayoutBox* fxBox = new LayoutBox();

			fxBox->layoutDirection = LayoutBox::Horizontal;
			{
				Button* fxlBtn = new Button(m_guiStyle);
				m_buttonButtons[5] = fxlBtn;
				fxlBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_FXL);
				fxlBtn->SetText(L"FX-L");
				fxlBtn->SetFontSize(32);
				btnSlot = fxBox->Add(fxlBtn->MakeShared());
				btnSlot->padding = Margin(20.f,2.f);
				btnSlot->fillX = true;
				btnSlot->alignment = Vector2(0.25f, 0.f);
			}
			{
				Button* fxrBtn = new Button(m_guiStyle);
				m_buttonButtons[6] = fxrBtn;
				fxrBtn->OnPressed.Add(this, &SettingsScreen_Impl::SetKey_FXR);
				fxrBtn->SetText(L"FX-R");
				fxrBtn->SetFontSize(32);
				btnSlot = fxBox->Add(fxrBtn->MakeShared());
				btnSlot->padding = Margin(20.f, 2.f);
				btnSlot->alignment = Vector2(0.75f, 0.f);
				btnSlot->fillX = true;
			}
			LayoutBox::Slot* fxSlot = box->Add(fxBox->MakeShared());
			fxSlot->alignment = Vector2(0.5f, 0.f);
		}

		// Setting bar
		{
			SettingsBar* sb = new SettingsBar(m_guiStyle);
			
			m_settings = Ref<SettingsBar>(sb);

			sb->AddSetting(&m_masterVolume, 0.f, 1.0f, "Master Volume");
			sb->AddSetting(&m_buttonMode, m_buttonModes, m_buttonModes.size(), "Button Input Mode");
			sb->AddSetting(&m_laserMode, m_laserModes, m_laserModes.size(), "Laser Input Mode");
			sb->AddSetting(&m_speedMod, m_speedMods, m_speedMods.size(), "Speed mod");
			sb->AddSetting(&m_laserSens, 0.f, 20.0f, "Laser Sensitivity");
			sb->AddSetting(&m_hispeed, 0.25f, 10.0f, "HiSpeed");
			sb->AddSetting(&m_modSpeed, 50.0f, 1500.0f, "ModSpeed");
			if (m_gamePads.size() > 0)
			{
				sb->AddSetting(&m_selectedGamepad, m_gamePads, m_gamePads.size(), "Selected Controller");

			}
			if (m_skins.size() > 0)
				sb->AddSetting(&m_selectedSkin, m_skins, m_skins.size(), "Selected Skin");
			sb->AddSetting(m_laserColors, 0.0, 360.0f, "Left Laser Color");
			sb->AddSetting(m_laserColors + 1, 0.0, 360.0f, "Right Laser Color");


			LayoutBox::Slot* slot = box->Add(sb->MakeShared());
			slot->fillX = true;
		}

		// Laser Colors
		{
			Label* laserColorLabel = new Label();
			laserColorLabel->SetText(L"Laser Colors:");
			laserColorLabel->SetFontSize(20);
			box->Add(laserColorLabel->MakeShared());


			// Make white square texture
			m_whiteTex = TextureRes::Create(g_gl);
			m_whiteTex->Init(Vector2i(50, 50), Graphics::TextureFormat::RGBA8);

			Colori pixels[2500];

			for (size_t i = 0; i < 2500; i++)
			{
				pixels[i] = Colori(255,255,255,255);
			}

			m_whiteTex->SetData(Vector2i(50, 50), pixels);

			LayoutBox* colorBox = new LayoutBox();
			colorBox->layoutDirection = LayoutBox::Horizontal;

			{
				Panel* lpanel = new Panel();
				m_laserColorPanels[0] = lpanel;
				lpanel->texture = m_whiteTex;
				LayoutBox::Slot* lslot = colorBox->Add(lpanel->MakeShared());

			}

			{
				Panel* rpanel = new Panel();
				m_laserColorPanels[1] = rpanel;
				rpanel->texture = m_whiteTex;
				LayoutBox::Slot* rslot = colorBox->Add(rpanel->MakeShared());
				rslot->padding = Margin(20, 0);

			}
			LayoutBox::Slot* slot = box->Add(colorBox->MakeShared());
			slot->fillX = true;
			slot->fillY = true;

		}


		Button* exitBtn = new Button(m_guiStyle);
		exitBtn->OnPressed.Add(this, &SettingsScreen_Impl::Exit);
		exitBtn->SetText(L"Back");
		exitBtn->SetFontSize(32);
		btnSlot = box->Add(exitBtn->MakeShared());
		btnSlot->padding = Margin(2);
		btnSlot->alignment = Vector2(0.5f, 0.f);

		return true;
	}

	void Tick(float deltatime)
	{
		for (size_t i = 0; i < 7; i++)
		{
			if (m_buttonMode == 1)
			{
				m_buttonButtons[i]->SetText(Utility::WSprintf(L"%d", g_gameConfig.GetInt(m_controllerKeys[i])));
			}
			else
			{
				m_buttonButtons[i]->SetText(Utility::ConvertToWString(SDL_GetKeyName(g_gameConfig.GetInt(m_keyboardKeys[i]))));
			}
		}
		for (size_t i = 0; i < 2; i++)
		{
			if (m_laserMode == 2)
			{
				m_laserButtons[i]->SetText(Utility::WSprintf(L"%d", g_gameConfig.GetInt(m_controllerLaserKeys[i])));
			}
			else
			{
				m_laserButtons[i]->SetText(Utility::WSprintf(
					L"%ls/%ls", 
					Utility::ConvertToWString(SDL_GetKeyName(g_gameConfig.GetInt(m_keyboardLaserKeys[i * 2]))),
					Utility::ConvertToWString(SDL_GetKeyName(g_gameConfig.GetInt(m_keyboardLaserKeys[i * 2 + 1])))
					));
			}

			m_laserColorPanels[i]->color = Color::FromHSV(m_laserColors[i], 1.f, 1.f);


		}
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
	Ref<Gamepad> m_gamepad;
	Label* m_prompt;
	GameConfigKeys m_key;
	bool m_isGamepad;
	int m_gamepadIndex;
	bool m_completed = false;
	bool m_knobs = false;
	Vector<float> m_gamepadAxes;

public:
	ButtonBindingScreen_Impl(GameConfigKeys key, bool gamepad, int controllerindex)
	{
		m_key = key;
		m_gamepadIndex = controllerindex;
		m_isGamepad = gamepad;
		m_knobs = (key == GameConfigKeys::Controller_Laser0Axis || key == GameConfigKeys::Controller_Laser1Axis);
			
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

		m_prompt = new Label();
		m_prompt->SetText(L"Press Key");
		m_prompt->SetFontSize(100);
		box->Add(m_prompt->MakeShared());
		if (m_knobs)
			m_prompt->SetText(L"Press Left Key");


		if (m_isGamepad)
		{
			m_prompt->SetText(L"Press Button");
			m_gamepad = g_gameWindow->OpenGamepad(m_gamepadIndex);
			if (m_knobs)
			{
				m_prompt->SetText(L"Turn Knob");
				for (size_t i = 0; i < m_gamepad->NumAxes(); i++)
				{
					m_gamepadAxes.Add(m_gamepad->GetAxis(i));
				}
			}
			m_gamepad->OnButtonPressed.Add(this, &ButtonBindingScreen_Impl::OnButtonPressed);
		}

		return true;
	}

	void Tick(float deltatime)
	{
		if (m_knobs && m_isGamepad)
		{
			for (uint8 i = 0; i < m_gamepad->NumAxes(); i++)
			{
				float delta = fabs(m_gamepad->GetAxis(i) - m_gamepadAxes[i]);
				if (delta > 0.3f)
				{
					g_gameConfig.Set(m_key, i);
					m_completed = true;
					break;
				}

			}
		}

		if (m_completed && m_gamepad)
		{
			m_gamepad->OnButtonPressed.RemoveAll(this);
			m_gamepad.Release();

			g_application->RemoveTickable(this);
		}
	}

	void OnButtonPressed(uint8 key)
	{
		if (!m_knobs)
		{
			g_gameConfig.Set(m_key, key);
			m_completed = true;
		}
	}

	virtual void OnKeyPressed(int32 key)
	{
		if (!m_isGamepad && !m_knobs)
		{
			g_gameConfig.Set(m_key, key);
			g_application->RemoveTickable(this);
		}
		else if (!m_isGamepad && m_knobs)
		{
			if (!m_completed)
			{
				switch (m_key)
				{
				case GameConfigKeys::Controller_Laser0Axis:
					g_gameConfig.Set(GameConfigKeys::Key_Laser0Neg, key);
					break;
				case GameConfigKeys::Controller_Laser1Axis:
					g_gameConfig.Set(GameConfigKeys::Key_Laser1Neg, key);
					break;
				default:
					break;
				}
				m_prompt->SetText(L"Press Right Key");
				m_completed = true;
			}
			else
			{
				switch (m_key)
				{
				case GameConfigKeys::Controller_Laser0Axis:
					g_gameConfig.Set(GameConfigKeys::Key_Laser0Pos, key);
					break;
				case GameConfigKeys::Controller_Laser1Axis:
					g_gameConfig.Set(GameConfigKeys::Key_Laser1Pos, key);
					break;
				default:
					break;
				}
				g_application->RemoveTickable(this);
			}
		}
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

ButtonBindingScreen* ButtonBindingScreen::Create(GameConfigKeys key, bool gamepad, int controllerIndex)
{
	ButtonBindingScreen_Impl* impl = new ButtonBindingScreen_Impl(key, gamepad, controllerIndex);
	return impl;
}