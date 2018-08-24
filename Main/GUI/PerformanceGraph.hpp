#pragma once

class PerformanceGraph
{
public:
	PerformanceGraph();
	virtual void PreRender();
	virtual void Render();
	virtual Vector2 GetDesiredSize();

	Texture borderTexture;
	Texture graphTex;
	Margini border;
	Color upperColor = Color(0.f, 1.f, 0.f);
	Color lowerColor = Color(1.f, 0.f, 0.f);;
	float colorBorder = 0.7f;
};