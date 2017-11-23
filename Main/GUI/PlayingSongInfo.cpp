#include "stdafx.h"
#include "PlayingSongInfo.hpp"
#include <GUI/GUI.hpp>
#include "Application.hpp"
#include "SongSelectStyle.hpp"

#include <Beatmap/Beatmap.hpp>

PlayingSongInfo::PlayingSongInfo(Game& game)
{
	m_settings = game.GetBeatmap()->GetMapSettings();
	Ref<SongSelectStyle> songSelectStyle = SongSelectStyle::Get(g_application);


	LayoutBox* layoutBox = new LayoutBox();
	m_layout = Ref<LayoutBox>(layoutBox);
	layoutBox->layoutDirection = LayoutBox::LayoutDirection::Horizontal;
	{
		Slot* layoutSlot = Add(layoutBox->MakeShared());
		layoutSlot->anchor = Anchors::Full;
	}

	m_jacketPath = Path::Normalize(game.GetMapRootPath() + "/" + m_settings.jacketPath);
	m_jacketImage = songSelectStyle->GetJacketThumnail(m_jacketPath);


	// Jacket
	Panel* jacketPanel = new Panel();
	m_jacket = Ref<Panel>(jacketPanel);
	jacketPanel->texture = m_jacketImage;
	jacketPanel->color = Color::White;
	jacketPanel->imageFillMode = FillMode::Fit;
	{
		LayoutBox::Slot* jackSlot = layoutBox->Add(jacketPanel->MakeShared());
		jackSlot->alignment = Vector2(0.f, 0.5f);
		jackSlot->fillY = true;
	}

	SongTitleArtist* sta = new SongTitleArtist(m_settings.title, m_settings.artist, this);
	m_titleArtist = Ref<SongTitleArtist>(sta);
	sta->color = Color::White;
	{
		LayoutBox::Slot* titleSlot = layoutBox->Add(sta->MakeShared());
		titleSlot->fillX = true;
		titleSlot->fillY = true;
		titleSlot->padding = Margin(10, 0, 0, 0);
	}

}

void PlayingSongInfo::PreRender(GUIRenderData rd, GUIElementBase *& inputElement)
{
	Canvas::PreRender(rd, inputElement);
}

void PlayingSongInfo::Render(GUIRenderData rd)
{
	// Create Text Graphics
	//if (!m_title || !m_artist)
	progressMaterial->opaque = false;
	Canvas::Render(rd);
}

Vector2 PlayingSongInfo::GetDesiredSize(GUIRenderData rd)
{
	return Vector2(g_resolution.y * 0.2 * 3, g_resolution.y * 0.2);
}

void PlayingSongInfo::SetProgress(float progress)
{
	m_titleArtist->progress = Math::Clamp(progress, 0.f, 1.f);
}

SongTitleArtist::SongTitleArtist(String title, String artist, PlayingSongInfo* info)
{
	m_title = Utility::ConvertToWString(title);
	m_artist = Utility::ConvertToWString(artist);
	m_psi = info;
}

void SongTitleArtist::PreRender(GUIRenderData rd, GUIElementBase *& inputElement)
{
	Panel::PreRender(rd, inputElement);
}

void SongTitleArtist::Render(GUIRenderData rd)
{
	/// TODO: Cache stuff and only regen if the resolution changes.
	Ref<TextRes> title = rd.guiRenderer->font->CreateText(m_title, rd.area.size.y / 2);
	Ref<TextRes> artist = rd.guiRenderer->font->CreateText(m_artist, rd.area.size.y / 3);
	Rect titleRect = rd.area;
	titleRect.size = titleRect.size * Vector2(1.f, .5f);
	Rect artistRect = rd.area;
	artistRect.size = titleRect.size * Vector2(1.f, .33f);
	artistRect.pos = artistRect.pos + Vector2(0.f, .5f * rd.area.size.y);

	titleRect = GUISlotBase::ApplyFill(FillMode::Fit, title->size, titleRect);
	artistRect = GUISlotBase::ApplyFill(FillMode::Fit, artist->size, artistRect);

	title = rd.guiRenderer->font->CreateText(m_title, titleRect.size.y * 0.75);
	artist = rd.guiRenderer->font->CreateText(m_artist, artistRect.size.y * 0.75);


	artistRect.pos.y = rd.area.pos.y + (rd.area.size.y / 2.0);
	titleRect.pos.y = artistRect.pos.y - title->size.y - 2;
	rd.guiRenderer->RenderText(title, titleRect.pos, color);
	rd.guiRenderer->RenderText(artist, artistRect.pos, color);
	
	Transform transform;
	transform *= Transform::Translation(rd.area.pos);
	transform *= Transform::Scale(Vector3(rd.area.size.x, rd.area.size.y, 1.0f));

	/// TODO: Actually use a progress bar object
	MaterialParameterSet params;
	params.SetParameter("progress", progress);
	rd.rq->Draw(transform, rd.guiRenderer->guiQuad, m_psi->progressMaterial, params);



	//Panel::Render(rd);
}

SongProgressBar::SongProgressBar()
{
}

void SongProgressBar::Render(GUIRenderData rd)
{
	MaterialParameterSet params;
	params.SetParameter("progress", progress);

}

Vector2 SongProgressBar::GetDesiredSize(GUIRenderData rd)
{
	return Vector2();
}
