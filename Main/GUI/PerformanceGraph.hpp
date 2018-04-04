#pragma once
#include <GUI/LayoutBox.hpp>

class PerformanceGraph : public GUIElementBase
{
public:
	PerformanceGraph();
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
	virtual void Render(GUIRenderData rd);
	virtual Vector2 GetDesiredSize(GUIRenderData rd);

	Texture borderTexture;
	Texture graphTex;
	Margini border;
	Color upperColor = Color(0.f, 1.f, 0.f);
	Color lowerColor = Color(1.f, 0.f, 0.f);;
	float colorBorder = 0.7f;
};