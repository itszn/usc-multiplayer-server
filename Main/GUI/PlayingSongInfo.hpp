#pragma once
#include "stdafx.h"
#include <Beatmap/BeatmapPlayback.hpp>
#include "Game.hpp"


class SongTitleArtist
{
public:
	SongTitleArtist(String title, String artist, class PlayingSongInfo* info);
	virtual void PreRender();
	virtual void Render();
	float progress = 0.5f;
	float BPM = 120.f;
	float hiSpeed = 1.5f;

private:

	//Sizes should add up to 0.9
	const float m_titleSize = 0.3f;
	const float m_artistSize = 0.2f;
	const float m_speedSize = 0.4f;

	WString m_title;
	WString m_artist;
	class PlayingSongInfo* m_psi;
};

class SongProgressBar
{
public:
	SongProgressBar();
	virtual void Render();
	virtual Vector2 GetDesiredSize();
	float progress = 0.5f;

};

class PlayingSongInfo
{
public:
	PlayingSongInfo(Game& game);
	virtual void PreRender();
	virtual void Render();
	virtual Vector2 GetDesiredSize();
	void SetProgress(float progress);
	void SetBPM(float bpm);
	void SetHiSpeed(float hiSpeed);
	void SetJacket(Texture jacket);
	Material progressMaterial;

private:
	Ref<SongTitleArtist> m_titleArtist;
	//Ref<Panel> m_titlePanel;
	//Ref<Panel> m_artistPanel;
	//Text m_title;
	//Text m_artist;
	BeatmapSettings m_settings;
	float m_progress = 0.5f;
	String m_jacketPath;
	Texture m_jacketImage;

};

