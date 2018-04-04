#pragma once
#include <GUI/GUIElement.hpp>

class HealthGauge : public GUIElementBase
{
public:
	HealthGauge();
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// The fill rate of the gauge
	float rate = 0.5f;
	float colorBorder = 0.7f;
	Color upperColor = Colori(255, 102, 255);
	Color lowerColor = Colori(0, 204, 255);

	Material fillMaterial;
	Texture frontTexture;
	Texture fillTexture;
	Texture backTexture;
	Texture maskTexture;
};