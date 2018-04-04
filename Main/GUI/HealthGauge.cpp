#include "stdafx.h"
#include <GUI/GUIRenderer.hpp>
#include "HealthGauge.hpp"
#include "Application.hpp"

HealthGauge::HealthGauge()
{
}

void HealthGauge::Render(GUIRenderData rd)
{
	// Calculate bar placement to fit parent rectangle
	Rect barArea = GUISlotBase::ApplyFill(FillMode::Fit, frontTexture->GetSize(), rd.area);
	float elementScale = barArea.size.x / frontTexture->GetSize().x; // Scale of the original bar
	GUISlotBase::ApplyAlignment(Vector2(0.5f), barArea, rd.area);

	// Optional Bg?
	if(backTexture)
		rd.guiRenderer->RenderRect(barArea, Color::White, backTexture);

	MaterialParameterSet params;
	params.SetParameter("mainTex", fillTexture);
	params.SetParameter("maskTex", maskTexture);
	params.SetParameter("rate", rate);

	Transform transform;
	transform *= Transform::Translation(rd.area.pos);
	transform *= Transform::Scale(Vector3(rd.area.size.x, rd.area.size.y, 1.0f));


	Color color;
	if(rate >= colorBorder)
	{
		color = upperColor;
	}
	else
	{
		color = lowerColor;
	}
	params.SetParameter("barColor", color);
	rd.rq->Draw(transform, rd.guiRenderer->guiQuad, fillMaterial, params);

	// Draw frame last
	rd.guiRenderer->RenderRect(barArea, Color::White, frontTexture);
}

Vector2 HealthGauge::GetDesiredSize(GUIRenderData rd)
{
	return GUISlotBase::ApplyFill(FillMode::Fit, frontTexture->GetSize(), rd.area).size;
}
