#include "stdafx.h"
#include "PlayingSongInfo.hpp"
#include "Application.hpp"

#include <Beatmap/Beatmap.hpp>

PlayingSongInfo::PlayingSongInfo(Game& game)
{
	m_settings = game.GetBeatmap()->GetMapSettings();
	SongTitleArtist* sta = new SongTitleArtist(m_settings.title, m_settings.artist, this);
	m_titleArtist = Ref<SongTitleArtist>(sta);


	//LayoutBox* layoutBox = new LayoutBox();
	//m_layout = Ref<LayoutBox>(layoutBox);
	//layoutBox->layoutDirection = LayoutBox::LayoutDirection::Horizontal;
	//{
	//	Slot* layoutSlot = Add(layoutBox->MakeShared());
	//	layoutSlot->anchor = Anchors::Full;
	//}

	//// Jacket
	//Panel* jacketPanel = new Panel();
	//m_jacket = Ref<Panel>(jacketPanel);
	//jacketPanel->color = Color::White;
	//jacketPanel->imageFillMode = FillMode::Fit;
	//{
	//	LayoutBox::Slot* jackSlot = layoutBox->Add(jacketPanel->MakeShared());
	//	jackSlot->alignment = Vector2(0.f, 0.5f);
	//	jackSlot->fillY = true;
	//}

	//sta->color = Color::White;
	//{
	//	LayoutBox::Slot* titleSlot = layoutBox->Add(sta->MakeShared());
	//	titleSlot->fillX = true;
	//	titleSlot->fillY = true;
	//	titleSlot->padding = Margin(10, 0, 0, 0);
	//}

}

void PlayingSongInfo::PreRender()
{
}

void PlayingSongInfo::Render()
{
}

Vector2 PlayingSongInfo::GetDesiredSize()
{
	return Vector2();
}

void PlayingSongInfo::SetProgress(float progress)
{
	m_titleArtist->progress = Math::Clamp(progress, 0.f, 1.f);
}

void PlayingSongInfo::SetBPM(float bpm)
{
	m_titleArtist->BPM = bpm;
}

void PlayingSongInfo::SetHiSpeed(float hiSpeed)
{
	m_titleArtist->hiSpeed = hiSpeed;
}

void PlayingSongInfo::SetJacket(Texture jacket)
{
	m_jacketImage = jacket;
}

SongTitleArtist::SongTitleArtist(String title, String artist, PlayingSongInfo* info)
{
	m_title = Utility::ConvertToWString(title);
	m_artist = Utility::ConvertToWString(artist);
	m_psi = info;
}

void SongTitleArtist::PreRender()
{
}

void SongTitleArtist::Render()
{
	// Make Bpm and Hispeed string
	WString speedText = Utility::WSprintf(
		L"BPM: %.1f\nHiSpeed: %d x %.1f = %d",
		BPM, (int)BPM, hiSpeed, (int)Math::Round(hiSpeed * BPM));


	/// TODO: Cache stuff and only regen if the resolution changes.
	//Ref<TextRes> title = rd.guiRenderer->font->CreateText(m_title, rd.area.size.y / 2);
	//Ref<TextRes> artist = rd.guiRenderer->font->CreateText(m_artist, rd.area.size.y / 3);
	//Ref<TextRes> speedTextGraphic = rd.guiRenderer->font->CreateText(speedText, rd.area.size.y / 3);
	//Rect titleRect = rd.area;
	//Rect artistRect = rd.area;
	//Rect speedRect = rd.area;

	//titleRect.size = titleRect.size * Vector2(1.f, m_titleSize);
	//artistRect.size = artistRect.size * Vector2(1.f, m_artistSize);
	//speedRect.size = speedRect.size * Vector2(1.f, m_speedSize);
	//artistRect.pos = artistRect.pos + Vector2(0.f, .5f * rd.area.size.y);


	//titleRect = GUISlotBase::ApplyFill(FillMode::Fit, title->size, titleRect);
	//artistRect = GUISlotBase::ApplyFill(FillMode::Fit, artist->size, artistRect);
	//speedRect = GUISlotBase::ApplyFill(FillMode::Fit, speedTextGraphic->size, speedRect);

	//title = rd.guiRenderer->font->CreateText(m_title, titleRect.size.y * 0.75);
	//artist = rd.guiRenderer->font->CreateText(m_artist, artistRect.size.y * 0.75);
	//speedTextGraphic = rd.guiRenderer->font->CreateText(speedText, (speedRect.size.y / 2) * 0.75);

	//titleRect.pos.y = rd.area.pos.y + (0.5f * m_titleSize * rd.area.size.y) - (0.5f * title->size.y);
	//artistRect.pos.y = rd.area.pos.y + (m_titleSize * rd.area.size.y) + (0.5f * m_artistSize * rd.area.size.y) - (0.5f * artist->size.y);;
	//Vector2 speedTextPos = Vector2(artistRect.pos.x, rd.area.pos.y + ((m_titleSize + m_artistSize) * rd.area.size.y));

	//rd.guiRenderer->RenderText(title, titleRect.pos, color);
	//rd.guiRenderer->RenderText(artist, artistRect.pos, color);
	//rd.guiRenderer->RenderText(speedTextGraphic, speedTextPos, color);
	//
	//Transform transform;
	//transform *= Transform::Translation(rd.area.pos);
	//transform *= Transform::Scale(Vector3(rd.area.size.x, rd.area.size.y, 1.0f));

	///// TODO: Actually use a progress bar object
	//MaterialParameterSet params;
	//params.SetParameter("progress", progress);
	//rd.rq->Draw(transform, rd.guiRenderer->guiQuad, m_psi->progressMaterial, params);



	//Panel::Render(rd);
}

SongProgressBar::SongProgressBar()
{
}

void SongProgressBar::Render()
{
	MaterialParameterSet params;
	params.SetParameter("progress", progress);

}

Vector2 SongProgressBar::GetDesiredSize()
{
	return Vector2();
}
