#include "stdafx.h"
#include "ScoreScreen.hpp"
#include "Application.hpp"
#include "GameConfig.hpp"
#include <Audio/Audio.hpp>
#include <Beatmap/MapDatabase.hpp>
#include "Scoring.hpp"
#include "Game.hpp"
#include "AsyncAssetLoader.hpp"
#include "HealthGauge.hpp"
#ifdef _WIN32
#include "SDL_keycode.h"
#else
#include "SDL2/SDL_keycode.h"
#endif
extern "C"
{
#include "lua.h"
#include "lauxlib.h"
}

class ScoreScreen_Impl : public ScoreScreen
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<Canvas> m_canvas;
	Ref<HealthGauge> m_gauge;
	//Ref<Panel> m_jacket;
	Ref<Canvas> m_timingStatsCanvas;
	//Ref<LayoutBox> m_itemBox;
	MapDatabase m_mapDatabase;
	// Things for score screen
	Graphics::Font m_specialFont;
	Sample m_applause;
	Texture m_categorizedHitTextures[4];
	lua_State* m_lua;
	bool m_autoplay;
	bool m_autoButtons;
	bool m_startPressed;
	bool m_showStats;
	uint32 m_score;
	uint32 m_maxCombo;
	uint32 m_categorizedHits[3];
	float m_finalGaugeValue;
	float* m_gaugeSamples;
	String m_jacketPath;
	uint32 m_timedHits[2];
	float m_meanHitDelta;
	MapTime m_medianHitDelta;

	Vector<ScoreIndex*> m_highScores;

	BeatmapSettings m_beatmapSettings;
	Texture m_jacketImage;
	Texture m_graphTex;
	GameFlags m_flags;

	void m_PushStringToTable(const char* name, String data)
	{
		lua_pushstring(m_lua, name);
		lua_pushstring(m_lua, data.c_str());
		lua_settable(m_lua, -3);
	}
	void m_PushFloatToTable(const char* name, float data)
	{
		lua_pushstring(m_lua, name);
		lua_pushnumber(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushIntToTable(const char* name, int data)
	{
		lua_pushstring(m_lua, name);
		lua_pushinteger(m_lua, data);
		lua_settable(m_lua, -3);
	}

public:
	ScoreScreen_Impl(class Game* game)
	{
		Scoring& scoring = game->GetScoring();
		m_autoplay = scoring.autoplay;
		m_autoButtons = scoring.autoplayButtons;
		m_score = scoring.CalculateCurrentScore();
		m_maxCombo = scoring.maxComboCounter;
		m_finalGaugeValue = scoring.currentGauge;
		m_gaugeSamples = game->GetGaugeSamples();
		m_timedHits[0] = scoring.timedHits[0];
		m_timedHits[1] = scoring.timedHits[1];
		m_flags = game->GetFlags();
		m_highScores = game->GetDifficultyIndex().scores;

		memcpy(m_categorizedHits, scoring.categorizedHits, sizeof(scoring.categorizedHits));
		m_meanHitDelta = scoring.GetMeanHitDelta();
		m_medianHitDelta = scoring.GetMedianHitDelta();

		// Don't save the score if autoplay was on or if the song was launched using command line
		// also don't save the score if the song was manually exited
		if(!m_autoplay && !m_autoButtons && game->GetDifficultyIndex().mapId != -1 && !game->GetManualExit())
			m_mapDatabase.AddScore(game->GetDifficultyIndex(), m_score, m_categorizedHits[2], m_categorizedHits[1], m_categorizedHits[0], m_finalGaugeValue, (uint32)m_flags);

		// Used for jacket images

		m_startPressed = false;
		
		m_beatmapSettings = game->GetBeatmap()->GetMapSettings();
		m_jacketPath = Path::Normalize(game->GetMapRootPath() + Path::sep + m_beatmapSettings.jacketPath);
		m_jacketImage = game->GetJacketImage();

		// Make texture for performance graph samples
		m_graphTex = TextureRes::Create(g_gl);
		m_graphTex->Init(Vector2i(256, 1), Graphics::TextureFormat::RGBA8);
		Colori graphPixels[256];
		for (uint32 i = 0; i < 256; i++)
		{
			graphPixels[i].x = 255.0f * Math::Clamp(m_gaugeSamples[i], 0.0f, 1.0f);
		}
		m_graphTex->SetData(Vector2i(256, 1), graphPixels);
		m_graphTex->SetWrap(Graphics::TextureWrap::Clamp, Graphics::TextureWrap::Clamp);

	}
	~ScoreScreen_Impl()
	{
		if (m_lua)
			g_application->DisposeLua(m_lua);
	}

	AsyncAssetLoader loader;
	virtual bool AsyncLoad() override
	{
		m_guiStyle = g_commonGUIStyle;
		String gaugePath = "gauges/normal/";
		m_gauge = Ref<HealthGauge>(new HealthGauge());
		if ((m_flags & GameFlags::Hard) != GameFlags::None)
		{
			gaugePath = "gauges/hard/";
			m_gauge->colorBorder = 0.3f;
			m_gauge->lowerColor = Colori(200, 50, 0);
			m_gauge->upperColor = Colori(255, 100, 0);
		}

		loader.AddTexture(m_gauge->fillTexture, gaugePath + "gauge_fill.png");
		loader.AddTexture(m_gauge->frontTexture, gaugePath + "gauge_front.png");
		loader.AddTexture(m_gauge->backTexture, gaugePath + "gauge_back.png");
		loader.AddTexture(m_gauge->maskTexture, gaugePath + "gauge_mask.png");
		loader.AddMaterial(m_gauge->fillMaterial, "gauge");
		m_gauge->rate = m_finalGaugeValue;
		//m_canvas = Utility::MakeRef(new Canvas());
		String skin = g_gameConfig.GetString(GameConfigKeys::Skin);
		CheckedLoad(m_applause = g_audio->CreateSample("skins/" + skin + "/audio/applause.wav"));

		return loader.Load();
	}
	virtual bool AsyncFinalize() override
	{
		if(!loader.Finalize())
			return false;
		m_lua = g_application->LoadScript("result");
		//set lua table
		lua_newtable(m_lua);
		m_PushIntToTable("score", m_score);
		m_PushFloatToTable("gauge", m_finalGaugeValue);
		m_PushIntToTable("misses", m_categorizedHits[0]);
		m_PushIntToTable("goods", m_categorizedHits[1]);
		m_PushIntToTable("perfects", m_categorizedHits[2]);
		m_PushIntToTable("maxCombo", m_maxCombo);
		m_PushIntToTable("level", m_beatmapSettings.level);
		m_PushIntToTable("difficulty", m_beatmapSettings.difficulty);
		m_PushStringToTable("title", m_beatmapSettings.title);
		m_PushStringToTable("artist", m_beatmapSettings.artist);
		m_PushStringToTable("effector", m_beatmapSettings.effector);
		m_PushStringToTable("bpm", m_beatmapSettings.bpm);
		m_PushStringToTable("jacketPath", m_jacketPath);
		m_PushIntToTable("medianHitDelta", m_medianHitDelta);
		m_PushFloatToTable("meanHitDelta", m_meanHitDelta);
		m_PushIntToTable("earlies", m_timedHits[0]);
		m_PushIntToTable("lates", m_timedHits[1]);
		m_PushStringToTable("grade", Scoring::CalculateGrade(m_score).c_str());
		//Push gauge samples
		lua_pushstring(m_lua, "gaugeSamples");
		lua_newtable(m_lua);
		for (size_t i = 0; i < 256; i++)
		{
			lua_pushnumber(m_lua, m_gaugeSamples[i]);
			lua_rawseti(m_lua, -2, i + 1);
		}
		lua_settable(m_lua, -3);

		lua_pushstring(m_lua, "highScores");
		lua_newtable(m_lua);
		int scoreIndex = 1;
		for (auto& score : m_highScores)
		{
			lua_pushinteger(m_lua, scoreIndex++);
			lua_newtable(m_lua);
			m_PushFloatToTable("gauge", score->gauge);
			m_PushIntToTable("flags", score->gameflags);
			m_PushIntToTable("score", score->score);
			m_PushIntToTable("perfects", score->crit);
			m_PushIntToTable("goods", score->almost);
			m_PushIntToTable("misses", score->miss);
			lua_settable(m_lua, -3);
		}
		lua_settable(m_lua, -3);


		///TODO: maybe push complete hit stats

		lua_setglobal(m_lua, "result");

		// Make gauge material transparent
		m_gauge->fillMaterial->opaque = false;
		return true;
	}
	bool Init() override
	{
		// Add to screen
		//Canvas::Slot* rootSlot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//rootSlot->anchor = Anchors::Full;

		// Play score screen sound
		m_applause->Play();

		return true;
	}


	virtual void OnKeyPressed(int32 key) override
	{
		if(key == SDLK_ESCAPE || key == SDLK_RETURN)
		{
			g_application->RemoveTickable(this);
		}
	}
	virtual void OnKeyReleased(int32 key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		lua_pushboolean(m_lua, m_showStats);
		if (lua_pcall(m_lua, 2, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
	}
	virtual void Tick(float deltaTime) override
	{
		// Check for button pressed here instead of adding to onbuttonpressed for stability reasons
		//TODO: Change to onbuttonpressed
		if(m_startPressed && !g_input.GetButton(Input::Button::BT_S))
			g_application->RemoveTickable(this);

		m_startPressed = g_input.GetButton(Input::Button::BT_S);

		if (g_input.GetButton(Input::Button::FX_0))
			m_showStats = true;
		else
			m_showStats = false;
	}

	virtual void OnSuspend()
	{
		//g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		//Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//slot->anchor = Anchors::Full;
	}

};

ScoreScreen* ScoreScreen::Create(class Game* game)
{
	ScoreScreen_Impl* impl = new ScoreScreen_Impl(game);
	return impl;
}
